#!/usr/bin/env python3
"""
Copyright (c) 2026 Rally Tactical Systems, Inc.
All rights reserved.

Listen for Cursor on Target (CoT) XML on a UDP multicast group and display a live
table of detected entities with color-coded liveness. Optionally serve a Leaflet
map in the browser with shape-coded markers (--map).

Status colors:
  green  — alive: last update within the warning window and before stale time
  yellow — suspect offline: no recent update but CoT stale time not yet reached
  red    — stale: current time is past the event's stale attribute
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import subprocess
import sys
import threading
import time
import webbrowser
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from datetime import datetime, timezone
from enum import Enum
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Dict, List, Optional
from urllib.parse import urlparse

# curses is optional at import time so --map-only works on Windows without
# windows-curses. The TUI path loads it via require_curses().
try:
    import curses  # type: ignore
except ImportError:  # pragma: no cover - platform dependent
    curses = None  # type: ignore


# MITRE CoT / ATAK Mesh SA default multicast (UDP).
DEFAULT_MCAST = "239.2.3.1"
DEFAULT_PORT = 6969
DEFAULT_WARN_SECS = 30.0
REFRESH_HZ = 4.0
DEFAULT_MAP_PORT = 8765
MAP_REFRESH_MS = 1500
# Stdlib-only: 3.9+ (list[]/dict[]/| with from __future__ import annotations).
MIN_PYTHON = (3, 9)


class EntityStatus(Enum):
    ALIVE = "alive"
    WARNING = "warning"
    STALE = "stale"


@dataclass
class CotEntity:
    uid: str
    entity_type: str = ""
    how: str = ""
    callsign: str = ""
    lat: str = ""
    lon: str = ""
    event_time: Optional[datetime] = None
    stale_time: Optional[datetime] = None
    last_rx_mono: float = field(default_factory=time.monotonic)
    last_rx_wall: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    source_addr: str = ""
    rx_count: int = 0

    def display_name(self) -> str:
        if self.callsign:
            return self.callsign
        if self.uid:
            return self.uid
        return "-"

    def status(self, now_utc: datetime, warn_secs: float) -> EntityStatus:
        if self.stale_time is not None and now_utc >= self.stale_time:
            return EntityStatus.STALE
        age_secs = time.monotonic() - self.last_rx_mono
        if age_secs > warn_secs:
            return EntityStatus.WARNING
        return EntityStatus.ALIVE

    def age_secs(self) -> float:
        return time.monotonic() - self.last_rx_mono

    def coordinates(self) -> Optional[tuple[float, float]]:
        try:
            lat = float(self.lat)
            lon = float(self.lon)
        except (TypeError, ValueError):
            return None
        if not (-90.0 <= lat <= 90.0 and -180.0 <= lon <= 180.0):
            return None
        return lat, lon

    def is_null_island(self) -> bool:
        coords = self.coordinates()
        return coords is not None and coords == (0.0, 0.0)


def parse_cot_time(value: str | None) -> Optional[datetime]:
    if not value:
        return None
    text = value.strip()
    if text.endswith("Z"):
        text = text[:-1] + "+00:00"
    try:
        dt = datetime.fromisoformat(text)
    except ValueError:
        return None
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    return dt.astimezone(timezone.utc)


def local_tag(tag: str) -> str:
    if "}" in tag:
        return tag.rsplit("}", 1)[-1]
    return tag


def find_first(root: ET.Element, name: str) -> Optional[ET.Element]:
    for elem in root.iter():
        if local_tag(elem.tag) == name:
            return elem
    return None


def parse_cot_datagram(data: bytes) -> Optional[dict]:
    try:
        text = data.decode("utf-8", errors="replace").strip()
    except Exception:
        return None
    if not text or "<event" not in text:
        return None
    try:
        root = ET.fromstring(text)
    except ET.ParseError:
        return None
    if local_tag(root.tag) != "event":
        return None

    point = find_first(root, "point")
    contact = find_first(root, "contact")

    callsign = ""
    if contact is not None:
        callsign = contact.get("callsign", "") or ""
    if not callsign:
        callsign_elem = find_first(root, "callsign")
        if callsign_elem is not None:
            callsign = (callsign_elem.text or "").strip()
            if not callsign and callsign_elem.get("callsign"):
                callsign = callsign_elem.get("callsign", "")

    return {
        "uid": root.get("uid") or "",
        "type": root.get("type") or "",
        "how": root.get("how") or "",
        "time": parse_cot_time(root.get("time")),
        "stale": parse_cot_time(root.get("stale")),
        "lat": point.get("lat", "") if point is not None else "",
        "lon": point.get("lon", "") if point is not None else "",
        "callsign": callsign,
    }


def require_curses():
    """Return the curses module or exit with an install hint."""
    global curses
    if curses is not None:
        return curses
    if sys.platform == "win32":
        print(
            "Error: curses is not available. On Windows install the helper package:\n"
            "  pip install windows-curses\n"
            "Or run with --map-only to use the browser map without a terminal UI.",
            file=sys.stderr,
        )
    else:
        print(
            "Error: the curses module is required for the terminal UI "
            "(or use --map-only).",
            file=sys.stderr,
        )
    raise SystemExit(1)


def _pack_u8_or_int(value: int) -> bytes:
    """Pack a sockopt value; some Windows builds want int-sized options."""
    if sys.platform == "win32":
        return struct.pack("i", int(value))
    return struct.pack("B", int(value))


def _iface_ip_via_netifaces(name: str) -> Optional[str]:
    try:
        import netifaces  # type: ignore
    except ImportError:
        return None
    try:
        addrs = netifaces.ifaddresses(name)
        if netifaces.AF_INET in addrs:
            return addrs[netifaces.AF_INET][0]["addr"]
    except Exception:
        return None
    return None


def _iface_ip_darwin(name: str) -> Optional[str]:
    proc = subprocess.run(
        ["ipconfig", "getifaddr", name],
        capture_output=True,
        text=True,
        check=False,
    )
    ip = proc.stdout.strip()
    if proc.returncode == 0 and ip:
        return ip
    return None


def _iface_ip_linux_ip_cmd(name: str) -> Optional[str]:
    proc = subprocess.run(
        ["ip", "-4", "-o", "addr", "show", "dev", name],
        capture_output=True,
        text=True,
        check=False,
    )
    if proc.returncode != 0 or not proc.stdout.strip():
        return None
    # Example: "2: eth0    inet 192.168.1.10/24 brd ... scope global eth0"
    for token in proc.stdout.split():
        if "/" in token and token[0].isdigit():
            return token.split("/", 1)[0]
    return None


def _iface_ip_ioctl(name: str) -> Optional[str]:
    """Unix SIOCGIFADDR fallback (Linux / some BSDs). Not available on Windows."""
    if sys.platform == "win32":
        return None
    try:
        import fcntl
    except ImportError:
        return None

    # SIOCGIFADDR: Linux 0x8915; macOS/BSD often 0xc0206921
    if sys.platform == "darwin":
        sio = 0xC0206921
    else:
        sio = 0x8915

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        packed = struct.pack("256s", name[:15].encode("utf-8"))
        ip_bytes = fcntl.ioctl(sock.fileno(), sio, packed)[20:24]
        return socket.inet_ntoa(ip_bytes)
    except OSError:
        return None
    finally:
        sock.close()


def _iface_ip_windows(name: str) -> Optional[str]:
    """Resolve a Windows adapter alias (e.g. 'Ethernet') to an IPv4 address."""
    if sys.platform != "win32":
        return None
    # Prefer PowerShell Get-NetIPAddress (Vista+/Win8+ with NetTCPIP module).
    safe = name.replace("'", "''")
    ps = (
        f"$a = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue "
        f"| Where-Object {{ $_.InterfaceAlias -eq '{safe}' "
        f"-or $_.InterfaceAlias -like '*{safe}*' }}; "
        f"if ($a) {{ ($a | Select-Object -First 1).IPAddress }}"
    )
    proc = subprocess.run(
        ["powershell", "-NoProfile", "-Command", ps],
        capture_output=True,
        text=True,
        check=False,
    )
    ip = (proc.stdout or "").strip().splitlines()
    if proc.returncode == 0 and ip:
        candidate = ip[0].strip()
        try:
            socket.inet_aton(candidate)
            return candidate
        except OSError:
            pass
    return None


def resolve_interface_ip(name_or_ip: str) -> str:
    """Resolve an interface name or dotted IPv4 address to an IPv4 address."""
    try:
        socket.inet_aton(name_or_ip)
        return name_or_ip
    except OSError:
        pass

    resolvers = [
        _iface_ip_via_netifaces,
    ]
    if sys.platform == "darwin":
        resolvers.append(_iface_ip_darwin)
    elif sys.platform.startswith("linux"):
        resolvers.append(_iface_ip_linux_ip_cmd)
        resolvers.append(_iface_ip_ioctl)
    elif sys.platform == "win32":
        resolvers.append(_iface_ip_windows)
    else:
        resolvers.append(_iface_ip_ioctl)

    for resolver in resolvers:
        ip = resolver(name_or_ip)
        if ip:
            return ip

    raise SystemExit(
        f"Could not resolve IP for interface {name_or_ip!r}.\n"
        "Pass the interface IPv4 address directly (portable on all platforms),\n"
        "or install netifaces (`pip install netifaces`) and retry with the interface name."
    )


def configure_multicast_socket(
    sock: socket.socket,
    address: str,
    port: int,
    iface_ip: str,
    bind_any: bool,
) -> str:
    """Bind and join a CoT multicast group. Returns a short bind-mode label for the UI."""
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    if hasattr(socket, "SO_REUSEPORT"):
        try:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        except OSError:
            # Present on some builds but not always settable (e.g. older Windows).
            pass

    try:
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_LOOP, _pack_u8_or_int(1))
    except OSError:
        # Best-effort; listening still works without loopback of our own sends.
        pass

    # Windows requires binding INADDR_ANY for multicast receives.
    if bind_any or sys.platform == "win32":
        sock.bind(("", port))
        bind_mode = "0.0.0.0"
    else:
        # Bind to the group address when the OS allows it so same-host multicast
        # loopback is delivered here, not only to the sender's socket.
        try:
            sock.bind((address, port))
            bind_mode = "group"
        except OSError:
            sock.bind(("", port))
            bind_mode = "0.0.0.0(fallback)"

    mreq = struct.pack(
        "4s4s",
        socket.inet_aton(address),
        socket.inet_aton(iface_ip),
    )
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    try:
        sock.setsockopt(
            socket.IPPROTO_IP,
            socket.IP_MULTICAST_IF,
            socket.inet_aton(iface_ip),
        )
    except OSError:
        pass
    return bind_mode


class CotMulticastListener:
    def __init__(
        self,
        interface: str,
        address: str,
        port: int,
        entities: Dict[str, CotEntity],
        lock: threading.Lock,
        stats: dict,
        bind_any: bool = False,
    ):
        self._interface = interface
        self._address = address
        self._port = port
        self._entities = entities
        self._lock = lock
        self._stats = stats
        self._bind_any = bind_any
        self._running = True
        self._sock: Optional[socket.socket] = None
        self._thread: Optional[threading.Thread] = None

    def start(self) -> None:
        self._thread = threading.Thread(target=self._run, name="cot-mcast-rx", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._running = False
        if self._sock is not None:
            try:
                self._sock.close()
            except OSError:
                pass

    def _run(self) -> None:
        iface_ip = resolve_interface_ip(self._interface)
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock = sock
        try:
            bind_mode = configure_multicast_socket(
                sock,
                self._address,
                self._port,
                iface_ip,
                self._bind_any,
            )

            with self._lock:
                self._stats["iface_ip"] = iface_ip
                self._stats["bind_mode"] = bind_mode
                self._stats["listening"] = True

            while self._running:
                try:
                    sock.settimeout(0.5)
                    data, addr = sock.recvfrom(65535)
                except socket.timeout:
                    continue
                except OSError:
                    break

                with self._lock:
                    self._stats["datagrams"] = self._stats.get("datagrams", 0) + 1
                    self._stats["bytes"] = self._stats.get("bytes", 0) + len(data)
                    if addr[0] == iface_ip:
                        self._stats["local_datagrams"] = self._stats.get("local_datagrams", 0) + 1

                parsed = parse_cot_datagram(data)
                if parsed is None or not parsed["uid"]:
                    with self._lock:
                        self._stats["parse_errors"] = self._stats.get("parse_errors", 0) + 1
                    continue

                now_wall = datetime.now(timezone.utc)
                now_mono = time.monotonic()
                with self._lock:
                    entity = self._entities.get(parsed["uid"])
                    if entity is None:
                        entity = CotEntity(uid=parsed["uid"])
                        self._entities[parsed["uid"]] = entity
                    entity.entity_type = parsed["type"]
                    entity.how = parsed["how"]
                    entity.callsign = parsed["callsign"]
                    entity.lat = parsed["lat"]
                    entity.lon = parsed["lon"]
                    entity.event_time = parsed["time"]
                    entity.stale_time = parsed["stale"]
                    entity.last_rx_mono = now_mono
                    entity.last_rx_wall = now_wall
                    entity.source_addr = f"{addr[0]}:{addr[1]}"
                    entity.rx_count += 1
        finally:
            try:
                mreq = struct.pack(
                    "4s4s",
                    socket.inet_aton(self._address),
                    socket.inet_aton(iface_ip),
                )
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_DROP_MEMBERSHIP, mreq)
            except OSError:
                pass
            sock.close()
            with self._lock:
                self._stats["listening"] = False


def fmt_dt(dt: Optional[datetime]) -> str:
    if dt is None:
        return "-"
    return dt.strftime("%H:%M:%S")


def fmt_age(secs: float) -> str:
    if secs < 60:
        return f"{secs:.0f}s"
    if secs < 3600:
        return f"{secs / 60:.1f}m"
    return f"{secs / 3600:.1f}h"


def truncate_display(text: str, max_len: int) -> str:
    if max_len <= 0:
        return ""
    if len(text) <= max_len:
        return text
    if max_len <= 1:
        return text[:max_len]
    return text[: max_len - 1] + "…"


def format_cell(text: str, max_len: int, col_w: int) -> str:
    """Truncate text, then pad to a fixed column width so adjacent columns do not run together."""
    return truncate_display(text, max_len).ljust(col_w)[:col_w]


def format_coord(value: str) -> str:
    """Six-character coordinate in an eight-character column (gap before LON/AGE)."""
    return (value or "-")[:6].ljust(8)[:8]


def status_label(status: EntityStatus) -> str:
    if status == EntityStatus.ALIVE:
        return "alive"
    if status == EntityStatus.WARNING:
        return "offline?"
    return "stale"


def entity_map_snapshot(
    entities: Dict[str, CotEntity],
    lock: threading.Lock,
    warn_seconds: float,
) -> List[dict[str, Any]]:
    now_utc = datetime.now(timezone.utc)
    rows: List[dict[str, Any]] = []
    with lock:
        for entity in entities.values():
            coords = entity.coordinates()
            status = entity.status(now_utc, warn_seconds)
            row: dict[str, Any] = {
                "uid": entity.uid,
                "name": entity.display_name(),
                "type": entity.entity_type or "",
                "status": status.value,
                "age_secs": round(entity.age_secs(), 1),
                "source": entity.source_addr,
                "rx_count": entity.rx_count,
                "sidc": cot_type_to_sidc2525c(entity.entity_type),
                "on_map": coords is not None,
            }
            if coords is not None:
                lat, lon = coords
                row["lat"] = lat
                row["lon"] = lon
                row["null_coords"] = entity.is_null_island()
            else:
                row["lat"] = None
                row["lon"] = None
                row["null_coords"] = False
            rows.append(row)
    rows.sort(key=lambda row: row["name"].lower())
    return rows


def cot_type_to_sidc2525c(cot_type: str) -> str:
    """Convert a CoT atom type string to a MIL-STD-2525C letter SIDC (15 chars).

    Follows the TAK / ATAK mapping: affiliation and battle dimension from the
    first tokens, function id from remaining tokens concatenated into positions 3-9.
    """
    sidc = ["-"] * 15
    sidc[0] = "S"  # Warfighting
    sidc[1] = "U"  # Unknown affiliation
    sidc[2] = "G"  # Ground
    sidc[3] = "P"  # Present
    sidc[4] = "U"  # Unit

    if not cot_type:
        return "".join(sidc)

    parts = cot_type.split("-")
    if not parts or parts[0] != "a":
        return "".join(sidc)

    affiliation_map = {
        "p": "P",
        "a": "A",
        "f": "F",
        "n": "N",
        "s": "S",
        "h": "H",
        "j": "J",
        "k": "K",
        "o": "O",
        "u": "U",
        "w": "W",
        "d": "D",
        "x": "X",
    }
    if len(parts) > 1:
        sidc[1] = affiliation_map.get(parts[1].lower(), "U")
    if len(parts) > 2 and parts[2]:
        sidc[2] = parts[2][0].upper()

    idx = 3
    for part in parts[3:]:
        for ch in part:
            if idx >= 10:
                break
            sidc[idx] = ch.upper()
            idx += 1
        if idx >= 10:
            break

    return "".join(sidc)


MAP_PAGE_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>CoT Entity Map</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"
        integrity="sha256-p4NxAoJBhIIN+hmNHrzRCf9tD/miZyoHS5obTRR9BMY=" crossorigin="" />
  <style>
    html, body { margin: 0; height: 100%; font-family: system-ui, sans-serif; }
    #map { position: absolute; inset: 0 280px 0 0; }
    #sidebar {
      position: absolute; top: 0; right: 0; bottom: 0; width: 280px;
      background: #111827; color: #e5e7eb; overflow-y: auto;
      border-left: 1px solid #374151; box-sizing: border-box; padding: 12px;
    }
    #sidebar h1 { font-size: 16px; margin: 0 0 8px; }
    #sidebar .meta { font-size: 12px; color: #9ca3af; margin-bottom: 12px; }
    .legend { display: flex; flex-wrap: wrap; gap: 8px; font-size: 11px; margin-bottom: 12px; }
    .legend span { display: inline-flex; align-items: center; gap: 4px; }
    .dot { width: 10px; height: 10px; border-radius: 50%; display: inline-block; }
    .entity-row {
      border: 1px solid #374151; border-radius: 8px; padding: 8px; margin-bottom: 8px;
      font-size: 12px; cursor: pointer;
    }
    .entity-row:hover { background: #1f2937; }
    .entity-row.off-map { opacity: 0.72; border-style: dashed; }
    .entity-row.off-map:hover { background: #1f2937; cursor: default; }
    .entity-row .title { font-weight: 600; margin-bottom: 4px; }
    .entity-row .sub { color: #9ca3af; }
    .cot-marker {
      background: transparent; border: none;
    }
    .cot-marker .icon-shell {
      position: relative; display: flex; flex-direction: column; align-items: center;
      filter: drop-shadow(0 1px 2px rgba(0,0,0,0.35));
    }
    .cot-marker .symbol-wrap {
      box-shadow: 0 0 0 3px var(--status-color, #22c55e);
      border-radius: 4px; background: rgba(255,255,255,0.15); line-height: 0;
    }
    .cot-marker .symbol-wrap.null-coords { opacity: 0.65; box-shadow: 0 0 0 3px #9ca3af; }
    .cot-marker.status-stale .symbol-wrap { opacity: 0.5; }
    .cot-marker .symbol-wrap svg { display: block; }
    .cot-marker .label {
      margin-top: 2px; white-space: nowrap; font-size: 10px; font-weight: 700; color: #111827;
      background: rgba(255,255,255,0.9); padding: 1px 4px; border-radius: 4px;
      max-width: 96px; overflow: hidden; text-overflow: ellipsis;
    }
  </style>
</head>
<body>
  <div id="map"></div>
  <div id="sidebar">
    <h1>CoT Entities</h1>
    <div class="meta" id="meta">Loading…</div>
    <div class="legend">
      <span><i class="dot" style="background:#22c55e"></i> alive</span>
      <span><i class="dot" style="background:#eab308"></i> offline?</span>
      <span><i class="dot" style="background:#ef4444"></i> stale</span>
    </div>
    <div class="meta">Icons: MIL-STD-2525C from CoT type via milsymbol</div>
    <div id="entity-list"></div>
  </div>
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"
          integrity="sha256-20nQCchB9co0qIjJZRGuk2/Z9VM+kNiyxNV1lvTlZBo=" crossorigin=""></script>
  <script src="https://unpkg.com/milsymbol@2.2.2/dist/milsymbol.js"></script>
  <script>
    const statusColors = { alive: "#22c55e", warning: "#eab308", stale: "#ef4444" };
    const map = L.map("map", { zoomControl: true }).setView([20, 0], 2);
    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
      maxZoom: 19,
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>'
    }).addTo(map);

    const markers = new Map();
    let didFit = false;

    function cotTypeToSidc(cotType) {
      const sidc = Array(15).fill("-");
      sidc[0] = "S";
      sidc[1] = "U";
      sidc[2] = "G";
      sidc[3] = "P";
      sidc[4] = "U";
      if (!cotType) return sidc.join("");
      const parts = cotType.split("-");
      if (!parts.length || parts[0] !== "a") return sidc.join("");
      const aff = {
        p: "P", a: "A", f: "F", n: "N", s: "S", h: "H",
        j: "J", k: "K", o: "O", u: "U", w: "W", d: "D", x: "X",
      };
      if (parts.length > 1) sidc[1] = aff[parts[1].toLowerCase()] || "U";
      if (parts.length > 2 && parts[2]) sidc[2] = parts[2][0].toUpperCase();
      let idx = 3;
      for (const part of parts.slice(3)) {
        for (const ch of part) {
          if (idx >= 10) break;
          sidc[idx++] = ch.toUpperCase();
        }
        if (idx >= 10) break;
      }
      return sidc.join("");
    }

    function mkIcon(entity) {
      const sidc = entity.sidc || cotTypeToSidc(entity.type);
      const color = statusColors[entity.status] || "#6b7280";
      const label = (entity.name || entity.uid || "?").slice(0, 12);
      const staleClass = entity.status === "stale" ? " status-stale" : "";
      const nullClass = entity.null_coords ? " null-coords" : "";
      let svg = "";
      let width = 48;
      let height = 48;
      let anchorX = 24;
      let anchorY = 24;
      try {
        const sym = new ms.Symbol(sidc, { size: 48, frame: true, fill: true, icon: true });
        svg = sym.asSVG();
        const size = sym.getSize();
        const anchor = sym.getAnchor();
        width = Math.max(size.x, 32);
        height = size.y + 14;
        anchorX = anchor.x;
        anchorY = anchor.y;
      } catch (err) {
        svg = '<svg width="32" height="32"><rect x="4" y="4" width="24" height="24" fill="#ddd" stroke="#333"/></svg>';
        width = 32;
        height = 46;
        anchorX = 16;
        anchorY = 16;
      }
      return L.divIcon({
        className: "cot-marker mil-symbol" + staleClass,
        html: `<div class="icon-shell" style="--status-color:${color}">
                 <div class="symbol-wrap${nullClass}">${svg}</div>
                 <div class="label">${label}</div>
               </div>`,
        iconSize: [width, height],
        iconAnchor: [anchorX, anchorY],
        popupAnchor: [0, -Math.max(anchorY - 4, 10)],
      });
    }

    function popupHtml(entity) {
      const sidc = entity.sidc || cotTypeToSidc(entity.type);
      const pos = (entity.on_map && entity.lat != null && entity.lon != null)
        ? `${entity.lat.toFixed(5)}, ${entity.lon.toFixed(5)}`
        : "no position";
      return `<b>${entity.name || entity.uid}</b><br/>
              UID: ${entity.uid}<br/>
              Type: ${entity.type || "-"}<br/>
              SIDC: ${sidc}<br/>
              Status: ${entity.status}<br/>
              Lat/Lon: ${pos}<br/>
              Age: ${entity.age_secs}s<br/>
              RX: ${entity.rx_count}<br/>
              Source: ${entity.source || "-"}`;
    }

    function renderSidebar(entities, updatedAt) {
      const onMap = entities.filter(e => e.on_map).length;
      document.getElementById("meta").textContent =
        `${entities.length} total · ${onMap} on map · updated ${updatedAt}`;
      const list = document.getElementById("entity-list");
      list.innerHTML = "";
      for (const entity of entities) {
        const row = document.createElement("div");
        row.className = "entity-row" + (entity.on_map ? "" : " off-map");
        const pos = (entity.on_map && entity.lat != null && entity.lon != null)
          ? `${entity.lat.toFixed(5)}, ${entity.lon.toFixed(5)}`
          : "no position";
        row.innerHTML = `<div class="title">${entity.name || entity.uid}</div>
          <div class="sub">${entity.type || "-"} · ${entity.status} · ${entity.age_secs}s</div>
          <div class="sub">${pos}</div>`;
        if (entity.on_map) {
          row.onclick = () => {
            const marker = markers.get(entity.uid);
            if (marker) {
              map.setView(marker.getLatLng(), Math.max(map.getZoom(), 12));
              marker.openPopup();
            }
          };
        }
        list.appendChild(row);
      }
    }

    async function refresh() {
      const resp = await fetch("/api/entities");
      const payload = await resp.json();
      const entities = payload.entities || [];
      const seen = new Set();

      for (const entity of entities) {
        seen.add(entity.uid);
        if (!entity.on_map || entity.lat == null || entity.lon == null) {
          const old = markers.get(entity.uid);
          if (old) {
            map.removeLayer(old);
            markers.delete(entity.uid);
          }
          continue;
        }
        const latlng = [entity.lat, entity.lon];
        let marker = markers.get(entity.uid);
        if (!marker) {
          marker = L.marker(latlng, { icon: mkIcon(entity) }).addTo(map);
          markers.set(entity.uid, marker);
        } else {
          marker.setLatLng(latlng);
          marker.setIcon(mkIcon(entity));
        }
        marker.setPopupContent(popupHtml(entity));
      }

      for (const [uid, marker] of markers) {
        if (!seen.has(uid)) {
          map.removeLayer(marker);
          markers.delete(uid);
        }
      }

      renderSidebar(entities, payload.updated_at || "");
      const mappable = entities.filter(e => e.on_map && e.lat != null && e.lon != null);
      if (!didFit && mappable.length > 0) {
        const bounds = L.latLngBounds(mappable.map(e => [e.lat, e.lon]));
        map.fitBounds(bounds.pad(0.25));
        didFit = true;
      }
    }

    refresh();
    setInterval(refresh, __REFRESH_MS__);
  </script>
</body>
</html>
"""


