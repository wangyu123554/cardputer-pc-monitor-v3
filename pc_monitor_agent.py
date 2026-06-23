#!/usr/bin/env python3
"""
Cardputer System Monitor - Wi-Fi PC Agent

Collects Windows performance metrics via psutil (+ optional LibreHardwareMonitor
WMI for CPU temp, nvidia-smi for GPU) and serves JSON at GET /stats on :8765.
"""

from __future__ import annotations

import json
import os
import re
import socket
import subprocess
import sys
import threading
import time
import urllib.error
import urllib.request
from datetime import datetime
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any, Optional
from urllib.parse import urlparse

import psutil

HOST = "0.0.0.0"
PORT = 8765
DISK_PATH = os.environ.get("MONITOR_DISK_PATH", "C:\\")
LHM_URL = os.environ.get("LHM_URL", "")
LHM_URLS = [
    u
    for u in [
        LHM_URL,
        "http://127.0.0.1:8090/data.json",
        "http://127.0.0.1:8085/data.json",
        "http://localhost:8090/data.json",
        "http://localhost:8085/data.json",
    ]
    if u
]

# Warm up cpu_percent cache once at startup (interval=0 avoids blocking per request).
psutil.cpu_percent(interval=None)


class NetworkSpeedTracker:
    def __init__(self) -> None:
        self._last_sent: Optional[int] = None
        self._last_recv: Optional[int] = None
        self._last_time: Optional[float] = None

    def snapshot(self) -> tuple[float, float]:
        try:
            counters = psutil.net_io_counters()
            now = time.monotonic()
            sent = counters.bytes_sent
            recv = counters.bytes_recv
        except Exception:
            return 0.0, 0.0

        if self._last_sent is None or self._last_time is None:
            self._last_sent = sent
            self._last_recv = recv
            self._last_time = now
            return 0.0, 0.0

        elapsed = now - self._last_time
        if elapsed <= 0:
            return 0.0, 0.0

        upload_kbps = max(0.0, (sent - self._last_sent) * 8 / 1024 / elapsed)
        download_kbps = max(0.0, (recv - self._last_recv) * 8 / 1024 / elapsed)

        self._last_sent = sent
        self._last_recv = recv
        self._last_time = now
        return round(upload_kbps, 1), round(download_kbps, 1)


class DiskIoTracker:
    def __init__(self) -> None:
        self._last_read: Optional[int] = None
        self._last_write: Optional[int] = None
        self._last_time: Optional[float] = None

    def snapshot(self) -> tuple[float, float]:
        try:
            counters = psutil.disk_io_counters()
            if counters is None:
                return 0.0, 0.0
            now = time.monotonic()
            read_bytes = counters.read_bytes
            write_bytes = counters.write_bytes
        except Exception:
            return 0.0, 0.0

        if self._last_read is None or self._last_time is None:
            self._last_read = read_bytes
            self._last_write = write_bytes
            self._last_time = now
            return 0.0, 0.0

        elapsed = now - self._last_time
        if elapsed <= 0:
            return 0.0, 0.0

        read_mbps = max(0.0, (read_bytes - self._last_read) / (1024 * 1024) / elapsed)
        write_mbps = max(0.0, (write_bytes - self._last_write) / (1024 * 1024) / elapsed)

        self._last_read = read_bytes
        self._last_write = write_bytes
        self._last_time = now
        return round(read_mbps, 2), round(write_mbps, 2)


_net_tracker = NetworkSpeedTracker()
_disk_io_tracker = DiskIoTracker()

_lhm_temp_cache: Optional[float] = None
_lhm_temp_smooth: Optional[float] = None
_lhm_sensor_key: Optional[str] = None
_lhm_cache_at: float = 0.0
_lhm_cache_ttl_s: float = 5.0
_lhm_fail_until: float = 0.0
_lhm_dir_cache: Optional[str] = None
_lhm_exe_cache: Optional[str] = None
_lhm_lock = threading.Lock()
_lhm_restart_last: float = 0.0
_lhm_restart_cooldown_s: float = 90.0
_lhm_dll_last_attempt: float = 0.0
_lhm_dll_interval_s: float = 60.0
_lhm_acpi_last_attempt: float = 0.0
_lhm_acpi_interval_s: float = 30.0
_worker_started = False

