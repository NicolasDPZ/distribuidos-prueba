#pragma once

#include <zmq.hpp>
#include <string>

std::string procesarOrdenDirecta(const std::string& consulta, zmq::socket_t& pushSemaforos);
bool tieneOverrideActivo(const std::string& interseccion);
