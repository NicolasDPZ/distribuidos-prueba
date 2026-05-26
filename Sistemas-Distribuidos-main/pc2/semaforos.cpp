#include <zmq.hpp>
#include "../common/config.hpp"
#include "../common/json_utils.hpp"
#include "semaforo_interseccion.hpp"

#include <iostream>
#include <string>
#include <map>
using namespace std;

struct Semaforo {
    string interseccion;
    string eje;
    string estado = "ROJO";
    int duracion  = 15;
};

map<string, Semaforo> semaforos;

void inicializarSemaforosDesdeConfig() {
    for (const string& inter : config::semaforos.intersecciones) {
        Semaforo base;
        base.interseccion = inter;
        base.estado = config::semaforos.estado_inicial;
        base.duracion = config::semaforos.duracion_inicial_seg;

        Semaforo carrera = base;
        carrera.eje = config::semaforos.eje_carrera;
        semaforos[claveSemaforo(inter, carrera.eje)] = carrera;

        Semaforo calle = base;
        calle.eje = config::semaforos.eje_calle;
        semaforos[claveSemaforo(inter, calle.eje)] = calle;
    }
}

int main() {
    if (!config::init("pc2", false, true, false)) return 1;

    inicializarSemaforosDesdeConfig();

    zmq::context_t context(1);

    zmq::socket_t receiver(context, ZMQ_PULL);
    receiver.connect(config::tcpLocalhost(config::red.puerto_push_semaforo));

    cout << "Servicio de semaforos: " << semaforos.size() << " luces ("
         << config::semaforos.intersecciones.size() << " intersecciones x 2 ejes)..." << endl;

    while (true) {
        zmq::message_t msg;
        receiver.recv(msg, zmq::recv_flags::none);
        string cmd(static_cast<char*>(msg.data()), msg.size());

        string interseccion = extraerValorString(cmd, "interseccion");
        string eje          = extraerValorString(cmd, "eje");
        string estado       = extraerValorString(cmd, "estado");
        string razon        = extraerValorString(cmd, "razon");
        int duracion        = extraerValorInt(cmd, "duracion_seg", config::semaforos.duracion_inicial_seg);

        if (interseccion.empty() || eje.empty()) {
            cout << "[SEMAFORO] Comando invalido (falta interseccion o eje): " << cmd << endl;
            continue;
        }

        string clave = claveSemaforo(interseccion, eje);
        Semaforo& sem = semaforos[clave];
        sem.interseccion = interseccion;
        sem.eje = eje;

        string estadoAnterior = sem.estado;
        sem.estado   = estado;
        sem.duracion = duracion;

        cout << "[SEMAFORO] " << interseccion << " [" << eje << "] "
             << estadoAnterior << " -> " << estado
             << " | " << duracion << "s | " << razon << endl;
    }
}
