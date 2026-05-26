# ============================================================
#  Makefile — Sistemas Distribuidos
#  Uso:
#    make pc1       -> compila broker y los 3 sensores
#    make pc2       -> compila analisis, semaforos, monitoreo, dbReplica
#    make pc3       -> compila database
#    make all       -> compila todo
#    make clean     -> elimina todos los binarios
# ============================================================

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -O2
LIBS     := -lzmq -lpqxx -lpq

# Directorios
COMMON   := common
PC1      := pc1
PC2      := pc2
PC3      := pc3
BIN      := bin

# ============================================================
#  Fuentes comunes (se compilan junto a cada ejecutable)
# ============================================================
COMMON_SRC := $(COMMON)/config.cpp \
              $(COMMON)/config_loader.cpp \
              $(COMMON)/json_utils.cpp \
              $(COMMON)/pg_log.cpp \
              $(COMMON)/pg_consultas.cpp

# ============================================================
#  PC1 — Broker + Sensores
# ============================================================
PC1_TARGETS := $(BIN)/broker \
               $(BIN)/sensorCamara \
               $(BIN)/EspirasInductivas \
               $(BIN)/SensorGps

$(BIN)/broker: $(PC1)/broker/broker.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/sensorCamara: $(PC1)/sensores.cpp/sensorCamara.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/EspirasInductivas: $(PC1)/sensores.cpp/EspirasInductivas.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/SensorGps: $(PC1)/sensores.cpp/SensorGps.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

.PHONY: pc1
pc1: $(PC1_TARGETS)
	@echo "✔  PC1 compilado: broker, sensorCamara, EspirasInductivas, SensorGps"

# ============================================================
#  PC2 — Analisis, Semaforos, Monitoreo, DbReplica
# ============================================================
PC2_COMMON := $(PC2)/reglas_trafico.cpp \
              $(PC2)/failover_pc3.cpp \
              $(PC2)/estado_ciudad.cpp \
              $(PC2)/monitoreo_rep.cpp \
              $(PC2)/ordenes_manual.cpp \
              $(PC2)/semaforo_interseccion.cpp

PC2_TARGETS := $(BIN)/analisis \
               $(BIN)/semaforos \
               $(BIN)/monitoreo \
               $(BIN)/dbReplica

$(BIN)/analisis: $(PC2)/analisis.cpp $(PC2_COMMON) $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/semaforos: $(PC2)/semaforos.cpp $(PC2)/semaforo_interseccion.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/monitoreo: $(PC2)/monitoreo.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(BIN)/dbReplica: $(PC2)/dbReplica.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

.PHONY: pc2
pc2: $(PC2_TARGETS)
	@echo "✔  PC2 compilado: analisis, semaforos, monitoreo, dbReplica"

# ============================================================
#  PC3 — Database
# ============================================================
PC3_TARGETS := $(BIN)/database

$(BIN)/database: $(PC3)/database.cpp $(PC3)/db_sync.cpp $(COMMON_SRC) | $(BIN)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

.PHONY: pc3
pc3: $(PC3_TARGETS)
	@echo "✔  PC3 compilado: database"

# ============================================================
#  Targets generales
# ============================================================
.PHONY: all
all: pc1 pc2 pc3
	@echo "✔  Todo compilado correctamente"

$(BIN):
	mkdir -p $(BIN)

.PHONY: clean
clean:
	rm -rf $(BIN)
	@echo "✔  Binarios eliminados"
