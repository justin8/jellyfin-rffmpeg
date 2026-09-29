# Jellyfin rffmpeg Container Images

This repository provides container images for running [Jellyfin](https://jellyfin.org/) distributed transcoding via [rffmpeg](https://github.com/joshuaboniface/rffmpeg) on Kubernetes and Docker.

Both images are built directly on top of the official `jellyfin/jellyfin` image to guarantee matching `jellyfin-ffmpeg8` versions, Intel QSV / VA-API hardware acceleration driver parity, and binary paths.

## Images

### 1. `ghcr.io/justin8/jellyfin-rffmpeg` (Client)
- Base: `jellyfin/jellyfin`
- Includes: Python 3, `python3-click`, `python3-yaml`, `openssh-client`, and the `rffmpeg` wrapper.
- Symlinks `/usr/local/bin/ffmpeg` and `/usr/local/bin/ffprobe` to `/usr/local/bin/rffmpeg`.
- Retains all official Jellyfin server features and entrypoint.

### 2. `ghcr.io/justin8/jellyfin-rffmpeg-worker` (Worker)
- Base: `jellyfin/jellyfin`
- Runs completely unprivileged as non-root user `jellyfin` (`UID 2000`, `GID 2000`).
- Member of `video` and `render` groups for hardware acceleration access (`/dev/dri/renderD128`).
- Includes OpenSSH server configured to listen on port `2222`.
- Accepts public key authentication via `/home/jellyfin/.ssh/authorized_keys` (or mounted secret at `/etc/rffmpeg-worker/keys/authorized_keys`).

## Transcode Flow

```text
[Jellyfin Server Pod]
       │
       ▼ (calls /usr/local/bin/ffmpeg)
  [rffmpeg wrapper]
       │ (dispatches over SSH :2222)
       ▼
[jellyfin-rffmpeg-worker DaemonSet Pod]
       │
       ▼ (executes /usr/lib/jellyfin-ffmpeg/ffmpeg)
  [Intel QSV / VA-API iGPU Hardware Transcoder]
```
