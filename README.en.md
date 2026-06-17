# M5Stack Cardputer Adv — Windows PC Wi-Fi Monitor **v3**

[中文](README.md) | **English**

> **v3:** 6-page HUD · CPU/RAM/process sparklines · Top 5 processes · multi-disk · Swap · Ping · GPU power  
> **v2 (4 pages):** [cardputer-pc-monitor](https://github.com/wangyu123554/cardputer-pc-monitor)

> **Keywords:** M5Stack Cardputer Adv · Cardputer Adv · M5Stack · ESP32-S3 · Windows system monitor · Wi-Fi · PlatformIO

Monitor your **Windows PC** from an **M5Stack Cardputer Adv** over Wi‑Fi — CPU, RAM, disk, network, optional GPU and CPU temperature.

<p align="center">
  <img src="docs/images/cpu-page.jpg" width="480" alt="CPU / RAM page"/>
  <img src="docs/images/disk-page.jpg" width="480" alt="Disk / IO page"/>
</p>
<p align="center">
  <img src="docs/images/gpu-page.jpg" width="480" alt="GPU page"/>
  <img src="docs/images/settings-page.jpg" width="480" alt="Settings page"/>
</p>

## Features

- **Wi‑Fi monitoring** — no USB cable after setup
- **PC Agent** — lightweight Python service (`GET /stats` on port 8765)
- **Web setup portal** — connect Cardputer Adv to your Wi‑Fi and set PC IP
- **Optional CPU temp** — via LibreHardwareMonitor (HTTP)
- **Optional GPU** — NVIDIA via `nvidia-smi`
- **One-click PC setup** — firewall, autostart, LHM watchdog (Windows)

## Requirements

| Component | Required |
|-----------|----------|
| **M5Stack Cardputer Adv** | Yes |
| **Windows 10/11** PC on same LAN | Yes |
| Python **3.10+** (Add to PATH) | Yes |
| PlatformIO (VS Code) for firmware upload | Yes |
| LibreHardwareMonitor | Optional (CPU temperature) |
| NVIDIA GPU + drivers | Optional (GPU page) |

## Quick start

### 1. PC — one-time setup

```bat
git clone https://github.com/wangyu123554/cardputer-pc-monitor-v3.git
cd cardputer-pc-monitor-v3
```

Right-click **`一键安装.bat`** or **`setup_pc_once.bat`** → **Run as administrator**.

It will:

1. Install Python dependencies (`psutil`)
2. Open firewall port **8765**
3. Start Agent in background + **login autostart**
4. Optionally install/configure **LibreHardwareMonitor** + watchdog (CPU temp)
5. Print your **LAN IP** — you need this for step 3

Verify on PC: `http://127.0.0.1:8765/stats`

### 2. Flash firmware (M5Stack Cardputer Adv)

Open this folder in VS Code with PlatformIO → environment **`m5stack-cardputer`** → **Upload**.

### 3. Configure Cardputer Adv

1. Connect to Wi‑Fi hotspot **`PCMonitor-Setup`**
2. Browser: **http://192.168.4.1**
3. Enter your Wi‑Fi SSID/password and PC **Agent IP** (from setup) + port **8765**

## Minimal setup (no autostart / no temperature)

```bat
pip install -r requirements.txt
run_agent.bat
```

Then flash firmware and configure via the portal. CPU temperature will show `--` without LHM.

## Feature matrix

| Data | Source | If missing |
|------|--------|------------|
| CPU / RAM / disk / network | psutil (Agent) | Fix Agent / firewall |
| CPU temperature | LibreHardwareMonitor `:8090` | Shows `--` |
| GPU | `nvidia-smi` | GPU page N/A |
| Autostart | `setup_pc_once.bat` | Run `run_agent.bat` manually |

## Keys (Cardputer Adv)

| Key | Page |
|-----|------|
| **1** | CPU sparkline / freq / temp |
| **2** | RAM sparkline / Swap |
| **3** | Top 5 processes / #1 CPU curve |
| **4** | Multi-disk / network / Ping / I/O |
| **5** | GPU / system |
| **6** | Settings |
| **R** | Refresh (settings page) |
| **Del / Esc** | Back to CPU page |

Status bar: date/time · **LINK** (connected) / **DOWN** (PC unreachable) · battery · Wi‑Fi

## PC scripts

| Script | Purpose |
|--------|---------|
| `一键安装.bat` | **Recommended** — full one-time setup |
| `setup_pc_once.bat` | Full one-time setup |
| `build_agent_exe.bat` | Build standalone EXE (no Python) |
| `pack_release.bat` | Pack PC-Agent zip for distribution |
| `run_agent.bat` | Debug (console window) |
| `restart_agent.bat` | Restart Agent |
| `install_autostart.bat` | Agent autostart only |
| `install_lhm_autostart.bat` | LHM + watchdog only |
| `uninstall_all.bat` | Remove autostart tasks |
| `open_firewall.bat` | Firewall rule only |

## Project layout

```
cardputer-pc-monitor-v3/
├── src/                 # M5Stack Cardputer Adv firmware (ESP32-S3)
├── platformio.ini
├── pc_monitor_agent.py  # Windows Agent v3
├── 一键安装.bat
├── setup_pc_once.bat
├── requirements.txt
└── docs/
    ├── TROUBLESHOOTING.md
    └── images/
```

## Troubleshooting

See [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

## License

MIT — see [LICENSE](LICENSE).
