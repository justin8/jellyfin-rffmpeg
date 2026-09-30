#!/bin/bash
set -euo pipefail

# Dynamically ensure the UID/GID assigned by Kubernetes exists in /etc/passwd
if ! getent passwd "$(id -u)" >/dev/null 2>&1; then
    echo "jellyfin:x:$(id -u):$(id -g):jellyfin:/home/jellyfin:/bin/bash" >> /etc/passwd
fi

# Prepare user SSH directory
mkdir -p /home/jellyfin/.ssh
chmod 700 /home/jellyfin/.ssh

# Mount or copy authorized_keys
if [ -f /etc/rffmpeg-worker/keys/authorized_keys ]; then
    cp /etc/rffmpeg-worker/keys/authorized_keys /home/jellyfin/.ssh/authorized_keys
    chmod 600 /home/jellyfin/.ssh/authorized_keys
fi

# Ensure host keys exist
if [ ! -f /etc/ssh/ssh_host_ed25519_key ]; then
    ssh-keygen -t ed25519 -f /etc/ssh/ssh_host_ed25519_key -N ''
fi

exec /usr/sbin/sshd -D -e -f /etc/ssh/sshd_config
