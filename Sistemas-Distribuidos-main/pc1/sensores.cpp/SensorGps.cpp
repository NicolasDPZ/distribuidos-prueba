#include <iostream>
#include <zmq.hpp>
#include "../../common/config.hpp"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <thread>
using namespace std;

string obtenerTimestamp() {
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm* gmt = gmtime(&t);
    stringstream ss;
    ss << put_time(gmt, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

int main() {
    if (!config::initCiudad()) return 1;

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PUB);
    socket.connect("tcp://localhost:5555");

    srand(static_cast<unsigned>(time(nullptr)));

    int intervalo = config::ciudad.intervalo_camara_gps_seg;
    cout << "GPS: matriz " << config::ciudad.filas.size() << "x"
         << config::ciudad.num_columnas << endl;

    while (true) {
        for (const string& fila : config::ciudad.filas) {
            for (int c = 0; c < config::ciudad.num_columnas; c++) {
                int columna = config::ciudad.columna_inicio + c;
                string interseccion = formatearInterseccion(config::ciudad, fila, columna);
                string sensorID = "GPS-" + fila + to_string(columna);
                int velocidad = rand() % 80;
                int densidad = rand() % 50;

                string nivel;
                if (velocidad < 10) nivel = "ALTA";
                else if (velocidad <= 39) nivel = "NORMAL";
                else nivel = "BAJA";

                string evento = "{"
                    "\"sensor_id\":\"" + sensorID + "\","
                    "\"tipo_sensor\":\"gps\","
                    "\"interseccion\":\"" + interseccion + "\","
                    "\"nivel_congestion\":\"" + nivel + "\","
                    "\"velocidad_promedio\":" + to_string(velocidad) + ","
                    "\"densidad\":" + to_string(densidad) + ","
                    "\"timestamp\":\"" + obtenerTimestamp() + "\""
                    "}";

                zmq::message_t msg(evento.begin(), evento.end());
                socket.send(msg, zmq::send_flags::none);
                cout << "Enviado: " << evento << endl;
            }
        }
        this_thread::sleep_for(chrono::seconds(intervalo));
    }
}
