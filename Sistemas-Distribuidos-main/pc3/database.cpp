#include <zmq.hpp>
#include "../common/config.hpp"
#include "../common/pg_log.hpp"
#include "db_sync.hpp"

#include <iostream>
#include <chrono>
using namespace std;

int main() {
    if (!config::init("pc3", false, false, false)) return 1;

    PGconn* pg = pgConectar();
    if (!pg) return 1;

    zmq::context_t context(1);

    zmq::socket_t receiver(context, ZMQ_PULL);
    receiver.connect(config::tcp(config::red.pc2_ip, config::red.puerto_push_bd));
    receiver.set(zmq::sockopt::rcvtimeo, 1000);

    zmq::socket_t heartbeat(context, ZMQ_PUB);
    heartbeat.bind(config::tcpBind(config::red.puerto_heartbeat));

    zmq::socket_t reqSync(context, ZMQ_REQ);
    reqSync.connect(config::tcp(config::red.pc2_ip, config::red.puerto_rep_monitoreo));
    reqSync.set(zmq::sockopt::rcvtimeo, 15000);

    int checkpoint = pgLeerCheckpoint(pg, config::red.nodo_checkpoint_pc3);
    cout << "BD principal PostgreSQL (PC3). Checkpoint tx_id=" << checkpoint << endl;

    sincronizarLog(reqSync, pg, checkpoint);

    auto ultimaSync = chrono::steady_clock::now();

    while (true) {
        string ping = "ALIVE";
        zmq::message_t hb(ping.begin(), ping.end());
        heartbeat.send(hb, zmq::send_flags::none);

        auto ahora = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::seconds>(ahora - ultimaSync).count() >= config::red.sync_intervalo_seg) {
            sincronizarLog(reqSync, pg, checkpoint);
            ultimaSync = ahora;
        }

        zmq::message_t msg;
        auto result = receiver.recv(msg, zmq::recv_flags::none);
        if (result) {
            string data(static_cast<char*>(msg.data()), msg.size());
            aplicarEntrada(pg, data, checkpoint);
        }
    }

    pgCerrar(pg);
}
