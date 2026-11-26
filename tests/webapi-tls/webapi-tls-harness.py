#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Test harness for the TLS setup of the WebAPI plugin.
#
# Runs "veyon-cli webapi runserver" from a build tree against freshly generated
# certificates/keys and checks the handshake behaviour from the client side.
#
# Isolation: the system configuration ("/etc/xdg/Veyon Solutions") is replaced by
# a temporary directory via a bind mount inside an unprivileged user+mount
# namespace, so the real configuration is never read or modified.
#
# Usage: webapi-tls-harness.py [--build-dir DIR] [--keep] [case ...]

import argparse
import http.client
import os
import shutil
import socket
import ssl
import subprocess
import sys
import tempfile
import time
from pathlib import Path

SYSTEM_CONFIG_DIR = "/etc/xdg/Veyon Solutions"
START_TIMEOUT = 15


def run(*cmd, **kwargs):
	subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, **kwargs)


def free_port():
	with socket.socket() as s:
		s.bind(("127.0.0.1", 0))
		return s.getsockname()[1]


# ---------------------------------------------------------------- key material

class Pki:
	def __init__(self, base: Path):
		self.base = base
		self.base.mkdir(parents=True, exist_ok=True)
		self._ext = base / "leaf.ext"
		self._ext.write_text("subjectAltName=DNS:localhost,IP:127.0.0.1\n"
							 "basicConstraints=CA:FALSE\n"
							 "keyUsage=digitalSignature,keyEncipherment\n"
							 "extendedKeyUsage=serverAuth\n")
		self._caext = base / "ca.ext"
		self._caext.write_text("basicConstraints=critical,CA:TRUE\nkeyUsage=keyCertSign,cRLSign\n")

	def p(self, name):
		return str(self.base / name)

	def genkey(self, name, kind):
		out = self.p(name)
		if kind == "rsa":
			run("openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", out)
		elif kind == "rsa-traditional":
			tmp = out + ".pk8"
			run("openssl", "genpkey", "-algorithm", "RSA", "-pkeyopt", "rsa_keygen_bits:2048", "-out", tmp)
			run("openssl", "rsa", "-in", tmp, "-traditional", "-out", out)
		elif kind == "ec":
			run("openssl", "genpkey", "-algorithm", "EC", "-pkeyopt", "ec_paramgen_curve:P-256", "-out", out)
		elif kind == "ec-traditional":
			run("openssl", "ecparam", "-name", "prime256v1", "-genkey", "-noout", "-out", out)
		elif kind == "ed25519":
			run("openssl", "genpkey", "-algorithm", "ED25519", "-out", out)
		else:
			raise ValueError(kind)
		return out

	def selfsigned(self, name, key):
		out = self.p(name)
		run("openssl", "req", "-new", "-x509", "-key", key, "-out", out, "-days", "2",
			"-subj", "/CN=localhost", "-addext", "subjectAltName=DNS:localhost,IP:127.0.0.1")
		return out

	def ca(self, name, key, issuer=None, issuer_key=None):
		out = self.p(name)
		if issuer is None:
			run("openssl", "req", "-new", "-x509", "-key", key, "-out", out, "-days", "2",
				"-subj", f"/CN={name}", "-addext", "basicConstraints=critical,CA:TRUE",
				"-addext", "keyUsage=keyCertSign,cRLSign")
		else:
			self._sign(out, key, f"/CN={name}", issuer, issuer_key, self._caext)
		return out

	def leaf(self, name, key, issuer, issuer_key):
		out = self.p(name)
		self._sign(out, key, "/CN=localhost", issuer, issuer_key, self._ext)
		return out

	def _sign(self, out, key, subj, issuer, issuer_key, ext):
		csr = out + ".csr"
		run("openssl", "req", "-new", "-key", key, "-subj", subj, "-out", csr)
		run("openssl", "x509", "-req", "-in", csr, "-CA", issuer, "-CAkey", issuer_key,
			"-CAcreateserial", "-days", "2", "-extfile", str(ext), "-out", out)

	def concat(self, name, *files):
		out = self.p(name)
		with open(out, "w") as f:
			for x in files:
				f.write(Path(x).read_text())
		return out

	def encrypt(self, name, key):
		out = self.p(name)
		run("openssl", "pkey", "-in", key, "-aes256", "-passout", "pass:secret", "-out", out)
		return out


# ---------------------------------------------------------------- server

class Stage:
	"""Install-like layout so that veyon-cli finds the plugins of the build tree."""

	def __init__(self, build: Path, base: Path):
		cache = (build / "CMakeCache.txt").read_text()
		libdir = next(line.split("=", 1)[1] for line in cache.splitlines()
					  if line.startswith("VEYON_LIB_DIR:"))
		self.bin = base / "bin"
		self.plugins = base / libdir
		self.bin.mkdir(parents=True)
		self.plugins.mkdir(parents=True)
		# applicationDirPath() resolves symlinks, so the binary itself has to be copied
		shutil.copy2(build / "cli" / "veyon-cli", self.bin / "veyon-cli")
		for so in (build / "plugins").rglob("*.so"):
			if "CMakeFiles" not in so.parts:
				(self.plugins / so.name).unlink(missing_ok=True)
				(self.plugins / so.name).symlink_to(so)
		self.cli = self.bin / "veyon-cli"