class CotMapServer:
    def __init__(
        self,
        entities: Dict[str, CotEntity],
        lock: threading.Lock,
        warn_seconds: float,
        host: str,
        port: int,
    ):
        self._entities = entities
        self._lock = lock
        self._warn_seconds = warn_seconds
        self._host = host
        self._port = port
        self._httpd: Optional[ThreadingHTTPServer] = None
        self._thread: Optional[threading.Thread] = None

    @property
    def url(self) -> str:
        return f"http://{self._host}:{self._port}/"

    def start(self) -> None:
        entities = self._entities
        lock = self._lock
        warn_seconds = self._warn_seconds
        page_html = MAP_PAGE_HTML.replace("__REFRESH_MS__", str(MAP_REFRESH_MS))

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, _format: str, *_args) -> None:
                return

            def do_GET(self) -> None:
                path = urlparse(self.path).path
                if path == "/":
                    body = page_html.encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "text/html; charset=utf-8")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return
                if path == "/api/entities":
                    payload = {
                        "updated_at": datetime.now(timezone.utc).strftime("%H:%M:%S UTC"),
                        "entities": entity_map_snapshot(entities, lock, warn_seconds),
                    }
                    body = json.dumps(payload).encode("utf-8")
                    self.send_response(200)
                    self.send_header("Content-Type", "application/json")
                    self.send_header("Cache-Control", "no-store")
                    self.send_header("Content-Length", str(len(body)))
                    self.end_headers()
                    self.wfile.write(body)
                    return
                self.send_error(404)

        self._httpd = ThreadingHTTPServer((self._host, self._port), Handler)
        self._thread = threading.Thread(target=self._httpd.serve_forever, name="cot-map-http", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        if self._httpd is not None:
            self._httpd.shutdown()
            self._httpd.server_close()
            self._httpd = None


def open_map_browser(url: str) -> None:
    threading.Timer(0.6, lambda: webbrowser.open(url)).start()


def run_map_only(map_server: CotMapServer) -> None:
    print(f"CoT map server running at {map_server.url}")
    print("Press Ctrl+C to stop.")
    try:
        while True:
            time.sleep(1.0)
    except KeyboardInterrupt:
        print()


def safe_addnstr(stdscr, y: int, x: int, text: str, n: int | None = None) -> None:
    """Write text clipped to the screen; never raises on tiny terminals."""
    try:
        height, width = stdscr.getmaxyx()
        if y < 0 or x < 0 or y >= height or x >= width or width <= 1 or height <= 0:
            return
        # Writing the bottom-right cell often returns ERR (causes scroll).
        max_len = width - x
        if y == height - 1:
            max_len = max(0, max_len - 1)
        if max_len <= 0:
            return
        limit = max_len if n is None else min(int(n), max_len)
        if limit <= 0:
            return
        stdscr.addnstr(y, x, text, limit)
    except Exception:
        pass


def draw_screen(
    stdscr,
    entities: Dict[str, CotEntity],
    lock: threading.Lock,
    stats: dict,
    args: argparse.Namespace,
) -> None:
    curses_mod = require_curses()
    try:
        curses_mod.use_default_colors()
        bg = -1
    except curses_mod.error:
        bg = curses_mod.COLOR_BLACK

    try:
        curses_mod.init_pair(1, curses_mod.COLOR_GREEN, bg)
        curses_mod.init_pair(2, curses_mod.COLOR_YELLOW, bg)
        curses_mod.init_pair(3, curses_mod.COLOR_RED, bg)
        curses_mod.init_pair(4, curses_mod.COLOR_CYAN, bg)
        curses_mod.init_pair(5, curses_mod.COLOR_WHITE, curses_mod.COLOR_BLUE)
    except curses_mod.error:
        # Monochrome / limited terminals: proceed without custom pairs.
        pass

    status_colors = {
        EntityStatus.ALIVE: curses_mod.color_pair(1),
        EntityStatus.WARNING: curses_mod.color_pair(2),
        EntityStatus.STALE: curses_mod.color_pair(3),
    }

    stdscr.nodelay(True)
    stdscr.timeout(int(1000 / REFRESH_HZ))
    try:
        curses_mod.curs_set(0)
    except curses_mod.error:
        pass

    columns = [
        ("STATUS", 10),
        ("NAME", 17),
        ("UID", 17),
        ("TYPE", 15),
        ("LAT", 8),
        ("LON", 8),
        ("AGE", 7),
        ("STALE AT", 10),
        ("RX", 5),
        ("SOURCE", 18),
    ]
    min_height = 5
    min_width = 40

    while True:
        key = stdscr.getch()
        if key in (ord("q"), ord("Q"), 27):
            break

        stdscr.erase()
        height, width = stdscr.getmaxyx()
        now_utc = datetime.now(timezone.utc)

        with lock:
            entity_list = sorted(
                entities.values(),
                key=lambda e: (e.status(now_utc, args.warn_seconds).value, e.uid.lower()),
            )
            datagrams = stats.get("datagrams", 0)
            local_datagrams = stats.get("local_datagrams", 0)
            parse_errors = stats.get("parse_errors", 0)
            iface_ip = stats.get("iface_ip", "?")
            bind_mode = stats.get("bind_mode", "?")
            listening = stats.get("listening", False)

        if height < min_height or width < min_width:
            safe_addnstr(
                stdscr,
                0,
                0,
                f" Terminal too small ({width}x{height}). "
                f"Resize to at least {min_width}x{min_height}, or press q to quit. ",
            )
            stdscr.refresh()
            continue

        header = (
            f" CoT entity monitor | {args.address}:{args.port} on {args.interface} ({iface_ip}) "
            f"| bind={bind_mode} rx={datagrams} local={local_datagrams} "
            f"parse_err={parse_errors} | warn={args.warn_seconds:.0f}s "
        )
        if getattr(args, "map", False):
            header += f"| map={args.map_host}:{args.map_port} "
        header += "| q=quit "
        try:
            stdscr.attron(curses_mod.color_pair(5))
            safe_addnstr(stdscr, 0, 0, header.ljust(max(0, width - 1)))
            stdscr.attroff(curses_mod.color_pair(5))
        except curses_mod.error:
            safe_addnstr(stdscr, 0, 0, header.ljust(max(0, width - 1)))

        if not listening:
            safe_addnstr(stdscr, 2, 0, "Starting multicast listener...")

        legend = " alive=recent update  offline?=quiet but before stale  stale=past CoT stale time "
        try:
            stdscr.attron(curses_mod.color_pair(4))
            safe_addnstr(stdscr, 1, 0, legend.ljust(max(0, width - 1)))
            stdscr.attroff(curses_mod.color_pair(4))
        except curses_mod.error:
            safe_addnstr(stdscr, 1, 0, legend.ljust(max(0, width - 1)))

        row = 3
        x = 0
        try:
            stdscr.attron(curses_mod.A_BOLD)
        except curses_mod.error:
            pass
        for title, col_w in columns:
            if x >= width - 1:
                break
            cell_w = min(col_w, width - 1 - x)
            safe_addnstr(stdscr, row, x, title.ljust(cell_w)[:cell_w], cell_w)
            x += col_w
        try:
            stdscr.attroff(curses_mod.A_BOLD)
        except curses_mod.error:
            pass

        row += 1
        max_rows = height - row - 1
        visible = entity_list[: max(0, max_rows)]

        counts = {EntityStatus.ALIVE: 0, EntityStatus.WARNING: 0, EntityStatus.STALE: 0}
        for entity in entity_list:
            counts[entity.status(now_utc, args.warn_seconds)] += 1

        if not visible:
            safe_addnstr(stdscr, row, 0, "Waiting for CoT events...")
        else:
            for entity in visible:
                status = entity.status(now_utc, args.warn_seconds)
                fields = [
                    format_cell(status_label(status), 9, 10),
                    format_cell(entity.display_name(), 16, 17),
                    format_cell(entity.uid, 16, 17),
                    format_cell(entity.entity_type or "-", 14, 15),
                    format_coord(entity.lat),
                    format_coord(entity.lon),
                    format_cell(fmt_age(entity.age_secs()), 6, 7),
                    format_cell(fmt_dt(entity.stale_time), 8, 10),
                    format_cell(str(entity.rx_count), 4, 5),
                    format_cell(entity.source_addr, 17, 18),
                ]
                x = 0
                try:
                    stdscr.attron(status_colors[status])
                except curses_mod.error:
                    pass
                for (_, col_w), value in zip(columns, fields):
                    if x >= width - 1:
                        break
                    cell_w = min(col_w, width - 1 - x)
                    safe_addnstr(stdscr, row, x, value[:cell_w], cell_w)
                    x += col_w
                try:
                    stdscr.attroff(status_colors[status])
                except curses_mod.error:
                    pass
                row += 1
                if row >= height - 1:
                    break

        footer = (
            f" entities={len(entity_list)} "
            f"alive={counts[EntityStatus.ALIVE]} "
            f"offline?={counts[EntityStatus.WARNING]} "
            f"stale={counts[EntityStatus.STALE]} "
        )
        if len(entity_list) > len(visible):
            footer += f" (showing {len(visible)}/{len(entity_list)})"
        safe_addnstr(stdscr, height - 1, 0, footer.ljust(max(0, width - 1)))
        stdscr.refresh()


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Monitor CoT entities from a UDP multicast group with live status coloring."
    )
    parser.add_argument(
        "--interface",
        "-i",
        required=True,
        help="Network interface name (e.g. en0, eth0) or its IPv4 address",
    )
    parser.add_argument(
        "--address",
        "-a",
        default=DEFAULT_MCAST,
        help=f"Multicast group address (default: {DEFAULT_MCAST})",
    )
    parser.add_argument(
        "--port",
        "-p",
        type=int,
        default=DEFAULT_PORT,
        help=f"UDP port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--warn-seconds",
        "-w",
        type=float,
        default=DEFAULT_WARN_SECS,
        help=(
            "Seconds without a new datagram before marking an entity offline? "
            f"(default: {DEFAULT_WARN_SECS})"
        ),
    )
    parser.add_argument(
        "--bind-any",
        action="store_true",
        help="Bind to 0.0.0.0 instead of the multicast group address (legacy behavior)",
    )
    parser.add_argument(
        "--map",
        action="store_true",
        help="Serve a live Leaflet map in the browser (requires network for map tiles)",
    )
    parser.add_argument(
        "--map-only",
        action="store_true",
        help="Run the map server without the terminal table",
    )
    parser.add_argument(
        "--map-port",
        type=int,
        default=DEFAULT_MAP_PORT,
        help=f"HTTP port for --map (default: {DEFAULT_MAP_PORT})",
    )
    parser.add_argument(
        "--map-host",
        default="127.0.0.1",
        help="HTTP bind address for --map (default: 127.0.0.1)",
    )
    parser.add_argument(
        "--no-browser",
        action="store_true",
        help="Do not automatically open the map in a browser",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    if sys.version_info < MIN_PYTHON:
        print(
            f"Error: Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ is required "
            f"(found {sys.version_info.major}.{sys.version_info.minor}).",
            file=sys.stderr,
        )
        return 1

    args = parse_args(argv)

    if not (1 <= args.port <= 65535):
        print("Error: port must be between 1 and 65535", file=sys.stderr)
        return 1
    if args.warn_seconds < 0:
        print("Error: --warn-seconds must be non-negative", file=sys.stderr)
        return 1
    if not (1 <= args.map_port <= 65535):
        print("Error: --map-port must be between 1 and 65535", file=sys.stderr)
        return 1
    if args.map_only and not args.map:
        args.map = True

    try:
        socket.inet_aton(args.address)
    except OSError:
        print(f"Error: invalid multicast address {args.address}", file=sys.stderr)
        return 1

    if not args.map_only:
        require_curses()

    entities: Dict[str, CotEntity] = {}
    lock = threading.Lock()
    stats: dict = {}

    listener = CotMulticastListener(
        interface=args.interface,
        address=args.address,
        port=args.port,
        entities=entities,
        lock=lock,
        stats=stats,
        bind_any=args.bind_any,
    )
    listener.start()

    map_server: Optional[CotMapServer] = None
    if args.map:
        map_server = CotMapServer(
            entities=entities,
            lock=lock,
            warn_seconds=args.warn_seconds,
            host=args.map_host,
            port=args.map_port,
        )
        try:
            map_server.start()
        except OSError as exc:
            print(f"Error: could not start map server on {args.map_host}:{args.map_port}: {exc}", file=sys.stderr)
            listener.stop()
            return 1
        if not args.no_browser:
            open_map_browser(map_server.url)

    try:
        if args.map_only:
            run_map_only(map_server)
        else:
            require_curses().wrapper(draw_screen, entities, lock, stats, args)
    except KeyboardInterrupt:
        pass
    finally:
        if map_server is not None:
            map_server.stop()
        listener.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
