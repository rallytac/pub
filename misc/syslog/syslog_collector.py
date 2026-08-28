#!/usr/bin/env python3
"""
Copyright (c) 2026 Rally Tactical Systems, Inc.
All rights reserved.

Standalone syslog collector: listen for RFC 3164 / RFC 5424 messages over UDP
(and optionally TCP) and write them to stdout and/or a log file.

Stdlib only. Python 3.9+.
"""

from __future__ import annotations

import argparse
import json
import re
import select
import signal
import socket
import sys
import threading
import time
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional

MIN_PYTHON = (3, 9)

# Default privileged syslog port; use --port 5514 (or similar) without root.
DEFAULT_BIND = "0.0.0.0"
DEFAULT_PORT = 514
DEFAULT_RECV_BUF = 65535
TCP_ACCEPT_BACKLOG = 32
TCP_CLIENT_BUF = 65536

FACILITY_NAMES = {
    0: "kern",
    1: "user",
    2: "mail",
    3: "daemon",
    4: "auth",
    5: "syslog",
    6: "lpr",
    7: "news",
    8: "uucp",
    9: "cron",
    10: "authpriv",
    11: "ftp",
    12: "ntp",
    13: "security",
    14: "console",
    15: "solaris-cron",
    16: "local0",
    17: "local1",
    18: "local2",
    19: "local3",
    20: "local4",
    21: "local5",
    22: "local6",
    23: "local7",
}

SEVERITY_NAMES = {
    0: "emerg",
    1: "alert",
    2: "crit",
    3: "err",
    4: "warning",
    5: "notice",
    6: "info",
    7: "debug",
}

# Match Engage ConsoleLogger severity colors.
ANSI_RESET = "\033[0m"
COLOR_FATAL = "\033[37;1;41m"  # emerg / alert / crit
COLOR_ERROR = "\033[31;1m"  # err
COLOR_WARNING = "\033[33;1m"  # warning
COLOR_INFO = "\033[32;1m"  # notice / info
COLOR_DEBUG = "\033[37m"  # debug
COLOR_UNKNOWN = "\033[90m"  # unparsed / no severity

# RFC 3164 PRI + optional timestamp/hostname/tag
_RFC3164_RE = re.compile(
    r"^<(?P<pri>\d{1,3})>"
    r"(?P<body>.*)$",
    re.DOTALL,
)

_RFC3164_STAMP_RE = re.compile(
    r"^(?P<stamp>"
    r"(?:Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+"
    r"\d{1,2}\s+\d{2}:\d{2}:\d{2}"
    r")\s+"
    r"(?P<host>\S+)\s+"
    r"(?P<rest>.*)$",
    re.DOTALL,
)

_RFC3164_TAG_RE = re.compile(
    r"^(?P<tag>[^:\s\[]+)(?:\[(?P<pid>\d+)\])?:\s*(?P<msg>.*)$",
    re.DOTALL,
)

# RFC 5424 structured header (NILVALUE is "-")
_RFC5424_RE = re.compile(
    r"^<(?P<pri>\d{1,3})>"
    r"(?P<version>\d)\s+"
    r"(?P<timestamp>\S+)\s+"
    r"(?P<hostname>\S+)\s+"
    r"(?P<app_name>\S+)\s+"
    r"(?P<procid>\S+)\s+"
    r"(?P<msgid>\S+)\s+"
    r"(?P<rest>.*)$",
    re.DOTALL,
)

_STRUCTURED_DATA_RE = re.compile(
    r"^(?P<sd>(?:-|\[(?:[^\]]|\\.)*\](?:\[(?:[^\]]|\\.)*\])*))"
    r"(?:\s(?P<msg>.*))?$",
    re.DOTALL,
)


@dataclass
class SyslogMessage:
    raw: str
    protocol: str  # "rfc3164" | "rfc5424" | "unknown"
    transport: str  # "udp" | "tcp"
    source_ip: str
    source_port: int
    received_at: str
    pri: Optional[int] = None
    facility: Optional[int] = None
    facility_name: Optional[str] = None
    severity: Optional[int] = None
    severity_name: Optional[str] = None
    version: Optional[int] = None
    timestamp: Optional[str] = None
    hostname: Optional[str] = None
    app_name: Optional[str] = None
    procid: Optional[str] = None
    msgid: Optional[str] = None
    structured_data: Optional[str] = None
    message: Optional[str] = None


