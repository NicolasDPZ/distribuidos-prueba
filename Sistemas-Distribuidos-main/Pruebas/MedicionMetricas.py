import csv
import time
import psutil
import statistics
from datetime import datetime

archivo_csv = "results/metricas.csv"

latencias = []
mensajes = 0


with open(archivo_csv, mode="w", newline="") as file:
    writer = csv.writer(file)

    writer.writerow([
        "timestamp",
        "cpu",
        "ram",
        "mensajes",
        "throughput"
    ])

    inicio = time.time()
    ultimo_mensaje = 0

    print("Recolectando métricas...")

    try:
        while True:
            cpu = psutil.cpu_percent()
            ram = psutil.virtual_memory().percent

            tiempo_actual = time.time() - inicio

            throughput = mensajes - ultimo_mensaje
            ultimo_mensaje = mensajes

            writer.writerow([
                datetime.now().isoformat(),
                cpu,
                ram,
                mensajes,
                throughput
            ])

            file.flush()

            print("CPU:", cpu)
            print("RAM:", ram)
            print("Throughput:", throughput)

            time.sleep(1)

    except KeyboardInterrupt:
        print("Guardando resultados...")