#!/usr/bin/env bash
# stop-fixsim-ssh.sh

# 1) VM IP from your env or hardcode it
VM_IP=${VM_IP:-1.2.3.4}

# 2) Kill servers and clean up
ssh ubuntu@"$VM_IP" <<'EOF'
  echo "🛑 Stopping FIXSIM servers…"
  pkill -f fixsim-server.py || true
  echo "🧹 Removing old fixsim_instances directory…"
  rm -rf /home/ubuntu/FIX-Simulator/fixsim_instances
  echo "✅ Cleanup complete."
EOF
