#include "json_utils.hpp"

using namespace std;

int extraerValorInt(const string& json, const string& clave, int porDefecto) {
    string buscador = "\"" + clave + "\": ";
    size_t pos = json.find(buscador);
    if (pos == string::npos) {
        buscador = "\"" + clave + "\":";
        pos = json.find(buscador);
    }
    if (pos == string::npos) return porDefecto;
    pos += buscador.size();
    while (pos < json.size() && json[pos] == ' ') pos++;
    size_t fin = pos;
    while (fin < json.size() && (isdigit(json[fin]) || json[fin] == '-')) fin++;
    if (fin == pos) return porDefecto;
    return stoi(json.substr(pos, fin - pos));
}

string extraerValorString(const string& json, const string& clave) {
    string buscador = "\"" + clave + "\": \"";
    size_t pos = json.find(buscador);
    if (pos == string::npos) {
        buscador = "\"" + clave + "\":\"";
        pos = json.find(buscador);
    }
    if (pos == string::npos) return "";
    pos += buscador.size();
    size_t fin = json.find("\"", pos);
    return json.substr(pos, fin - pos);
}

string agregarTxId(const string& json, int txId) {
    if (json.empty() || json[0] != '{') return json;
    return "{\"tx_id\":" + to_string(txId) + "," + json.substr(1);
}
