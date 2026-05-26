#include "config.hpp"

#include <iostream>

using namespace std;

namespace config {

ConfigRed red;
ConfigSemaforos semaforos;
ConfigMonitoreo monitoreo;
ConfigCiudad ciudad;

bool initCiudad() {
    string dir = directorioConfig();
    return cargarConfigCiudad(dir + "/ciudad.json", ciudad);
}

bool init(const string& perfil, bool cargar_reglas, bool cargar_semaforos, bool cargar_monitoreo) {
    string dir = directorioConfig();
    string archivoRed = (perfil == "pc3") ? dir + "/red_pc3.json" : dir + "/red.json";

    if (!cargarConfigRed(archivoRed, red)) return false;
    if (!initCiudad()) return false;

    if (cargar_reglas && !cargarConfigReglas(dir + "/reglas.json")) return false;
    if (cargar_semaforos) {
        if (!cargarConfigSemaforos(dir + "/semaforos.json", semaforos)) return false;
        aplicarInterseccionesDesdeCiudad(semaforos, ciudad);
    }
    if (cargar_monitoreo && !cargarConfigMonitoreo(dir + "/monitoreo.json", monitoreo)) return false;

    return true;
}

string tcp(const string& ip, int puerto) {
    return "tcp://" + ip + ":" + to_string(puerto);
}

string tcpLocalhost(int puerto) {
    return tcp("localhost", puerto);
}

string tcpBind(int puerto) {
    return "tcp://*:" + to_string(puerto);
}

}  // namespace config