_cpu_metrics: dict[str, Any] = {"usage": 0.0, "core_max": 0.0, "freq_mhz": None}
_cpu_metrics_lock = threading.Lock()
_cpu_refresh_lock = threading.Lock()
_top_processes: list[dict[str, Any]] = []
_process_lock = threading.Lock()
_ping_ms_cache: Optional[float] = None
_ping_lock = threading.Lock()


def _find_lhm_exe() -> Optional[str]:
    global _lhm_exe_cache
    if _lhm_exe_cache and os.path.isfile(_lhm_exe_cache):
        return _lhm_exe_cache

    base = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Microsoft", "WinGet", "Packages")
    if os.path.isdir(base):
        for name in os.listdir(base):
            if not name.startswith("LibreHardwareMonitor"):
                continue
            candidate = os.path.join(base, name, "LibreHardwareMonitor.exe")
            if os.path.isfile(candidate):
                _lhm_exe_cache = candidate
                return candidate
    return None


def _is_lhm_process_running() -> bool:
    try:
        for proc in psutil.process_iter(["name"]):
            name = proc.info.get("name") or ""
            if name.lower() == "librehardwaremonitor.exe":
                return True
    except Exception:
        pass
    return False


def _configure_lhm_config(exe: str) -> None:
    """Ensure Remote Web Server settings are valid (listenerIp must not be blank/corrupt)."""
    cfg = os.path.join(os.path.dirname(exe), "LibreHardwareMonitor.config")
    if not os.path.isfile(cfg):
        return
    ps_script = f"""
$ErrorActionPreference = 'Stop'
[xml]$xml = Get-Content -LiteralPath '{cfg.replace("'", "''")}' -Encoding UTF8
$apps = $xml.configuration.appSettings
function Set-Key($k,$v) {{
  $item = $apps.add | Where-Object {{ $_.key -eq $k }}
  if (-not $item) {{
    $item = $xml.CreateElement('add')
    $item.SetAttribute('key',$k)
    [void]$apps.AppendChild($item)
  }}
  $item.value = $v
}}
Set-Key 'listenerIp' '0.0.0.0'
Set-Key 'listenerPort' '8090'
Set-Key 'runWebServerMenuItem' 'true'
Set-Key 'authenticationEnabled' 'false'
$xml.Save('{cfg.replace("'", "''")}')
"""
    try:
        subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", ps_script],
            capture_output=True,
            text=True,
            timeout=8,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
    except Exception:
        pass


def ensure_lhm_running() -> None:
    if _is_lhm_process_running():
        return
    exe = _find_lhm_exe()
    if not exe:
        return
    workdir = os.path.dirname(exe)
    _configure_lhm_config(exe)
    try:
        # GUI app — minimize startup flash; Agent (pythonw) has no console of its own.
        kwargs: dict[str, Any] = {"cwd": workdir}
        if sys.platform == "win32":
            si = subprocess.STARTUPINFO()
            si.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            si.wShowWindow = 7  # SW_MINIMIZE — LHM goes to tray
            kwargs["startupinfo"] = si
        subprocess.Popen([exe], **kwargs)
    except Exception:
        pass


def _is_lhm_http_ok() -> bool:
    return _fetch_lhm_tree() is not None


