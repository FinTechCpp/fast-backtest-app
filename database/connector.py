import psycopg2
from psycopg2 import pool
from config import DB_CONFIG

connection_pool = psycopg2.pool.SimpleConnectionPool(
    minconn=1,
    maxconn=10,
    **DB_CONFIG
)