#!/usr/bin/env python3
"""
gen_benchmark_report.py — Convert Google Benchmark JSON output to Markdown.

Usage:
    python3 gen_benchmark_report.py results1.json [results2.json ...] \
        [--output report.md] [--tag v1.0.0]

Each JSON file becomes its own section in the report. Add more benchmark
binaries in the future by passing additional JSON files — no script changes
needed.
"""

import argparse
import json
import math
import os
import sys
from datetime import datetime, timezone


# ─── Formatting helpers ───────────────────────────────────────────────────────

def _fmt_time(ns: float, unit: str) -> str:
    """Format a time value already expressed in `unit` (ns/us/ms/s)."""
    unit_map = {"ns": "ns", "us": "µs", "ms": "ms", "s": "s"}
    label = unit_map.get(unit, unit)
    return f"{ns:.1f} {label}"


def _fmt_rate(value: float) -> str:
    """Format items/sec or bytes/sec with SI suffix."""
    if value == 0:
        return "—"
    for threshold, suffix in [(1e9, "G"), (1e6, "M"), (1e3, "K")]:
        if value >= threshold:
            return f"{value / threshold:.2f} {suffix}"
    return f"{value:.2f} "


def _bytes_per_sec(value: float) -> str:
    if value == 0:
        return "—"
    for threshold, suffix in [(1e9, "GB/s"), (1e6, "MB/s"), (1e3, "KB/s")]:
        if value >= threshold:
            return f"{value / threshold:.2f} {suffix}"
    return f"{value:.2f} B/s"


def _items_per_sec(value: float) -> str:
    return _fmt_rate(value) + "/s" if value else "—"


# ─── Section renderer ─────────────────────────────────────────────────────────

def _render_section(title: str, benchmarks: list) -> list[str]:
    """Render one benchmark JSON result set as Markdown lines."""
    lines = [f"## {title}", ""]

    # Detect which optional columns are present across all rows
    has_bytes = any(b.get("bytes_per_second", 0) for b in benchmarks)
    has_items = any(b.get("items_per_second", 0) for b in benchmarks)

    # Build header
    cols = ["Benchmark", "Time", "CPU", "Iterations"]
    if has_items:
        cols.append("Items/sec")
    if has_bytes:
        cols.append("Bytes/sec")

    lines.append("| " + " | ".join(cols) + " |")
    lines.append("| " + " | ".join(["---"] * len(cols)) + " |")

    for bm in benchmarks:
        if bm.get("run_type") == "aggregate" and bm.get("aggregate_name") != "mean":
            # Only show aggregates once (mean); stddev/median are noise for a report
            continue

        name = bm["name"]
        unit = bm.get("time_unit", "ns")
        time_val = bm.get("real_time", 0)
        cpu_val = bm.get("cpu_time", 0)
        iters = bm.get("iterations", 0)

        row = [
            f"`{name}`",
            _fmt_time(time_val, unit),
            _fmt_time(cpu_val, unit),
            str(iters),
        ]
        if has_items:
            row.append(_items_per_sec(bm.get("items_per_second", 0)))
        if has_bytes:
            row.append(_bytes_per_sec(bm.get("bytes_per_second", 0)))

        lines.append("| " + " | ".join(row) + " |")

    lines.append("")
    return lines


# ─── System info ──────────────────────────────────────────────────────────────

def _render_system_info(context: dict) -> list[str]:
    lines = ["## System", ""]
    lines.append("| Property | Value |")
    lines.append("| --- | --- |")
    for key in ["host_name", "num_cpus", "mhz_per_cpu", "cpu_scaling_enabled",
                "library_version", "library_build_type"]:
        val = context.get(key)
        if val is not None:
            label = key.replace("_", " ").title()
            lines.append(f"| {label} | {val} |")
    lines.append("")
    return lines


# ─── Main ─────────────────────────────────────────────────────────────────────

def generate_report(json_files: list[str], tag: str) -> list[str]:
    date_str = datetime.now(timezone.utc).strftime("%Y-%m-%d")
    lines = [
        f"# iBridger Benchmark Report — {tag}",
        "",
        f"**Date:** {date_str}  ",
        f"**Version:** {tag}  ",
        "",
    ]

    first = True
    for path in json_files:
        try:
            with open(path) as f:
                data = json.load(f)
        except (OSError, json.JSONDecodeError) as e:
            print(f"Warning: skipping {path}: {e}", file=sys.stderr)
            continue

        # System info from the first file only
        if first and "context" in data:
            lines += _render_system_info(data["context"])
            first = False

        # Section title: prefer context["name"], fall back to filename stem
        context_name = data.get("context", {}).get("executable", "")
        title = os.path.splitext(os.path.basename(context_name or path))[0]
        title = title.replace("_", " ").title()

        benchmarks = data.get("benchmarks", [])
        if benchmarks:
            lines += _render_section(title, benchmarks)

    return lines


def main():
    parser = argparse.ArgumentParser(
        description="Convert Google Benchmark JSON output to a Markdown report."
    )
    parser.add_argument("json_files", nargs="+", metavar="FILE",
                        help="One or more benchmark JSON output files")
    parser.add_argument("--output", default="benchmark_report.md",
                        help="Output Markdown file (default: benchmark_report.md)")
    parser.add_argument("--tag", default="dev",
                        help="Version tag to embed in the report header")
    args = parser.parse_args()

    lines = generate_report(args.json_files, args.tag)
    content = "\n".join(lines) + "\n"

    with open(args.output, "w") as f:
        f.write(content)

    print(f"Report written to {args.output}")


if __name__ == "__main__":
    main()
