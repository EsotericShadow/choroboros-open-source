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
require "time"

path = ARGV[0]
tail_count = Integer(ARGV[1])

events = []
invalid_blocks = 0
buffer = +""
brace_depth = 0
in_string = false
escaped = false

File.foreach(path) do |line|
  stripped = line.strip
  next if brace_depth == 0 && stripped.empty?

  buffer << line

  line.each_char do |ch|
    if escaped
      escaped = false
      next
    end

    if ch == "\\" && in_string
      escaped = true
      next
    end

    if ch == "\""
      in_string = !in_string
      next
    end

    next if in_string

    if ch == "{"
      brace_depth += 1
    elsif ch == "}"
      brace_depth -= 1
    end
  end

  if brace_depth == 0 && !buffer.strip.empty?
    begin
      events << JSON.parse(buffer)
    rescue JSON::ParserError
      invalid_blocks += 1
    ensure
      buffer.clear
      in_string = false
      escaped = false
    end
  end
end

invalid_blocks += 1 unless buffer.strip.empty?

puts
puts "=== Choroboros Load Performance Trace (macOS) ==="
puts "LogPath: #{path}"
puts "Events: #{events.size} | Invalid blocks: #{invalid_blocks}"

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
  editor_active_theme_ready_ms
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

def first_metric(instance_events, event_name)
  match = instance_events.find { |e| e["event"] == event_name && !e["elapsedMs"].nil? }
  match ? match["elapsedMs"].to_f : nil
end

instance_groups = events.group_by { |e| e["instanceId"] }.sort_by { |instance_id, _| instance_id.to_i }
unless instance_groups.empty?
  puts
  puts "Per-instance startup timeline (ms):"
  header = format("%-10s %-10s %-8s %10s %10s %10s %10s %12s %12s %12s",
                  "Instance", "Host", "Wrap",
                  "CreateEd", "Ctor", "1stPaint", "ThemeReady",
                  "Create->Paint", "Ctor->Paint", "Paint->Theme")
  puts header
  puts "-" * header.length

  instance_groups.each do |instance_id, group_events|
    instance_events = group_events.sort_by { |e| Time.iso8601(e["tsUtc"].to_s) rescue Time.at(0) }
    processor_create_ms = first_metric(instance_events, "processor_create_editor_ms")
    editor_ctor_ms = first_metric(instance_events, "editor_ctor_total_ms")
    first_paint_ms = first_metric(instance_events, "editor_first_paint_ms")
    theme_ready_ms = first_metric(instance_events, "editor_active_theme_ready_ms")

    create_to_paint = (!processor_create_ms.nil? && !first_paint_ms.nil?) ? (first_paint_ms - processor_create_ms) : nil
    ctor_to_paint = (!editor_ctor_ms.nil? && !first_paint_ms.nil?) ? (first_paint_ms - editor_ctor_ms) : nil
    paint_to_theme = (!theme_ready_ms.nil? && !first_paint_ms.nil?) ? (theme_ready_ms - first_paint_ms) : nil

    puts format("%-10s %-10s %-8s %10s %10s %10s %10s %12s %12s %12s",
                instance_id,
                (instance_events[0]["host"] || "-")[0, 10],
                (instance_events[0]["wrapperType"] || "-")[0, 8],
                processor_create_ms.nil? ? "-" : format("%.3f", processor_create_ms),
                editor_ctor_ms.nil? ? "-" : format("%.3f", editor_ctor_ms),
                first_paint_ms.nil? ? "-" : format("%.3f", first_paint_ms),
                theme_ready_ms.nil? ? "-" : format("%.3f", theme_ready_ms),
                create_to_paint.nil? ? "-" : format("%.3f", create_to_paint),
                ctor_to_paint.nil? ? "-" : format("%.3f", ctor_to_paint),
                paint_to_theme.nil? ? "-" : format("%.3f", paint_to_theme))
  end
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
