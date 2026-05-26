#include "config_loader.hpp"
#include "json_utils.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

static vector<string> g_ordenReglas;
static vector<ReglaConfig> g_reglas;
static ReglaConfig g_reglaDefecto;

string directorioConfig() {
    const char* dir = getenv("CONFIG_DIR");
    if (dir && dir[0] != '\0') return string(dir);
    return "config";
}

string leerArchivoTexto(const string& ruta) {
    ifstream f(ruta);
    if (!f.is_open()) return "";
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

string extraerBloqueJson(const string& json, const string& clave) {
    string key = "\"" + clave + "\"";
    size_t pos = json.find(key);
    if (pos == string::npos) return "";
    pos = json.find('{', pos);
    if (pos == string::npos) return "";

    int depth = 0;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '{') depth++;
        if (json[i] == '}') {
            depth--;
            if (depth == 0) return json.substr(pos, i - pos + 1);
        }
    }
    return "";
}

static vector<string> extraerOrdenEvaluacion(const string& json) {
    vector<string> orden;
    size_t pos = json.find("\"orden_evaluacion\"");
    if (pos == string::npos) return orden;
    pos = json.find('[', pos);
    size_t fin = json.find(']', pos);
    if (pos == string::npos || fin == string::npos) return orden;

    string slice = json.substr(pos + 1, fin - pos - 1);
    size_t i = 0;
    while (i < slice.size()) {
        size_t q1 = slice.find('"', i);
        if (q1 == string::npos) break;
        size_t q2 = slice.find('"', q1 + 1);
        if (q2 == string::npos) break;
        orden.push_back(slice.substr(q1 + 1, q2 - q1 - 1));
        i = q2 + 1;
    }
    return orden;
}

static vector<string> extraerListaJson(const string& json, const string& clave) {
    vector<string> lista;
    size_t pos = json.find("\"" + clave + "\"");
    if (pos == string::npos) return lista;
    pos = json.find('[', pos);
    size_t fin = json.find(']', pos);
    if (pos == string::npos || fin == string::npos) return lista;

    string slice = json.substr(pos + 1, fin - pos - 1);
    size_t i = 0;
    while (i < slice.size()) {
        size_t q1 = slice.find('"', i);
        if (q1 == string::npos) break;
        size_t q2 = slice.find('"', q1 + 1);
        if (q2 == string::npos) break;
        lista.push_back(slice.substr(q1 + 1, q2 - q1 - 1));
        i = q2 + 1;
    }
    return lista;
}

static vector<string> extraerListaIntersecciones(const string& json) {
    return extraerListaJson(json, "intersecciones");
}

string formatearInterseccion(const ConfigCiudad& ciudad, const string& fila, int columna) {
    string r = ciudad.formato_interseccion;
    const string filaTok = "{fila}";
    const string colTok = "{columna}";
    size_t p = r.find(filaTok);
    if (p != string::npos) r.replace(p, filaTok.size(), fila);
    p = r.find(colTok);
    if (p != string::npos) r.replace(p, colTok.size(), to_string(columna));
    return r;
}

void generarInterseccionesDesdeMatriz(ConfigCiudad& ciudad) {
    ciudad.intersecciones_generadas.clear();
    for (const string& fila : ciudad.filas) {
        for (int c = 0; c < ciudad.num_columnas; c++) {
            int columna = ciudad.columna_inicio + c;
            ciudad.intersecciones_generadas.push_back(formatearInterseccion(ciudad, fila, columna));
        }
    }
}

int totalIntersecciones(const ConfigCiudad& ciudad) {
    return static_cast<int>(ciudad.intersecciones_generadas.size());
}

bool interseccionValida(const ConfigCiudad& ciudad, const string& interseccion) {
    for (const string& i : ciudad.intersecciones_generadas) {
        if (i == interseccion) return true;
    }
    return false;
}

bool cargarConfigCiudad(const string& ruta, ConfigCiudad& out) {
    string json = leerArchivoTexto(ruta);
    if (json.empty()) {
        cerr << "[CONFIG] No se pudo leer ciudad: " << ruta << endl;
        return false;
    }

    out.filas = extraerListaJson(json, "filas");
    out.columna_inicio = extraerValorInt(json, "columna_inicio", 1);
    out.num_columnas = extraerValorInt(json, "num_columnas", 4);
    if (out.num_columnas < 1) out.num_columnas = 1;

    string formato = extraerValorString(json, "formato_interseccion");
    if (!formato.empty()) out.formato_interseccion = formato;

    out.intervalo_camara_gps_seg = extraerValorInt(json, "intervalo_camara_gps_seg", 5);
    out.intervalo_espiras_seg = extraerValorInt(json, "intervalo_espiras_seg", 30);

    generarInterseccionesDesdeMatriz(out);
    cout << "[CONFIG] Ciudad dinamica: " << out.filas.size() << " filas x "
         << out.num_columnas << " columnas = " << out.intersecciones_generadas.size()
         << " intersecciones" << endl;
    return !out.filas.empty();
}

bool cargarReglaDesdeBloque(const string& bloque, const string& nombre, ReglaConfig& out) {
    if (bloque.empty()) return false;
    out.nombre = nombre;
    out.descripcion = extraerValorString(bloque, "descripcion");
    out.semaforo = extraerValorString(bloque, "semaforo");
    out.duracion_seg = extraerValorInt(bloque, "duracion_seg", 30);
    out.volumen_max = extraerValorInt(bloque, "volumen_max", -1);
    out.volumen_min = extraerValorInt(bloque, "volumen_min", -1);
    out.velocidad_min = extraerValorInt(bloque, "velocidad_min", -1);
    out.velocidad_max = extraerValorInt(bloque, "velocidad_max", -1);
    out.vehiculos_max = extraerValorInt(bloque, "vehiculos_max", -1);
    out.vehiculos_min = extraerValorInt(bloque, "vehiculos_min", -1);
    string logica = extraerValorString(bloque, "logica");
    out.logica_or = (logica == "OR");
    return !out.semaforo.empty();
}