def _restart_lhm_process() -> None:
    """LHM process can run with Remote Web Server stopped — restart to recover HTTP."""
    global _lhm_restart_last, _lhm_fail_until
    now = time.monotonic()
    if now - _lhm_restart_last < _lhm_restart_cooldown_s:
        return
    _lhm_restart_last = now
    try:
        subprocess.run(
            ["taskkill", "/IM", "LibreHardwareMonitor.exe", "/F"],
            capture_output=True,
            timeout=5,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
    except Exception:
        pass
    time.sleep(2)
    exe = _find_lhm_exe()
    if exe:
        _configure_lhm_config(exe)
    ensure_lhm_running()
    time.sleep(8)
    _lhm_fail_until = 0.0


def _ensure_lhm_healthy() -> None:
    if _is_lhm_http_ok():
        return
    if _is_lhm_process_running():
        _restart_lhm_process()
    else:
        ensure_lhm_running()
        time.sleep(6)
        _lhm_fail_until = 0.0


def _find_lhm_dir() -> Optional[str]:
    global _lhm_dir_cache
    if _lhm_dir_cache and os.path.isfile(os.path.join(_lhm_dir_cache, "LibreHardwareMonitorLib.dll")):
        return _lhm_dir_cache

    base = os.path.join(os.environ.get("LOCALAPPDATA", ""), "Microsoft", "WinGet", "Packages")
    if os.path.isdir(base):
        for name in os.listdir(base):
            if not name.startswith("LibreHardwareMonitor"):
                continue
            candidate = os.path.join(base, name)
            if os.path.isfile(os.path.join(candidate, "LibreHardwareMonitorLib.dll")):
                _lhm_dir_cache = candidate
                return candidate
    return None


def _collect_cpu_temp_lhm_dll() -> Optional[float]:
    lhm_dir = _find_lhm_dir()
    if not lhm_dir:
        return None

    ps_script = f"""
$ErrorActionPreference = 'Stop'
$dir = '{lhm_dir.replace("'", "''")}'
Add-Type -Path (Join-Path $dir 'LibreHardwareMonitorLib.dll')
$computer = New-Object LibreHardwareMonitor.Hardware.Computer
$computer.IsCpuEnabled = $true
$computer.IsGpuEnabled = $false
$computer.IsMemoryEnabled = $false
$computer.IsMotherboardEnabled = $false
$computer.IsControllerEnabled = $false
$computer.IsNetworkEnabled = $false
$computer.IsStorageEnabled = $false
$computer.Open()
for ($i = 0; $i -lt 8; $i++) {{
  foreach ($hw in $computer.Hardware) {{
    $hw.Update()
    foreach ($sub in $hw.SubHardware) {{ $sub.Update() }}
  }}
  Start-Sleep -Milliseconds 150
}}
$best = $null
$priority = @('CPU Package', 'Core Max', 'Tctl', 'Tdie', 'CPU Core')
foreach ($name in $priority) {{
  foreach ($hw in $computer.Hardware) {{
    foreach ($sensor in $hw.Sensors) {{
      if ($sensor.SensorType -eq [LibreHardwareMonitor.Hardware.SensorType]::Temperature -and $sensor.Name -eq $name -and $null -ne $sensor.Value) {{
        $best = [double]$sensor.Value
        break
      }}
    }}
    if ($null -ne $best) {{ break }}
    foreach ($sub in $hw.SubHardware) {{
      foreach ($sensor in $sub.Sensors) {{
        if ($sensor.SensorType -eq [LibreHardwareMonitor.Hardware.SensorType]::Temperature -and $sensor.Name -eq $name -and $null -ne $sensor.Value) {{
          $best = [double]$sensor.Value
          break
        }}
      }}
      if ($null -ne $best) {{ break }}
    }}
  }}
  if ($null -ne $best) {{ break }}
}}
if ($null -eq $best) {{
  foreach ($hw in $computer.Hardware) {{
    foreach ($sensor in $hw.Sensors) {{
      if ($sensor.SensorType -eq [LibreHardwareMonitor.Hardware.SensorType]::Temperature -and $null -ne $sensor.Value -and $sensor.Name -notmatch 'GPU|Hot Spot|Memory') {{
        $val = [double]$sensor.Value
        if ($null -eq $best -or $val -gt $best) {{ $best = $val }}
      }}
    }}
    foreach ($sub in $hw.SubHardware) {{
      foreach ($sensor in $sub.Sensors) {{
        if ($sensor.SensorType -eq [LibreHardwareMonitor.Hardware.SensorType]::Temperature -and $null -ne $sensor.Value -and $sensor.Name -notmatch 'GPU|Hot Spot|Memory') {{
          $val = [double]$sensor.Value
          if ($null -eq $best -or $val -gt $best) {{ $best = $val }}
        }}
      }}
    }}
  }}
}}
$computer.Close()
if ($null -ne $best) {{ Write-Output $best }}
"""

    try:
        result = subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", ps_script],
            capture_output=True,
            text=True,
            timeout=6,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        if result.returncode != 0:
            return None
        for line in result.stdout.splitlines():
            line = line.strip()
            if not line:
                continue
            try:
                val = float(line)
            except ValueError:
                continue
            if 0 < val < 150:
                return round(val, 1)
    except Exception:
        pass
    return None


def _fetch_lhm_tree() -> Optional[Any]:
    global _lhm_fail_until
    now = time.monotonic()
    if now < _lhm_fail_until:
        return None

    urls = LHM_URLS or ["http://127.0.0.1:8090/data.json", "http://127.0.0.1:8085/data.json"]
    for url in urls:
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "CardputerMonitor/1.0"})
            with urllib.request.urlopen(request, timeout=0.8) as response:
                data = json.loads(response.read().decode("utf-8"))
                _lhm_fail_until = 0.0
                return data
        except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, ValueError, OSError):
            continue

    _lhm_fail_until = now + 10.0
    return None


