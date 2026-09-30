# Jellyfin rffmpeg Container Images

This repository provides container images for running [Jellyfin](https://jellyfin.org/) distributed transcoding via [rffmpeg](https://github.com/joshuaboniface/rffmpeg) on Kubernetes and Docker.

Both images are built directly on top of the official `jellyfin/jellyfin` image to guarantee matching `jellyfin-ffmpeg8` versions, Intel QSV / VA-API hardware acceleration driver parity, and binary paths.

---

## Images

### 1. `ghcr.io/justin8/jellyfin-rffmpeg` (Client / Server)
- **Base**: `jellyfin/jellyfin`
- **Includes**: Python 3, `python3-click`, `python3-yaml`, `openssh-client`, upstream `rffmpeg`, and `rffmpeg-init`.
- **Symlinks**: `/usr/local/bin/ffmpeg` and `/usr/local/bin/ffprobe` point to `/usr/local/bin/rffmpeg`.
- **Entrypoint**: Retains the official Jellyfin server entrypoint and configuration.

### 2. `ghcr.io/justin8/jellyfin-rffmpeg-worker` (Worker)
- **Base**: `jellyfin/jellyfin`
- **Security**: Runs completely unprivileged as non-root user `jellyfin` (`UID 2000`, `GID 2000`).
- **Hardware Acceleration**: Member of `video` and `render` groups for hardware acceleration access (`/dev/dri/renderD128`).
- **SSH Daemon**: OpenSSH server listening on unprivileged port `2222`.
- **Authentication**: Accepts public key authentication via `/home/jellyfin/.ssh/authorized_keys` (or mounted secret at `/etc/rffmpeg-worker/keys/authorized_keys`).

---

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

---

## How to Configure Jellyfin

In the Jellyfin Web UI, navigate to **Administration → Dashboard → Playback → Transcoding**:

### 1. FFmpeg Path: `/usr/local/bin/ffmpeg`
Set the **FFmpeg path** to:
```text
/usr/local/bin/ffmpeg
```

#### Why `/usr/local/bin/ffmpeg` and Not `/usr/local/bin/rffmpeg`?
Upstream `rffmpeg` uses the binary name (`argv[0]`) to differentiate between administrative commands and transcoding operations:
```python
if "rffmpeg" in cmd_name:
    run_control(config)      # Management CLI: rffmpeg init, rffmpeg add, etc.
else:
    run_ffmpeg(config, ffmpeg_args)  # Transcode proxy mode
```
- If Jellyfin calls `/usr/local/bin/rffmpeg`, the script attempts to parse FFmpeg transcode flags (`-analyzeduration`, `-i`, `-codec`) as Click CLI subcommands (`init`, `add`, `remove`), resulting in an error.
- When Jellyfin calls `/usr/local/bin/ffmpeg` (which is a symlink to `/usr/local/bin/rffmpeg`), `rffmpeg` is not in `argv[0]`, so it transparently intercepts the transcode job and offloads it over SSH to the remote worker.

### 2. Hardware Acceleration
- **Hardware acceleration**: Select **Intel QuickSync (QSV)** or **VA-API**.
- **QSV / VA-API Device**: `/dev/dri/renderD128`.
- **Hardware decoding codecs**: Check the codecs supported by your Intel processors (e.g. H264, HEVC, VC1, VP9).
- **Enable Hardware Encoding**: Checked.

### 3. Transcode & Temporary Directory
- **Transcode path**: Leave blank or set to `/cache/transcodes`.
- **Shared Temp**: Jellyfin 10.10+ / 12+ requires `TMPDIR=/cache/temp` exported to shared storage so remote workers can read intermediate subtitle and transcode chunks.

---

## Dynamic Cluster Discovery (`rffmpeg-init`)

This image includes a dedicated discovery tool installed at `/usr/local/bin/rffmpeg-init`. It is designed to run in a Kubernetes `initContainer` before the Jellyfin server container boots.

### What `rffmpeg-init` Does:
1. **Prepares Permissions**: Copies the SSH private key from the read-only Secret mount into `/config/.ssh/id_ed25519` and enforces `0600` permissions (preventing OpenSSH "unprotected private key" errors).
2. **Prepares Cache**: Creates the shared `/cache/temp` directory on the shared volume with `0777` permissions so workers can write transcode segments.
3. **Initializes Database**: Runs `rffmpeg init --no-root` to create `/config/rffmpeg/rffmpeg.db` if it does not already exist.
4. **Queries Kubernetes API**: Queries the cluster API for scheduled worker pods to dynamically obtain their node IPs.
5. **Synchronizes State**:
   - Queries `SELECT hostname FROM hosts` in `rffmpeg.db`.
   - Calls `rffmpeg add <ip>` for any newly discovered worker node IPs.
   - Calls `rffmpeg remove <ip>` for any decommissioned or stale node IPs.
   - Outputs `rffmpeg status` so the active node list is visible in the container logs.

### Assumptions & Prerequisites:
- **Same Namespace**: Assumes worker pods run in the same namespace as the Jellyfin pod (auto-detected via `/var/run/secrets/kubernetes.io/serviceaccount/namespace`).
- **Pod Label**: Assumes worker pods are labeled with `app=jellyfin-rffmpeg-worker` (configurable via `RFFMPEG_WORKER_LABEL`).
- **RBAC Permissions**: The pod's `ServiceAccount` requires a `Role` with `get` and `list` permissions on `pods` within its namespace:
  ```yaml
  apiVersion: rbac.authorization.k8s.io/v1
  kind: Role
  metadata:
    name: jellyfin-worker-discovery
  rules:
    - apiGroups: [""]
      resources: ["pods"]
      verbs: ["get", "list"]
  ```
- **Host Networking / hostPort**: Assumes worker pods bind to `hostPort: 2222` on each node, allowing `rffmpeg` to route jobs directly to each node's IP address (`pod.status.hostIP`).

### Options & Overrides for `rffmpeg-init`:

`rffmpeg-init` can be configured either via CLI flags or environment variables:

| CLI Option | Environment Variable | Default | Description |
|---|---|---|---|
| `-n`, `--namespace` | `RFFMPEG_WORKER_NAMESPACE` / `NAMESPACE` | *(autodetected)* | Namespace to search for worker pods (autodetected from pod's service account mount) |
| `-l`, `--label` | `RFFMPEG_WORKER_LABEL` | `app=jellyfin-rffmpeg-worker` | Kubernetes label selector for worker pods |
| `-c`, `--config` | `RFFMPEG_CONFIG` | `/etc/rffmpeg/rffmpeg.yml` | Path to `rffmpeg.yml` configuration |
| `--db` | `RFFMPEG_DB` | `/config/rffmpeg/rffmpeg.db` | Path to rffmpeg SQLite database |
| `--ssh-key-source` | `SSH_KEY_SOURCE` | `/etc/rffmpeg-ssh/id_ed25519` | Source path of mounted SSH private key |
| `--ssh-key-dest` | `SSH_KEY_DEST` | `/config/.ssh/id_ed25519` | Destination path where key is copied with 0600 mode |
| `--cache-temp-dir` | `CACHE_TEMP_DIR` | `/cache/temp` | Shared temporary cache directory |
| `--encoding-xml` | `ENCODING_XML` | `/config/config/encoding.xml` | Path to Jellyfin `encoding.xml` |

---

## Kubernetes Example

### Init Container in Jellyfin Deployment:
```yaml
initContainers:
  - name: init-rffmpeg
    image: ghcr.io/justin8/jellyfin-rffmpeg:12.1
    imagePullPolicy: IfNotPresent
    securityContext:
      runAsUser: 2000
      runAsGroup: 2000
    command:
      - rffmpeg-init
    volumeMounts:
      - name: config
        mountPath: /config
      - name: cache
        mountPath: /cache
      - name: rffmpeg-config
        mountPath: /etc/rffmpeg/rffmpeg.yml
        subPath: rffmpeg.yml
      - name: ssh-key
        mountPath: /etc/rffmpeg-ssh
        readOnly: true
```
