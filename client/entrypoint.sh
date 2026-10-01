#!/bin/bash
set -euo pipefail

# Dynamically ensure the UID/GID assigned by Kubernetes exists in /etc/passwd
if ! getent passwd "$(id -u)" >/dev/null 2>&1; then
    echo "jellyfin:x:$(id -u):$(id -g):jellyfin:/config:/bin/bash" >> /etc/passwd
fi

if [ -f /usr/local/lib/libnfsretry.so ]; then
    export LD_PRELOAD="/usr/local/lib/libnfsretry.so${LD_PRELOAD:+:$LD_PRELOAD}"
fi

exec /jellyfin/jellyfin "$@"
