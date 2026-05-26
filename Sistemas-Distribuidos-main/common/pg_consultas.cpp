#include "pg_consultas.hpp"
#include "pg_log.hpp"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

string normalizarFechaEntrada(const string& entrada, bool finDelDia) {
    string s = entrada;
    for (char& c : s) {
        if (c == 'T') c = ' ';
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == 'Z' || s.back() == 'z')) {
        s.pop_back();
    }

    if (s.size() == 10) {
        s += finDelDia ? " 23:59:59" : " 00:00:00";
    } else if (s.size() == 16) {
        s += ":00";
    }
    return s;
}

void pgImprimirResultados(PGconn* conn, PGresult* res) {
    if (!res) return;

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "[PG] Error en consulta: " << PQerrorMessage(conn) << endl;
        return;
    }

    int filas = PQntuples(res);
    int cols  = PQnfields(res);

    if (filas == 0) {
        cout << "Sin resultados para esos criterios." << endl;
        return;
    }

    cout << "\n--- Resultados (" << filas << " filas) ---\n";
    for (int c = 0; c < cols; c++) {
        cout << left << setw(18) << PQfname(res, c);
    }
    cout << "\n" << string(cols * 18, '-') << "\n";

    for (int i = 0; i < filas; i++) {
        for (int c = 0; c < cols; c++) {
            const char* val = PQgetvalue(res, i, c);
            cout << left << setw(18) << (val ? val : "");
        }
        cout << "\n";
    }
    cout << endl;
}

static bool ejecutarConsulta(PGconn* conn, const string& sql) {
    PGresult* res = PQexec(conn, sql.c_str());
    pgImprimirResultados(conn, res);
    bool ok = PQresultStatus(res) == PGRES_TUPLES_OK;
    PQclear(res);
    return ok;
}

bool pgConsultarPorLugar(PGconn* conn, const string& interseccion, int limite) {
    if (!conn || interseccion.empty()) return false;

    ostringstream sql;
    sql << "SELECT tx_id, interseccion, tipo_sensor, sensor_id, "
        << "to_char(evento_en, 'YYYY-MM-DD HH24:MI:SS') AS fecha_hora, "
        << "volumen, velocidad, vehiculos_contados, densidad, nivel_congestion "
        << "FROM eventos_log WHERE interseccion = '" << escaparSql(interseccion) << "' "
        << "ORDER BY evento_en DESC LIMIT " << limite;

    return ejecutarConsulta(conn, sql.str());
}

bool pgConsultarPorFecha(PGconn* conn, const string& desde, const string& hasta, int limite) {
    if (!conn || desde.empty() || hasta.empty()) return false;

    string d = normalizarFechaEntrada(desde, false);
    string h = normalizarFechaEntrada(hasta, true);

    ostringstream sql;
    sql << "SELECT tx_id, interseccion, tipo_sensor, sensor_id, "
        << "to_char(evento_en, 'YYYY-MM-DD HH24:MI:SS') AS fecha_hora, "
        << "volumen, velocidad, vehiculos_contados, densidad, nivel_congestion "
        << "FROM eventos_log WHERE evento_en >= '" << escaparSql(d) << "'::timestamptz "
        << "AND evento_en <= '" << escaparSql(h) << "'::timestamptz "
        << "ORDER BY evento_en ASC LIMIT " << limite;

    return ejecutarConsulta(conn, sql.str());
}

bool pgConsultarPorLugarYFecha(PGconn* conn, const string& interseccion,
                               const string& desde, const string& hasta, int limite) {
    if (!conn || interseccion.empty() || desde.empty() || hasta.empty()) return false;

    string d = normalizarFechaEntrada(desde, false);
    string h = normalizarFechaEntrada(hasta, true);

    ostringstream sql;
    sql << "SELECT tx_id, interseccion, tipo_sensor, sensor_id, "
        << "to_char(evento_en, 'YYYY-MM-DD HH24:MI:SS') AS fecha_hora, "
        << "volumen, velocidad, vehiculos_contados, densidad, nivel_congestion "
        << "FROM eventos_log WHERE interseccion = '" << escaparSql(interseccion) << "' "
        << "AND evento_en >= '" << escaparSql(d) << "'::timestamptz "
        << "AND evento_en <= '" << escaparSql(h) << "'::timestamptz "
        << "ORDER BY evento_en ASC LIMIT " << limite;

    return ejecutarConsulta(conn, sql.str());
}
