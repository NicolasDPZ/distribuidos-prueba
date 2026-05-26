#pragma once

#include "config_loader.hpp"
#include <string>

namespace config {

extern ConfigRed red;
extern ConfigSemaforos semaforos;
extern ConfigMonitoreo monitoreo;
extern ConfigCiudad ciudad;

bool init(const std::string& perfil = "pc2", bool cargar_reglas = false,
          bool cargar_semaforos = false, bool cargar_monitoreo = false);

bool initCiudad();

std::string tcp(const std::string& ip, int puerto);
std::string tcpLocalhost(int puerto);
std::string tcpBind(int puerto);

}  // namespace config
