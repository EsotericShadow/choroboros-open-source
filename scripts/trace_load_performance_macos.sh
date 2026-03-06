#!/usr/bin/env bash
set -euo pipefail

LOG_PATH="$HOME/Library/Application Support/Choroboros/load_trace.ndjson"
TAIL=20
RESET=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --reset)
      RESET=1
      shift
      ;;
    --path)
      LOG_PATH="$2"
      shift 2
      ;;
    --tail)
      TAIL="$2"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 [--reset] [--path <load_trace.ndjson>] [--tail <N>]" >&2
      exit 1
      ;;
  esac
done

if [[ "$RESET" -eq 1 ]]; then
  if [[ -f "$LOG_PATH" ]]; then
    rm -f "$LOG_PATH"
    echo "Cleared load trace log: $LOG_PATH"
  else
    echo "No load trace log to clear at: $LOG_PATH"
  fi
  exit 0
fi

if [[ ! -f "$LOG_PATH" ]]; then
  echo "Load trace log not found: $LOG_PATH" >&2
  echo "Open the plugin at least once, then re-run this script." >&2
  exit 1
fi

/usr/bin/ruby - "$LOG_PATH" "$TAIL" <<'RUBY'
require "json"

path = ARGV[0]
tail_count = Integer(ARGV[1])

events = []
invalid_lines = 0

File.foreach(path) do |line|
  text = line.strip
  next if text.empty?
  begin
    events << JSON.parse(text)
  rescue JSON::ParserError
    invalid_lines += 1
  end
end

puts
puts "=== Choroboros Load Performance Trace (macOS) ==="
puts "LogPath: #{path}"
puts "Events: #{events.size} | Invalid lines: #{invalid_lines}"

if events.empty?
  puts "No parsable events found."
  exit 0
end

instance_count = events.map { |e| e["instanceId"] }.compact.uniq.size
host_count = events.map { |e| e["host"] }.compact.uniq.size
puts "Instances: #{instance_count} | Hosts: #{host_count}"

latest = events.max_by { |e| e["tsUtc"].to_s }
puts
puts "System / Host Snapshot (latest event):"
[
  ["TimeUtc", latest["tsUtc"]],
  ["Host", latest["host"]],
  ["HostPath", latest["hostPath"]],
  ["WrapperType", latest["wrapperType"]],
  ["OS", latest["os"]],
  ["IsOS64Bit", latest["isOS64Bit"]],
  ["CpuVendor", latest["cpuVendor"]],
  ["CpuModel", latest["cpuModel"]],
  ["CpuSpeedMHz", latest["cpuSpeedMHz"]],
  ["CpuCores", latest["cpuCores"]],
  ["RamMB", latest["ramMB"]],
  ["PluginVersion", latest["pluginVersion"]],
  ["BuildConfig", latest["buildConfig"]]
].each { |k, v| puts "  #{k}: #{v}" }

def percentile(values, p)
  sorted = values.sort
  return Float::NAN if sorted.empty?
  idx = ((sorted.length - 1) * p).round
  sorted[idx]
end

interesting = %w[
  processor_ctor_total_ms
  processor_load_persisted_defaults_ms
  processor_create_editor_ms
  editor_ctor_total_ms
  editor_theme_setup_ms
  editor_controls_setup_ms
  editor_first_paint_ms
  devpanel_tab_switch_ms
  devpanel_tab_build_ms
]

puts
puts "Timing Summary (ms):"
header = format("%-36s %6s %10s %10s %10s %10s %10s", "Event", "Count", "Min", "P50", "P95", "Max", "Avg")
puts header
puts "-" * header.length

interesting.each do |event_name|
  samples = events
    .select { |e| e["event"] == event_name && !e["elapsedMs"].nil? }
    .map { |e| e["elapsedMs"].to_f }
  next if samples.empty?

  min = samples.min
  max = samples.max
  avg = samples.sum / samples.length
  p50 = percentile(samples, 0.50)
  p95 = percentile(samples, 0.95)
  puts format("%-36s %6d %10.3f %10.3f %10.3f %10.3f %10.3f",
              event_name, samples.length, min, p50, p95, max, avg)
end

puts
puts "Slowest events captured:"
events
  .select { |e| !e["elapsedMs"].nil? }
  .sort_by { |e| -e["elapsedMs"].to_f }
  .first(15)
  .each do |e|
    puts format("  %s | %-28s | %9.3f ms | host=%s | instance=%s | %s",
                e["tsUtc"], e["event"], e["elapsedMs"].to_f,
                e["host"], e["instanceId"], e["notes"])
  end

puts
puts "Recent #{tail_count} raw events:"
lines = File.readlines(path).last(tail_count)
lines.each { |line| puts line }

puts
puts "Tip: clear log before a controlled test run with:"
puts "  ./scripts/trace_load_performance_macos.sh --reset"
RUBY
