-- Ejecutar como superusuario: sudo -u postgres psql -f sql/setup.sql

CREATE USER traffic WITH PASSWORD 'traffic';

CREATE DATABASE traffic_main OWNER traffic;
CREATE DATABASE traffic_replica OWNER traffic;

\connect traffic_main
\i sql/schema.sql

\connect traffic_replica
\i sql/schema.sql
