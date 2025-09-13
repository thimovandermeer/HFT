#!/usr/bin/env bash
# start-fixsim-local.sh

set -euo pipefail

# 1) Where your FIX-Simulator lives on your Mac:
SIM_DIR="${SIM_DIR:-$HOME/PycharmProjects/FIX-Simulator}"

echo "🚀 Starting FIXSIM servers locally in ${SIM_DIR}…"

# 2) Kill any already-running simulator processes
echo "🛑 Stopping existing FIXSIM servers…"
pkill -f fixsim-server.py || true

# 3) Spawn new instances
echo "🔧 Launching new FIXSIM instances…"
cd "$SIM_DIR"
chmod +x spawn_servers.sh
./spawn_servers.sh

echo "✅ FIXSIM servers launched locally."
