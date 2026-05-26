#include <zmq.hpp>
#include "../common/config.hpp"
#include "../common/pg_log.hpp"
#include "../common/json_utils.hpp"
#include "reglas_trafico.hpp"
#include "failover_pc3.hpp"
#include "estado_ciudad.hpp"
#include "monitoreo_rep.hpp"
#include "ordenes_manual.hpp"
#include "semaforo_interseccion.hpp"

#include <iostream>
#include <string>
using namespace std;

PGconn* pgReplica = nullptr;

int main() {
    if (!config::init("pc2", true, false, false)) return 1;

    pgReplica = pgConectar();
    if (!pgReplica) return 1;

    zmq::context_t context(1);

    zmq::socket_t subSocket(context, ZMQ_SUB);
    subSocket.connect(config::tcp(config::red.pc1_ip, config::red.puerto_broker_pub));
    subSocket.set(zmq::sockopt::subscribe, "");

    zmq::socket_t pushMain(context, ZMQ_PUSH);
    pushMain.bind(config::tcpBind(config::red.puerto_push_bd));

    zmq::socket_t pushReplica(context, ZMQ_PUSH);
    pushReplica.connect(config::tcpLocalhost(config::red.puerto_pull_replica));

    zmq::socket_t pushSemaforos(context, ZMQ_PUSH);
    pushSemaforos.bind(config::tcpBind(config::red.puerto_push_semaforo));

    zmq::socket_t repMonitoreo(context, ZMQ_REP);
    repMonitoreo.bind(config::tcpBind(config::red.puerto_rep_monitoreo));

    zmq::socket_t subHeartbeat(context, ZMQ_SUB);
    subHeartbeat.connect(config::tcp(config::red.pc3_ip, config::red.puerto_heartbeat));
    subHeartbeat.set(zmq::sockopt::subscribe, "");

    int siguienteTxId = pgMaxTxId(pgReplica);
    cout << "Analitica encendida. Ciudad: " << config::ciudad.filas.size() << "x"
         << config::ciudad.num_columnas << " = " << totalIntersecciones(config::ciudad)
         << " intersecciones. tx_id=" << (siguienteTxId + 1) << endl;

    FailoverPc3 failover;

    zmq::pollitem_t items[] = {
        {subSocket,    0, ZMQ_POLLIN, 0},
        {repMonitoreo, 0, ZMQ_POLLIN, 0},
        {subHeartbeat, 0, ZMQ_POLLIN, 0}
    };

    while (true) {
        zmq::poll(items, 3, 100);

        if (items[2].revents & ZMQ_POLLIN) {
            zmq::message_t hb;
            subHeartbeat.recv(hb, zmq::recv_flags::none);
            failover.registrarHeartbeat();
        }

        failover.verificarTimeout();

        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            subSocket.recv(msg, zmq::recv_flags::none);
            string texto(static_cast<char*>(msg.data()), msg.size());
            cout << "\n[EVENTO] " << texto << endl;

            string interseccion = extraerValorString(texto, "interseccion");
            if (!interseccionValida(config::ciudad, interseccion)) {
                cout << "[ANALITICA] Interseccion fuera de matriz: " << interseccion << endl;
                continue;
            }
            if (tieneOverrideActivo(interseccion)) {
                cout << "[ANALITICA] " << interseccion
                     << " con orden manual activa; se omiten reglas automaticas." << endl;
                continue;
            }

            int volumen         = extraerValorInt(texto, "volumen");
            int velocidad       = extraerValorInt(texto, "velocidad");
            if (velocidad == 0) velocidad = extraerValorInt(texto, "velocidad_promedio");
            int vehiculos       = extraerValorInt(texto, "vehiculos_contados");

            DecisionTrafico decision = evaluarReglas(volumen, velocidad, vehiculos);
            ParSemaforosInter par = coordinarParSemaforos(decision);
            imprimirDecision(interseccion, decision, par.carrera, par.calle);
            actualizarEstadoInterseccion(interseccion, volumen, velocidad, vehiculos, decision, par);

            siguienteTxId++;
            string entradaLog = agregarTxId(texto, siguienteTxId);
            cout << "[LOG] tx_id=" << siguienteTxId << " -> replica PostgreSQL." << endl;

            auto comandos = generarComandosSemaforoInterseccion(
                interseccion, par, decision.duracion, decision.estadoTrafico);

            zmq::message_t mLog(entradaLog.begin(), entradaLog.end());
            pushReplica.send(mLog, zmq::send_flags::none);

            if (failover.activo) {
                pushMain.send(mLog, zmq::send_flags::dontwait);
                cout << "[ANALITICA] Enviado a BD principal (PC3), replica y semaforos." << endl;
            } else {
                cout << "[ANALITICA] PC3 inactivo. Solo replica hasta recuperacion." << endl;
            }

            for (const string& cmd : comandos) {
                zmq::message_t cmdMsg(cmd.begin(), cmd.end());
                pushSemaforos.send(cmdMsg, zmq::send_flags::none);
            }
        }

        if (items[1].revents & ZMQ_POLLIN) {
            zmq::message_t req;
            repMonitoreo.recv(req, zmq::recv_flags::none);
            string consulta(static_cast<char*>(req.data()), req.size());
            cout << "\n[REQ] " << consulta << endl;

            string tipo = extraerValorString(consulta, "tipo");

            if (tipo == "SYNC_LOG") {
                int desde = extraerValorInt(consulta, "desde_tx_id");
                enviarSyncLog(repMonitoreo, pgReplica, desde);
            } else {
                string respuesta = procesarSolicitudRep(consulta, pushSemaforos, pgReplica);
                zmq::message_t rep(respuesta.begin(), respuesta.end());
                repMonitoreo.send(rep, zmq::send_flags::none);
                cout << "[REP] " << respuesta << endl;
            }
        }
    }
}
