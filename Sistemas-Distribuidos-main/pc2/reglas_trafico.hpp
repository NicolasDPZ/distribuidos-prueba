#pragma once

#include <string>

struct DecisionTrafico {
    std::string estadoTrafico;
    std::string estadoSemaforo;
    int duracion = 30;
};

DecisionTrafico evaluarReglas(int volumen, int velocidad, int vehiculos);
void imprimirDecision(const std::string& interseccion, const DecisionTrafico& d,
                      const std::string& carrera, const std::string& calle);
std::string jsonReglasSistema();
