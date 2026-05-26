#include "db_sync.hpp"
#include "../common/config.hpp"
#include "../common/pg_log.hpp"
#include "../common/json_utils.hpp"

#include <iostream>
#include <sstream>

using namespace std;

bool aplicarEntrada(PGconn* pg, const string& linea, int& checkpoint) {
    if (linea.empty()) return false;

    int txId = extraerValorInt(linea, "tx_id");
    if (txId <= 0) {
        cout << "[PG] Entrada sin tx_id, se omite." << endl;
        return false;
    }
    if (txId <= checkpoint) {
        cout << "[PG] Duplicado tx_id=" << txId << ", se omite." << endl;
        return false;
    }

    if (!pgInsertarEvento(pg, linea) && !pgExisteTxId(pg, txId)) return false;

    checkpoint = txId;
    pgGuardarCheckpoint(pg, config::red.nodo_checkpoint_pc3, checkpoint);
    cout << "[PG] Aplicado tx_id=" << txId << endl;
    return true;
}

void sincronizarLog(zmq::socket_t& reqSync, PGconn* pg, int& checkpoint) {
    string solicitud = "{\"tipo\":\"SYNC_LOG\",\"desde_tx_id\":" + to_string(checkpoint) + "}";
    zmq::message_t req(solicitud.begin(), solicitud.end());
    reqSync.send(req, zmq::send_flags::none);
    cout << "[SYNC] Solicitando log pendiente desde tx_id=" << checkpoint << "..." << endl;

    zmq::message_t frame1;
    reqSync.recv(frame1, zmq::recv_flags::none);
    string cabecera(static_cast<char*>(frame1.data()), frame1.size());

    if (cabecera != "SYNC_OK") {
        cout << "[SYNC] Respuesta inesperada: " << cabecera << endl;
        return;
    }

    zmq::message_t frame2;
    reqSync.recv(frame2, zmq::recv_flags::none);
    string payload(static_cast<char*>(frame2.data()), frame2.size());

    if (payload.empty()) {
        cout << "[SYNC] BD principal al dia. Nada pendiente." << endl;
        return;
    }

    istringstream stream(payload);
    string linea;
    int aplicadas = 0;
    while (getline(stream, linea)) {
        if (aplicarEntrada(pg, linea, checkpoint)) aplicadas++;
    }
    cout << "[SYNC] Replay completado. Entradas aplicadas: " << aplicadas
         << ". Checkpoint=" << checkpoint << endl;
}
