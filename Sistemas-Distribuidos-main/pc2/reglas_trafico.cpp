#include "reglas_trafico.hpp"
#include "../common/config_loader.hpp"

#include <iostream>
#include <sstream>

using namespace std;

static bool cumpleRegla(const ReglaConfig& r, int volumen, int velocidad, int vehiculos) {
    bool okVolumenMax = r.volumen_max < 0 || volumen <= r.volumen_max;
    bool okVolumenMin = r.volumen_min < 0 || volumen >= r.volumen_min;
    bool okVelMin = r.velocidad_min < 0 || velocidad >= r.velocidad_min;
    bool okVelMax = r.velocidad_max < 0 || velocidad <= r.velocidad_max;
    bool okVehMax = r.vehiculos_max < 0 || vehiculos <= r.vehiculos_max;
    bool okVehMin = r.vehiculos_min < 0 || vehiculos >= r.vehiculos_min;

    if (r.logica_or) {
        bool alguna = false;
        if (r.volumen_min >= 0 && volumen >= r.volumen_min) alguna = true;
        if (r.velocidad_max >= 0 && velocidad <= r.velocidad_max) alguna = true;
        return alguna;
    }

    return okVolumenMax && okVolumenMin && okVelMin && okVelMax && okVehMax && okVehMin;
}

static DecisionTrafico desdeRegla(const ReglaConfig& r) {
    DecisionTrafico d;
    d.estadoTrafico = r.nombre;
    d.estadoSemaforo = r.semaforo;
    d.duracion = r.duracion_seg;
    return d;
}

DecisionTrafico evaluarReglas(int volumen, int velocidad, int vehiculos) {
    for (const auto& r : reglasCargadas()) {
        if (cumpleRegla(r, volumen, velocidad, vehiculos)) {
            return desdeRegla(r);
        }
    }
    return desdeRegla(reglaPorDefecto());
}

void imprimirDecision(const string& interseccion, const DecisionTrafico& d,
                      const string& carrera, const string& calle) {
    cout << "[ANALITICA] " << interseccion << " -> " << d.estadoTrafico
         << " | CARRERA=" << carrera << " CALLE=" << calle
         << " (" << d.duracion << "s)" << endl;
}

string jsonReglasSistema() {
    ostringstream out;
    out << "{\"reglas\":{";
    bool primero = true;
    for (const auto& r : reglasCargadas()) {
        if (!primero) out << ",";
        out << "\"" << r.nombre << "\":\"" << r.descripcion << "\"";
        primero = false;
    }
    const auto& def = reglaPorDefecto();
    if (!primero) out << ",";
    out << "\"" << def.nombre << "\":\"" << def.descripcion << "\"";
    out << "}}";
    return out.str();
}
