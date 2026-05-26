#pragma once

#include <chrono>
#include <iostream>

class FailoverPc3 {
public:
    bool activo = true;
    std::chrono::steady_clock::time_point ultimoHb = std::chrono::steady_clock::now();

    void registrarHeartbeat();
    void verificarTimeout();
};