class Server:
	def __init__(self, stage: Stage, workdir: Path, cert, key, https=True):
		self.port = free_port()
		self.confdir = workdir / "config"
		self.confdir.mkdir(parents=True)
		self.logdir = workdir / "log"
		self.logdir.mkdir()
		(self.confdir / "Veyon.conf").write_text(
			"[Logging]\n"
			"LogToStdErr=true\n"
			f"LogFileDirectory={self.logdir}\n"
			"\n[WebAPI]\n"
			"HttpServerEnabled=true\n"
			f"HttpServerPort={self.port}\n"
			f"HttpsEnabled={'true' if https else 'false'}\n"
			f"TlsCertificateFile={cert or ''}\n"
			f"TlsPrivateKeyFile={key or ''}\n")
		self.output = workdir / "server.log"
		self._out = open(self.output, "w")
		env = dict(os.environ, VEYON_LOG_LEVEL="debug", QT_QPA_PLATFORM="offscreen")
		self.proc = subprocess.Popen(
			["unshare", "--user", "--map-root-user", "--mount", "--", "sh", "-c",
			 'mount --bind "$1" "$2" && exec "$3" webapi runserver',
			 "sh", str(self.confdir), SYSTEM_CONFIG_DIR, str(stage.cli)],
			stdout=self._out, stderr=subprocess.STDOUT, env=env)

	def wait_ready(self):
		"""True if the port accepts connections, False if the process exited first."""
		deadline = time.time() + START_TIMEOUT
		while time.time() < deadline:
			if self.proc.poll() is not None:
				return False
			try:
				socket.create_connection(("127.0.0.1", self.port), timeout=0.2).close()
				return True
			except OSError:
				time.sleep(0.1)
		return False

	def stop(self):
		if self.proc.poll() is None:
			self.proc.terminate()
			try:
				self.proc.wait(5)
			except subprocess.TimeoutExpired:
				self.proc.kill()
				self.proc.wait()
		self._out.close()

	def log(self):
		return self.output.read_text(errors="replace")


# ---------------------------------------------------------------- client probes

def tls_probe(port, cafile, minimum=None, maximum=None):
	"""Returns (ok, detail)."""
	ctx = ssl.create_default_context(cafile=cafile)
	if minimum:
		ctx.minimum_version = minimum
	if maximum:
		ctx.maximum_version = maximum
	try:
		with socket.create_connection(("127.0.0.1", port), timeout=5) as raw:
			with ctx.wrap_socket(raw, server_hostname="localhost") as s:
				return True, f"{s.version()} {s.cipher()[0]}"
	except (ssl.SSLError, OSError) as e:
		return False, f"{type(e).__name__}: {e}"


def https_get(port, cafile, path="/api/v1/authentication/"):
	ctx = ssl.create_default_context(cafile=cafile)
	try:
		c = http.client.HTTPSConnection("localhost", port, context=ctx, timeout=5)
		c.request("GET", path)
		r = c.getresponse()
		return r.status, r.read()[:200]
	except (ssl.SSLError, OSError, http.client.HTTPException) as e:
		return None, f"{type(e).__name__}: {e}"


def plain_http_get(port, path="/api/v1/authentication/"):
	"""A plaintext request against the HTTPS port must not get an HTTP answer."""
	try:
		c = http.client.HTTPConnection("127.0.0.1", port, timeout=3)
		c.request("GET", path)
		r = c.getresponse()
		return r.status
	except (OSError, http.client.HTTPException):
		return None


# ---------------------------------------------------------------- test cases

class Result:
	def __init__(self):
		self.failures = 0

	def check(self, cond, msg, detail=""):
		mark = "PASS" if cond else "FAIL"
		if not cond:
			self.failures += 1
		print(f"    [{mark}] {msg}" + (f"  ({detail})" if detail else ""))


def expect_working(res, srv, cafile):
	res.check(srv.wait_ready(), "server starts and listens")
	if srv.proc.poll() is not None:
		return
	ok, d = tls_probe(srv.port, cafile, minimum=ssl.TLSVersion.TLSv1_3)
	res.check(ok, "TLS 1.3 handshake with certificate verification", d)
	ok, d = tls_probe(srv.port, cafile, maximum=ssl.TLSVersion.TLSv1_2)
	res.check(not ok, "TLS 1.2 (max) is rejected", d)
	status, body = https_get(srv.port, cafile)
	# any HTTP answer from the WebAPI router proves the request went through TLS end-to-end
	res.check(status is not None and b"Invalid command" in body,
			  "HTTP request over TLS is answered by the WebAPI router", f"{status} {body!r}")
	status = plain_http_get(srv.port)
	res.check(status is None, "plaintext HTTP on HTTPS port gets no HTTP response", f"status={status}")


