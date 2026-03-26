#!/usr/bin/env python3
"""
Mock server para pruebas de http_sender.c.

Uso:
  python3 scripts/mock_server.py                    # escucha en :8080, responde 200
  python3 scripts/mock_server.py --port 9090        # puerto personalizado
  python3 scripts/mock_server.py --fail 401         # responde siempre con 401
  python3 scripts/mock_server.py --fail 500         # responde siempre con 500
  python3 scripts/mock_server.py --fail timeout     # cierra sin responder (simula timeout)

En config del validador:
  "base_url": "http://192.168.188.64:8080/IOTData"
"""

import argparse
import json
import sys
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer


def parse_args():
    p = argparse.ArgumentParser(description="Mock HTTP server para QW3100")
    p.add_argument("--port", type=int, default=8080)
    p.add_argument("--fail", default=None,
                   help="Código de fallo: 401, 500, timeout")
    return p.parse_args()


ARGS = parse_args()


class Handler(BaseHTTPRequestHandler):

    def log_message(self, fmt, *args):
        pass  # silenciar log nativo de BaseHTTPRequestHandler

    def do_POST(self):
        ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length) if length else b""

        if ARGS.fail == "timeout":
            print(f"[{ts}] POST {self.path} — cerrando sin responder (timeout simulado)")
            sys.stdout.flush()
            self.connection.close()
            return

        code = int(ARGS.fail) if ARGS.fail else 200

        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(b"{}")

        try:
            payload = json.loads(body)
            pretty = json.dumps(payload, indent=2)
        except Exception:
            pretty = body.decode("utf-8", errors="replace")

        print(f"[{ts}] POST {self.path}  →  HTTP {code}")
        print(pretty)
        sys.stdout.flush()


def main():
    server = HTTPServer(("0.0.0.0", ARGS.port), Handler)
    print(f"Mock server escuchando en :{ARGS.port}  "
          f"(modo: {'fail ' + ARGS.fail if ARGS.fail else 'OK 200'})")
    sys.stdout.flush()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido.")


if __name__ == "__main__":
    main()
