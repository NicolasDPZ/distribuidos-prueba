#pragma once

#include <libpq-fe.h>
#include <string>

std::string pgConninfo();
PGconn* pgConectar();
void pgCerrar(PGconn* conn);

std::string escaparSql(const std::string& s);

bool pgInsertarEvento(PGconn* conn, const std::string& lineaLog);
bool pgExisteTxId(PGconn* conn, int txId);
int pgMaxTxId(PGconn* conn);
std::string pgPayloadSync(PGconn* conn, int desdeTxId);

int pgLeerCheckpoint(PGconn* conn, const std::string& nodo);
bool pgGuardarCheckpoint(PGconn* conn, const std::string& nodo, int txId);
