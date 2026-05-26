-- PC3 (principal): traffic_main
-- PC2 (replica):   traffic_replica  (mismo esquema)

CREATE TABLE IF NOT EXISTS eventos_log (
    tx_id              BIGINT PRIMARY KEY,
    linea_log          TEXT NOT NULL,
    sensor_id          VARCHAR(32),
    tipo_sensor        VARCHAR(32),
    interseccion       VARCHAR(16),
    volumen            INTEGER,
    velocidad          INTEGER,
    vehiculos_contados INTEGER,
    densidad           INTEGER,
    nivel_congestion   VARCHAR(16),
    evento_en          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    registrado_en      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS sistema_checkpoint (
    nodo           VARCHAR(32) PRIMARY KEY,
    ultimo_tx_id   BIGINT NOT NULL DEFAULT 0,
    actualizado_en TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

INSERT INTO sistema_checkpoint (nodo, ultimo_tx_id)
VALUES ('pc3_principal', 0)
ON CONFLICT (nodo) DO NOTHING;

CREATE INDEX IF NOT EXISTS idx_eventos_tx ON eventos_log (tx_id);
CREATE INDEX IF NOT EXISTS idx_eventos_interseccion ON eventos_log (interseccion);
CREATE INDEX IF NOT EXISTS idx_eventos_evento_en ON eventos_log (evento_en);
