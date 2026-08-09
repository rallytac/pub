#!/usr/bin/env python3
"""
Copyright (c) 2026 Rally Tactical Systems, Inc.
All rights reserved.

Generate UTF-8 XML payloads for Cursor on Target (CoT) transmission.

Two wire shapes (both are valid CoT event version 2.0):

  - original — XML declaration without standalone= (common server / lightweight clients).
  - new      — declaration includes standalone=\"yes\" (typical mobile / ATAK-style emitters).

Payload text (JSON or freeform) is placed in <remarks> inside <detail>, which is a
common way to attach application data while staying within ordinary CoT structure.

Use --pretty for indented XML with one attribute per line.
"""

from __future__ import annotations

import argparse
import json
import sys
import xml.etree.ElementTree as ET
from datetime import datetime, timedelta, timezone
from typing import Any, Mapping


DEFAULT_REMARKS_JSON = {"note": "CoT sample payload", "source": "cotmon"}


def iso_utc_millis(dt: datetime) -> str:
    if dt.tzinfo is None:
        dt = dt.replace(tzinfo=timezone.utc)
    dt = dt.astimezone(timezone.utc)
    ms = dt.microsecond // 1000
    return dt.strftime("%Y-%m-%dT%H:%M:%S") + f".{ms:03d}Z"


def escape_xml_attr(utf8: str) -> str:
    out: list[str] = []
    for c in utf8:
        if c == "&":
            out.append("&amp;")
        elif c == '"':
            out.append("&quot;")
        elif c == "<":
            out.append("&lt;")
        else:
            out.append(c)
    return "".join(out)


def escape_xml_elem_text(utf8: str) -> str:
    out: list[str] = []
    for c in utf8:
        if c == "&":
            out.append("&amp;")
        elif c == "<":
            out.append("&lt;")
        elif c == ">":
            out.append("&gt;")
        else:
            out.append(c)
    return "".join(out)


def xml_declaration(style: str) -> str:
    if style == "new":
        return '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
    return '<?xml version="1.0" encoding="UTF-8"?>'


def split_xml_declaration(xml_text: str) -> tuple[str | None, str]:
    """Return (declaration through ?>, remainder) or (None, stripped input)."""
    s = xml_text.strip()
    if not s.startswith("<?xml"):
        return None, s
    end = s.find("?>")
    if end == -1:
        return None, s
    return s[: end + 2], s[end + 2 :].lstrip()


def _serialize_element_pretty(elem: ET.Element, indent: str, depth: int, out: list[str]) -> None:
    """Write elem to out as indented XML with each attribute on its own line."""
    pad = indent * depth
    tag = elem.tag
    children = list(elem)
    attrs = elem.attrib

    text_head = elem.text or ""
    has_kids = len(children) > 0
    self_closing = not has_kids and not text_head.strip()

    if attrs:
        out.append(f"{pad}<{tag}")
        ip = pad + indent
        items = list(attrs.items())
        for i, (k, v) in enumerate(items):
            esc = escape_xml_attr(str(v))
            last = i == len(items) - 1
            if last and self_closing:
                out.append(f'{ip}{k}="{esc}" />')
            elif last:
                out.append(f'{ip}{k}="{esc}">')
            else:
                out.append(f'{ip}{k}="{esc}"')
    elif self_closing:
        out.append(f"{pad}<{tag} />")
    else:
        out.append(f"{pad}<{tag}>")

    if self_closing:
        return

    ip = pad + indent
    if text_head.strip():
        for line in text_head.splitlines():
            stripped = line.strip()
            if stripped:
                out.append(ip + escape_xml_elem_text(stripped))

    for child in children:
        _serialize_element_pretty(child, indent, depth + 1, out)
        tail = child.tail or ""
        if tail.strip():
            for line in tail.splitlines():
                stripped = line.strip()
                if stripped:
                    out.append(ip + escape_xml_elem_text(stripped))

    out.append(f"{pad}</{tag}>")


