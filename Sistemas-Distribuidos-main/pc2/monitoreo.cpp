#include <zmq.hpp>
#include "../common/config.hpp"
#include "../common/pg_log.hpp"
#include "../common/pg_consultas.hpp"

#include <iostream>
#include <string>
#include <limits>
using namespace std;

void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string leerLinea(const string& prompt) {
    cout << prompt;
    string s;
    getline(cin, s);
    return s;
}

string consultarAnalitica(zmq::socket_t& req, const string& mensaje) {
    zmq::message_t reqMsg(mensaje.begin(), mensaje.end());
    req.send(reqMsg, zmq::send_flags::none);

    zmq::message_t rep;
    req.recv(rep, zmq::recv_flags::none);
    return string(static_cast<char*>(rep.data()), rep.size());
}

int main() {
    if (!config::init("pc2", false, false, true)) return 1;  // carga ciudad.json

    PGconn* pg = pgConectar();
    if (!pg) {
        cerr << "No se pudo conectar a PostgreSQL (replica en PC2)." << endl;
        return 1;
    }

    zmq::context_t context(1);
    zmq::socket_t reqSocket(context, ZMQ_REQ);
    reqSocket.connect(config::tcp(config::monitoreo.conexion_analitica, config::red.puerto_rep_monitoreo));
    reqSocket.set(zmq::sockopt::rcvtimeo, 10000);

    cout << config::monitoreo.titulo << endl;
    cout << "\n--- Estado en tiempo real (analitica en esta PC) ---" << endl;
    cout << "  1 - Estado actual de una interseccion" << endl;
    cout << "  2 - Estado de toda la ciudad" << endl;
    cout << "  3 - Reglas del sistema (desde config/reglas.json)" << endl;
    cout << "\n--- Historial PostgreSQL replica (lugar / fecha) ---" << endl;
    cout << "  4 - Consultar por lugar" << endl;
    cout << "  5 - Consultar por fecha y hora" << endl;
    cout << "  6 - Consultar por lugar y fecha/hora" << endl;
    cout << "\n--- Ordenes directas a analitica ---" << endl;
    cout << "  7 - Forzar semaforo (eje CARRERA/CALLE)" << endl;
    cout << "  8 - Priorizar ambulancia (verde en un eje)" << endl;
    cout << "  9 - Liberar override manual" << endl;
    cout << "\n  0 - Salir" << endl;
    if (!config::ciudad.intersecciones_generadas.empty()) {
        cout << "Ejemplo interseccion: " << config::ciudad.intersecciones_generadas[0] << endl;
    }
    cout << "\nFormato fecha: AAAA-MM-DD o AAAA-MM-DD HH:MM:SS" << endl;

    int limite = config::monitoreo.limite_filas;

    while (true) {
        cout << "\nOpcion: ";
        int opcion;
        if (!(cin >> opcion)) break;
        limpiarEntrada();

        if (opcion == 0) break;

        if (opcion == 1) {
            string inter = leerLinea("Interseccion (ej: INT-A1): ");
            string msg = "{\"tipo\":\"ESTADO_INTERSECCION\",\"interseccion\":\"" + inter + "\"}";
            cout << "Respuesta: " << consultarAnalitica(reqSocket, msg) << endl;

        } else if (opcion == 2) {
            cout << "Respuesta: " << consultarAnalitica(reqSocket, "{\"tipo\":\"ESTADO_CIUDAD\"}") << endl;

        } else if (opcion == 3) {
            cout << "Respuesta: " << consultarAnalitica(reqSocket, "{\"tipo\":\"VER_REGLAS\"}") << endl;

        } else if (opcion == 4) {
            string inter = leerLinea("Interseccion (ej: INT-B2): ");
            if (inter.empty()) {
                cout << "Lugar invalido." << endl;
                continue;
            }
            pgConsultarPorLugar(pg, inter, limite);

        } else if (opcion == 5) {
            string desde = leerLinea("Desde (fecha u hora): ");
            string hasta = leerLinea("Hasta (fecha u hora): ");
            if (desde.empty() || hasta.empty()) {
                cout << "Debe indicar rango completo." << endl;
                continue;
            }
            pgConsultarPorFecha(pg, desde, hasta, limite);

        } else if (opcion == 6) {
            string inter = leerLinea("Interseccion (ej: INT-C3): ");
            string desde = leerLinea("Desde (fecha u hora): ");
            string hasta = leerLinea("Hasta (fecha u hora): ");
            if (inter.empty() || desde.empty() || hasta.empty()) {
                cout << "Debe indicar lugar y rango de fechas." << endl;
                continue;
            }
            pgConsultarPorLugarYFecha(pg, inter, desde, hasta, limite);

        } else if (opcion == 7) {
            string inter = leerLinea("Interseccion: ");
            string eje = leerLinea("Eje (CARRERA o CALLE): ");
            string estado = leerLinea("Estado (VERDE o ROJO): ");
            string dur = leerLinea("Duracion segundos: ");
            int d = dur.empty() ? 60 : stoi(dur);
            string msg = "{\"tipo\":\"FORZAR_SEMAFORO\",\"interseccion\":\"" + inter + "\","
                         "\"eje\":\"" + eje + "\",\"estado\":\"" + estado + "\","
                         "\"duracion_seg\":" + to_string(d) + ",\"motivo\":\"usuario\"}";
            cout << "Respuesta: " << consultarAnalitica(reqSocket, msg) << endl;

        } else if (opcion == 8) {
            string inter = leerLinea("Interseccion: ");
            string eje = leerLinea("Eje prioridad (CARRERA o CALLE, vacio=CARRERA): ");
            string dur = leerLinea("Duracion segundos (vacio=120): ");
            int d = dur.empty() ? 120 : stoi(dur);
            string msg = "{\"tipo\":\"PRIORIZAR_AMBULANCIA\",\"interseccion\":\"" + inter + "\"";
            if (!eje.empty()) msg += ",\"eje\":\"" + eje + "\"";
            msg += ",\"duracion_seg\":" + to_string(d) + "}";
            cout << "Respuesta: " << consultarAnalitica(reqSocket, msg) << endl;

        } else if (opcion == 9) {
            string inter = leerLinea("Interseccion a liberar: ");
            string msg = "{\"tipo\":\"LIBERAR_OVERRIDE\",\"interseccion\":\"" + inter + "\"}";
            cout << "Respuesta: " << consultarAnalitica(reqSocket, msg) << endl;

        } else {
            cout << "Opcion invalida." << endl;
        }
    }

    pgCerrar(pg);
    return 0;
}