bool cargarConfigRed(const string& ruta, ConfigRed& out) {
    string json = leerArchivoTexto(ruta);
    if (json.empty()) {
        cerr << "[CONFIG] No se pudo leer " << ruta << endl;
        return false;
    }

    out.pc1_ip = extraerValorString(json, "pc1_ip");
    out.pc2_ip = extraerValorString(json, "pc2_ip");
    out.pc3_ip = extraerValorString(json, "pc3_ip");
    out.puerto_sensores_pub = extraerValorInt(json, "puerto_sensores_pub", out.puerto_sensores_pub);
    out.puerto_broker_pub = extraerValorInt(json, "puerto_broker_pub", out.puerto_broker_pub);
    out.puerto_push_bd = extraerValorInt(json, "puerto_push_bd", out.puerto_push_bd);
    out.puerto_pull_replica = extraerValorInt(json, "puerto_pull_replica", out.puerto_pull_replica);
    out.puerto_push_semaforo = extraerValorInt(json, "puerto_push_semaforo", out.puerto_push_semaforo);
    out.puerto_rep_monitoreo = extraerValorInt(json, "puerto_rep_monitoreo", out.puerto_rep_monitoreo);
    out.puerto_heartbeat = extraerValorInt(json, "puerto_heartbeat", out.puerto_heartbeat);
    out.failover_timeout_seg = extraerValorInt(json, "failover_timeout_seg", out.failover_timeout_seg);
    out.sync_intervalo_seg = extraerValorInt(json, "sync_intervalo_seg", out.sync_intervalo_seg);
    out.nodo_checkpoint_pc3 = extraerValorString(json, "nodo_checkpoint_pc3");
    if (out.nodo_checkpoint_pc3.empty()) out.nodo_checkpoint_pc3 = "pc3_principal";
    out.pg_conninfo = extraerValorString(json, "pg_conninfo");

    cout << "[CONFIG] Red cargada desde " << ruta << endl;
    return true;
}

bool cargarConfigReglas(const string& ruta) {
    string json = leerArchivoTexto(ruta);
    if (json.empty()) return false;

    g_ordenReglas = extraerOrdenEvaluacion(json);
    string bloqueReglas = extraerBloqueJson(json, "reglas");
    g_reglas.clear();

    for (const string& nombre : g_ordenReglas) {
        string bloque = extraerBloqueJson(bloqueReglas, nombre);
        ReglaConfig regla;
        if (cargarReglaDesdeBloque(bloque, nombre, regla)) {
            g_reglas.push_back(regla);
        }
    }

    string bloqueDef = extraerBloqueJson(json, "por_defecto");
    g_reglaDefecto = ReglaConfig();
    if (!bloqueDef.empty()) {
        cargarReglaDesdeBloque(bloqueDef, extraerValorString(bloqueDef, "nombre"), g_reglaDefecto);
        if (g_reglaDefecto.nombre.empty()) g_reglaDefecto.nombre = "MODERADO";
    } else {
        g_reglaDefecto.nombre = "MODERADO";
        g_reglaDefecto.semaforo = "VERDE";
        g_reglaDefecto.duracion_seg = 30;
    }

    cout << "[CONFIG] Reglas cargadas: " << g_reglas.size() << " + defecto" << endl;
    return !g_reglas.empty();
}

bool cargarConfigSemaforos(const string& ruta, ConfigSemaforos& out) {
    string json = leerArchivoTexto(ruta);
    if (json.empty()) return false;

    out.estado_inicial = extraerValorString(json, "estado_inicial");
    if (out.estado_inicial.empty()) out.estado_inicial = "ROJO";
    out.duracion_inicial_seg = extraerValorInt(json, "duracion_inicial_seg", 15);
    out.eje_carrera = extraerValorString(json, "eje_carrera");
    out.eje_calle = extraerValorString(json, "eje_calle");
    if (out.eje_carrera.empty()) out.eje_carrera = "CARRERA";
    if (out.eje_calle.empty()) out.eje_calle = "CALLE";
    out.intersecciones = extraerListaIntersecciones(json);

    cout << "[CONFIG] Semaforos: " << out.intersecciones.size() << " intersecciones x 2 ejes ("
         << out.eje_carrera << ", " << out.eje_calle << ")" << endl;
    return true;
}

void aplicarInterseccionesDesdeCiudad(ConfigSemaforos& sem, const ConfigCiudad& ciudad) {
    if (sem.intersecciones.empty() && !ciudad.intersecciones_generadas.empty()) {
        sem.intersecciones = ciudad.intersecciones_generadas;
        cout << "[CONFIG] Intersecciones de semaforos tomadas de ciudad.json ("
             << sem.intersecciones.size() << ")" << endl;
    }
}

bool cargarConfigMonitoreo(const string& ruta, ConfigMonitoreo& out) {
    string json = leerArchivoTexto(ruta);
    if (json.empty()) return false;

    out.titulo = extraerValorString(json, "titulo");
    out.limite_filas = extraerValorInt(json, "limite_filas", 30);
    out.conexion_analitica = extraerValorString(json, "conexion_analitica");
    if (out.conexion_analitica.empty()) out.conexion_analitica = "localhost";

    cout << "[CONFIG] Monitoreo cargado desde " << ruta << endl;
    return true;
}

const vector<ReglaConfig>& reglasCargadas() { return g_reglas; }
const ReglaConfig& reglaPorDefecto() { return g_reglaDefecto; }
