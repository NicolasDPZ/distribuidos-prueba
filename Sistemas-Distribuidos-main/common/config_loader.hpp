#pragma once

#include <string>
#include <vector>

struct ReglaConfig {
    std::string nombre;
    std::string descripcion;
    std::string semaforo;
    int duracion_seg = 30;
    int volumen_max = -1;
    int volumen_min = -1;
    int velocidad_min = -1;
    int velocidad_max = -1;
    int vehiculos_max = -1;
    int vehiculos_min = -1;
    bool logica_or = false;
};

struct ConfigRed {
    std::string pc1_ip = "10.43.100.43";
    std::string pc2_ip = "10.43.100.176";
    std::string pc3_ip = "10.43.99.111";
    int puerto_sensores_pub = 5555;
    int puerto_broker_pub = 5556;
    int puerto_push_bd = 5558;
    int puerto_pull_replica = 5559;
    int puerto_push_semaforo = 5557;
    int puerto_rep_monitoreo = 5560;
    int puerto_heartbeat = 5561;
    int failover_timeout_seg = 12;
    int sync_intervalo_seg = 30;
    std::string nodo_checkpoint_pc3 = "pc3_principal";
    std::string pg_conninfo;
};

struct ConfigSemaforos {
    std::string estado_inicial = "ROJO";
    int duracion_inicial_seg = 15;
    std::string eje_carrera = "CARRERA";
    std::string eje_calle = "CALLE";
    std::vector<std::string> intersecciones;
};

struct ConfigMonitoreo {
    std::string titulo = "Monitoreo";
    int limite_filas = 30;
    std::string conexion_analitica = "localhost";
};

struct ConfigCiudad {
    std::vector<std::string> filas;
    int columna_inicio = 1;
    int num_columnas = 4;
    std::string formato_interseccion = "INT_{fila}{columna}";
    int intervalo_camara_gps_seg = 5;
    int intervalo_espiras_seg = 30;
    std::vector<std::string> intersecciones_generadas;
};

std::string formatearInterseccion(const ConfigCiudad& ciudad, const std::string& fila, int columna);
void generarInterseccionesDesdeMatriz(ConfigCiudad& ciudad);
int totalIntersecciones(const ConfigCiudad& ciudad);
bool interseccionValida(const ConfigCiudad& ciudad, const std::string& interseccion);
void aplicarInterseccionesDesdeCiudad(ConfigSemaforos& sem, const ConfigCiudad& ciudad);

std::string directorioConfig();
std::string leerArchivoTexto(const std::string& ruta);
std::string extraerBloqueJson(const std::string& json, const std::string& clave);

bool cargarConfigRed(const std::string& ruta, ConfigRed& out);
bool cargarConfigReglas(const std::string& ruta);
bool cargarConfigSemaforos(const std::string& ruta, ConfigSemaforos& out);
bool cargarConfigMonitoreo(const std::string& ruta, ConfigMonitoreo& out);
bool cargarConfigCiudad(const std::string& ruta, ConfigCiudad& out);

bool cargarReglaDesdeBloque(const std::string& bloque, const std::string& nombre, ReglaConfig& out);

const std::vector<ReglaConfig>& reglasCargadas();
const ReglaConfig& reglaPorDefecto();