@dataclass
class CollectorStats:
    udp_datagrams: int = 0
    tcp_messages: int = 0
    parse_ok: int = 0
    parse_unknown: int = 0
    bytes_in: int = 0
    started_at: float = 0.0


def check_python() -> None:
    if sys.version_info < MIN_PYTHON:
        print(
            f"Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ required "
            f"(found {sys.version_info.major}.{sys.version_info.minor})",
            file=sys.stderr,
        )
        sys.exit(1)


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3] + "Z"


def decode_pri(pri: int) -> tuple[int, int, str, str]:
    facility = pri >> 3
    severity = pri & 0x07
    return (
        facility,
        severity,
        FACILITY_NAMES.get(facility, f"facility{facility}"),
        SEVERITY_NAMES.get(severity, f"severity{severity}"),
    )


def nil_or(value: str) -> Optional[str]:
    if value == "-":
        return None
    return value


def parse_syslog(raw: str, transport: str, source_ip: str, source_port: int) -> SyslogMessage:
    received_at = utc_now_iso()
    text = raw.rstrip("\0").strip("\r\n")

    base = SyslogMessage(
        raw=text,
        protocol="unknown",
        transport=transport,
        source_ip=source_ip,
        source_port=source_port,
        received_at=received_at,
        message=text,
    )

    m5424 = _RFC5424_RE.match(text)
    if m5424:
        pri = int(m5424.group("pri"))
        facility, severity, fac_name, sev_name = decode_pri(pri)
        rest = m5424.group("rest")
        sd_match = _STRUCTURED_DATA_RE.match(rest)
        structured = None
        msg = rest
        if sd_match:
            structured = nil_or(sd_match.group("sd") or "-")
            msg = sd_match.group("msg")
        return SyslogMessage(
            raw=text,
            protocol="rfc5424",
            transport=transport,
            source_ip=source_ip,
            source_port=source_port,
            received_at=received_at,
            pri=pri,
            facility=facility,
            facility_name=fac_name,
            severity=severity,
            severity_name=sev_name,
            version=int(m5424.group("version")),
            timestamp=nil_or(m5424.group("timestamp")),
            hostname=nil_or(m5424.group("hostname")),
            app_name=nil_or(m5424.group("app_name")),
            procid=nil_or(m5424.group("procid")),
            msgid=nil_or(m5424.group("msgid")),
            structured_data=structured,
            message=msg if msg is not None else "",
        )

    m3164 = _RFC3164_RE.match(text)
    if m3164:
        pri = int(m3164.group("pri"))
        facility, severity, fac_name, sev_name = decode_pri(pri)
        body = m3164.group("body")
        stamp = None
        host = None
        app_name = None
        procid = None
        message = body

        stamp_m = _RFC3164_STAMP_RE.match(body)
        if stamp_m:
            stamp = stamp_m.group("stamp")
            host = stamp_m.group("host")
            rest = stamp_m.group("rest")
            tag_m = _RFC3164_TAG_RE.match(rest)
            if tag_m:
                app_name = tag_m.group("tag")
                procid = tag_m.group("pid")
                message = tag_m.group("msg")
            else:
                message = rest

        return SyslogMessage(
            raw=text,
            protocol="rfc3164",
            transport=transport,
            source_ip=source_ip,
            source_port=source_port,
            received_at=received_at,
            pri=pri,
            facility=facility,
            facility_name=fac_name,
            severity=severity,
            severity_name=sev_name,
            timestamp=stamp,
            hostname=host,
            app_name=app_name,
            procid=procid,
            message=message,
        )

    return base


def severity_ansi(severity: Optional[int]) -> str:
    if severity is None:
        return COLOR_UNKNOWN
    if severity <= 2:
        return COLOR_FATAL
    if severity == 3:
        return COLOR_ERROR
    if severity == 4:
        return COLOR_WARNING
    if severity <= 6:
        return COLOR_INFO
    return COLOR_DEBUG


