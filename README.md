# M5Stack Cardputer Adv — Windows PC Wi-Fi 系统监控 v3

**中文** | [English](README.en.md)

在 **M5Stack Cardputer Adv** 上通过 Wi-Fi 实时查看 **Windows 电脑** 的运行状态。v3 采用 **6 页科幻 HUD**，支持 CPU / 内存 / 进程曲线、Top5 进程、多磁盘、Swap、Ping、GPU 功耗等。

> 四页经典版：[cardputer-pc-monitor (v2)](https://github.com/wangyu123554/cardputer-pc-monitor)

<p align="center">
  <img src="docs/images/cpu-page.jpg" width="360" alt="CPU 页"/>
  <img src="docs/images/ram-page.jpg" width="360" alt="RAM 页"/>
  <img src="docs/images/proc-page.jpg" width="360" alt="进程页"/>
</p>
<p align="center">
  <img src="docs/images/io-page.jpg" width="360" alt="IO 页"/>
  <img src="docs/images/gpu-page.jpg" width="360" alt="GPU 页"/>
  <img src="docs/images/settings-page.jpg" width="360" alt="设置页"/>
</p>

---

## v3 亮点

| 类别 | 说明 |
|------|------|
| **6 页 HUD** | CPU · RAM · 进程 · IO · GPU · 设置，按键 1–6 切换 |
| **实时曲线** | CPU / RAM / 榜首进程 CPU 占用历史曲线（填充式 sparkline） |
| **进程监控** | Top5 进程（CPU + 内存），带进度条 |
| **存储 / 网络** | 最多 4 个磁盘、TX/RX 网速、Ping、磁盘读写 |
| **GPU** | 利用率、VRAM、温度、功耗（需 NVIDIA + `nvidia-smi`） |
| **PC 一键部署** | `一键安装.bat` 自动配置防火墙、自启、LHM 看门狗 |
| **免 Python** | 可选 `build_agent_exe.bat` 打包 `PCMonitorAgent.exe` |

---

## 六页界面

| 键 | 页面 | 显示内容 |
|:--:|------|----------|
| **1** | **CPU** | CPU 曲线、占用率、峰值、频率、温度、主机名 |
| **2** | **RAM** | 内存曲线、已用/总量、Swap 占用 |
| **3** | **PROC** | Top5 CPU 进程列表 + 榜首进程 CPU 曲线 |
| **4** | **IO** | 多磁盘空间、上传/下载、Ping、磁盘 IO、笔记本电量 |
| **5** | **GPU** | GPU 利用率、VRAM、温度、功耗、运行时间 |
| **6** | **Set** | Wi-Fi / Agent IP / 配网门户信息 |

**其他按键：** `R` 刷新设置页 · `Del` / `Esc` 回到 CPU 页

**状态栏：** 日期时间 · `LINK` / `DOWN` / `SLOW` · Cardputer 电量 · Wi-Fi 信号

---

## 环境要求

| 组件 | 必须 |
|------|:----:|
| M5Stack **Cardputer Adv** | ✓ |
| Windows 10/11 PC（同一局域网） | ✓ |
| Python 3.10+（或 Release 里的 EXE） | ✓ |
| VS Code + PlatformIO（烧录固件） | ✓ |
| LibreHardwareMonitor | 可选（CPU 温度） |
| NVIDIA 显卡 + 驱动 | 可选（GPU 页） |

---

## 快速开始

### 方式 A — 下载 Release（推荐给 PC 端用户）

1. 打开 [Releases](https://github.com/wangyu123554/cardputer-pc-monitor-v3/releases) 下载 **PC-Agent-v3.zip**
2. 解压后 **右键以管理员身份运行** `一键安装.bat`
3. 记下脚本输出的 **PC 局域网 IP**

### 方式 B — 克隆完整仓库（含固件）

```bat
git clone https://github.com/wangyu123554/cardputer-pc-monitor-v3.git
cd cardputer-pc-monitor-v3
```

**PC 端：** 管理员运行 `一键安装.bat`  
**固件：** VS Code + PlatformIO → 环境 `m5stack-cardputer` → Upload

### Cardputer 配网

1. 连接 Wi-Fi 热点 **`PCMonitor-Setup`**
2. 浏览器打开 **http://192.168.4.1**
3. 填写 Wi-Fi 账号密码 + PC **Agent IP** + 端口 **8765**

**PC 验证：** 浏览器访问 `http://127.0.0.1:8765/stats` 应返回 JSON

---

## 架构

```
Cardputer Adv  ──Wi-Fi GET /stats──►  Windows PC Agent (:8765)
                                         ├── psutil（CPU/RAM/磁盘/网络/进程）
                                         ├── LibreHardwareMonitor（CPU 温度，可选）
                                         └── nvidia-smi（GPU，可选）
```

Agent 在后台线程采样，Cardputer 每 ~2.5 秒拉取一次；**8765 端口单实例**，重复启动会自动退出，不会叠加进程。

---

## Agent 数据一览

| JSON 字段 | 说明 |
|-----------|------|
| `cpu` | 占用率、单核峰值、频率、温度 |
| `ram` | 内存占用 + Swap |
| `processes` | Top5 进程（name / cpu / mem_mb） |
| `disks` | 最多 4 个盘符空间 |
| `network` | 上传/下载 kbps、Ping ms |
| `disk_io` | 磁盘读写 MB/s |
| `gpu` | 利用率、VRAM、温度、功耗 |
| `battery` | 笔记本电量（如有） |
| `system` | 运行时间、进程总数 |

---

## PC 脚本

| 脚本 | 用途 |
|------|------|
| **`一键安装.bat`** | 推荐 — 防火墙 + 自启 + LHM + 立即启动 |
| `setup_pc_once.bat` | 同上 |
| `build_agent_exe.bat` | 打包独立 EXE（免装 Python） |
| `pack_release.bat` | 生成分发 zip |
| `restart_agent.bat` | 重启 Agent（先杀 8765 再启动） |
| `run_agent.bat` | 调试（有控制台窗口） |
| `uninstall_all.bat` | 移除所有计划任务 |
| `install_lhm_autostart.bat` | 仅 LHM + 3 分钟看门狗 |

---

## 资源占用（参考）

| 进程 | 典型内存 | 空闲 CPU |
|------|----------|----------|
| Agent (pythonw) | ~35 MB | < 1% |
| LibreHardwareMonitor | ~145 MB | ~0% |

---

## 目录结构

```
cardputer-pc-monitor-v3/
├── src/                    # Cardputer Adv 固件 (ESP32-S3)
├── platformio.ini
├── pc_monitor_agent.py     # Windows Agent
├── 一键安装.bat
├── setup_pc_once.bat
├── requirements.txt
└── docs/
    ├── TROUBLESHOOTING.zh-CN.md
    └── images/             # 六页截图
```

---

## 常见问题

详见 [docs/TROUBLESHOOTING.zh-CN.md](docs/TROUBLESHOOTING.zh-CN.md)

| 现象 | 处理 |
|------|------|
| Cardputer 显示 DOWN | 检查 PC IP、防火墙 8765、`restart_agent.bat` |
| CPU 温度 `--` | 安装 LHM，或等 Agent 自动拉起（约 30 秒） |
| GPU N/A | 需要 NVIDIA 显卡与 `nvidia-smi` |
| 升级后 Agent 仍是旧版 | 运行 `restart_agent.bat` |

---

## 许可证

MIT — 见 [LICENSE](LICENSE)
