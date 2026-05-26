#include "ordenes_manual.hpp"
#include "estado_ciudad.hpp"
#include "semaforo_interseccion.hpp"
#include "../common/config.hpp"
#include "../common/json_utils.hpp"

#include <chrono>
#include <iostream>
#include <map>
#include <sstream>

using namespace std;

struct OverrideActivo {
    ParSemaforosInter par;
    int duracion_seg = 30;
    string motivo = "manual";
    chrono::steady_clock::time_point expira;
};

static map<string, OverrideActivo> g_overrides;

static void enviarComandos(zmq::socket_t& push, const string& inter, const ParSemaforosInter& par,
                           int duracion, const string& razon) {
    auto cmds = generarComandosSemaforoInterseccion(inter, par, duracion, razon);
    for (const string& cmd : cmds) {
        zmq::message_t m(cmd.begin(), cmd.end());
        push.send(m, zmq::send_flags::none);
    }
}

static string aplicarOverride(const string& inter, const ParSemaforosInter& par, int duracion,
                              const string& motivo, zmq::socket_t& push) {
    if (!interseccionValida(config::ciudad, inter)) {
        return "{\"error\":\"interseccion no existe en la matriz configurada\"}";
    }

    OverrideActivo ov;
    ov.par = par;
    ov.duracion_seg = duracion;
    ov.motivo = motivo;
    ov.expira = chrono::steady_clock::now() + chrono::seconds(duracion);
    g_overrides[inter] = ov;

    DecisionTrafico d;
    d.estadoTrafico = motivo;
    d.estadoSemaforo = par.carrera;
    d.duracion = duracion;
    actualizarEstadoInterseccion(inter, 0, 0, 0, d, par);

    enviarComandos(push, inter, par, duracion, motivo);

    cout << "[ORDEN MANUAL] " << inter << " CARRERA=" << par.carrera << " CALLE=" << par.calle
         << " (" << duracion << "s) motivo=" << motivo << endl;

    ostringstream out;
    out << "{\"ok\":true,\"interseccion\":\"" << inter << "\","
        << "\"semaforo_carrera\":\"" << par.carrera << "\","
        << "\"semaforo_calle\":\"" << par.calle << "\","
        << "\"duracion_seg\":" << duracion << ","
        << "\"motivo\":\"" << motivo << "\"}";
    return out.str();
}

bool tieneOverrideActivo(const string& interseccion) {
    auto it = g_overrides.find(interseccion);
    if (it == g_overrides.end()) return false;
    if (chrono::steady_clock::now() > it->second.expira) {
        g_overrides.erase(it);
        return false;
    }
    return true;
}

string procesarOrdenDirecta(const string& consulta, zmq::socket_t& pushSemaforos) {
    string tipo = extraerValorString(consulta, "tipo");
    string inter = extraerValorString(consulta, "interseccion");

    if (tipo == "LIBERAR_OVERRIDE") {
        g_overrides.erase(inter);
        cout << "[ORDEN MANUAL] Override liberado en " << inter << endl;
        return "{\"ok\":true,\"mensaje\":\"override liberado, vuelven reglas automaticas\"}";
    }

    if (tipo == "PRIORIZAR_AMBULANCIA") {
        string eje = extraerValorString(consulta, "eje");
        if (eje.empty()) eje = config::semaforos.eje_carrera;

        ParSemaforosInter par;
        if (eje == config::semaforos.eje_calle) {
            par.calle = "VERDE";
            par.carrera = "ROJO";
        } else {
            par.carrera = "VERDE";
            par.calle = "ROJO";
        }

        int duracion = extraerValorInt(consulta, "duracion_seg", 120);
        return aplicarOverride(inter, par, duracion, "PRIORIZACION_AMBULANCIA", pushSemaforos);
    }

    if (tipo == "FORZAR_SEMAFORO") {
        string eje = extraerValorString(consulta, "eje");
        string estado = extraerValorString(consulta, "estado");
        int duracion = extraerValorInt(consulta, "duracion_seg", 60);
        string motivo = extraerValorString(consulta, "motivo");
        if (motivo.empty()) motivo = "FORZADO_USUARIO";

        if (inter.empty() || eje.empty() || estado.empty()) {
            return "{\"error\":\"requiere interseccion, eje y estado\"}";
        }

        auto it = g_overrides.find(inter);
        ParSemaforosInter par;
        if (it != g_overrides.end()) par = it->second.par;
        else {
            par.carrera = "ROJO";
            par.calle = "ROJO";
        }

        if (eje == config::semaforos.eje_carrera) par.carrera = estado;
        else if (eje == config::semaforos.eje_calle) par.calle = estado;
        else return "{\"error\":\"eje debe ser CARRERA o CALLE\"}";

        return aplicarOverride(inter, par, duracion, motivo, pushSemaforos);
    }

    if (tipo == "FORZAR_INTERSECCION") {
        string carrera = extraerValorString(consulta, "carrera");
        string calle = extraerValorString(consulta, "calle");
        int duracion = extraerValorInt(consulta, "duracion_seg", 60);
        string motivo = extraerValorString(consulta, "motivo");
        if (motivo.empty()) motivo = "FORZADO_USUARIO";

        if (inter.empty() || carrera.empty() || calle.empty()) {
            return "{\"error\":\"requiere interseccion, carrera y calle\"}";
        }

        ParSemaforosInter par{carrera, calle};
        return aplicarOverride(inter, par, duracion, motivo, pushSemaforos);
    }

    return "";
}