def human_readable_xml(xml_compact: str, *, indent: str) -> str:
    """Pretty-print XML with line breaks, nesting, and one attribute per line."""
    decl, body = split_xml_declaration(xml_compact)
    root = ET.fromstring(body)
    lines: list[str] = []
    _serialize_element_pretty(root, indent, 0, lines)
    formatted = "\n".join(lines).rstrip() + "\n"
    if decl:
        return decl + "\n" + formatted
    return formatted


def append_detail_string_elements(oss: list[str], detail_obj: Mapping[str, Any]) -> None:
    """Append <name>text</name> for each top-level string value."""
    for key, val in detail_obj.items():
        if isinstance(val, str):
            oss.append(f"<{key}>{escape_xml_elem_text(val)}</{key}>")


def build_cot_xml(
    *,
    style: str,
    uid: str,
    cot_type: str,
    how: str,
    lat: str,
    lon: str,
    ce: str,
    hae: str,
    le: str,
    stale_offset_secs: int,
    remarks: str | None,
    callsign: str | None,
    endpoint: str | None,
    detail_extra: Mapping[str, Any] | None,
    now: datetime | None,
) -> str:
    t0 = now if now is not None else datetime.now(timezone.utc)
    stale_dt = t0 + timedelta(seconds=stale_offset_secs)
    now_s = iso_utc_millis(t0)
    stale_s = iso_utc_millis(stale_dt)

    parts: list[str] = [
        xml_declaration(style),
        "<event version=\"2.0\" uid=\""
        + escape_xml_attr(uid)
        + "\" type=\""
        + escape_xml_attr(cot_type)
        + "\" time=\""
        + escape_xml_attr(now_s)
        + "\" start=\""
        + escape_xml_attr(now_s)
        + "\" stale=\""
        + escape_xml_attr(stale_s)
        + "\" how=\""
        + escape_xml_attr(how)
        + "\">",
        "<point lat=\""
        + escape_xml_attr(lat)
        + "\" lon=\""
        + escape_xml_attr(lon)
        + "\" ce=\""
        + escape_xml_attr(ce)
        + "\" hae=\""
        + escape_xml_attr(hae)
        + "\" le=\""
        + escape_xml_attr(le)
        + "\"/>",
        "<detail>",
    ]

    if callsign:
        ep = ""
        if endpoint is not None:
            ep = ' endpoint="' + escape_xml_attr(endpoint) + '"'
        parts.append('<contact callsign="' + escape_xml_attr(callsign) + '"' + ep + "/>")

    if remarks is not None and remarks != "":
        parts.append("<remarks>" + escape_xml_elem_text(remarks) + "</remarks>")

    if detail_extra:
        append_detail_string_elements(parts, detail_extra)

    parts.extend(["</detail>", "</event>"])
    return "".join(parts)


def remarks_from_args(args: argparse.Namespace) -> str | None:
    if args.no_remarks:
        return None
    if args.payload_file is not None:
        raw = args.payload_file.read()
        if isinstance(raw, bytes):
            return raw.decode("utf-8", errors="replace")
        return raw
    if args.payload_json is not None:
        try:
            obj = json.loads(args.payload_json)
        except json.JSONDecodeError as e:
            raise SystemExit(f"invalid --payload-json: {e}") from e
        return json.dumps(obj, separators=(",", ":"), ensure_ascii=False)
    return None


