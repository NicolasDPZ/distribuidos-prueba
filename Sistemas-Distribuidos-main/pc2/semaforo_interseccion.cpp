#include "semaforo_interseccion.hpp"
#include "../common/config.hpp"

using namespace std;

static string estadoOpuesto(const string& estado) {
    if (estado == "VERDE") return "ROJO";
    if (estado == "ROJO") return "VERDE";
    return "ROJO";
}

ParSemaforosInter coordinarParSemaforos(const DecisionTrafico& decision) {
    ParSemaforosInter par;
    const string& t = decision.estadoTrafico;
    const string& base = decision.estadoSemaforo;

    if (t == "CONGESTION") {
        par.carrera = "ROJO";
        par.calle   = "ROJO";
    } else if (t == "ALTA_DENSIDAD") {
        par.carrera = base;
        par.calle   = "ROJO";
    } else {
        par.carrera = base;
        par.calle   = estadoOpuesto(base);
    }
    return par;
}

string jsonComandoSemaforoEje(const string& interseccion, const string& eje,
                              const string& estado, int duracion_seg, const string& razon) {
    return "{\"comando\":\"CAMBIAR_LUZ\","
           "\"interseccion\":\"" + interseccion + "\","
           "\"eje\":\"" + eje + "\","
           "\"estado\":\"" + estado + "\","
           "\"duracion_seg\":" + to_string(duracion_seg) + ","
           "\"razon\":\"" + razon + "\"}";
}

vector<string> generarComandosSemaforoInterseccion(const string& interseccion,
                                                  const ParSemaforosInter& par,
                                                  int duracion_seg, const string& razon) {
    return {
        jsonComandoSemaforoEje(interseccion, config::semaforos.eje_carrera, par.carrera, duracion_seg, razon),
        jsonComandoSemaforoEje(interseccion, config::semaforos.eje_calle, par.calle, duracion_seg, razon)
    };
}

string claveSemaforo(const string& interseccion, const string& eje) {
    return interseccion + "|" + eje;
}
