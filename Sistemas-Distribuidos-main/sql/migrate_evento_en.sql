-- Si ya tenias la tabla sin evento_en, ejecutar en traffic_main y traffic_replica:
ALTER TABLE eventos_log
    ADD COLUMN IF NOT EXISTS evento_en TIMESTAMPTZ NOT NULL DEFAULT NOW();

UPDATE eventos_log
SET evento_en = registrado_en
WHERE evento_en IS NULL;

CREATE INDEX IF NOT EXISTS idx_eventos_interseccion ON eventos_log (interseccion);
CREATE INDEX IF NOT EXISTS idx_eventos_evento_en ON eventos_log (evento_en);