def format_pretty(msg: SyslogMessage, color: bool = False) -> str:
    sev = msg.severity_name or "?"
    fac = msg.facility_name or "?"
    host = msg.hostname or msg.source_ip
    app = msg.app_name or "-"
    body = msg.message if msg.message is not None else msg.raw
    line = (
        f"{msg.received_at} {msg.transport.upper()} "
        f"{msg.source_ip}:{msg.source_port} "
        f"[{fac}.{sev}] {host} {app}: {body}"
    )
    if not color:
        return line
    return f"{severity_ansi(msg.severity)}{line}{ANSI_RESET}"


def format_message(msg: SyslogMessage, fmt: str, color: bool = False) -> str:
    if fmt == "raw":
        return msg.raw
    if fmt == "json":
        return json.dumps(asdict(msg), ensure_ascii=False, separators=(",", ":"))
    return format_pretty(msg, color=color)


def want_color(mode: str, stream) -> bool:
    if mode == "always":
        return True
    if mode == "never":
        return False
    # auto
    if hasattr(stream, "isatty") and stream.isatty():
        if sys.platform == "win32":
            # Enable VT processing when possible; ignore failures.
            try:
                import ctypes

                kernel32 = ctypes.windll.kernel32  # type: ignore[attr-defined]
                handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
                mode_flags = ctypes.c_uint32()
                if kernel32.GetConsoleMode(handle, ctypes.byref(mode_flags)):
                    ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
                    kernel32.SetConsoleMode(
                        handle, mode_flags.value | ENABLE_VIRTUAL_TERMINAL_PROCESSING
                    )
            except Exception:
                pass
        return True
    return False


class OutputWriter:
    def __init__(self, path: Optional[Path], also_stdout: bool) -> None:
        self._path = path
        self._also_stdout = also_stdout
        self._fp = None
        self._lock = threading.Lock()
        if path is not None:
            path.parent.mkdir(parents=True, exist_ok=True)
            self._fp = path.open("a", encoding="utf-8", buffering=1)

    def write(self, plain: str, colored: Optional[str] = None) -> None:
        with self._lock:
            if self._also_stdout or self._fp is None:
                print(colored if colored is not None else plain, flush=True)
            if self._fp is not None:
                self._fp.write(plain + "\n")
                self._fp.flush()

    def close(self) -> None:
        with self._lock:
            if self._fp is not None:
                self._fp.close()
                self._fp = None


