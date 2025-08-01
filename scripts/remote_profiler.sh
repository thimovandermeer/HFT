#!/usr/bin/env bash
set -euo pipefail

# Defaults (must come before set -u)
DURATION=30
OUT_SVG="flamegraph.svg"
# --------------------------------------------------

usage() {
  cat <<EOF
Usage: $0 [--duration N] [--out FILE]

  --duration N   seconds to record (default: $DURATION)
  --out FILE     local SVG filename (default: $OUT_SVG)
  -h, --help     show this help
EOF
  exit 1
}

# Parse args
while [[ $# -gt 0 ]]; do
  case $1 in
    -d|--duration)
      [[ $# -ge 2 && $2 =~ ^[0-9]+$ ]] || { echo "🔴 duration must be a number"; exit 1; }
      DURATION=$2; shift 2 ;;
    -o|--out)
      [[ $# -ge 2 ]] || { echo "🔴 missing value for --out"; exit 1; }
      OUT_SVG=$2; shift 2 ;;
    -h|--help) usage ;;
    *) echo "🔴 unknown option: $1"; usage ;;
  esac
done

# Determine VM IP
VM_IP="${VM_IP:-$(multipass list | awk '/\bhft-vm\b/ {print $3}')}"
[[ -n "$VM_IP" ]] || { echo "🔴 Could not find VM IP"; exit 2; }

echo "📡 Profiling HFT on $VM_IP for ${DURATION}s…"

# SSH in and do everything step by step
ssh -o StrictHostKeyChecking=no ubuntu@"$VM_IP" bash - <<EOF
set -euo pipefail

cd /workspace

# 1) Find PID
PID=\$(pgrep -x HFT) || { echo "🔴 HFT not running"; exit 3; }
echo "🔍 HFT PID = \$PID"

# 2) Record perf
echo "⏺ Recording perf for $DURATION s..."
sudo timeout ${DURATION} perf record -q -F 99 -g -p \$PID -o perf.data >/dev/null 2>&1 || true
ls -lh perf.data

# 3) Collapse stacks
echo "🔧 Collapsing stacks..."
perf script -i perf.data > perf.raw
ls -lh perf.raw
~/FlameGraph/stackcollapse-perf.pl perf.raw > perf.folded
ls -lh perf.folded

# 4) Generate flamegraph.svg
echo "🖼 Generating flamegraph.svg..."
~/FlameGraph/flamegraph.pl perf.folded > perf.svg
ls -lh perf.svg

# 5) Clean intermediates if you like
rm perf.data perf.raw perf.folded

echo "✅ Remote work done. Flamegraph at /workspace/perf.svg"
EOF

# Pull the SVG back
echo "⬇️ Pulling SVG to ./$OUT_SVG"
multipass transfer hft-vm:/workspace/perf.svg ./"$OUT_SVG"
echo "🎉 Done! Open $OUT_SVG to view your flamegraph."
