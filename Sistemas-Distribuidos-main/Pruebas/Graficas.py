"""
Graficas.py
-----------
Genera gráficas de rendimiento a partir del CSV producido por MedicionMetricas.py.
También puede graficar comparativas entre dos archivos CSV (diseño original vs. multihilo).

"""

import argparse
import os
import sys

try:
    import pandas as pd
except ImportError:
    print("[ERROR] Instala pandas: pip install pandas")
    sys.exit(1)

try:
    import matplotlib
    matplotlib.use("Agg")          # sin GUI, para servidores
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker
except ImportError:
    print("[ERROR] Instala matplotlib: pip install matplotlib")
    sys.exit(1)

DIRECTORIO_SALIDA = "results"


def estilo_grafica(ax, titulo: str, xlabel: str, ylabel: str):
    ax.set_title(titulo, fontsize=13, fontweight="bold", pad=10)
    ax.set_xlabel(xlabel, fontsize=10)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.spines[["top", "right"]].set_visible(False)


def cargar_csv(ruta: str) -> pd.DataFrame:
    if not os.path.exists(ruta):
        print(f"[ERROR] No se encontró: {ruta}")
        sys.exit(1)
    df = pd.read_csv(ruta)
    # índice temporal
    if "timestamp" in df.columns:
        df["t_seg"] = range(len(df))
    print(f"[INFO] Cargado: {ruta}  ({len(df)} muestras)")
    return df


def graficar_individual(df: pd.DataFrame, salida: str):
    os.makedirs(salida, exist_ok=True)
    t = df["t_seg"] if "t_seg" in df.columns else range(len(df))

    # 1. CPU
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, df["cpu_pct"], color="#2E74B5", linewidth=1.5)
    ax.fill_between(t, df["cpu_pct"], alpha=0.15, color="#2E74B5")
    ax.yaxis.set_major_formatter(ticker.PercentFormatter())
    ax.set_ylim(0, 100)
    estilo_grafica(ax, "Uso de CPU durante la prueba", "Tiempo (s)", "CPU (%)")
    plt.tight_layout()
    fig.savefig(os.path.join(salida, "cpu.png"), dpi=150)
    plt.close(fig)
    print("[OK] Guardado: cpu.png")

    # 2. RAM
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, df["ram_pct"], color="#ED7D31", linewidth=1.5)
    ax.fill_between(t, df["ram_pct"], alpha=0.15, color="#ED7D31")
    ax.yaxis.set_major_formatter(ticker.PercentFormatter())
    ax.set_ylim(0, 100)
    estilo_grafica(ax, "Uso de RAM durante la prueba", "Tiempo (s)", "RAM (%)")
    plt.tight_layout()
    fig.savefig(os.path.join(salida, "ram.png"), dpi=150)
    plt.close(fig)
    print("[OK] Guardado: ram.png")

    # 3. Throughput
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, df["throughput_msg_s"], color="#70AD47", linewidth=1.5)
    ax.fill_between(t, df["throughput_msg_s"], alpha=0.15, color="#70AD47")
    estilo_grafica(ax, "Throughput — Mensajes por segundo", "Tiempo (s)", "Mensajes/s")
    plt.tight_layout()
    fig.savefig(os.path.join(salida, "throughput.png"), dpi=150)
    plt.close(fig)
    print("[OK] Guardado: throughput.png")

    # 4. Mensajes acumulados
    fig, ax = plt.subplots(figsize=(10, 4))
    ax.plot(t, df["mensajes_total"], color="#9B59B6", linewidth=1.5)
    estilo_grafica(ax, "Mensajes acumulados en el tiempo", "Tiempo (s)", "Total de mensajes")
    plt.tight_layout()
    fig.savefig(os.path.join(salida, "mensajes_acumulados.png"), dpi=150)
    plt.close(fig)
    print("[OK] Guardado: mensajes_acumulados.png")


