#include <zmq.hpp>
#include "../common/config.hpp"
#include "../common/pg_log.hpp"
#include "../common/json_utils.hpp"
#include <iostream>
using namespace std;

int main() {
    if (!config::init("pc2", false, false, false)) return 1;

    PGconn* pg = pgConectar();
    if (!pg) return 1;

    zmq::context_t context(1);

    zmq::socket_t receiver(context, ZMQ_PULL);
    receiver.bind(config::tcpBind(config::red.puerto_pull_replica));

    cout << "BD replica PostgreSQL (PC2) esperando datos..." << endl;

    while (true) {
        zmq::message_t msg;
        receiver.recv(msg, zmq::recv_flags::none);
        string data(static_cast<char*>(msg.data()), msg.size());

        if (pgInsertarEvento(pg, data)) {
            int txId = extraerValorInt(data, "tx_id");
            cout << "Replica PG actualizada tx_id=" << txId << endl;
        }
    }

    pgCerrar(pg);
}
