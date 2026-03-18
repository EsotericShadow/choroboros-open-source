#!/usr/bin/env bash
set -euo pipefail

LINES="${1:-40}"
LOG_FILE="${CHOROBOROS_RECENT_TOUCHES_LOG:-$HOME/Library/Application Support/Choroboros/devpanel_recent_touches.log}"

if [[ ! -f "$LOG_FILE" ]]; then
  echo "Recent touches log not found: $LOG_FILE" >&2
  exit 1
fi

if [[ "${2:-}" == "--follow" || "${2:-}" == "-f" ]]; then
  tail -n "$LINES" -f "$LOG_FILE"
else
  tail -n "$LINES" "$LOG_FILE"
fi
