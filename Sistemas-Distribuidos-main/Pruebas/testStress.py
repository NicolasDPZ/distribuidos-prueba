"""
testStress.py
-------------
Prueba de estrés: simula N sensores concurrentes enviando mensajes al broker ZeroMQ.
Muestra estadísticas en tiempo real y guarda un resumen al finalizar.

"""

import argparse
import json
import os
import random
import threading
import time
from datetime import datetime, timezone

try:
    import zmq
except ImportError:
    print("[ERROR] Instala pyzmq: pip install pyzmq")
    raise

# ── Configuración ─────────────────────────────────────────────────────────────
BROKER_IP   = "10.43.100.43"
BROKER_PORT = 5555              # Puerto de INGESTA del broker (no el de publicación)
NUM_SENSORES = 3
INTERVALO   = 10.0
DURACION    = 120

INTERSECCIONES = [
    f"INT-{fila}{col}" for fila in "ABCDE" for col in range(1, 6)
]


# ── Contadores thread-safe ────────────────────────────────────────────────────
class Estadisticas:
    def __init__(self):
        self._lock   = threading.Lock()
        self.enviados = 0
        self.errores  = 0

    def ok(self):
        with self._lock:
            self.enviados += 1

    def err(self):
        with self._lock:
            self.errores += 1

    def snapshot(self):
        with self._lock:
            return self.enviados, self.errores


stats      = Estadisticas()
detener    = threading.Event()


# ── Generadores de eventos ────────────────────────────────────────────────────
def evento_camara(sensor_id: str) -> dict:
    inter    = random.choice(INTERSECCIONES)
    volumen  = random.randint(0, 20)
    velocidad = random.randint(0, 50)
    return {
        "sensor_id": f"CAM-{sensor_id}",
        "tipo_sensor": "camara",
        "interseccion": inter,
        "volumen": volumen,
        "velocidad_promedio": velocidad,
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    }

def evento_espira(sensor_id: str) -> dict:
    inter     = random.choice(INTERSECCIONES)
    vehiculos = random.randint(0, 20)
    intervalo_s = 30
    return {
        "sensor_id": f"ESP-{sensor_id}",
        "tipo_sensor": "espira_inductiva",
        "interseccion": inter,
        "vehiculos_contados": vehiculos,
        "intervalo_segundos": intervalo_s,
        "timestamp_inicio": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "timestamp_fin": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    }

def evento_gps(sensor_id: str) -> dict:
    inter     = random.choice(INTERSECCIONES)
    velocidad = random.randint(0, 80)
    densidad  = random.randint(0, 50)
    if velocidad < 10:
        nivel = "ALTA"
    elif velocidad <= 39:
        nivel = "NORMAL"
    else:
        nivel = "BAJA"
    return {
        "sensor_id": f"GPS-{sensor_id}",
        "tipo_sensor": "gps",
        "interseccion": inter,
        "nivel_congestion": nivel,
        "velocidad_promedio": velocidad,
        "densidad": densidad,
        "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    }

GENERADORES = [evento_camara, evento_espira, evento_gps]


# ── Hilo de sensor ────────────────────────────────────────────────────────────
def sensor_worker(sensor_id: int, broker_ip: str, broker_port: int, intervalo: float):
    context = zmq.Context()
    socket  = context.socket(zmq.PUB)
    socket.connect(f"tcp://{broker_ip}:{broker_port}")
    time.sleep(0.5)   # esperar al broker (slow joiner)

    tipo = GENERADORES[sensor_id % len(GENERADORES)]

    while not detener.is_set():
        try:
            evento = tipo(str(sensor_id))
            msg    = json.dumps(evento)
            socket.send_string(msg)
            stats.ok()
        except zmq.ZMQError as e:
            print(f"[SENSOR-{sensor_id}] ZMQ error: {e}")
            stats.err()

        detener.wait(intervalo)

    socket.close()
    context.term()


# ── Main ──────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(description="Prueba de estrés — Sistema GITU")
    parser.add_argument("--broker-ip",   default=BROKER_IP)
    parser.add_argument("--broker-port", default=BROKER_PORT, type=int)
    parser.add_argument("--sensores",    default=NUM_SENSORES, type=int,
                        help="Número total de sensores simulados")
    parser.add_argument("--intervalo",   default=INTERVALO, type=float,
                        help="Segundos entre envíos por sensor")
    parser.add_argument("--duracion",    default=DURACION, type=int,
                        help="Duración total de la prueba en segundos (0 = infinito)")
    args = parser.parse_args()

    print(f"\n{'='*55}")
    print(f"  Prueba de estrés — Sistema GITU")
    print(f"{'='*55}")
    print(f"  Broker:    tcp://{args.broker_ip}:{args.broker_port}")
    print(f"  Sensores:  {args.sensores}")
    print(f"  Intervalo: {args.intervalo} s")
    print(f"  Duración:  {'∞' if args.duracion == 0 else str(args.duracion) + 's'}")
    print(f"{'='*55}\n")

    hilos = []
    for i in range(args.sensores):
        t = threading.Thread(
            target=sensor_worker,
            args=(i, args.broker_ip, args.broker_port, args.intervalo),
            daemon=True
        )
        t.start()
        hilos.append(t)

    print(f"[INFO] {args.sensores} sensores iniciados. Presiona Ctrl+C para detener.\n")

    inicio          = time.time()
    ultimo_enviados = 0

    try:
        while True:
            tiempo = time.time() - inicio
            if args.duracion > 0 and tiempo >= args.duracion:
                break

            time.sleep(5)
            enviados, errores = stats.snapshot()
            throughput = (enviados - ultimo_enviados) / 5
            ultimo_enviados = enviados
            print(f"[t={round(tiempo):4d}s]  Enviados: {enviados:6d}  "
                  f"Errores: {errores:3d}  "
                  f"Throughput: {throughput:.1f} msg/s")

    except KeyboardInterrupt:
        print("\n[INFO] Detenido por el usuario.")

    detener.set()

    enviados, errores = stats.snapshot()
    duracion_real = time.time() - inicio
    tput_prom = enviados / duracion_real if duracion_real > 0 else 0

    print(f"\n{'='*55}")
    print(f"  Resumen de la prueba")
    print(f"{'='*55}")
    print(f"  Duración real:       {duracion_real:.1f} s")
    print(f"  Mensajes enviados:   {enviados}")
    print(f"  Errores:             {errores}")
    print(f"  Throughput promedio: {tput_prom:.1f} msg/s")
    print(f"{'='*55}\n")

    # Guardar resumen
    os.makedirs("results", exist_ok=True)
    resumen = os.path.join("results", "resumen_stress.txt")
    with open(resumen, "w") as f:
        f.write(f"Fecha:               {datetime.now().isoformat()}\n")
        f.write(f"Broker:              {args.broker_ip}:{args.broker_port}\n")
        f.write(f"Sensores:            {args.sensores}\n")
        f.write(f"Intervalo (s):       {args.intervalo}\n")
        f.write(f"Duracion real (s):   {duracion_real:.1f}\n")
        f.write(f"Mensajes enviados:   {enviados}\n")
        f.write(f"Errores:             {errores}\n")
        f.write(f"Throughput prom:     {tput_prom:.1f} msg/s\n")
    print(f"[INFO] Resumen guardado en {resumen}")


if __name__ == "__main__":
    main()