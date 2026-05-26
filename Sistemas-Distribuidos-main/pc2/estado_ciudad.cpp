#include "estado_ciudad.hpp"
#include "../common/config.hpp"

using namespace std;

map<string, EstadoInter> estadoCiudad;

void actualizarEstadoInterseccion(const string& interseccion, int volumen, int velocidad,
                                  int vehiculos, const DecisionTrafico& decision,
                                  const ParSemaforosInter& par) {
    estadoCiudad[interseccion] = {volumen, velocidad, vehiculos,
                                  par.carrera, par.calle, decision.estadoTrafico};
}

string respuestaEstadoInterseccion(const string& interseccion) {
    auto it = estadoCiudad.find(interseccion);
    if (it == estadoCiudad.end()) {
        return "{\"error\":\"interseccion no encontrada, espere datos del sensor\"}";
    }
    auto& e = it->second;
    return "{\"interseccion\":\"" + interseccion + "\","
           "\"volumen\":" + to_string(e.volumen) + ","
           "\"velocidad\":" + to_string(e.velocidad) + ","
           "\"vehiculos\":" + to_string(e.vehiculos) + ","
           "\"semaforo_carrera\":\"" + e.semaforo_carrera + "\","
           "\"semaforo_calle\":\"" + e.semaforo_calle + "\","
           "\"trafico\":\"" + e.trafico + "\"}";
}

string respuestaEstadoCiudad() {
    int n = totalIntersecciones(config::ciudad);
    int filas = static_cast<int>(config::ciudad.filas.size());
    return "{\"ok\":true,\"matriz\":\"" + to_string(filas) + "x" +
           to_string(config::ciudad.num_columnas) + "\","
           "\"intersecciones\":" + to_string(n) + ","
           "\"semaforos_por_interseccion\":2,"
           "\"mensaje\":\"ciudad dinamica, 2 semaforos (carrera y calle) por interseccion\"}";
}
