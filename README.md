# M5Stack Cardputer Adv — Windows 电脑 Wi-Fi 监控 **v3**

**中文** | [English](README.en.md)

> **v3 新特性：** 6 页 HUD · CPU/RAM/进程实时曲线 · Top5 进程 · 多磁盘 · Swap · Ping · GPU 功耗  
> **固件路径：** `C:\PlatformIO\cardputer-pc-monitor-v3`（不覆盖 v2）

> **关键词：** M5Stack Cardputer Adv · Cardputer Adv · M5Stack · ESP32-S3 · Windows 系统监控 · Wi-Fi · PlatformIO

在 **M5Stack Cardputer Adv** 上通过 Wi‑Fi 监控 **Windows 电脑** — CPU、内存、进程、磁盘、网络，可选 GPU 与 CPU 温度。

## v3 页面（按键 1–6）

| 键 | 页面 | 内容 |
|----|------|------|
| 1 | CPU | 大面积 CPU 曲线 + 频率/温度/峰值 |
| 2 | RAM | 内存曲线 + Swap |
| 3 | PROC | Top5 进程 + 榜首进程 CPU 曲线 |
| 4 | IO | 多磁盘 + 网速 + Ping + 磁盘 IO |
| 5 | GPU | 利用率 / VRAM / 功耗 |
| 6 | Set | Wi-Fi / Agent 配置信息 |

<p align="center">
  <img src="docs/images/cpu-page.jpg" width="480" alt="CPU / RAM 页面"/>
  <img src="docs/images/disk-page.jpg" width="480" alt="磁盘 / 网络页面"/>
</p>
<p align="center">
  <img src="docs/images/gpu-page.jpg" width="480" alt="GPU 页面"/>
  <img src="docs/images/settings-page.jpg" width="480" alt="设置页面"/>
</p>

## 功能

- **6 页科幻 HUD** — CPU/RAM/进程曲线、Top5 进程、多盘、Swap、Ping
- **Wi‑Fi 监控** — 配网后无需 USB 线
- **PC Agent** — Python 或独立 EXE（端口 **8765**，`GET /stats`）
- **Web 配网门户** — 连接 Wi‑Fi 并填写电脑 IP
- **CPU 温度（可选）** — LibreHardwareMonitor HTTP
- **GPU（可选）** — NVIDIA + `nvidia-smi`（含功耗）
- **一键 PC 部署** — 双击 `一键安装.bat` 或 `setup_pc_once.bat`

## 环境要求

| 组件 | 是否必须 |
|------|----------|
| **M5Stack Cardputer Adv** | 是 |
| **Windows 10/11** 电脑（同一局域网） | 是 |
| Python **3.10+**（安装时勾选 Add to PATH） | 是（或用 `build_agent_exe.bat` 生成 EXE） |
| VS Code + PlatformIO（烧录固件） | 是 |
| LibreHardwareMonitor | 可选（CPU 温度） |
| NVIDIA 显卡 + 驱动 | 可选（GPU 页） |

## 快速开始

### 1. 电脑端 — 一次配置

```bat
git clone https://github.com/wangyu123554/cardputer-pc-monitor-v3.git
cd cardputer-pc-monitor-v3
```

**右键「以管理员身份运行」→ `一键安装.bat`**（或 `setup_pc_once.bat`）

会自动完成：

1. 安装 Python 依赖（`psutil`）
2. 防火墙放行 **8765**
3. Agent 后台运行 + **开机自启**
4. （可选）安装/配置 **LibreHardwareMonitor** + 看门狗
5. 显示 **局域网 IP**（配网要用）

电脑验证：浏览器打开 `http://127.0.0.1:8765/stats`

### 2. 烧录固件（M5Stack Cardputer Adv）

VS Code 打开本项目 → PlatformIO → 环境 **`m5stack-cardputer`** → **Upload**

### 3. Cardputer Adv 配网

1. 连接 Wi‑Fi 热点 **`PCMonitor-Setup`**
2. 浏览器打开 **http://192.168.4.1**
3. 填写 Wi‑Fi 账号密码 + 电脑 **Agent IP** + 端口 **8765**

## 最小安装（不自启 / 不要温度）

```bat
pip install -r requirements.txt
run_agent.bat
```

再烧录固件并配网。不装 LHM 时 CPU 温度显示 `--`，其余功能正常。

## 功能说明

| 数据 | 来源 | 没有时会怎样 |
|------|------|----------------|
| CPU / 内存 / 磁盘 / 网络 | psutil（Agent） | 检查 Agent / 防火墙 |
| CPU 温度 | LibreHardwareMonitor `:8090` | 显示 `--` |
| GPU | `nvidia-smi` | GPU 页 N/A |
| 开机自启 | `setup_pc_once.bat` | 手动运行 `run_agent.bat` |

## 按键

| 键 | 页面 |
|----|------|
| **1** | CPU 曲线 / 频率 / 温度 |
| **2** | RAM 曲线 / Swap |
| **3** | Top5 进程 / 榜首进程曲线 |
| **4** | 多磁盘 / 网络 / Ping / IO |
| **5** | GPU / 系统信息 |
| **6** | 设置 |
| **R** | 刷新（设置页） |
| **Del / Esc** | 回到 CPU 页 |

状态栏：**日期时间 · LINK / DOWN · 电量 · WiFi**

> v2 四页版本见 [cardputer-pc-monitor](https://github.com/wangyu123554/cardputer-pc-monitor)

## PC 脚本

| 脚本 | 用途 |
|------|------|
| `一键安装.bat` | **推荐** — 一键完整配置（同 setup_pc_once） |
| `setup_pc_once.bat` | 一键完整配置 |
| `build_agent_exe.bat` | 打包独立 EXE（免 Python） |
| `pack_release.bat` | 打包 PC-Agent zip 分发包 |
| `run_agent.bat` | 调试（有窗口） |
| `restart_agent.bat` | 重启 Agent |
| `install_autostart.bat` | 仅 Agent 自启 |
| `install_lhm_autostart.bat` | 仅 LHM + 看门狗 |
| `uninstall_all.bat` | 取消所有自启 |
| `open_firewall.bat` | 仅防火墙 |

## 目录结构

```
cardputer-pc-monitor-v3/
├── src/                 # M5Stack Cardputer Adv 固件（ESP32-S3）
├── platformio.ini
├── pc_monitor_agent.py  # Windows Agent v3
├── 一键安装.bat          # 电脑一键配置
├── setup_pc_once.bat
├── requirements.txt
└── docs/
    ├── TROUBLESHOOTING.zh-CN.md
    └── images/
```

## 常见问题

见 [docs/TROUBLESHOOTING.zh-CN.md](docs/TROUBLESHOOTING.zh-CN.md)

## 许可证

MIT — 见 [LICENSE](LICENSE)