class SyslogCollector:
    def __init__(
        self,
        bind: str,
        port: int,
        tcp: bool,
        fmt: str,
        writer: OutputWriter,
        max_severity: Optional[int],
        facility_filter: Optional[set[int]],
        color: bool,
    ) -> None:
        self._bind = bind
        self._port = port
        self._tcp = tcp
        self._fmt = fmt
        self._writer = writer
        self._max_severity = max_severity
        self._facility_filter = facility_filter
        self._color = color and fmt == "pretty"
        self._stats = CollectorStats(started_at=time.monotonic())
        self._stop = threading.Event()
        self._udp: Optional[socket.socket] = None
        self._tcp_sock: Optional[socket.socket] = None

    def stop(self) -> None:
        self._stop.set()

    def stats_line(self) -> str:
        elapsed = max(0.001, time.monotonic() - self._stats.started_at)
        return (
            f"stats udp={self._stats.udp_datagrams} "
            f"tcp={self._stats.tcp_messages} "
            f"ok={self._stats.parse_ok} "
            f"unknown={self._stats.parse_unknown} "
            f"bytes={self._stats.bytes_in} "
            f"rate={self._stats.udp_datagrams / elapsed:.1f}/s"
        )

    def _accept_message(self, msg: SyslogMessage) -> bool:
        if self._max_severity is not None and msg.severity is not None:
            if msg.severity > self._max_severity:
                return False
        if self._facility_filter is not None and msg.facility is not None:
            if msg.facility not in self._facility_filter:
                return False
        return True

    def _handle(self, raw: str, transport: str, addr: tuple[str, int]) -> None:
        self._stats.bytes_in += len(raw.encode("utf-8", errors="replace"))
        msg = parse_syslog(raw, transport, addr[0], addr[1])
        if msg.protocol == "unknown":
            self._stats.parse_unknown += 1
        else:
            self._stats.parse_ok += 1
        if transport == "udp":
            self._stats.udp_datagrams += 1
        else:
            self._stats.tcp_messages += 1
        if not self._accept_message(msg):
            return
        plain = format_message(msg, self._fmt, color=False)
        colored = (
            format_message(msg, self._fmt, color=True) if self._color else None
        )
        self._writer.write(plain, colored=colored)

    def _bind_udp(self) -> socket.socket:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # SO_REUSEPORT helps when restarting quickly on some platforms.
        if hasattr(socket, "SO_REUSEPORT"):
            try:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            except OSError:
                pass
        sock.bind((self._bind, self._port))
        sock.setblocking(False)
        return sock

    def _bind_tcp(self) -> socket.socket:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((self._bind, self._port))
        sock.listen(TCP_ACCEPT_BACKLOG)
        sock.setblocking(False)
        return sock

    def _tcp_client_loop(self, conn: socket.socket, addr: tuple[str, int]) -> None:
        buf = b""
        try:
            conn.settimeout(1.0)
            while not self._stop.is_set():
                try:
                    chunk = conn.recv(TCP_CLIENT_BUF)
                except socket.timeout:
                    continue
                except OSError:
                    break
                if not chunk:
                    break
                buf += chunk
                while True:
                    # Octet-counting framing: "123 <PRI>..."
                    if buf and buf[0:1].isdigit():
                        sp = buf.find(b" ")
                        if sp <= 0:
                            break
                        try:
                            length = int(buf[:sp].decode("ascii"))
                        except ValueError:
                            # Fall through to newline framing for this buffer.
                            break
                        total = sp + 1 + length
                        if len(buf) < total:
                            break
                        payload = buf[sp + 1 : total]
                        buf = buf[total:]
                        text = payload.decode("utf-8", errors="replace")
                        self._handle(text, "tcp", addr)
                        continue

                    # Trailer / newline framing
                    nl = buf.find(b"\n")
                    if nl < 0:
                        break
                    line = buf[:nl]
                    buf = buf[nl + 1 :]
                    text = line.decode("utf-8", errors="replace").rstrip("\r")
                    if text:
                        self._handle(text, "tcp", addr)
        finally:
            try:
                conn.close()
            except OSError:
                pass

    def run(self) -> None:
        self._udp = self._bind_udp()
        sockets: list[socket.socket] = [self._udp]
        if self._tcp:
            self._tcp_sock = self._bind_tcp()
            sockets.append(self._tcp_sock)

        proto = "UDP+TCP" if self._tcp else "UDP"
        print(
            f"syslog collector listening on {self._bind}:{self._port} ({proto})",
            file=sys.stderr,
        )

        try:
            while not self._stop.is_set():
                readable, _, _ = select.select(sockets, [], [], 0.5)
                for sock in readable:
                    if sock is self._udp:
                        try:
                            data, addr = sock.recvfrom(DEFAULT_RECV_BUF)
                        except BlockingIOError:
                            continue
                        text = data.decode("utf-8", errors="replace")
                        self._handle(text, "udp", (addr[0], addr[1]))
                    elif sock is self._tcp_sock:
                        try:
                            conn, addr = sock.accept()
                        except BlockingIOError:
                            continue
                        t = threading.Thread(
                            target=self._tcp_client_loop,
                            args=(conn, addr),
                            daemon=True,
                        )
                        t.start()
        finally:
            if self._udp is not None:
                self._udp.close()
            if self._tcp_sock is not None:
                self._tcp_sock.close()


def parse_facility_list(value: str) -> set[int]:
    names = {v: k for k, v in FACILITY_NAMES.items()}
    out: set[int] = set()
    for part in value.split(","):
        part = part.strip().lower()
        if not part:
            continue
        if part.isdigit():
            out.add(int(part))
            continue
        if part not in names:
            raise argparse.ArgumentTypeError(
                f"unknown facility '{part}' "
                f"(use name like local0 or number 0-23)"
            )
        out.add(names[part])
    if not out:
        raise argparse.ArgumentTypeError("facility list is empty")
    return out


