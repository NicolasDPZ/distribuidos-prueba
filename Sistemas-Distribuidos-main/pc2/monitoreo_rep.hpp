#pragma once

#include <libpq-fe.h>
#include <zmq.hpp>
#include <string>

std::string responderSolicitudMonitoreo(const std::string& consulta);
std::string procesarSolicitudRep(const std::string& consulta, zmq::socket_t& pushSemaforos,
                                 PGconn* pgReplica);
void enviarSyncLog(zmq::socket_t& repSocket, PGconn* pgReplica, int desdeTxId);
