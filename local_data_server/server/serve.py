#!/usr/bin/env python3
"""REST API + minimal admin web UI serving Cereal binary market-data files.

Standard library only (no third-party dependencies), so it runs unchanged
inside the Docker image or directly on a dev machine.

Client-facing endpoints (used by BacktestApp):
    GET  /files              -> JSON array of available .bin file names
    GET  /files/<filename>    -> raw binary content of the file

Admin endpoints (used by the web UI):
    GET  /health              -> {"status": "ok"}
    GET  /admin/status        -> csv/bin listings + directories + converter path
    POST /admin/convert       -> runs the CSV -> BIN converter, returns a summary
    GET  /                    -> admin HTML page

Usage:
    python3 serve.py [--csv-dir DIR] [--bin-dir DIR] [--converter PATH]
                      [--host HOST] [--port PORT]
"""

import argparse
import json
import os
import subprocess
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote, urlparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_CSV_DIR = os.environ.get("CSV_DIR", os.path.join(SCRIPT_DIR, "..", "data", "csv"))
DEFAULT_BIN_DIR = os.environ.get("BIN_DIR", os.path.join(SCRIPT_DIR, "..", "data", "bin"))
DEFAULT_CONVERTER = os.environ.get("CONVERTER_PATH", "/app/bin/csv_to_bin_converter")


def _log(message: str) -> None:
    print(f"[data-server] {message}", flush=True)


def _list_dir_entries(directory: str, extension: str):
    entries = []
    try:
        for name in sorted(os.listdir(directory)):
            path = os.path.join(directory, name)
            if not os.path.isfile(path) or not name.endswith(extension):
                continue
            stat = os.stat(path)
            entries.append({
                "name": name,
                "sizeBytes": stat.st_size,
                "modifiedAt": time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(stat.st_mtime)),
            })
    except FileNotFoundError:
        pass
    return entries