def parse_severity_name(value: str) -> int:
    value = value.strip().lower()
    if value.isdigit():
        n = int(value)
        if n < 0 or n > 7:
            raise argparse.ArgumentTypeError("severity must be 0-7")
        return n
    names = {v: k for k, v in SEVERITY_NAMES.items()}
    if value not in names:
        raise argparse.ArgumentTypeError(
            f"unknown severity '{value}' "
            f"(emerg|alert|crit|err|warning|notice|info|debug)"
        )
    return names[value]


def build_arg_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            "Standalone syslog collector (RFC 3164 / RFC 5424 over UDP, "
            "optional TCP)."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("--bind", default=DEFAULT_BIND, help="Local bind address")
    p.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help="UDP (and optional TCP) listen port; 514 usually needs root",
    )
    p.add_argument(
        "--tcp",
        action="store_true",
        help="Also accept TCP on the same port (octet-counting or newline framing)",
    )
    p.add_argument(
        "--format",
        choices=("pretty", "raw", "json"),
        default="pretty",
        help="Output format",
    )
    p.add_argument(
        "-o",
        "--output-file",
        type=Path,
        default=None,
        help="Append collected lines to this file",
    )
    p.add_argument(
        "--file-only",
        action="store_true",
        help="With --output-file, do not also print to stdout",
    )
    p.add_argument(
        "--max-severity",
        type=parse_severity_name,
        default=None,
        metavar="LEVEL",
        help=(
            "Keep messages at this severity or more severe "
            "(0=emerg … 7=debug). Example: warning"
        ),
    )
    p.add_argument(
        "--facility",
        type=parse_facility_list,
        default=None,
        metavar="LIST",
        help="Comma-separated facility names/numbers to keep (e.g. local0,daemon)",
    )
    p.add_argument(
        "--stats-interval",
        type=float,
        default=0.0,
        help="Print collector stats to stderr every N seconds (0=disabled)",
    )
    p.add_argument(
        "--color",
        choices=("auto", "always", "never"),
        default="auto",
        help="ANSI color for pretty stdout (file output is always plain)",
    )
    return p


def main(argv: Optional[list[str]] = None) -> int:
    check_python()
    args = build_arg_parser().parse_args(argv)

    if args.file_only and args.output_file is None:
        print("--file-only requires --output-file", file=sys.stderr)
        return 2

    also_stdout = not (args.file_only and args.output_file is not None)
    color = want_color(args.color, sys.stdout)
    writer = OutputWriter(args.output_file, also_stdout=also_stdout)
    collector = SyslogCollector(
        bind=args.bind,
        port=args.port,
        tcp=args.tcp,
        fmt=args.format,
        writer=writer,
        max_severity=args.max_severity,
        facility_filter=args.facility,
        color=color,
    )

    def _handle_signal(signum: int, _frame: object) -> None:
        if signum == signal.SIGUSR1:
            print(collector.stats_line(), file=sys.stderr)
            return
        collector.stop()

    signal.signal(signal.SIGINT, _handle_signal)
    signal.signal(signal.SIGTERM, _handle_signal)
    if hasattr(signal, "SIGUSR1"):
        signal.signal(signal.SIGUSR1, _handle_signal)

    stats_thread: Optional[threading.Thread] = None
    if args.stats_interval and args.stats_interval > 0:

        def _stats_loop() -> None:
            while not collector._stop.wait(args.stats_interval):
                print(collector.stats_line(), file=sys.stderr)

        stats_thread = threading.Thread(target=_stats_loop, daemon=True)
        stats_thread.start()

    try:
        collector.run()
    except PermissionError:
        print(
            f"Permission denied binding {args.bind}:{args.port}. "
            f"Try --port 5514 (or run with elevated privileges for port 514).",
            file=sys.stderr,
        )
        return 1
    except OSError as exc:
        print(f"Failed to listen on {args.bind}:{args.port}: {exc}", file=sys.stderr)
        return 1
    finally:
        writer.close()
        print(collector.stats_line(), file=sys.stderr)

    return 0


if __name__ == "__main__":
    sys.exit(main())