def expect_startup_failure(res, srv, cafile=None):
	ready = srv.wait_ready()
	if ready and cafile:
		ok, d = tls_probe(srv.port, cafile)
		res.check(False, "server refuses to start", f"started; handshake ok={ok}: {d}")
	else:
		res.check(not ready, "server refuses to start",
				  f"exit code {srv.proc.returncode}" if not ready else "started")


def build_cases(pki: Pki):
	cases = {}

	def keyed(kind):
		key = pki.genkey(f"{kind}.key", kind)
		cert = pki.selfsigned(f"{kind}.crt", key)
		return cert, key

	for kind in ("rsa", "rsa-traditional", "ec", "ec-traditional"):
		cert, key = keyed(kind)
		cases[kind] = dict(cert=cert, key=key, ca=cert, expect=expect_working,
						   desc=f"self-signed certificate, {kind} key")

	cert, key = keyed("ed25519")
	# QSslKey cannot parse Ed25519 keys, so the server has to fail closed
	cases["ed25519"] = dict(cert=cert, key=key, ca=cert, expect=expect_startup_failure,
							desc="self-signed certificate, Ed25519 key (unsupported by QSslKey)")

	rsa_cert = cases["rsa"]["cert"]
	other_key = pki.genkey("other.key", "rsa")
	cases["mismatch"] = dict(cert=rsa_cert, key=other_key, ca=rsa_cert, expect=expect_startup_failure,
							 desc="private key does not belong to the certificate")

	enc = pki.encrypt("encrypted.key", cases["rsa"]["key"])
	cases["encrypted"] = dict(cert=rsa_cert, key=enc, ca=rsa_cert, expect=expect_startup_failure,
							  desc="passphrase-protected private key")

	cases["missing-key"] = dict(cert=rsa_cert, key=pki.p("does-not-exist.key"), ca=None,
								expect=expect_startup_failure, desc="private key file missing")

	cases["garbage-key"] = dict(cert=rsa_cert, key=pki.concat("garbage.key", rsa_cert), ca=None,
								expect=expect_startup_failure, desc="key file contains a certificate")

	cases["swapped"] = dict(cert=cases["rsa"]["key"], key=rsa_cert, ca=None,
							expect=expect_startup_failure, desc="certificate and key file swapped")

	root_key = pki.genkey("root.key", "ec")
	root = pki.ca("root.crt", root_key)
	inter_key = pki.genkey("inter.key", "ec")
	inter = pki.ca("inter.crt", inter_key, root, root_key)
	leaf_key = pki.genkey("leaf.key", "rsa")
	leaf = pki.leaf("leaf.crt", leaf_key, inter, inter_key)
	cases["chain"] = dict(cert=pki.concat("fullchain.crt", leaf, inter), key=leaf_key, ca=root,
						  expect=expect_working,
						  desc="leaf + intermediate in certificate file, client trusts root only")
	return cases


def main():
	here = Path(__file__).resolve()
	default_build = here.parents[3] / "build-v4-Desktop_Qt_6_11_1-Debug"
	ap = argparse.ArgumentParser()
	ap.add_argument("--build-dir", type=Path, default=Path(os.environ.get("VEYON_BUILD_DIR", default_build)))
	ap.add_argument("--keep", action="store_true", help="keep temporary directory")
	ap.add_argument("--verbose", "-v", action="store_true", help="print server log for every case")
	ap.add_argument("cases", nargs="*")
	args = ap.parse_args()

	if not (args.build_dir / "cli" / "veyon-cli").exists():
		sys.exit(f"veyon-cli not found in build dir {args.build_dir}")

	tmp = Path(tempfile.mkdtemp(prefix="webapi-tls-"))
	try:
		stage = Stage(args.build_dir, tmp / "stage")
		cases = build_cases(Pki(tmp / "pki"))
		selected = args.cases or list(cases)
		res = Result()
		for name in selected:
			c = cases[name]
			print(f"[{name}] {c['desc']}")
			before = res.failures
			srv = Server(stage, tmp / "run" / name, c["cert"], c["key"])
			try:
				c["expect"](res, srv, c["ca"])
			finally:
				srv.stop()
			if args.verbose or res.failures > before:
				relevant = [l for l in srv.log().splitlines()
							if any(k in l for k in ("TLS", "tls", "key", "certificate", "listen", "Failed", "SSL", "ssl"))]
				for line in relevant[-15:]:
					print(f"      | {line}")
		print(f"\n{len(selected)} cases, {res.failures} failed checks")
		if args.keep:
			print(f"kept {tmp}")
		return 1 if res.failures else 0
	finally:
		if not args.keep:
			shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
	sys.exit(main())
