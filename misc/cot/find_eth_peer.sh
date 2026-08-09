#!/usr/bin/env bash
# find-ethernet-peer.sh — find the L2/L3 neighbor on a direct-ish Ethernet link
# Usage: find-ethernet-peer.sh IFACE [--json]
set -euo pipefail

IFACE="${1:?interface required (e.g. en12, eth0)}"
JSON=0
for arg in "${@:2}"; do
  case "$arg" in
    --json) JSON=1 ;;
    *) echo "unknown arg: $arg" >&2; exit 2 ;;
  esac
done

OS="$(uname -s)"

# --- local IPv4 ---
local_ip() {
  case "$OS" in
    Linux)
      ip -4 -o addr show dev "$IFACE" | awk '{print $4}' | cut -d/ -f1 | head -1
      ;;
    Darwin)
      ipconfig getifaddr "$IFACE" 2>/dev/null || true
      ;;
    *)
      echo "unsupported OS: $OS" >&2; return 1
      ;;
  esac
}

# --- enumerate host IPs in same subnet (simple /24 fallback if needed) ---
subnet_hosts() {
  local ip="$1"
  local a b c _d
  IFS=. read -r a b c _d <<< "$ip"
  local i
  for i in $(seq 1 254); do
    echo "${a}.${b}.${c}.${i}"
  done
}

# --- wake ARP cache ---
wake_subnet() {
  local ip="$1"
  local host
  while read -r host; do
    [[ "$host" == "$ip" ]] && continue
    case "$OS" in
      Linux)  ping -c 1 -W 1 "$host" >/dev/null 2>&1 || true ;;
      Darwin) ping -c 1 -W 200 "$host" >/dev/null 2>&1 || true ;;
    esac
  done < <(subnet_hosts "$ip")
}

# --- ARP entries on IFACE -> "ip mac" lines ---
arp_on_iface() {
  case "$OS" in
    Linux)
      ip neigh show dev "$IFACE" 2>/dev/null | while read -r line; do
        ip=$(echo "$line" | awk '{print $1}')
        mac=$(echo "$line" | awk '{print $5}')
        state=$(echo "$line" | awk '{print $6}')
        [[ "$mac" == "FAILED" || -z "$mac" ]] && continue
        echo "$ip $mac"
      done
      ;;
    Darwin)
      arp -an | while read -r line; do
        echo "$line" | grep -q " on ${IFACE} " || continue
        echo "$line" | grep -q "incomplete" && continue
        ip=$(echo "$line" | sed -n 's/.*(\([0-9.]*\)).*/\1/p')
        mac=$(echo "$line" | sed -n 's/.* at \([0-9a-f:]*\).*/\1/p')
        [[ -n "$ip" && -n "$mac" ]] && echo "$ip $mac"
      done
      ;;
  esac
}

LOCAL="$(local_ip)"
[[ -z "$LOCAL" ]] && { echo "no IPv4 on $IFACE" >&2; exit 1; }

wake_subnet "$LOCAL"

declare -a PEERS=()
while read -r pip pmac; do
  [[ "$pip" == "$LOCAL" ]] && continue
  PEERS+=("${pmac}|${pip}")
done < <(arp_on_iface)

if [[ ${#PEERS[@]} -eq 0 ]]; then
  echo "no peer found on $IFACE" >&2
  exit 1
fi
if [[ ${#PEERS[@]} -gt 1 ]]; then
  echo "multiple peers on $IFACE: ${PEERS[*]}" >&2
  exit 2
fi

PEER_MAC="${PEERS[0]%%|*}"
PEER_IP="${PEERS[0]##*|}"

if [[ "$JSON" -eq 1 ]]; then
  printf '{"iface":"%s","local_ip":"%s","peer_mac":"%s","peer_ip":"%s"}\n' \
    "$IFACE" "$LOCAL" "$PEER_MAC" "$PEER_IP"
else
  printf 'IFACE=%s\nLOCAL_IP=%s\nPEER_MAC=%s\nPEER_IP=%s\n' \
    "$IFACE" "$LOCAL" "$PEER_MAC" "$PEER_IP"
fi