ADMIN_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>Market Data Server - Admin</title>
<style>
  body { font-family: sans-serif; margin: 2rem; background: #111; color: #eee; }
  h1 { font-size: 1.4rem; }
  h2 { font-size: 1.1rem; margin-top: 2rem; }
  table { border-collapse: collapse; width: 100%; margin-top: 0.5rem; }
  th, td { border: 1px solid #333; padding: 4px 8px; text-align: left; font-size: 0.9rem; }
  th { background: #222; }
  button { padding: 6px 14px; margin-right: 8px; cursor: pointer; }
  #result { white-space: pre-wrap; background: #1a1a1a; padding: 10px; margin-top: 1rem;
            border: 1px solid #333; min-height: 2rem; }
  .ok { color: #6f6; }
  .err { color: #f66; }
</style>
</head>
<body>
  <h1>Market Data Server - Admin</h1>
  <div>
    <button onclick="refresh()">Refresh</button>
    <button onclick="convert()">Convert / Refresh BIN</button>
  </div>
  <div id="result"></div>

  <h2>CSV files (server-side source, data/csv)</h2>
  <table id="csvTable"><thead><tr><th>Name</th><th>Size</th><th>Modified</th></tr></thead><tbody></tbody></table>

  <h2>BIN files (served to the application, data/bin)</h2>
  <table id="binTable"><thead><tr><th>Name</th><th>Size</th><th>Modified</th></tr></thead><tbody></tbody></table>

<script>
function fmtSize(n) {
  if (n < 1024) return n + " B";
  if (n < 1024*1024) return (n/1024).toFixed(1) + " KB";
  return (n/1024/1024).toFixed(1) + " MB";
}

function fillTable(id, rows) {
  const tbody = document.querySelector(id + " tbody");
  tbody.innerHTML = "";
  for (const r of rows) {
    const tr = document.createElement("tr");
    tr.innerHTML = `<td>${r.name}</td><td>${fmtSize(r.sizeBytes)}</td><td>${r.modifiedAt}</td>`;
    tbody.appendChild(tr);
  }
}

async function refresh() {
  const res = await fetch("/admin/status");
  const data = await res.json();
  fillTable("#csvTable", data.csvFiles);
  fillTable("#binTable", data.binFiles);
}

async function convert() {
  const result = document.getElementById("result");
  result.textContent = "Converting...";
  try {
    const res = await fetch("/admin/convert", { method: "POST" });
    const data = await res.json();
    if (res.ok) {
      result.className = "ok";
      result.textContent =
        `Conversion done.\\n${data.csvCount} CSV file(s) detected.\\n${data.convertedCount} BIN file(s) generated.\\n${data.errorCount} error(s).\\n\\n` + (data.log || "");
    } else {
      result.className = "err";
      result.textContent = "Conversion failed: " + (data.error || "unknown error");
    }
  } catch (e) {
    result.className = "err";
    result.textContent = "Request failed: " + e;
  }
  refresh();
}

refresh();
</script>
</body>
</html>
"""


class DataServerHandler(BaseHTTPRequestHandler):
    csv_dir = DEFAULT_CSV_DIR
    bin_dir = DEFAULT_BIN_DIR
    converter_path = DEFAULT_CONVERTER

    # ---- response helpers -------------------------------------------------

    def _send_json(self, status: int, payload) -> None:
        body = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _send_error_json(self, status: int, message: str) -> None:
        _log(f"ERROR {status}: {message}")
        self._send_json(status, {"error": message})

    def _send_html(self, status: int, html: str) -> None:
        body = html.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # ---- routing ------------------------------------------------------

    def do_GET(self):  # noqa: N802 (http.server API)
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/" or path == "/index.html":
            self._send_html(200, ADMIN_PAGE)
            return
        if path == "/health":
            self._send_json(200, {"status": "ok"})
            return
        if path == "/admin/status":
            self._handle_status()
            return
        if path == "/files" or path == "/files/":
            self._handle_list_files()
            return
        if path.startswith("/files/"):
            self._handle_download_file(path[len("/files/"):])
            return

        self._send_error_json(404, f"Unknown endpoint: {path}")

    def do_POST(self):  # noqa: N802 (http.server API)
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/admin/convert":
            self._handle_convert()
            return

        self._send_error_json(404, f"Unknown endpoint: {path}")

    # ---- client-facing endpoints ---------------------------------------

    def _handle_list_files(self) -> None:
        try:
            names = sorted(
                f for f in os.listdir(self.bin_dir)
                if f.endswith(".bin") and os.path.isfile(os.path.join(self.bin_dir, f))
            )
        except FileNotFoundError:
            self._send_error_json(500, f"Binary directory not found: {self.bin_dir}")
            return
        _log(f"GET /files -> {len(names)} file(s)")
        self._send_json(200, names)

    def _handle_download_file(self, filename: str) -> None:
        safe_name = os.path.basename(filename)  # prevent path traversal
        if not safe_name.endswith(".bin"):
            self._send_error_json(400, "Only .bin files can be downloaded")
            return

        file_path = os.path.join(self.bin_dir, safe_name)
        if not os.path.isfile(file_path):
            self._send_error_json(404, f"File not found: {safe_name}")
            return

        try:
            with open(file_path, "rb") as f:
                data = f.read()
        except OSError as exc:
            self._send_error_json(500, f"Failed to read file: {exc}")
            return

        _log(f"GET /files/{safe_name} -> {len(data)} bytes")
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    # ---- admin endpoints -------------------------------------------------

    def _handle_status(self) -> None:
        payload = {
            "csvDir": self.csv_dir,
            "binDir": self.bin_dir,
            "csvFiles": _list_dir_entries(self.csv_dir, ".csv"),
            "binFiles": _list_dir_entries(self.bin_dir, ".bin"),
        }
        payload["csvCount"] = len(payload["csvFiles"])
        payload["binCount"] = len(payload["binFiles"])
        self._send_json(200, payload)

    def _handle_convert(self) -> None:
        if not os.path.isfile(self.converter_path) or not os.access(self.converter_path, os.X_OK):
            self._send_error_json(500, f"Converter binary not found or not executable: {self.converter_path}")
            return

        os.makedirs(self.bin_dir, exist_ok=True)
        csv_count = len(_list_dir_entries(self.csv_dir, ".csv"))

        _log(f"Running converter: {self.converter_path} {self.csv_dir} {self.bin_dir}")
        try:
            proc = subprocess.run(
                [self.converter_path, self.csv_dir, self.bin_dir],
                capture_output=True, text=True, timeout=300,
            )
        except subprocess.TimeoutExpired:
            self._send_error_json(500, "Conversion timed out after 300s")
            return
        except OSError as exc:
            self._send_error_json(500, f"Failed to launch converter: {exc}")
            return

        log_output = (proc.stdout or "") + (proc.stderr or "")
        converted_count = len(_list_dir_entries(self.bin_dir, ".bin"))
        error_count = log_output.count("ERROR:")

        _log(f"Conversion finished (exit={proc.returncode}): {csv_count} CSV, "
             f"{converted_count} BIN, {error_count} error(s)")

        self._send_json(200 if proc.returncode == 0 else 500, {
            "csvCount": csv_count,
            "convertedCount": converted_count,
            "errorCount": error_count,
            "exitCode": proc.returncode,
            "log": log_output,
        })

    def log_message(self, fmt, *args):
        _log(f"{self.address_string()} - {fmt % args}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Market data REST API + admin UI")
    parser.add_argument("--csv-dir", default=DEFAULT_CSV_DIR, help="Directory containing source .csv files")
    parser.add_argument("--bin-dir", default=DEFAULT_BIN_DIR, help="Directory containing served .bin files")
    parser.add_argument("--converter", default=DEFAULT_CONVERTER, help="Path to the csv_to_bin_converter binary")
    parser.add_argument("--host", default="0.0.0.0", help="Interface to bind (0.0.0.0 for Docker/VPN access)")
    parser.add_argument("--port", type=int, default=9015, help="Port to listen on")
    args = parser.parse_args()

    csv_dir = os.path.abspath(args.csv_dir)
    bin_dir = os.path.abspath(args.bin_dir)
    os.makedirs(csv_dir, exist_ok=True)
    os.makedirs(bin_dir, exist_ok=True)

    DataServerHandler.csv_dir = csv_dir
    DataServerHandler.bin_dir = bin_dir
    DataServerHandler.converter_path = args.converter

    server = ThreadingHTTPServer((args.host, args.port), DataServerHandler)
    _log("API started")
    _log(f"Listening on: http://{args.host}:{args.port}")
    _log(f"CSV dir: {csv_dir}")
    _log(f"BIN dir: {bin_dir}")
    _log(f"Converter: {args.converter}")
    _log("Endpoints: GET /health, GET /files, GET /files/<name>, GET /admin/status, POST /admin/convert, GET /")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        _log("Shutting down.")
        server.shutdown()


if __name__ == "__main__":
    main()
