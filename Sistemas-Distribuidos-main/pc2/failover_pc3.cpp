#include "failover_pc3.hpp"
#include "../common/config.hpp"

using namespace std;

void FailoverPc3::registrarHeartbeat() {
    ultimoHb = chrono::steady_clock::now();
    if (!activo) {
        cout << "[RECUPERACION] PC3 volvio. BD principal disponible." << endl;
        cout << "[RECUPERACION] PC3 puede solicitar SYNC_LOG desde PostgreSQL replica." << endl;
        activo = true;
    }
}

void FailoverPc3::verificarTimeout() {
    auto ahora = chrono::steady_clock::now();
    auto seg = chrono::duration_cast<chrono::seconds>(ahora - ultimoHb).count();
    if (seg > config::red.failover_timeout_seg && activo) {
        cout << "[FALLA] PC3 caido. Log solo en PostgreSQL replica (PC2)." << endl;
        activo = false;
    }
}
