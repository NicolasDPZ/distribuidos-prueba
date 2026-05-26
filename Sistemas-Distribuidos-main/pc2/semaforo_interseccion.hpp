#pragma once

#include "reglas_trafico.hpp"
#include <string>
#include <vector>

struct ParSemaforosInter {
    std::string carrera;
    std::string calle;
};

ParSemaforosInter coordinarParSemaforos(const DecisionTrafico& decision);
std::string jsonComandoSemaforoEje(const std::string& interseccion, const std::string& eje,
                                   const std::string& estado, int duracion_seg,
                                   const std::string& razon);
std::vector<std::string> generarComandosSemaforoInterseccion(
    const std::string& interseccion, const ParSemaforosInter& par,
    int duracion_seg, const std::string& razon);

std::string claveSemaforo(const std::string& interseccion, const std::string& eje);
