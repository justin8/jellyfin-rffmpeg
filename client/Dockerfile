FROM jellyfin/jellyfin:12.1

LABEL org.opencontainers.image.source="https://github.com/justin8/jellyfin-rffmpeg"
LABEL org.opencontainers.image.description="Jellyfin server with rffmpeg client and OpenSSH client"

ADD --chmod=755 https://raw.githubusercontent.com/joshuaboniface/rffmpeg/master/rffmpeg /usr/local/bin/rffmpeg

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3 \
        python3-click \
        python3-yaml \
        openssh-client && \
    rm -rf /var/lib/apt/lists/* && \
    (getent group jellyfin || groupadd -g 2000 jellyfin) && \
    (getent passwd jellyfin || useradd -u 2000 -g jellyfin -m -s /bin/bash jellyfin) && \
    chmod 666 /etc/passwd /etc/group

COPY rffmpeg-init /usr/local/bin/rffmpeg-init

RUN chmod 755 /usr/local/bin/rffmpeg-init && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffmpeg && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffprobe && \
    mkdir -p /etc/rffmpeg /var/lib/rffmpeg /run/shm && \
    chmod 777 /var/lib/rffmpeg /run/shm
