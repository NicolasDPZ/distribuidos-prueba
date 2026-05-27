#include <zmq.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

// Contador de mensajes procesados (para estadísticas)
atomic<long long> mensajes_procesados{0};
mutex mtx_log;

void log(const string& msg) {
    lock_guard<mutex> lock(mtx_log);
    cout << msg << endl;
}

// Cada mensaje se reenvía en su propio hilo
void procesar_mensaje(zmq::context_t& context, const string& datos) {
    try {
        zmq::socket_t publisher(context, ZMQ_PUB);
        publisher.connect("tcp://localhost:5556");

        zmq::message_t msg(datos.begin(), datos.end());
        publisher.send(msg, zmq::send_flags::none);

        long long total = ++mensajes_procesados;
        if (total % 100 == 0) {
            log("[BROKER-MT] Mensajes procesados: " + to_string(total));
        }

        publisher.close();
    } catch (const zmq::error_t& e) {
        log("[BROKER-MT] Error en hilo: " + string(e.what()));
    }
}

int main() {
    // Contexto con más hilos de I/O para soportar concurrencia
    zmq::context_t context(4);

    // Socket receptor (sensores publican aquí)
    zmq::socket_t frontend(context, ZMQ_SUB);
    frontend.bind("tcp://*:5555");
    frontend.set(zmq::sockopt::subscribe, "");

    // Socket publicador (analítica se suscribe aquí)
    zmq::socket_t backend(context, ZMQ_PUB);
    backend.bind("tcp://*:5556");

    log("[BROKER-MT] Broker multihilo iniciado en puertos 5555 (IN) y 5556 (OUT)");

    while (true) {
        zmq::message_t message;
        auto result = frontend.recv(message, zmq::recv_flags::none);

        if (result) {
            // Copiar datos del mensaje antes de pasarlos al hilo
            string datos(static_cast<char*>(message.data()), message.size());

            // Lanzar hilo para reenviar el mensaje
            thread t(procesar_mensaje, ref(context), datos);
            t.detach();  // hilo independiente, no bloqueamos el loop principal
        }
    }

    return 0;
}