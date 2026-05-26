#include "pg_log.hpp"
#include "json_utils.hpp"
#include "config.hpp"

// config::red se rellena con config::init() antes de pgConectar()

#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace std;

string pgConninfo() {
    const char* url = getenv("PG_CONNINFO");
    if (url && url[0] != '\0') return string(url);
    if (!config::red.pg_conninfo.empty()) return config::red.pg_conninfo;
    return "host=localhost port=5432 dbname=traffic user=traffic password=traffic connect_timeout=5";
}

PGconn* pgConectar() {
    PGconn* conn = PQconnectdb(pgConninfo().c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "[PG] Error de conexion: " << PQerrorMessage(conn) << endl;
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

void pgCerrar(PGconn* conn) {
    if (conn) PQfinish(conn);
}

string escaparSql(const string& s) {
    string r;
    r.reserve(s.size());
    for (char c : s) {
        if (c == '\'') r += "''";
        else r += c;
    }
    return r;
}

bool pgInsertarEvento(PGconn* conn, const string& lineaLog) {
    if (!conn || lineaLog.empty()) return false;

    int txId = extraerValorInt(lineaLog, "tx_id");
    if (txId <= 0) {
        cerr << "[PG] linea sin tx_id valido" << endl;
        return false;
    }

    string sensorId   = extraerValorString(lineaLog, "sensor_id");
    string tipoSensor = extraerValorString(lineaLog, "tipo_sensor");
    string inter      = extraerValorString(lineaLog, "interseccion");
    int volumen       = extraerValorInt(lineaLog, "volumen");
    int velocidad     = extraerValorInt(lineaLog, "velocidad");
    int vehiculos     = extraerValorInt(lineaLog, "vehiculos_contados");
    int densidad      = extraerValorInt(lineaLog, "densidad");
    string nivel      = extraerValorString(lineaLog, "nivel_congestion");
    string eventoTs   = extraerValorString(lineaLog, "timestamp");
    if (eventoTs.empty()) eventoTs = extraerValorString(lineaLog, "timestamp_inicio");

    auto sqlInt = [](int v) {
        return v == 0 ? string("NULL") : to_string(v);
    };
    auto sqlStr = [](const string& v) {
        return v.empty() ? string("NULL") : "'" + escaparSql(v) + "'";
    };

    string eventoEn = eventoTs.empty() ? "NOW()" : "'" + escaparSql(eventoTs) + "'::timestamptz";

    ostringstream sql;
    sql << "INSERT INTO eventos_log (tx_id, linea_log, sensor_id, tipo_sensor, interseccion, "
        << "volumen, velocidad, vehiculos_contados, densidad, nivel_congestion, evento_en) VALUES ("
        << txId << ", '" << escaparSql(lineaLog) << "', "
        << sqlStr(sensorId) << ", " << sqlStr(tipoSensor) << ", " << sqlStr(inter) << ", "
        << sqlInt(volumen) << ", " << sqlInt(velocidad) << ", " << sqlInt(vehiculos) << ", "
        << sqlInt(densidad) << ", " << sqlStr(nivel) << ", " << eventoEn
        << ") ON CONFLICT (tx_id) DO NOTHING";

    PGresult* res = PQexec(conn, sql.str().c_str());
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    bool inserted = ok && atoi(PQcmdTuples(res)) > 0;
    if (!ok) {
        cerr << "[PG] Insert fallido: " << PQerrorMessage(conn) << endl;
    }
    PQclear(res);
    return inserted;
}

bool pgExisteTxId(PGconn* conn, int txId) {
    if (!conn || txId <= 0) return false;

    string q = "SELECT 1 FROM eventos_log WHERE tx_id = " + to_string(txId) + " LIMIT 1";
    PGresult* res = PQexec(conn, q.c_str());
    bool existe = PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0;
    PQclear(res);
    return existe;
}

int pgMaxTxId(PGconn* conn) {
    if (!conn) return 0;
    PGresult* res = PQexec(conn, "SELECT COALESCE(MAX(tx_id), 0) FROM eventos_log");
    int maxId = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        maxId = atoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    return maxId;
}

string pgPayloadSync(PGconn* conn, int desdeTxId) {
    if (!conn) return "";

    string query =
        "SELECT linea_log FROM eventos_log WHERE tx_id > " + to_string(desdeTxId) +
        " ORDER BY tx_id ASC";

    PGresult* res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "[PG] Sync query fallida: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return "";
    }

    ostringstream payload;
    int filas = PQntuples(res);
    for (int i = 0; i < filas; i++) {
        if (i > 0) payload << "\n";
        payload << PQgetvalue(res, i, 0);
    }
    PQclear(res);
    return payload.str();
}

int pgLeerCheckpoint(PGconn* conn, const string& nodo) {
    if (!conn) return 0;

    string q =
        "SELECT ultimo_tx_id FROM sistema_checkpoint WHERE nodo = '" +
        escaparSql(nodo) + "'";

    PGresult* res = PQexec(conn, q.c_str());
    int valor = 0;

    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        valor = atoi(PQgetvalue(res, 0, 0));
    } else {
        string ins =
            "INSERT INTO sistema_checkpoint (nodo, ultimo_tx_id) VALUES ('" +
            escaparSql(nodo) + "', 0) ON CONFLICT (nodo) DO NOTHING";
        PQclear(PQexec(conn, ins.c_str()));
    }
    PQclear(res);
    return valor;
}

bool pgGuardarCheckpoint(PGconn* conn, const string& nodo, int txId) {
    if (!conn) return false;

    string sql =
        "INSERT INTO sistema_checkpoint (nodo, ultimo_tx_id, actualizado_en) VALUES ('" +
        escaparSql(nodo) + "', " + to_string(txId) + ", NOW()) "
        "ON CONFLICT (nodo) DO UPDATE SET ultimo_tx_id = EXCLUDED.ultimo_tx_id, "
        "actualizado_en = NOW()";

    PGresult* res = PQexec(conn, sql.c_str());
    bool ok = PQresultStatus(res) == PGRES_COMMAND_OK;
    if (!ok) cerr << "[PG] Checkpoint fallido: " << PQerrorMessage(conn) << endl;
    PQclear(res);
    return ok;
}
