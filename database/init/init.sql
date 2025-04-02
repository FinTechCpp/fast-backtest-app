CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

CREATE TABLE market_data (
    asset TEXT NOT NULL,  -- Ex: 'BTC-USD', 'EUR/USD', etc.
    timestamp TIMESTAMPTZ NOT NULL,
    open NUMERIC NOT NULL,
    high NUMERIC NOT NULL,
    low NUMERIC NOT NULL,
    close NUMERIC NOT NULL,
    volume NUMERIC NOT NULL,
    PRIMARY KEY (asset, timestamp)  -- Clé composite
);

-- Création de l'hypertable avec partitionnement
SELECT create_hypertable(
    'market_data'::regclass,  -- Conversion explicite en type OID
    'timestamp'::name,        -- Colonne temporelle
    chunk_time_interval => INTERVAL '1 day',  -- Intervalle de temps pour les chunks
    partitioning_column => 'asset'::name, 
    number_partitions => 4
);

-- Index composite pour les requêtes temporelles par actif
CREATE INDEX idx_asset_time ON market_data (asset, timestamp DESC);