FROM jellyfin/jellyfin:12.1

LABEL org.opencontainers.image.source="https://github.com/justin8/jellyfin-rffmpeg"
LABEL org.opencontainers.image.description="Unprivileged rffmpeg transcode worker with Intel QSV and VA-API"

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        openssh-server && \
    rm -rf /var/lib/apt/lists/* && \
    getent group video || groupadd -r video && \
    getent group render || groupadd -r render && \
    groupadd -g 2000 jellyfin && \
    useradd -u 2000 -g jellyfin -G video,render -m -s /bin/bash jellyfin && \
    mkdir -p /run/sshd /etc/ssh /home/jellyfin/.ssh /etc/rffmpeg-worker/keys && \
    chown -R jellyfin:jellyfin /run/sshd /etc/ssh /home/jellyfin && \
    chmod 666 /etc/passwd /etc/group

COPY sshd_config /etc/ssh/sshd_config
COPY entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod 755 /usr/local/bin/entrypoint.sh && \
    chown jellyfin:jellyfin /etc/ssh/sshd_config

USER 2000:2000
WORKDIR /home/jellyfin
EXPOSE 2222

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
