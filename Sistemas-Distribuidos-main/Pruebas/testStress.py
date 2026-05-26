import zmq
import time
import json
import random
import threading
from datetime import datetime

BROKER_IP = "127.0.0.1"
BROKER_PORT = 5555

NUM_SENSORES = 100
INTERVALO = 0.1

context = zmq.Context()

latencias = []
mensajes_enviados = 0


def crear_evento(sensor_id):
    tipos = ["camara", "gps", "espira"]

    tipo = random.choice(tipos)

    return {
        "sensor_id": sensor_id,
        "tipo_sensor": tipo,
        "interseccion": f"INT-{random.randint(1,10)}",
        "velocidad_promedio": random.randint(5, 60),
        "vehiculos": random.randint(1, 50),
        "timestamp": time.time()
    }


def sensor_worker(sensor_id):
    global mensajes_enviados

    socket = context.socket(zmq.PUB)
    socket.connect(f"tcp://{BROKER_IP}:{BROKER_PORT}")

    while True:
        evento = crear_evento(sensor_id)

        socket.send_json(evento)

        mensajes_enviados += 1

        print(f"[{sensor_id}] enviado")

        time.sleep(INTERVALO)


print("Iniciando sensores...")

for i in range(NUM_SENSORES):
    thread = threading.Thread(
        target=sensor_worker,
        args=(f"SENSOR-{i}",),
        daemon=True
    )

    thread.start()


inicio = time.time()

try:
    while True:
        tiempo_actual = time.time() - inicio

        print(f"Tiempo: {round(tiempo_actual,2)}s")
        print(f"Mensajes enviados: {mensajes_enviados}")

        time.sleep(5)

except KeyboardInterrupt:
    print("Finalizando prueba")