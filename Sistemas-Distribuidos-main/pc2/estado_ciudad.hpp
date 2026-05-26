#pragma once

#include "reglas_trafico.hpp"
#include "semaforo_interseccion.hpp"
#include <map>
#include <string>

struct EstadoInter {
    int volumen   = 0;
    int velocidad = 0;
    int vehiculos = 0;
    std::string semaforo_carrera = "ROJO";
    std::string semaforo_calle   = "ROJO";
    std::string trafico  = "DESCONOCIDO";
};

extern std::map<std::string, EstadoInter> estadoCiudad;

void actualizarEstadoInterseccion(const std::string& interseccion, int volumen, int velocidad,
                                 int vehiculos, const DecisionTrafico& decision,
                                 const ParSemaforosInter& par);
std::string respuestaEstadoInterseccion(const std::string& interseccion);
std::string respuestaEstadoCiudad();
