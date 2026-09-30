#!/bin/bash
set -euo pipefail

# Dynamically ensure the UID/GID assigned by Kubernetes exists in /etc/passwd
if ! getent passwd "$(id -u)" >/dev/null 2>&1; then
    echo "jellyfin:x:$(id -u):$(id -g):jellyfin:/config:/bin/bash" >> /etc/passwd
fi

exec /jellyfin/jellyfin "$@"