def main() -> None:
    p = argparse.ArgumentParser(
        description="Generate Cursor on Target (CoT) event XML (original vs new declaration style)."
    )
    p.add_argument(
        "--format",
        choices=("original", "new"),
        default="new",
        help='original = XML decl without standalone; new = standalone="yes" (common mobile style)',
    )

    src = p.add_mutually_exclusive_group()
    src.add_argument(
        "--payload-json",
        default=None,
        metavar="JSON",
        help=(
            "JSON object stored in <remarks> (default: built-in sample). "
            "Ignored with --no-remarks."
        ),
    )
    src.add_argument(
        "--payload-file",
        type=argparse.FileType("rb"),
        metavar="PATH",
        help="UTF-8 text file whose contents become <remarks>.",
    )

    p.add_argument(
        "--no-remarks",
        action="store_true",
        help="Omit <remarks>; emit contact / detail-json elements only.",
    )
    p.add_argument("--uid", default="cotmon-uid", help="event uid attribute.")
    p.add_argument(
        "--type",
        dest="cot_type",
        default="a-f-G-U-C",
        help='Mil-STD-2525 style type string (default: a-f-G-U-C friendly ground unit).',
    )
    p.add_argument("--how", default="h-e", help='how attribute (default: h-e human-entered).')
    p.add_argument("--lat", default="0.0", help="point lat.")
    p.add_argument("--lon", default="0.0", help="point lon.")
    p.add_argument(
        "--ce",
        default="9999999",
        help='point ce (default 9999999 = unknown per common CoT convention).',
    )
    p.add_argument("--hae", default="0", help="point hae (meters).")
    p.add_argument(
        "--le",
        default="9999999",
        help='point le (default 9999999 = unknown per common CoT convention).',
    )
    p.add_argument(
        "--stale-offset-secs",
        type=int,
        default=300,
        help="Seconds after now for stale=.",
    )
    p.add_argument(
        "--callsign",
        default="",
        help='If non-empty, adds <contact callsign="..."/>.',
    )
    p.add_argument(
        "--endpoint",
        default="",
        help='Optional endpoint attribute on <contact> (e.g. 192.168.1.1:4242:tcp).',
    )
    p.add_argument(
        "--detail-json",
        default="",
        help='Optional JSON object: each string value becomes a <detail> child element.',
    )
    p.add_argument(
        "--pretty",
        action="store_true",
        help="Emit indented XML: elements nested, each attribute on its own line.",
    )
    p.add_argument(
        "--indent",
        default="  ",
        metavar="TEXT",
        help="Indentation string per nesting level when using --pretty (default: two spaces).",
    )
    p.add_argument(
        "--output-file",
        "-o",
        type=argparse.FileType("wb"),
        metavar="PATH",
        help="Write bytes to file instead of stdout.",
    )

    args = p.parse_args()

    if args.payload_json is None and args.payload_file is None and not args.no_remarks:
        args.payload_json = json.dumps(DEFAULT_REMARKS_JSON, separators=(",", ":"), ensure_ascii=False)

    remarks = remarks_from_args(args)

    detail_extra: dict[str, Any] | None = None
    if args.detail_json.strip():
        try:
            dj = json.loads(args.detail_json)
        except json.JSONDecodeError as e:
            raise SystemExit(f"invalid --detail-json: {e}") from e
        if not isinstance(dj, dict):
            raise SystemExit("--detail-json must be a JSON object")
        detail_extra = dj

    cs = args.callsign.strip() or None
    ep = args.endpoint.strip() or None

    xml = build_cot_xml(
        style=args.format,
        uid=args.uid,
        cot_type=args.cot_type,
        how=args.how,
        lat=args.lat,
        lon=args.lon,
        ce=args.ce,
        hae=args.hae,
        le=args.le,
        stale_offset_secs=args.stale_offset_secs,
        remarks=remarks,
        callsign=cs,
        endpoint=ep,
        detail_extra=detail_extra,
        now=None,
    )
    if args.pretty:
        xml = human_readable_xml(xml, indent=args.indent)

    out = xml.encode("utf-8")

    if args.output_file:
        args.output_file.write(out)
        return

    # Binary stdout works on Unix and Windows; fall back if buffer is unavailable.
    try:
        buf = getattr(sys.stdout, "buffer", None)
        if buf is not None:
            buf.write(out)
            buf.flush()
        else:
            sys.stdout.write(out.decode("utf-8"))
            sys.stdout.flush()
    except BrokenPipeError:
        # e.g. `... | head` closed early on Unix.
        try:
            sys.stdout.close()
        except Exception:
            pass
        raise SystemExit(0) from None


if __name__ == "__main__":
    main()
