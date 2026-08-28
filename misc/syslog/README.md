# syslog — standalone syslog collector

Copyright (c) 2026 Rally Tactical Systems, Inc.  
All rights reserved.

`syslog_collector.py` is a small **stdlib-only** Python tool that listens for
remote syslog messages (RFC 3164 and RFC 5424) and writes them to the terminal
and/or a file. Useful for lab captures from Rallypoint / Engage hosts, rsyslog
forwarders, or any device that can send UDP (or TCP) syslog.

---

## Requirements

| Item | Notes |
|------|--------|
| Python | **3.9+**, standard library only |
| Platforms | Linux, macOS, Windows |
| Privileges | Binding **UDP/TCP 514** usually needs root (or `CAP_NET_BIND_SERVICE` on Linux). Prefer a high port such as **5514** for unprivileged runs. |

---

## Quick start

```bash
python3 syslog_collector.py --port 5514
```

In another terminal, send a test message:

```bash
# RFC 3164-style
echo '<134>Aug 20 19:54:01 labhost rallypointd[1234]: peer connected' \
  | nc -u -w 1 127.0.0.1 5514

# RFC 5424-style
echo '<134>1 2026-08-20T19:54:01.000Z labhost rallypointd 1234 - - peer connected' \
  | nc -u -w 1 127.0.0.1 5514
```

On macOS, Apple’s `nc` uses `-w` for the send timeout (do not use OpenBSD-only `-N`).

---

## Options

| Option | Description |
|--------|-------------|
| `--bind ADDR` | Local bind address (default `0.0.0.0`) |
| `--port N` | Listen port (default `514`) |
| `--tcp` | Also accept TCP on the same port |
| `--format pretty\|raw\|json` | Output style (default `pretty`) |
| `-o`, `--output-file PATH` | Append lines to a file |
| `--file-only` | With `-o`, suppress stdout |
| `--max-severity LEVEL` | Keep this severity or worse (`emerg`…`debug` or `0`…`7`) |
| `--facility LIST` | Comma-separated facilities to keep (`local0,daemon` or numbers) |
| `--stats-interval N` | Print stats to stderr every *N* seconds |
| `--color auto\|always\|never` | ANSI colors for **pretty** stdout (default `auto` = TTY only). File output is always plain. |

Severity colors match Engage console logging: white-on-red for emerg/alert/crit, bold red for err, yellow for warning, green for notice/info, gray for debug.

Send `SIGUSR1` to the process (Unix) to print stats immediately:

```bash
kill -USR1 <pid>
```

---

## Output formats

**pretty** (default):

```text
2026-08-20T19:54:01.123Z UDP 127.0.0.1:54321 [local0.info] labhost rallypointd: peer connected
```

**raw** — original payload only.

**json** — one JSON object per line with parsed fields (`pri`, `facility`, `severity`, `hostname`, `app_name`, `message`, etc.).

---

## TCP framing

With `--tcp`, each client connection accepts:

1. **Octet-counting** (RFC 6587): `123 <PRI>...payload of 123 bytes...`
2. **Newline / trailer** framing: one message per line ending in `\n`

UDP remains one datagram per message (typical for classic syslog).

---

## Pointing a forwarder at the collector

Example **rsyslog** action (adjust host/port):

```text
*.*  @192.168.1.50:5514
```

(`@` = UDP; `@@` = TCP.)

Example **syslog-ng**:

```text
destination d_lab {
    network("192.168.1.50" port(5514) transport("udp"));
};
log { source(s_src); destination(d_lab); };
```

Engage / Rallypoint log to the **local** OS syslog subsystem. To capture them with this collector, configure the host’s syslog daemon (or a sidecar) to **forward** to the collector’s address and port.

---

## Examples

Listen unprivileged, JSON to a file and stdout:

```bash
python3 syslog_collector.py --port 5514 --format json -o /tmp/syslog.jsonl
```

UDP + TCP, only `warning` and more severe, facilities `local0` and `daemon`:

```bash
python3 syslog_collector.py --port 5514 --tcp \
  --max-severity warning --facility local0,daemon
```

Privileged classic port:

```bash
sudo python3 syslog_collector.py --bind 0.0.0.0 --port 514
```

---

## Disclaimer

This is a **lab / test utility**, not a full SIEM or durable log pipeline. It does not implement TLS (syslog-over-TLS), RELP, disk spooling, or authentication. Use a production collector (rsyslog, syslog-ng, Vector, etc.) when you need those properties.
