#!/usr/bin/env python3
"""
Mock server para pruebas de http_sender.c.

Uso:
  python3 scripts/mock_server.py                    # HTTP, :8080, responde 200
  python3 scripts/mock_server.py --port 9090        # puerto personalizado
  python3 scripts/mock_server.py --fail 401         # responde siempre con 401
  python3 scripts/mock_server.py --fail 500         # responde siempre con 500
  python3 scripts/mock_server.py --fail timeout     # cierra sin responder (simula timeout)
  python3 scripts/mock_server.py --tls              # HTTPS con cert autofirmado

Modo TLS:
  Al usar --tls se genera un certificado autofirmado temporal (válido 1 día).
  El daemon debe apuntar a ese cert en "api.ca_bundle_path" del config JSON.
  El servidor imprime la ruta del cert al iniciar para copiarlo al validador.

  Generar un cert reutilizable (para pruebas repetidas sin reconfigurar):
    openssl req -x509 -newkey rsa:2048 -keyout /tmp/mock.key \\
      -out /tmp/mock.crt -days 30 -nodes -subj "/CN=localhost"
    python3 scripts/mock_server.py --tls --cert /tmp/mock.crt --key /tmp/mock.key

  En config del validador con TLS:
    "api": {
      "base_url": "https://192.168.188.64:8080/IOTData",
      "ca_bundle_path": "/tmp/mock.crt"
    }

En config del validador sin TLS:
  "base_url": "http://192.168.188.64:8080/IOTData"
"""

import argparse
import json
import os
import ssl
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, HTTPServer


def parse_args():
    p = argparse.ArgumentParser(description="Mock HTTP server para QW3100")
    p.add_argument("--port", type=int, default=8080)
    p.add_argument("--fail", default=None,
                   help="Código de fallo: 401, 500, timeout")
    p.add_argument("--tls", action="store_true",
                   help="Servir sobre HTTPS con certificado autofirmado")
    p.add_argument("--cert", default=None,
                   help="Ruta al certificado TLS (PEM). Si no se indica, se genera uno temporal.")
    p.add_argument("--key", default=None,
                   help="Ruta a la clave privada TLS (PEM). Requerida si --cert se indica.")
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


def generate_self_signed_cert():
    """Genera cert autofirmado en un directorio temporal. Retorna (cert_path, key_path, tmpdir)."""
    tmpdir = tempfile.mkdtemp(prefix="qw3100_mock_tls_")
    cert_path = os.path.join(tmpdir, "server.crt")
    key_path  = os.path.join(tmpdir, "server.key")
    subprocess.run(
        [
            "openssl", "req", "-x509", "-newkey", "rsa:2048",
            "-keyout", key_path, "-out", cert_path,
            "-days", "1", "-nodes", "-subj", "/CN=localhost",
        ],
        check=True,
        capture_output=True,
    )
    return cert_path, key_path, tmpdir


def main():
    server = HTTPServer(("0.0.0.0", ARGS.port), Handler)
    tmpdir = None
    proto  = "http"

    if ARGS.tls:
        proto = "https"
        if ARGS.cert:
            cert_path = ARGS.cert
            key_path  = ARGS.key or ARGS.cert  # PEM combinado si --key no se indica
        else:
            cert_path, key_path, tmpdir = generate_self_signed_cert()
            print(f"Cert autofirmado generado en: {cert_path}")
            print(f"  → copiar a validador y configurar api.ca_bundle_path = \"{cert_path}\"")

        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(certfile=cert_path, keyfile=key_path)
        server.socket = context.wrap_socket(server.socket, server_side=True)

    modo = "fail " + ARGS.fail if ARGS.fail else "OK 200"
    print(f"Mock server escuchando en {proto}://0.0.0.0:{ARGS.port}  (modo: {modo})")
    sys.stdout.flush()

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido.")
    finally:
        if tmpdir:
            import shutil
            shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
