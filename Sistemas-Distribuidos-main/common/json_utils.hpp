#pragma once

#include <string>

int extraerValorInt(const std::string& json, const std::string& clave, int porDefecto = 0);
std::string extraerValorString(const std::string& json, const std::string& clave);
std::string agregarTxId(const std::string& json, int txId);