def _parse_sensor_value(value_raw: Any) -> Optional[float]:
    if value_raw is None:
        return None
    if isinstance(value_raw, (int, float)):
        value = float(value_raw)
    elif isinstance(value_raw, str):
        text = value_raw.strip().replace(",", ".")
        match = re.search(r"[-+]?\d*\.?\d+", text)
        if not match:
            return None
        try:
            value = float(match.group())
        except ValueError:
            return None
    else:
        try:
            value = float(value_raw)
        except (TypeError, ValueError):
            return None
    if 0 < value < 150:
        return value
    return None


def _walk_lhm_sensors(node: Any, out: list[tuple[str, float, str]]) -> None:
    if isinstance(node, dict):
        sensor_type = str(node.get("Type", ""))
        value_raw = node.get("Value")
        text = str(node.get("Text", ""))
        node_id = str(node.get("id", ""))
        sensor_id = str(node.get("SensorId", ""))
        value = _parse_sensor_value(value_raw)

        is_temp_type = sensor_type.lower() == "temperature"
        is_temp_path = "/temperature/" in sensor_id.lower()

        # Only real temperature sensors — name-only matching wrongly picked CPU Package
        # power (W) and GPU core clock (MHz) as degrees Celsius.
        if value is not None and (is_temp_type or is_temp_path):
            out.append((text, value, node_id))

        for child in node.get("Children", []) or []:
            _walk_lhm_sensors(child, out)
    elif isinstance(node, list):
        for item in node:
            _walk_lhm_sensors(item, out)


def _pick_cpu_temp(temps: list[tuple[str, float, str]]) -> Optional[float]:
    """Pick one stable CPU sensor (CPU Package first, matches ASUS/LHM)."""
    global _lhm_sensor_key
    if not temps:
        return None

    if _lhm_sensor_key:
        for text, value, node_id in temps:
            if f"{text}|{node_id}" == _lhm_sensor_key:
                return round(value, 1)

    skip = (
        "gpu", "hot spot", "memory", "vrm", "mos", "chipset", "pch",
        "socket", "ambient", "drive", "nvme", "ssd", "hdd", "wifi",
        "distance to tjmax",
    )

    def usable(text: str, node_id: str) -> bool:
        blob = f"{text} {node_id}".lower()
        return not any(k in blob for k in skip)

    # Exact sensor names — same priority as LibreHardwareMonitor / ASUS Armoury Crate
    exact_names = (
        "CPU Package",
        "Tctl",
        "Tdie",
        "Core Max",
        "CPU Core",
        "Core Average",
    )
    for name in exact_names:
        for text, value, node_id in temps:
            if text == name and usable(text, node_id):
                _lhm_sensor_key = f"{text}|{node_id}"
                return round(value, 1)

    # Partial match fallback (still one sensor, not max-of-all)
    partial_keys = ("cpu package", "tctl", "tdie", "core max", "cpu core")
    for key in partial_keys:
        for text, value, node_id in temps:
            blob = f"{text} {node_id}".lower()
            if key in blob and usable(text, node_id):
                _lhm_sensor_key = f"{text}|{node_id}"
                return round(value, 1)

    return None


def _smooth_cpu_temp(raw: float) -> float:
    """EMA smoothing — stops digit bouncing between polls."""
    global _lhm_temp_smooth
    if _lhm_temp_smooth is None:
        _lhm_temp_smooth = raw
        return round(raw, 1)
    # Ignore single-sample spikes > 4 C (wrong sensor glitch)
    if abs(raw - _lhm_temp_smooth) > 4.0:
        return round(_lhm_temp_smooth, 1)
    _lhm_temp_smooth = _lhm_temp_smooth * 0.82 + raw * 0.18
    return round(_lhm_temp_smooth, 1)


