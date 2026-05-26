#include "monitoreo_rep.hpp"
#include "estado_ciudad.hpp"
#include "reglas_trafico.hpp"
#include "ordenes_manual.hpp"
#include "../common/pg_log.hpp"
#include "../common/json_utils.hpp"

#include <iostream>
#include <sstream>

using namespace std;

string responderSolicitudMonitoreo(const string& consulta) {
    string tipo = extraerValorString(consulta, "tipo");

    if (tipo == "ESTADO_INTERSECCION") {
        return respuestaEstadoInterseccion(extraerValorString(consulta, "interseccion"));
    }
    if (tipo == "ESTADO_CIUDAD") {
        return respuestaEstadoCiudad();
    }
    if (tipo == "VER_REGLAS") {
        return jsonReglasSistema();
    }
    return "{\"error\":\"comando desconocido\"}";
}

string procesarSolicitudRep(const string& consulta, zmq::socket_t& pushSemaforos,
                            PGconn* pgReplica) {
    string tipo = extraerValorString(consulta, "tipo");

    if (tipo == "SYNC_LOG") {
        return "";
    }

    string orden = procesarOrdenDirecta(consulta, pushSemaforos);
    if (!orden.empty()) return orden;

    return responderSolicitudMonitoreo(consulta);
}

void enviarSyncLog(zmq::socket_t& repSocket, PGconn* pgReplica, int desdeTxId) {
    string payload = pgPayloadSync(pgReplica, desdeTxId);
    int pendientes = 0;
    if (!payload.empty()) {
        istringstream s(payload);
        string tmp;
        while (getline(s, tmp)) pendientes++;
    }

    zmq::message_t cab("SYNC_OK", 7);
    zmq::message_t cuerpo(payload.begin(), payload.end());
    repSocket.send(cab, zmq::send_flags::sndmore);
    repSocket.send(cuerpo, zmq::send_flags::none);
    cout << "[SYNC] Enviadas " << pendientes << " entradas desde tx_id>" << desdeTxId << endl;
}
