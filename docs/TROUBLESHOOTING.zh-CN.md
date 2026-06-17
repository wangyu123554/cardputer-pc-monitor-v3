# 常见问题

## Cardputer 显示 DOWN / 连不上 Agent

1. 在电脑上浏览器打开 `http://127.0.0.1:8765/stats`
   - 打不开：运行 `restart_agent.bat` 或管理员运行 `setup_pc_once.bat`
2. 确认 Cardputer 与电脑在**同一 Wi‑Fi**（访客网络常禁止设备互访）
3. Cardputer 设置页（键 **4**）检查 Agent IP 是否与 `setup_pc_once.bat` 显示的局域网 IP 一致
4. 端口必须是 **8765**
5. 防火墙：管理员运行 `open_firewall.bat`

## CPU 温度显示 `--`

温度为**可选功能**，没有温度也能正常监控。

1. 安装 [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor)（或管理员运行 `install_lhm_autostart.bat`）
2. LHM 中：**Options → Remote Web Server → Run**，IP `0.0.0.0`，端口 **8090**
3. 电脑验证：`http://127.0.0.1:8090/data.json`
4. 重启 Agent：`restart_agent.bat`

LHM 被关闭后，Agent 看门狗会在几分钟内自动拉起。

## CPU 使用率一直 0%

更新后请 `restart_agent.bat` 重启 Agent。旧进程可能没有后台 CPU 采样修复。

## GPU 页显示 N/A

需要 **NVIDIA** 驱动且 `nvidia-smi` 可用。暂不支持 AMD/Intel 核显。

## setup_pc_once.bat 闪退

- **右键 → 以管理员身份运行**
- UAC 点「是」，看新弹出的管理员窗口
- 脚本用 `cmd /k` 保持窗口，注意红色 ERROR

## Wi‑Fi 配网

1. 连接热点 **PCMonitor-Setup**
2. 浏览器 **http://192.168.4.1**
3. 填 Wi‑Fi 和密码 + 电脑 Agent IP + 端口 **8765**

## 取消开机自启

运行 `uninstall_all.bat`（只删计划任务，不删项目文件）
