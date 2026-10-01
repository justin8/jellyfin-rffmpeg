FROM jellyfin/jellyfin:12.1

LABEL org.opencontainers.image.source="https://github.com/justin8/jellyfin-rffmpeg"
LABEL org.opencontainers.image.description="Jellyfin server with rffmpeg client and OpenSSH client"

ADD --chmod=755 https://raw.githubusercontent.com/joshuaboniface/rffmpeg/master/rffmpeg /usr/local/bin/rffmpeg

COPY nfs-retry.c /tmp/nfs-retry.c

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3 \
        python3-click \
        python3-yaml \
        openssh-client \
        gcc \
        libc6-dev && \
    gcc -O2 -fPIC -shared /tmp/nfs-retry.c -o /usr/local/lib/libnfsretry.so -ldl && \
    apt-get purge -y gcc libc6-dev && \
    apt-get autoremove -y && \
    rm -rf /tmp/nfs-retry.c /var/lib/apt/lists/* && \
    (getent group jellyfin || groupadd -g 2000 jellyfin) && \
    (getent passwd jellyfin || useradd -u 2000 -g jellyfin -m -s /bin/bash jellyfin) && \
    chmod 666 /etc/passwd /etc/group

COPY rffmpeg-init /usr/local/bin/rffmpeg-init
COPY entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod 755 /usr/local/bin/rffmpeg-init /usr/local/bin/entrypoint.sh && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffmpeg && \
    ln -sf /usr/local/bin/rffmpeg /usr/local/bin/ffprobe && \
    mkdir -p /etc/rffmpeg /var/lib/rffmpeg /run/shm && \
    chmod 777 /var/lib/rffmpeg /run/shm

ENV LD_PRELOAD=/usr/local/lib/libnfsretry.so

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
