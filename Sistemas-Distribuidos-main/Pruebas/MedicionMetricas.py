"""
MedicionMetricas.py
-------------------
Recolecta métricas del sistema durante las pruebas de rendimiento.
Se suscribe al broker ZeroMQ para contar mensajes en tiempo real.
"""

import csv
import time
import threading
import argparse
import os
from datetime import datetime

try:
    import psutil
except ImportError:
    print("[ERROR] Instala psutil: pip install psutil")
    raise

try:
    import zmq
except ImportError:
    print("[ERROR] Instala pyzmq: pip install pyzmq")
    raise

# ── Configuración por defecto ────────────────────────────────────────────────
BROKER_IP   = "10.43.100.43"   # IP del PC1 donde corre el broker
BROKER_PORT = 5556              # Puerto de publicación del broker
DURACION_SEG = 120              # Duración total de la recolección (0 = infinito)
INTERVALO_SEG = 1               # Intervalo de muestreo en segundos
DIRECTORIO  = "results"         # Carpeta de salida
ARCHIVO_CSV = os.path.join(DIRECTORIO, "metricas.csv")


# ── Contador de mensajes compartido ─────────────────────────────────────────
class Contador:
    def __init__(self):
        self._lock  = threading.Lock()
        self._total = 0

    def incrementar(self):
        with self._lock:
            self._total += 1

    @property
    def total(self):
        with self._lock:
            return self._total


contador = Contador()
ejecutando = threading.Event()
ejecutando.set()


def receptor_zmq(broker_ip: str, broker_port: int):
    """Hilo que se suscribe al broker y cuenta cada mensaje recibido."""
    context = zmq.Context()
    socket  = context.socket(zmq.SUB)
    socket.connect(f"tcp://{broker_ip}:{broker_port}")
    socket.setsockopt_string(zmq.SUBSCRIBE, "")   # recibir todo
    socket.setsockopt(zmq.RCVTIMEO, 500)          # timeout 500 ms para poder salir limpio

    print(f"[ZMQ] Conectado a tcp://{broker_ip}:{broker_port}")

    while ejecutando.is_set():
        try:
            socket.recv()
            contador.incrementar()
        except zmq.Again:
            pass   # timeout: verificar si seguimos ejecutando
        except zmq.ZMQError as e:
            print(f"[ZMQ] Error: {e}")
            break

    socket.close()
    context.term()
    print("[ZMQ] Receptor detenido.")


def recolectar(duracion: int, intervalo: int, broker_ip: str, broker_port: int):
    os.makedirs(DIRECTORIO, exist_ok=True)

    # Arrancar hilo receptor ZeroMQ
    hilo = threading.Thread(
        target=receptor_zmq,
        args=(broker_ip, broker_port),
        daemon=True
    )
    hilo.start()

    with open(ARCHIVO_CSV, mode="w", newline="", encoding="utf-8") as archivo:
        writer = csv.writer(archivo)
        writer.writerow([
            "timestamp", "cpu_pct", "ram_pct",
            "mensajes_total", "throughput_msg_s",
            "ram_usada_mb", "ram_total_mb"
        ])

        inicio        = time.time()
        ultimo_total  = 0
        muestra       = 0

        print(f"[INFO] Recolectando métricas (duración: {'∞' if duracion == 0 else str(duracion) + 's'}, "
              f"intervalo: {intervalo}s)...")
        print("[INFO] Presiona Ctrl+C para detener.\n")

        try:
            while True:
                tiempo_actual = time.time() - inicio
                if duracion > 0 and tiempo_actual >= duracion:
                    break

                cpu  = psutil.cpu_percent(interval=None)
                mem  = psutil.virtual_memory()
                total_ahora  = contador.total
                throughput   = total_ahora - ultimo_total
                ultimo_total = total_ahora

                ts = datetime.now().isoformat(timespec="seconds")
                writer.writerow([
                    ts,
                    round(cpu, 1),
                    round(mem.percent, 1),
                    total_ahora,
                    throughput,
                    round(mem.used / 1_048_576, 1),
                    round(mem.total / 1_048_576, 1)
                ])
                archivo.flush()

                muestra += 1
                print(f"[{ts}]  CPU: {cpu:5.1f}%  RAM: {mem.percent:5.1f}%  "
                      f"Msgs: {total_ahora:6d}  Throughput: {throughput:4d} msg/s")

                time.sleep(intervalo)

        except KeyboardInterrupt:
            print("\n[INFO] Interrupción recibida.")

    ejecutando.clear()
    hilo.join(timeout=2)
    print(f"\n[INFO] Métricas guardadas en {ARCHIVO_CSV}  ({muestra} muestras).")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Recolector de métricas del sistema GITU")
    parser.add_argument("--broker-ip",   default=BROKER_IP,   help="IP del broker ZeroMQ (PC1)")
    parser.add_argument("--broker-port", default=BROKER_PORT, type=int, help="Puerto del broker")
    parser.add_argument("--duracion",    default=DURACION_SEG, type=int,
                        help="Duración en segundos (0 = infinito)")
    parser.add_argument("--intervalo",   default=INTERVALO_SEG, type=int, help="Intervalo de muestreo")
    args = parser.parse_args()

    recolectar(args.duracion, args.intervalo, args.broker_ip, args.broker_port)