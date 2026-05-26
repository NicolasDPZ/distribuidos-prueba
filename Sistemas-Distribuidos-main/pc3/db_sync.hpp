#pragma once

#include <libpq-fe.h>
#include <zmq.hpp>
#include <string>

bool aplicarEntrada(PGconn* pg, const std::string& linea, int& checkpoint);
void sincronizarLog(zmq::socket_t& reqSync, PGconn* pg, int& checkpoint);
