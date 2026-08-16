# Market Data Server — CSV → Cereal Binary → REST API (Docker, VPN)

Server-side data pipeline described in
[`docs/server_data_migration_plan.md`](../docs/server_data_migration_plan.md).
It runs **Dockerized on a private server** (`10.8.0.1`), reachable only over
the VPN — never exposed to the public internet.

```
local_data_server/
    shared/            BinaryBar.h — Cereal-serializable bar shared by converter & app
    converter/         csv_to_bin_converter — standalone C++ tool (reused, no Qt dep)
    server/serve.py     REST API + admin web UI (Python stdlib only)
    data/csv/          Source CSV files (server-side only, never sent to clients)
    data/bin/          Generated .bin files (served to BacktestApp)
    Dockerfile         Multi-stage build: compiles the converter, then runs the API
    docker-compose.yml Exposes 9015, mounts data/csv and data/bin as volumes
```

## Architecture

```
VPN
 │
 ▼
10.8.0.1:9015 (Docker container)
 │
 ├── data/csv   (source, admin only)
 ├── data/bin   (served to BacktestApp)
 ├── csv_to_bin_converter (compiled in the image)
 └── Web Admin UI (GET /)
```

## 1. Deploy / update the server

From a machine that can SSH into the server:

```bash
rsync -az --delete local_data_server/ hugo@10.8.0.1:~/fast-backtest-data-server/local_data_server/ \
    --exclude 'data/csv/*' --exclude 'data/bin/*'
rsync -az ThirdParty/cereal/ hugo@10.8.0.1:~/fast-backtest-data-server/ThirdParty/cereal/
```

Then, on the server (`ssh hugo@10.8.0.1`):

```bash
cd ~/fast-backtest-data-server/local_data_server
docker compose up -d --build
```

The user running this must be in the `docker` group (`sudo usermod -aG docker $USER`,
then re-login) since the API is started as a normal user, not root.

Stop with:

```bash
docker compose down
```

## 2. Add / update CSV source files

Drop CSVs into `~/fast-backtest-data-server/local_data_server/data/csv/` on the
server (e.g. via `scp`). Both existing formats are supported:

- `timestamp_unix,open,high,low,close[,volume]`
- `index,date_iso,open,high,low,close[,volume]` (e.g. `2025-05-16 18:43:10+00:00`)

## 3. Convert CSV → BIN

Either click **"Convert / Refresh BIN"** on the admin page (see below), or:

```bash
curl -X POST http://10.8.0.1:9015/admin/convert
```

Re-run any time the CSVs change; `.bin` files are simply regenerated.

## 4. Admin web UI

From a machine connected to the VPN:

```
http://10.8.0.1:9015
```

Shows CSV/BIN files (name, size, modified date) with **Refresh** and
**Convert / Refresh BIN** buttons and the conversion result/log.

## 5. REST API

Client-facing (used by BacktestApp):

- `GET /files` → JSON array of available `.bin` file names
- `GET /files/<filename>` → raw binary content of the file

Admin / ops:

- `GET /health` → `{"status": "ok"}`
- `GET /admin/status` → CSV/BIN listings + directories
- `POST /admin/convert` → runs the converter, returns a summary + log

## 6. Configure the application

The client uses `http://10.8.0.1:9015` by default (see
`backtestApp/include/components/Utils/apiConfig.h`). Override it via
`ApiConfig::setBaseUrl(...)` (stored in `QSettings`) to point at a different
host later (e.g. a future public endpoint) — no other code needs to change.

## 7. Client-side pipeline

- `ApiClient` (network layer) — `GET /files`, `GET /files/<name>` via
  `QNetworkAccessManager`. Connection failures (VPN off, host unreachable,
  timeout) are reported as *"Unable to connect to data server. Please check
  your VPN connection."* instead of crashing the app.
- `BinarySerializer` (serialization layer) — Cereal `BinaryInputArchive` →
  `std::vector<OHLCBar>`.
- `DataLoader::loadDataFromApi(symbol, interval, period, endDate, errorMessage)`
  — glues both together and reuses the existing period filtering /
  resampling logic. `BacktestWorker::run()` now calls this instead of the CSV
  path; the CSV parser lives server-side only (`converter/main.cpp`).

## Validated end-to-end (2026-08-16)

- Docker image built and started on `10.8.0.1` (`docker compose up -d --build`).
- `curl http://127.0.0.1:9015/health` (on the server) and
  `curl http://10.8.0.1:9015/health` (over VPN) both return `{"status": "ok"}`.
- `POST /admin/convert` converted a 2.14M-row `NDX_10secs_...csv` and a small
  sample CSV with 0 errors.
- `GET /files/<name>.bin` downloads bytes identical to the file on disk.
- The admin web page loads and lists CSV/BIN files.
- `BacktestApp` built successfully against the new `DataLoader::loadDataFromApi`
  path; the same symbol/interval → base-file matching logic used by the old
  CSV path (`NDX`/`20secs` → `NDX_10secs_...bin` for resampling) was confirmed
  against the live file list.