def _collect_cpu_temp_acpi() -> Optional[float]:
    try:
        result = subprocess.run(
            [
                "wmic",
                r"/namespace:\\root\wmi",
                "path",
                "MSAcpi_ThermalZoneTemperature",
                "get",
                "CurrentTemperature",
            ],
            capture_output=True,
            text=True,
            timeout=1,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        for line in result.stdout.splitlines():
            line = line.strip()
            if not line.isdigit():
                continue
            celsius = int(line) / 10.0 - 273.15
            if 0 < celsius < 120:
                return round(celsius, 1)
    except Exception:
        pass
    return None


def _refresh_process_cache() -> None:
    """Background top-process sampling (avoid blocking /stats)."""
    global _top_processes
    rows: list[dict[str, Any]] = []
    try:
        procs = list(psutil.process_iter(["pid", "name", "memory_info"]))
        for proc in procs:
            try:
                proc.cpu_percent(interval=None)
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                continue
        for proc in procs:
            try:
                name = proc.info.get("name") or "?"
                if name.lower() in ("system idle process", "_total"):
                    continue
                cpu = proc.cpu_percent(interval=None) or 0.0
                mem_info = proc.info.get("memory_info")
                mem_mb = round(mem_info.rss / (1024 * 1024), 1) if mem_info else 0.0
                rows.append({"name": name, "cpu": round(float(cpu), 1), "mem_mb": mem_mb})
            except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
                continue
        rows.sort(key=lambda r: r["cpu"], reverse=True)
        top = rows[:5]
    except Exception:
        top = []
    with _process_lock:
        _top_processes = top


def _refresh_ping_cache() -> None:
    global _ping_ms_cache
    target = "8.8.8.8"
    try:
        for _iface, addrs in psutil.net_if_addrs().items():
            for addr in addrs:
                if addr.family == socket.AF_INET and not addr.address.startswith("127."):
                    target = addr.address.rsplit(".", 1)[0] + ".1"
                    break
    except Exception:
        pass
    try:
        result = subprocess.run(
            ["ping", "-n", "1", "-w", "800", target],
            capture_output=True,
            text=True,
            timeout=2,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        match = re.search(r"[=<]\s*(\d+)\s*ms", result.stdout, re.IGNORECASE)
        if not match:
            match = re.search(r"(\d+)\s*ms", result.stdout, re.IGNORECASE)
        if match:
            ms = int(match.group(1))
            with _ping_lock:
                _ping_ms_cache = float(ms)
            return
    except Exception:
        pass
    with _ping_lock:
        _ping_ms_cache = None


def _refresh_cpu_metrics() -> None:
    """Background CPU sampling — one percpu snapshot avoids Windows race / zero core_max."""
    global _cpu_metrics
    try:
        with _cpu_refresh_lock:
            per_core = psutil.cpu_percent(interval=0.3, percpu=True)
            if per_core:
                usage = round(sum(per_core) / len(per_core), 1)
                core_max = round(max(per_core), 1)
            else:
                usage = 0.0
                core_max = 0.0
            freq_mhz: Optional[int] = None
            freq = psutil.cpu_freq()
            if freq and freq.current:
                freq_mhz = int(freq.current)
            with _cpu_metrics_lock:
                _cpu_metrics["usage"] = usage
                _cpu_metrics["core_max"] = core_max
                _cpu_metrics["freq_mhz"] = freq_mhz
    except Exception:
        pass


def _refresh_cpu_temp_cache() -> None:
    """Background-only: LHM HTTP first, then DLL/ACPI fallbacks (never blocks /stats)."""
    global _lhm_temp_cache, _lhm_cache_at

    temp: Optional[float] = None
    lhm_tree = _fetch_lhm_tree()
    if lhm_tree is not None:
        lhm_temps: list[tuple[str, float, str]] = []
        _walk_lhm_sensors(lhm_tree, lhm_temps)
        raw = _pick_cpu_temp(lhm_temps)
        if raw is not None:
            temp = _smooth_cpu_temp(raw)

    if temp is None:
        now = time.monotonic()
        if now - _lhm_dll_last_attempt >= _lhm_dll_interval_s:
            _lhm_dll_last_attempt = now
            raw = _collect_cpu_temp_lhm_dll()
            if raw is not None:
                temp = _smooth_cpu_temp(raw)

    if temp is None:
        now = time.monotonic()
        if now - _lhm_acpi_last_attempt >= _lhm_acpi_interval_s:
            _lhm_acpi_last_attempt = now
            raw = _collect_cpu_temp_acpi()
            if raw is not None:
                temp = _smooth_cpu_temp(raw)

    with _lhm_lock:
        if temp is not None:
            _lhm_temp_cache = temp
        _lhm_cache_at = time.monotonic()


def collect_cpu_temp_c() -> Optional[float]:
    """Fast path for /stats: return cached value only."""
    with _lhm_lock:
        return _lhm_temp_cache


def _background_worker() -> None:
    _ensure_lhm_healthy()
    time.sleep(2)
    _refresh_cpu_metrics()
    _refresh_cpu_temp_cache()
    _refresh_process_cache()
    tick = 0
    while True:
        try:
            _refresh_cpu_metrics()
            if tick % 3 == 0:
                _refresh_process_cache()
            if tick % 10 == 0:
                _refresh_ping_cache()
            if tick % 5 == 0:
                _refresh_cpu_temp_cache()
            if tick % 20 == 0:
                _ensure_lhm_healthy()
        except Exception:
            pass
        tick += 1
        time.sleep(1)


def start_background_worker() -> None:
    global _worker_started
    if _worker_started:
        return
    _worker_started = True
    _refresh_cpu_metrics()
    thread = threading.Thread(target=_background_worker, name="lhm-watchdog", daemon=True)
    thread.start()


def collect_cpu() -> dict[str, Any]:
    with _cpu_metrics_lock:
        usage = _cpu_metrics.get("usage")
        core_max = _cpu_metrics.get("core_max")
        freq_mhz = _cpu_metrics.get("freq_mhz")
    temp_c = collect_cpu_temp_c()
    return {
        "usage": usage,
        "core_max": core_max,
        "freq_mhz": freq_mhz,
        "temp_c": temp_c,
    }


def collect_ram() -> dict[str, Any]:
    try:
        mem = psutil.virtual_memory()
        swap = psutil.swap_memory()
        return {
            "usage": round(mem.percent, 1),
            "used_gb": round(mem.used / (1024 ** 3), 1),
            "total_gb": round(mem.total / (1024 ** 3), 1),
            "swap_percent": round(swap.percent, 1),
            "swap_used_gb": round(swap.used / (1024 ** 3), 1),
            "swap_total_gb": round(swap.total / (1024 ** 3), 1),
        }
    except Exception:
        return {
            "usage": None,
            "used_gb": None,
            "total_gb": None,
            "swap_percent": None,
            "swap_used_gb": None,
            "swap_total_gb": None,
        }


def collect_disk() -> dict[str, Any]:
    try:
        usage = psutil.disk_usage(DISK_PATH)
        return {
            "usage": round(usage.percent, 1),
            "used_gb": round(usage.used / (1024 ** 3), 1),
            "total_gb": round(usage.total / (1024 ** 3), 1),
            "path": DISK_PATH,
        }
    except Exception:
        return {"usage": None, "used_gb": None, "total_gb": None, "path": DISK_PATH}


def collect_disks() -> list[dict[str, Any]]:
    drives: list[dict[str, Any]] = []
    for letter in "CDEFGHIJKLMNOPQRSTUVWXYZ":
        path = f"{letter}:\\"
        if not os.path.exists(path):
            continue
        try:
            usage = psutil.disk_usage(path)
            drives.append(
                {
                    "letter": letter,
                    "usage": round(usage.percent, 1),
                    "used_gb": round(usage.used / (1024 ** 3), 1),
                    "total_gb": round(usage.total / (1024 ** 3), 1),
                }
            )
        except Exception:
            continue
        if len(drives) >= 4:
            break
    return drives


def collect_top_processes() -> list[dict[str, Any]]:
    with _process_lock:
        return list(_top_processes)


def collect_network() -> dict[str, Any]:
    upload_kbps, download_kbps = _net_tracker.snapshot()
    with _ping_lock:
        ping_ms = _ping_ms_cache
    return {"upload_kbps": upload_kbps, "download_kbps": download_kbps, "ping_ms": ping_ms}


def collect_disk_io() -> dict[str, Any]:
    read_mbps, write_mbps = _disk_io_tracker.snapshot()
    return {"read_mbps": read_mbps, "write_mbps": write_mbps}


def collect_gpu() -> dict[str, Any]:
    empty: dict[str, Any] = {
        "available": False,
        "name": None,
        "usage": None,
        "temp_c": None,
        "mem_used_mb": None,
        "mem_total_mb": None,
    }
    try:
        result = subprocess.run(
            [
                "nvidia-smi",
                "--query-gpu=name,utilization.gpu,temperature.gpu,memory.used,memory.total,power.draw",
                "--format=csv,noheader,nounits",
            ],
            capture_output=True,
            text=True,
            timeout=1,
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
        )
        if result.returncode != 0 or not result.stdout.strip():
            return empty
        parts = [p.strip() for p in result.stdout.strip().split("\n")[0].split(",")]
        if len(parts) < 5:
            return empty
        mem_used = int(float(parts[3]))
        mem_total = int(float(parts[4]))
        mem_percent = round(mem_used * 100.0 / mem_total, 1) if mem_total > 0 else None
        power_w = float(parts[5]) if len(parts) > 5 and parts[5] not in ("[N/A]", "N/A", "") else None
        return {
            "available": True,
            "name": parts[0],
            "usage": float(parts[1]),
            "temp_c": float(parts[2]),
            "mem_used_mb": mem_used,
            "mem_total_mb": mem_total,
            "mem_percent": mem_percent,
            "power_w": power_w,
        }
    except Exception:
        return empty


def collect_battery() -> dict[str, Any]:
    try:
        bat = psutil.sensors_battery()
        if bat is None:
            return {"percent": None, "plugged": None}
        return {"percent": int(bat.percent), "plugged": bool(bat.power_plugged)}
    except Exception:
        return {"percent": None, "plugged": None}


def collect_system() -> dict[str, Any]:
    uptime_h = None
    process_count = None
    try:
        uptime_h = round((time.time() - psutil.boot_time()) / 3600, 1)
    except Exception:
        pass
    try:
        process_count = len(psutil.pids())
    except Exception:
        pass
    return {"uptime_h": uptime_h, "process_count": process_count}


def collect_stats() -> dict[str, Any]:
    return {
        "status": "online",
        "hostname": socket.gethostname(),
        "time": datetime.now().strftime("%H:%M:%S"),
        "cpu": collect_cpu(),
        "ram": collect_ram(),
        "disk": collect_disk(),
        "disks": collect_disks(),
        "disk_io": collect_disk_io(),
        "network": collect_network(),
        "processes": collect_top_processes(),
        "gpu": collect_gpu(),
        "battery": collect_battery(),
        "system": collect_system(),
    }


class StatsHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, format: str, *args: Any) -> None:
        pass

    def _send_json(self, status_code: int, payload: dict[str, Any]) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/stats":
            try:
                self._send_json(200, collect_stats())
            except Exception as exc:
                self._send_json(500, {"status": "error", "message": str(exc)})
        elif path in ("/", "/health"):
            self._send_json(200, {"status": "online", "endpoint": "/stats"})
        else:
            self._send_json(404, {"status": "error", "message": "Not found"})


def get_local_ip() -> str:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except Exception:
        return "127.0.0.1"


def print_banner() -> None:
    local_ip = get_local_ip()
    print("=" * 56)
    print("  Cardputer PC Monitor Agent v3")
    print("=" * 56)
    print(f"  Hostname : {socket.gethostname()}")
    print(f"  Listen   : http://{HOST}:{PORT}/stats")
    print(f"  LAN URL  : http://{local_ip}:{PORT}/stats")
    print("  CPU temp : LHM http://127.0.0.1:8090/data.json (or :8085)")
    print("  GPU      : requires nvidia-smi (NVIDIA drivers)")
    print("=" * 56)


def port_in_use(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            sock.bind((HOST, port))
        except OSError:
            return True
    return False


def main() -> None:
    if port_in_use(PORT):
        # Another instance is already serving /stats (common after logon task + manual start).
        if not sys.stdout.isatty():
            sys.exit(0)
        print(f"Port {PORT} already in use. Agent is probably already running.")
        sys.exit(0)

    print_banner()
    start_background_worker()
    server = ThreadingHTTPServer((HOST, PORT), StatsHandler)
    server.daemon_threads = True
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nAgent stopped.")
        server.server_close()
        sys.exit(0)


if __name__ == "__main__":
    main()
