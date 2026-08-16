# Multi-stage build:
#   1) "builder" compiles the existing C++ csv_to_bin_converter (reused as-is,
#      no Qt/CMake needed - it only depends on the Cereal headers).
#   2) runtime image only needs Python 3 stdlib to serve the REST API + admin UI,
#      plus the compiled converter binary produced above.

FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY local_data_server/shared /build/shared
COPY local_data_server/converter/main.cpp /build/converter/main.cpp
COPY ThirdParty/cereal/include /build/cereal/include

RUN g++ -std=c++17 -O2 \
    -I/build/shared -I/build/cereal/include \
    /build/converter/main.cpp -o /build/csv_to_bin_converter


FROM python:3.12-slim AS runtime

WORKDIR /app
COPY local_data_server/server/serve.py /app/server/serve.py
COPY --from=builder /build/csv_to_bin_converter /app/bin/csv_to_bin_converter
RUN chmod +x /app/bin/csv_to_bin_converter

ENV CSV_DIR=/app/data/csv
ENV BIN_DIR=/app/data/bin
ENV CONVERTER_PATH=/app/bin/csv_to_bin_converter

EXPOSE 9015

CMD ["python3", "/app/server/serve.py", "--host", "0.0.0.0", "--port", "9015"]
