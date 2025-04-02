from datetime import datetime
from connector import connection_pool

def insert_market_data(asset: str, data: list):
    """Insère des données pour un actif spécifique"""
    query = """
    INSERT INTO market_data 
    (asset, timestamp, open, high, low, close, volume)
    VALUES (%s, %s, %s, %s, %s, %s, %s)
    ON CONFLICT (asset, timestamp) DO NOTHING
    """

    prepared_data = [(asset, *row) for row in data]

    with connection_pool.getconn() as conn:
        with conn.cursor() as cursor:
            cursor.executemany(query, prepared_data)
            conn.commit()
        connection_pool.putconn(conn)

def get_data(asset: str, start: datetime, end: datetime):
    """Récupère les données d'un actif sur une période"""
    query = """
    SELECT * FROM market_data
    WHERE
        asset = %s AND 
        timestamp BETWEEN %s AND %s
    ORDER BY timestamp
    """
    
    with connection_pool.getconn() as conn:
        with conn.cursor() as cursor:
            cursor.execute(query, (asset, start, end))
            data = cursor.fetchall()
        connection_pool.putconn(conn)
    
    return data

# # Utilisation de COPY pour >100k lignes
# def bulk_insert(data):
#     from io import StringIO
#     buffer = StringIO()
#     for point in data:
#         buffer.write(f"{point['timestamp']},{point['open']},...\n")
#     buffer.seek(0)
    
#     with connection_pool.getconn() as conn:
#         with conn.cursor() as cursor:
#             cursor.copy_expert(
#                 "COPY market_data (timestamp, open, high, low, close) FROM STDIN CSV",
#                 buffer
#             )
#             conn.commit()