#!/usr/bin/env bash
# Fail if factory defaults JSON copies diverge (silent cross-platform bugs).
# Run from repo root: ./scripts/verify_factory_json_sync.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAC="$ROOT/Assets/defaults_factory_mac.json"
LINUX="$ROOT/linux/linux_factory_defaults.json"
WIN="$ROOT/windows/windows_factory_defaults.json"
DUMP="$ROOT/json_defaults_dump.json"
for f in "$MAC" "$LINUX" "$WIN" "$DUMP"; do
  if [[ ! -f "$f" ]]; then
    echo "verify_factory_json_sync: missing $f" >&2
    exit 1
  fi
done
fail=0
if ! cmp -s "$LINUX" "$MAC"; then
  echo "verify_factory_json_sync: DIFF linux/linux_factory_defaults.json vs Assets/defaults_factory_mac.json" >&2
  diff -u "$MAC" "$LINUX" >&2 || true
  fail=1
fi
if ! cmp -s "$WIN" "$MAC"; then
  echo "verify_factory_json_sync: DIFF windows/windows_factory_defaults.json vs Assets/defaults_factory_mac.json" >&2
  diff -u "$MAC" "$WIN" >&2 || true
  fail=1
fi
if ! cmp -s "$DUMP" "$MAC"; then
  echo "verify_factory_json_sync: DIFF json_defaults_dump.json vs Assets/defaults_factory_mac.json" >&2
  diff -u "$MAC" "$DUMP" >&2 || true
  fail=1
fi
if [[ "$fail" -ne 0 ]]; then
  echo "verify_factory_json_sync: Fix drift by copying the canonical mac asset to the others, then commit." >&2
  exit 1
fi
echo "verify_factory_json_sync: OK (all four files byte-identical to Assets/defaults_factory_mac.json)"
