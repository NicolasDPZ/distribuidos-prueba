### Compilar con g++ ... -lzmq -lpq -o ... dependiendo del archivo
### ejecutar desde la **raiz del repo** (carpeta `config/` debe estar visible)
### Dependencias: g++, libzmq3-dev, libpq-dev, PostgreSQL

```bash
sudo apt install g++ libzmq3-dev libpq-dev postgresql postgresql-contrib
export CONFIG_DIR=./config   # opcional si no ejecutas desde la raiz
```

## Matriz dinamica (`config/ciudad.json`)

Define filas, numero de columnas y formato (ej. `INT_{fila}{columna}` → `INT_C5`).
No hay tamano fijo: cambia `filas` / `num_columnas` y reinicia sensores + semaforos.

Ejemplo actual: 5 filas x 5 columnas = 25 intersecciones.

Cada interseccion tiene **dos semaforos**: **CARRERA** (fila) y **CALLE** (columna).
La analitica envia dos comandos por evento; en congestion ambos en rojo; en flujo normal carrera/calle alternan (opuestos).

## Configuracion en JSON (`config/`)

| Archivo | Que controla |
|---------|----------------|
| `red.json` | IPs, puertos, timeouts, PostgreSQL **replica** (PC2) |
| `red_pc3.json` | Conexion a PC2, PostgreSQL **principal** (PC3) |
| `reglas.json` | Reglas de trafico y semaforos (NORMAL, CONGESTION, ...) |
| `semaforos.json` | Estado inicial y lista de intersecciones |
| `monitoreo.json` | Titulo, limite de filas, conexion a analitica |
| `ciudad.json` | **Matriz dinamica** (filas x columnas) |

### Ordenes directas (monitoreo → analitica, REQ/REP)

- `FORZAR_SEMAFORO` — un eje (CARRERA/CALLE) a VERDE/ROJO
- `PRIORIZAR_AMBULANCIA` — verde en un eje, el otro en rojo
- `FORZAR_INTERSECCION` — ambos ejes explicitos (JSON)
- `LIBERAR_OVERRIDE` — vuelven las reglas automaticas

Edita los JSON para cambiar comportamiento **sin recompilar** (solo reinicia el proceso).

## Roles por PC

| PC | Programas | BD PostgreSQL |
|----|-----------|---------------|
| PC1 | broker, sensores | — |
| PC2 | dbReplica, analisis, semaforos, **monitoreo** | `traffic_replica` |
| PC3 | database | `traffic_main` |

**Monitoreo vive en PC2**: consulta en vivo a `analisis` (localhost) e historial en la replica local.

## PostgreSQL

```bash
sudo -u postgres psql -f sql/setup.sql
```

Variables (opcional; si no, usa `pg_conninfo` del JSON):

```bash
# PC2
export PG_CONNINFO="host=localhost dbname=traffic_replica user=traffic password=traffic"

# PC3
export PG_CONNINFO="host=localhost dbname=traffic_main user=traffic password=traffic"
```

## Estructura del codigo

```
config/           # JSON: red, reglas, semaforos, monitoreo
common/
  config_loader.* # lee JSON
  config.*        # acceso global config::red, etc.
  json_utils.*
  pg_log.*        # PostgreSQL
  pg_consultas.*  # consultas historial
pc2/
  analisis.cpp    # orquestacion
  monitoreo.cpp   # consola de consultas
  reglas_trafico.*, failover_pc3.*, estado_ciudad.*, monitoreo_rep.*
  dbReplica.cpp, semaforos.cpp
pc3/
  database.cpp, db_sync.*
sql/
```

## Compilar con makefile (raiz del repo)

make all, para generar todos los ejecutables en una carpeta bin

ejecutar con ./bin/*nombre del ejecutable*

## Orden de ejecucion

1. PostgreSQL en PC2 y PC3.
2. **PC2:** `dbReplica` → `analisis` → `semaforos` → (opcional) `monitoreo`
3. **PC3:** `database`
4. **PC1:** broker + sensores

## Recuperacion PC3

Replica en PC2 acumula el log; PC3 hace `SYNC_LOG` al volver. Ver `sql/schema.sql` y `evento_en` para consultas por fecha.