def graficar_comparativa(df1: pd.DataFrame, df2: pd.DataFrame,
                         etiqueta1: str, etiqueta2: str, salida: str):
    os.makedirs(salida, exist_ok=True)
    n = min(len(df1), len(df2))
    t = list(range(n))

    fig, axes = plt.subplots(2, 2, figsize=(14, 9))
    fig.suptitle(f"Comparativa: {etiqueta1} vs. {etiqueta2}", fontsize=14, fontweight="bold")

    # CPU
    axes[0, 0].plot(t, df1["cpu_pct"][:n].values, label=etiqueta1, color="#2E74B5")
    axes[0, 0].plot(t, df2["cpu_pct"][:n].values, label=etiqueta2, color="#ED7D31", linestyle="--")
    axes[0, 0].yaxis.set_major_formatter(ticker.PercentFormatter())
    axes[0, 0].set_ylim(0, 100)
    estilo_grafica(axes[0, 0], "Uso de CPU", "Tiempo (s)", "CPU (%)")
    axes[0, 0].legend()

    # RAM
    axes[0, 1].plot(t, df1["ram_pct"][:n].values, label=etiqueta1, color="#2E74B5")
    axes[0, 1].plot(t, df2["ram_pct"][:n].values, label=etiqueta2, color="#ED7D31", linestyle="--")
    axes[0, 1].yaxis.set_major_formatter(ticker.PercentFormatter())
    axes[0, 1].set_ylim(0, 100)
    estilo_grafica(axes[0, 1], "Uso de RAM", "Tiempo (s)", "RAM (%)")
    axes[0, 1].legend()

    # Throughput
    axes[1, 0].plot(t, df1["throughput_msg_s"][:n].values, label=etiqueta1, color="#2E74B5")
    axes[1, 0].plot(t, df2["throughput_msg_s"][:n].values, label=etiqueta2, color="#ED7D31", linestyle="--")
    estilo_grafica(axes[1, 0], "Throughput", "Tiempo (s)", "Mensajes/s")
    axes[1, 0].legend()

    # Barras de resumen (promedios)
    metricas = ["cpu_pct", "ram_pct", "throughput_msg_s"]
    nombres  = ["CPU (%)", "RAM (%)", "Throughput\n(msg/s)"]
    prom1 = [df1[m].mean() for m in metricas]
    prom2 = [df2[m].mean() for m in metricas]
    x = range(len(metricas))
    w = 0.35
    axes[1, 1].bar([xi - w/2 for xi in x], prom1, w, label=etiqueta1, color="#2E74B5", alpha=0.85)
    axes[1, 1].bar([xi + w/2 for xi in x], prom2, w, label=etiqueta2, color="#ED7D31", alpha=0.85)
    axes[1, 1].set_xticks(list(x))
    axes[1, 1].set_xticklabels(nombres, fontsize=9)
    estilo_grafica(axes[1, 1], "Promedios comparativos", "", "Valor promedio")
    axes[1, 1].legend()

    plt.tight_layout()
    fig.savefig(os.path.join(salida, "comparativa.png"), dpi=150)
    plt.close(fig)
    print("[OK] Guardado: comparativa.png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generador de gráficas GITU")
    parser.add_argument("--csv",      default="results/metricas.csv", help="CSV de métricas (individual)")
    parser.add_argument("--csv1",     default=None, help="CSV diseño original (comparativa)")
    parser.add_argument("--csv2",     default=None, help="CSV diseño multihilo (comparativa)")
    parser.add_argument("--comparar", action="store_true", help="Modo comparativa")
    parser.add_argument("--salida",   default=DIRECTORIO_SALIDA, help="Carpeta de salida")
    parser.add_argument("--etiqueta1", default="Diseño Original", help="Etiqueta CSV1")
    parser.add_argument("--etiqueta2", default="Diseño Multihilo", help="Etiqueta CSV2")
    args = parser.parse_args()

    if args.comparar:
        if not args.csv1 or not args.csv2:
            print("[ERROR] Para comparativa necesitas --csv1 y --csv2.")
            sys.exit(1)
        df1 = cargar_csv(args.csv1)
        df2 = cargar_csv(args.csv2)
        graficar_comparativa(df1, df2, args.etiqueta1, args.etiqueta2, args.salida)
    else:
        df = cargar_csv(args.csv)
        graficar_individual(df, args.salida)

    print(f"\n[INFO] Gráficas guardadas en '{args.salida}/'.")