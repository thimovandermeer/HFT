#!/usr/bin/env bash
# start-fixsim.sh

# Use existing VM_IP or grab it from Multipass
VM_IP="${VM_IP:-$(multipass list | grep '\bhft-vm\b' | tr -s ' ' | cut -d ' ' -f3)}"
[ -z "$VM_IP" ] && { echo "❌ VM_IP not set and multipass query failed"; exit 1; }

echo "🚀 Starting FIXSIM servers on VM at $VM_IP"

ssh -o StrictHostKeyChecking=no ubuntu@"$VM_IP" <<'EOF'
  echo "🛑 Killing existing FIXSIM servers…"
  pkill -f fixsim-server.py || true

  echo "🧹 Cleaning old config/log dirs…"
  rm -rf /home/ubuntu/FIX-Simulator/fixsim_instances

  echo "🔧 Spawning new FIXSIM servers…"
  cd /home/ubuntu/FIX-Simulator
  chmod +x ./spawn_servers.sh
  ./spawn_servers.sh

  echo "✅ FIXSIM servers launched."
EOF
