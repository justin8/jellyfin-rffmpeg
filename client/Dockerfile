FROM jellyfin/jellyfin:12.1

LABEL org.opencontainers.image.source="https://github.com/justin8/jellyfin-rffmpeg"
LABEL org.opencontainers.image.description="Jellyfin server with rffmpeg client and OpenSSH client"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3 \
        python3-click \
        python3-yaml \
        openssh-client && \
    rm -rf /var/lib/apt/lists/*

COPY rffmpeg /usr/local/bin/rffmpeg
COPY rffmpeg-init /usr/local/bin/rffmpeg-init

RUN chmod 755 /usr/local/bin/rffmpeg /usr/local/bin/rffmpeg-init && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffmpeg && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffprobe && \
    mkdir -p /etc/rffmpeg /var/lib/rffmpeg /run/shm && \
    chmod 777 /var/lib/rffmpeg /run/shm

