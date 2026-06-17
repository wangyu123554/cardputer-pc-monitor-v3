# Troubleshooting

## Cardputer shows DOWN / Agent offline

1. On PC, open `http://127.0.0.1:8765/stats` in a browser.
   - If it fails: run `restart_agent.bat` or `setup_pc_once.bat` (as Administrator).
2. Confirm Cardputer and PC are on the **same Wi‑Fi** (guest networks often block device-to-device traffic).
3. In Cardputer settings (key **4**), verify Agent IP matches the PC LAN IP shown by `setup_pc_once.bat`.
4. Port must be **8765**.
5. Windows Firewall: run `open_firewall.bat` as Administrator.

## CPU temperature shows `--`

Temperature is **optional**. Monitoring works without it.

1. Install [LibreHardwareMonitor](https://github.com/LibreHardwareMonitor/LibreHardwareMonitor) (or run `install_lhm_autostart.bat` as Administrator).
2. In LHM: **Options → Remote Web Server → Run**, IP `0.0.0.0`, port **8090**.
3. Verify on PC: `http://127.0.0.1:8090/data.json`
4. Restart Agent: `restart_agent.bat`

If LHM is closed, the Agent watchdog restarts it within a few minutes.

## CPU usage always 0%

Restart the Agent after updating (`restart_agent.bat`). The Agent samples CPU in a background thread; an old Agent process may not include this fix.

## GPU page shows N/A

GPU stats require **NVIDIA** drivers and `nvidia-smi` in PATH. AMD/Intel iGPU is not supported yet.

## setup_pc_once.bat closes immediately

- Right-click → **Run as administrator**.
- If UAC appears, click **Yes** and watch the new admin window.
- The script uses `cmd /k` so a window should stay open; read any red ERROR lines.

## Wi‑Fi setup portal

1. Connect phone/PC to hotspot **PCMonitor-Setup**.
2. Open **http://192.168.4.1**
3. Enter home Wi‑Fi SSID/password and PC Agent IP + port **8765**.

## Uninstall autostart

Run `uninstall_all.bat`. This removes scheduled tasks but does not delete project files.
