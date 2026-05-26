import pandas as pd
import matplotlib.pyplot as plt

archivo = "results/metricas.csv"

print("Leyendo datos...")


df = pd.read_csv(archivo)


# CPU
plt.figure(figsize=(10,5))
plt.plot(df["cpu"])
plt.title("Uso de CPU")
plt.xlabel("Tiempo")
plt.ylabel("CPU %")
plt.grid(True)
plt.savefig("results/cpu.png")


# RAM
plt.figure(figsize=(10,5))
plt.plot(df["ram"])
plt.title("Uso de RAM")
plt.xlabel("Tiempo")
plt.ylabel("RAM %")
plt.grid(True)
plt.savefig("results/ram.png")


# Throughput
plt.figure(figsize=(10,5))
plt.plot(df["throughput"])
plt.title("Throughput")
plt.xlabel("Tiempo")
plt.ylabel("Mensajes por segundo")
plt.grid(True)
plt.savefig("results/throughput.png")

print("Gráficas generadas")