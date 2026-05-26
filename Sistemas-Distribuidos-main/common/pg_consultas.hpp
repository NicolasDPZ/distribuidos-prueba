#pragma once

#include <libpq-fe.h>
#include <string>

void pgImprimirResultados(PGconn* conn, PGresult* res);
bool pgConsultarPorLugar(PGconn* conn, const std::string& interseccion, int limite = 30);
bool pgConsultarPorFecha(PGconn* conn, const std::string& desde, const std::string& hasta, int limite = 30);
bool pgConsultarPorLugarYFecha(PGconn* conn, const std::string& interseccion,
                               const std::string& desde, const std::string& hasta, int limite = 30);

std::string normalizarFechaEntrada(const std::string& entrada, bool finDelDia = false);
