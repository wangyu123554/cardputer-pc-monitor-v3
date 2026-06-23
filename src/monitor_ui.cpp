#include "monitor_ui.h"

#include <math.h>

namespace {
const int kPad = 3;
const int kHeaderH = 14;
const int kStatusY = 15;
const int kStatusH = 11;
const int kContentTop = 28;
const int kFooterY = 122;

const uint16_t kPanelBg = 0x0841;
const uint16_t kDimLine = 0x3186;
}  // namespace

void MonitorUi::begin() {
  M5Cardputer.Display.setBrightness(170);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setTextSize(1);
}

void MonitorUi::begin(StatsClient& statsClient, WifiManager& wifiManager, ConfigStore& configStore) {
  statsClient_ = &statsClient;
  wifiManager_ = &wifiManager;
  configStore_ = &configStore;
  begin();
}

void MonitorUi::showBootScreen() {
  auto& d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  drawCornerBrackets(8, 36, 224, 52, TFT_CYAN);
  d.setTextSize(2);
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.drawString("SYS-LINK", 52, 48);
  d.setTextSize(1);
  d.setTextColor(TFT_MAGENTA, TFT_BLACK);
  d.drawString("PC MONITOR v3", 68, 72);
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString("6-page HUD loading...", 62, 88);
  d.drawFastHLine(0, 16, d.width(), kDimLine);
}

void MonitorUi::update() {
  handleKeyboard();

  PcStats barStats;
  const PcStats* statsPtr = nullptr;
  if (statsClient_ != nullptr) {
    barStats = statsClient_->currentStats();
    statsPtr = &barStats;
  }

  uint32_t now = millis();
  if (now - lastStatusDrawMs_ >= 1000) {
    drawStatusBar(statsPtr);
    lastStatusDrawMs_ = now;
  }

  bool statsUpdated = statsClient_ != nullptr && statsClient_->consumeUpdated();
  if (statsUpdated && statsClient_ != nullptr) {
    const PcStats& s = statsClient_->currentStats();
    float histVal = s.cpuUsage;
    if (s.cpuCoreMax > histVal) {
      histVal = s.cpuCoreMax;
    }
    if (histVal < 0.0f) {
      histVal = 0.0f;
    }
    pushHistory(cpuHistory_, cpuHistoryIdx_, cpuHistoryFilled_, histVal);
    pushHistory(ramHistory_, ramHistoryIdx_, ramHistoryFilled_, s.ramUsage);
    if (s.topProcessCount > 0) {
      pushHistory(procHistory_, procHistoryIdx_, procHistoryFilled_, s.topProcesses[0].cpuPercent);
    }
  }

  if (page_ == MONITOR_PAGE_SETTINGS) {
    if (dirty_ || !chromeDrawn_) {
      renderPageFull();
    }
    return;
  }

  if (dirty_ || !chromeDrawn_ || page_ != chromePage_) {
    renderPageFull();
    return;
  }

  if (statsUpdated && statsClient_ != nullptr) {
    renderPagePartial(statsClient_->currentStats());
  }
}

void MonitorUi::handleKeyboard() {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return;
  }

  Keyboard_Class::KeysState keys = M5Cardputer.Keyboard.keysState();
  if (keyWordContains(keys, '1')) {
    switchPage(MONITOR_PAGE_CPU);
  } else if (keyWordContains(keys, '2')) {
    switchPage(MONITOR_PAGE_RAM);
  } else if (keyWordContains(keys, '3')) {
    switchPage(MONITOR_PAGE_PROC);
  } else if (keyWordContains(keys, '4')) {
    switchPage(MONITOR_PAGE_IO);
  } else if (keyWordContains(keys, '5')) {
    switchPage(MONITOR_PAGE_GPU);
  } else if (keyWordContains(keys, '6')) {
    switchPage(MONITOR_PAGE_SETTINGS);
  } else if (keyWordContains(keys, 'r') || keyWordContains(keys, 'R')) {
    if (page_ == MONITOR_PAGE_SETTINGS) {
      dirty_ = true;
    }
  } else if (keyWordContains(keys, 27) || keys.del) {
    switchPage(MONITOR_PAGE_CPU);
  }
}

void MonitorUi::switchPage(MonitorPage page) {
  if (page_ == page && chromeDrawn_) {
    return;
  }
  page_ = page;
  dirty_ = true;
  chromeDrawn_ = false;
}

void MonitorUi::renderPageFull() {
  auto& d = M5Cardputer.Display;
  d.fillScreen(TFT_BLACK);
  drawPageChrome(page_);
  chromeDrawn_ = true;
  chromePage_ = page_;
  dirty_ = false;

  if (page_ == MONITOR_PAGE_SETTINGS) {
    drawSettingsData();
    return;
  }

  PcStats stats;
  if (statsClient_ != nullptr) {
    stats = statsClient_->currentStats();
  }
  renderPagePartial(stats);
}

void MonitorUi::renderPagePartial(const PcStats& stats) {
  switch (page_) {
    case MONITOR_PAGE_RAM:
      drawRamData(stats);
      break;
    case MONITOR_PAGE_PROC:
      drawProcData(stats);
      break;
    case MONITOR_PAGE_IO:
      drawIoData(stats);
      break;
    case MONITOR_PAGE_GPU:
      drawGpuData(stats);
      break;
    case MONITOR_PAGE_CPU:
    default:
      drawCpuData(stats);
      break;
  }
}

void MonitorUi::drawPageChrome(MonitorPage page) {
  PcStats stats;
  if (statsClient_ != nullptr) {
    stats = statsClient_->currentStats();
  }
  drawHeader(page);
  drawStatusBar(&stats);
  drawFooter();

  uint16_t accent = accentForPage(page);
  switch (page) {
    case MONITOR_PAGE_CPU:
      drawSciFiPanel(kPad, kContentTop, 234, 52, accent);
      drawSciFiPanel(kPad, 82, 234, 38, kDimLine);
      break;
    case MONITOR_PAGE_RAM:
      drawSciFiPanel(kPad, kContentTop, 234, 52, accent);
      drawSciFiPanel(kPad, 82, 234, 38, kDimLine);
      break;
    case MONITOR_PAGE_PROC:
      drawSciFiPanel(kPad, kContentTop, 234, 68, accent);
      drawSciFiPanel(kPad, 98, 234, 22, kDimLine);
      break;
    case MONITOR_PAGE_IO:
      drawSciFiPanel(kPad, kContentTop, 234, 44, accent);
      drawSciFiPanel(kPad, 74, 234, 46, TFT_BLUE);
      break;
    case MONITOR_PAGE_GPU:
      drawSciFiPanel(kPad, kContentTop, 234, 56, accent);
      drawSciFiPanel(kPad, 86, 234, 34, kDimLine);
      break;
    case MONITOR_PAGE_SETTINGS:
      break;
  }
}

void MonitorUi::drawCpuData(const PcStats& stats) {
  auto& d = M5Cardputer.Display;
  uint16_t accent = accentForPage(MONITOR_PAGE_CPU);

  clearRegion(kPad + 2, kContentTop + 2, 230, 48);
  drawSparklineArea(kPad + 4, kContentTop + 4, 228, 44, cpuHistory_, cpuHistoryIdx_, cpuHistoryFilled_,
                    accent, 0x0220);

  clearRegion(kPad + 2, 84, 230, 34);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("CPU", kPad + 6, 86);
  d.setTextColor(accent, kPanelBg);
  d.drawString(formatPercent(stats.cpuUsage), kPad + 34, 86);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("PEAK", kPad + 88, 86);
  d.setTextColor(TFT_WHITE, kPanelBg);
  d.drawString(formatPercent(stats.cpuCoreMax), kPad + 118, 86);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString(formatFreqMhz(stats.cpuFreqMhz), kPad + 6, 98);

  if (stats.cpuTempC >= 0) {
    if (displayTempC_ < 0) {
      displayTempC_ = stats.cpuTempC;
    } else {
      float diff = stats.cpuTempC - displayTempC_;
      if (fabs(diff) > 3.0f) {
        displayTempC_ = stats.cpuTempC;
      } else {
        displayTempC_ = displayTempC_ * 0.85f + stats.cpuTempC * 0.15f;
      }
    }
  } else {
    displayTempC_ = -1.0f;
  }
  float showTemp = displayTempC_ >= 0 ? displayTempC_ : stats.cpuTempC;
  d.setTextColor(showTemp >= 0 ? tempColor(showTemp) : TFT_DARKGREY, kPanelBg);
  d.drawString(formatTemp(showTemp), kPad + 88, 98);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("HOST", kPad + 140, 98);
  d.setTextColor(TFT_CYAN, kPanelBg);
  d.drawString(truncateText(stats.hostname, 12), kPad + 170, 98);

  if (!stats.online && stats.error.length() > 0) {
    d.setTextColor(TFT_RED, kPanelBg);
    d.drawString(truncateText(stats.error, 14), kPad + 6, 108);
  }
}

void MonitorUi::drawRamData(const PcStats& stats) {
  auto& d = M5Cardputer.Display;
  uint16_t accent = accentForPage(MONITOR_PAGE_RAM);

  clearRegion(kPad + 2, kContentTop + 2, 230, 48);
  drawSparklineArea(kPad + 4, kContentTop + 4, 228, 44, ramHistory_, ramHistoryIdx_, ramHistoryFilled_,
                    accent, 0x2120);

  clearRegion(kPad + 2, 84, 230, 34);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("RAM", kPad + 6, 86);
  d.setTextColor(accent, kPanelBg);
  d.drawString(formatPercent(stats.ramUsage), kPad + 34, 86);
  String ramDetail = String(stats.ramUsedGb, 1) + "/" + String(stats.ramTotalGb, 1) + "G";
  d.setTextColor(TFT_WHITE, kPanelBg);
  d.drawString(ramDetail, kPad + 88, 86);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("SWAP", kPad + 6, 98);
  if (stats.swapTotalGb > 0.01f) {
    d.setTextColor(TFT_MAGENTA, kPanelBg);
    d.drawString(formatPercent(stats.swapPercent), kPad + 40, 98);
    d.setTextColor(TFT_WHITE, kPanelBg);
    d.drawString(String(stats.swapUsedGb, 1) + "/" + String(stats.swapTotalGb, 1) + "G", kPad + 78, 98);
    drawSegmentBar(kPad + 6, 108, 222, 4, stats.swapPercent, TFT_MAGENTA);
  } else {
    d.setTextColor(TFT_DARKGREY, kPanelBg);
    d.drawString("N/A", kPad + 40, 98);
  }
}

void MonitorUi::drawProcData(const PcStats& stats) {
  auto& d = M5Cardputer.Display;
  uint16_t accent = accentForPage(MONITOR_PAGE_PROC);

  clearRegion(kPad + 2, kContentTop + 2, 230, 64);
  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString("TOP CPU PROCESSES", kPad + 6, kContentTop + 4);

  int y = kContentTop + 16;
  int count = stats.topProcessCount > 0 ? stats.topProcessCount : 0;
  if (count == 0) {
    d.setTextColor(TFT_DARKGREY, kPanelBg);
    d.drawString("waiting for data...", kPad + 6, y + 8);
  } else {
    for (int i = 0; i < count && i < kMaxTopProcesses; ++i) {
      const ProcessInfo& p = stats.topProcesses[i];
      d.setTextColor(i == 0 ? accent : TFT_WHITE, kPanelBg);
      d.drawString(String(i + 1) + "." + truncateText(p.name, 10), kPad + 6, y);
      d.setTextColor(TFT_DARKGREY, kPanelBg);
      d.drawString(String(p.memMb, 0) + "M", kPad + 118, y);
      d.setTextColor(accent, kPanelBg);
      d.drawString(formatPercent(p.cpuPercent), kPad + 156, y);
      drawMiniBar(kPad + 6, y + 9, 222, 3, p.cpuPercent, accent);
      y += 14;
    }
  }

  clearRegion(kPad + 2, 100, 230, 18);
  if (stats.topProcessCount > 0) {
    d.setTextColor(TFT_DARKGREY, kPanelBg);
    d.drawString(truncateText(stats.topProcesses[0].name, 12) + " curve", kPad + 6, 100);
    drawSparklineArea(kPad + 4, 108, 228, 10, procHistory_, procHistoryIdx_, procHistoryFilled_, TFT_ORANGE,
                      0x2100);
  }
}

void MonitorUi::drawIoData(const PcStats& stats) {
  auto& d = M5Cardputer.Display;
  uint16_t accent = accentForPage(MONITOR_PAGE_IO);

  clearRegion(kPad + 2, kContentTop + 2, 230, 40);
  int y = kContentTop + 4;
  if (stats.diskCount == 0) {
    d.setTextColor(TFT_DARKGREY, kPanelBg);
    d.drawString("DISK C:", kPad + 6, y);
    d.setTextColor(accent, kPanelBg);
    d.drawString(formatPercent(stats.diskUsage), kPad + 56, y);
    drawMiniBar(kPad + 96, y + 2, 130, 4, stats.diskUsage, accent);
    y += 12;
  } else {
    for (int i = 0; i < stats.diskCount && i < kMaxDisks; ++i) {
      const DiskInfo& disk = stats.disks[i];
      d.setTextColor(TFT_DARKGREY, kPanelBg);
      d.drawString(String(disk.letter) + ":", kPad + 6, y);
      d.setTextColor(accent, kPanelBg);
      d.drawString(formatPercent(disk.usage), kPad + 22, y);
      d.setTextColor(TFT_WHITE, kPanelBg);
      d.drawString(String(disk.usedGb, 0) + "/" + String(disk.totalGb, 0) + "G", kPad + 56, y);
      drawMiniBar(kPad + 130, y + 2, 96, 4, disk.usage, accent);
      y += 10;
    }
  }

  clearRegion(kPad + 2, 76, 230, 42);
  drawValueAt(kPad + 6, 78, 110, "TX " + String(stats.uploadKbps, 0) + "k", TFT_GREEN, 1);
  drawValueAt(kPad + 118, 78, 110, "RX " + String(stats.downloadKbps, 0) + "k", TFT_BLUE, 1);
  String pingLine = stats.pingMs >= 0 ? String(stats.pingMs, 0) + " ms" : "-- ms";
  drawValueAt(kPad + 6, 90, 110, "PING " + pingLine, TFT_CYAN, 1);
  drawValueAt(kPad + 118, 90, 110,
              "IO " + String(stats.diskReadMbps, 1) + "/" + String(stats.diskWriteMbps, 1), TFT_WHITE, 1);

  if (stats.batteryPercent >= 0) {
    String bat = "PWR " + String(stats.batteryPercent) + "%";
    bat += stats.batteryPlugged ? " AC" : " DC";
    drawValueAt(kPad + 6, 102, 220, bat, TFT_YELLOW, 1);
  }
}

void MonitorUi::drawGpuData(const PcStats& stats) {
  uint16_t accent = accentForPage(MONITOR_PAGE_GPU);

  clearRegion(kPad + 2, kContentTop + 2, 230, 52);
  if (stats.gpuAvailable) {
    M5Cardputer.Display.setTextColor(TFT_DARKGREY, kPanelBg);
    M5Cardputer.Display.drawString("GPU", kPad + 6, kContentTop + 4);
    drawValueAt(kPad + 6, kContentTop + 16, 60, String(stats.gpuUsage, 0) + "%", accent, 2);
    String gpuLine = truncateText(stats.gpuName, 14);
    if (stats.gpuTempC >= 0) {
      gpuLine += " " + formatTemp(stats.gpuTempC);
    }
    drawValueAt(kPad + 60, kContentTop + 18, 170, gpuLine, TFT_WHITE, 1);
    if (stats.gpuMemTotalMb > 0) {
      String vram = "VRAM " + String(stats.gpuMemUsedMb) + "/" + String(stats.gpuMemTotalMb) + "M";
      if (stats.gpuMemPercent > 0) {
        vram += " " + formatPercent(stats.gpuMemPercent);
      }
      drawValueAt(kPad + 6, kContentTop + 32, 220, vram, TFT_DARKGREY, 1);
    }
    if (stats.gpuPowerW >= 0) {
      drawValueAt(kPad + 6, kContentTop + 44, 120, String(stats.gpuPowerW, 0) + " W", TFT_YELLOW, 1);
    }
    drawSegmentBar(kPad + 6, kContentTop + 52, 222, 4, stats.gpuUsage, accent);
  } else {
    drawValueAt(kPad + 6, kContentTop + 20, 220, "GPU N/A (need nvidia-smi)", TFT_DARKGREY, 1);
  }

  clearRegion(kPad + 2, 88, 230, 30);
  drawValueAt(kPad + 6, 90, 220, "UPTIME  " + String(stats.uptimeH, 1) + " h", TFT_WHITE, 1);
  drawValueAt(kPad + 6, 102, 220, "PROC    " + String(stats.processCount), TFT_WHITE, 1);
  drawValueAt(kPad + 6, 114, 220, "WIFI    " + wifiStatusText(), TFT_CYAN, 1);
}

void MonitorUi::drawSettingsData() {
  String agentIp = "";
  uint16_t agentPort = DEFAULT_AGENT_PORT;
  size_t wifiCount = 0;
  if (configStore_ != nullptr) {
    agentIp = configStore_->getAgentIp();
    agentPort = configStore_->getAgentPort();
    wifiCount = configStore_->getWifiCount();
  }

  clearRegion(0, kContentTop, 240, kFooterY - kContentTop);
  M5Cardputer.Display.fillRect(0, kContentTop, 240, kFooterY - kContentTop, TFT_BLACK);
  int y = kContentTop + 4;
  drawTextLine(y, "WIFI", wifiStatusText());
  y += 12;
  drawTextLine(y, "SSID", wifiManager_ != nullptr ? truncateText(wifiManager_->getSSID(), 20) : "");
  y += 12;
  drawTextLine(y, "LAN", wifiManager_ != nullptr ? truncateText(wifiManager_->localIp(), 20) : "");
  y += 12;
  drawTextLine(y, "SAVE", String(wifiCount));
  y += 12;
  drawTextLine(y, "AGNT", truncateText(agentIp, 20));
  y += 12;
  drawTextLine(y, "PORT", String(agentPort));
  y += 12;
  drawTextLine(y, "AP", "PCMonitor-Setup");
  y += 12;
  drawTextLine(y, "WEB", "192.168.4.1");
  y += 14;
  M5Cardputer.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5Cardputer.Display.drawString("[R] refresh  v3 HUD", kPad + 2, y);
}

void MonitorUi::drawHeader(MonitorPage page) {
  auto& d = M5Cardputer.Display;
  d.setTextSize(1);
  d.setTextColor(TFT_CYAN, TFT_BLACK);
  d.drawString("SYS", kPad, 3);
  d.setTextColor(TFT_MAGENTA, TFT_BLACK);
  d.drawString("-LINK", 22, 3);

  const char* tabs[] = {"C", "R", "P", "I", "G", "S"};
  MonitorPage pages[] = {MONITOR_PAGE_CPU, MONITOR_PAGE_RAM, MONITOR_PAGE_PROC,
                         MONITOR_PAGE_IO, MONITOR_PAGE_GPU, MONITOR_PAGE_SETTINGS};
  int tx = 108;
  for (int i = 0; i < 6; ++i) {
    bool active = pages[i] == page;
    d.setTextColor(active ? accentForPage(pages[i]) : TFT_DARKGREY, TFT_BLACK);
    d.drawString(active ? String("[") + tabs[i] + "]" : String(" ") + tabs[i] + " ", tx, 3);
    tx += 22;
  }
  d.drawFastHLine(0, kHeaderH, d.width(), kDimLine);
}

void MonitorUi::drawStatusBar(const PcStats* stats) {
  auto& d = M5Cardputer.Display;

  String datetime = formatDateTime();
  String battText = formatDeviceBattery();

  String link = "----";
  uint16_t linkColor = TFT_DARKGREY;
  if (stats != nullptr) {
    link = linkStatusText(*stats);
    linkColor = linkStatusColor(*stats);
  }

  String wifiTag = "--";
  if (wifiManager_ != nullptr) {
    if (wifiManager_->isConnected()) {
      wifiTag = String(wifiManager_->getRSSI());
    } else if (wifiManager_->getStatus() == WIFI_AP_MODE) {
      wifiTag = "AP";
    }
  }

  String status = datetime + "  " + link + "  " + battText + "  " + wifiTag;
  if (status == lastStatusText_) {
    return;
  }
  lastStatusText_ = status;

  clearRegion(0, kStatusY, d.width(), kStatusH);
  d.fillRect(0, kStatusY, d.width(), kStatusH, kPanelBg);
  d.drawFastHLine(0, kStatusY + kStatusH - 1, d.width(), kDimLine);

  d.setTextSize(1);
  d.setTextColor(TFT_CYAN, kPanelBg);
  d.drawString(datetime, kPad, kStatusY + 2);

  int linkX = 128;
  d.setTextColor(linkColor, kPanelBg);
  d.drawString(link, linkX, kStatusY + 2);

  d.setTextColor(TFT_DARKGREY, kPanelBg);
  d.drawString(battText, 176, kStatusY + 2);
  d.setTextColor(TFT_MAGENTA, kPanelBg);
  d.drawString(wifiTag, 214, kStatusY + 2);
}

void MonitorUi::drawFooter() {
  auto& d = M5Cardputer.Display;
  d.drawFastHLine(0, kFooterY, d.width(), kDimLine);
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString("1CPU 2RAM 3PROC 4IO 5GPU 6Set", kPad, kFooterY + 3);
}

void MonitorUi::drawSciFiPanel(int x, int y, int w, int h, uint16_t accent) {
  auto& d = M5Cardputer.Display;
  d.fillRect(x, y, w, h, kPanelBg);
  d.drawRect(x, y, w, h, accent);
  drawCornerBrackets(x, y, w, h, accent);
}

void MonitorUi::drawCornerBrackets(int x, int y, int w, int h, uint16_t color) {
  auto& d = M5Cardputer.Display;
  const int len = 5;
  d.drawFastHLine(x, y, len, color);
  d.drawFastVLine(x, y, len, color);
  d.drawFastHLine(x + w - len, y, len, color);
  d.drawFastVLine(x + w - 1, y, len, color);
  d.drawFastHLine(x, y + h - 1, len, color);
  d.drawFastVLine(x, y + h - len, len, color);
  d.drawFastHLine(x + w - len, y + h - 1, len, color);
  d.drawFastVLine(x + w - 1, y + h - len, len, color);
}

void MonitorUi::drawSegmentBar(int x, int y, int w, int h, float percent, uint16_t color) {
  auto& d = M5Cardputer.Display;
  if (percent < 0.0f) {
    percent = 0.0f;
  }
  if (percent > 100.0f) {
    percent = 100.0f;
  }
  clearRegion(x, y, w, h);
  const int segs = 16;
  int segW = (w - segs + 1) / segs;
  int filled = static_cast<int>((percent / 100.0f) * segs + 0.5f);
  for (int i = 0; i < segs; ++i) {
    int sx = x + i * (segW + 1);
    uint16_t c = i < filled ? color : TFT_DARKGREY;
    d.fillRect(sx, y, segW, h, c);
  }
}

void MonitorUi::drawMiniBar(int x, int y, int w, int h, float percent, uint16_t color) {
  auto& d = M5Cardputer.Display;
  if (percent < 0.0f) {
    percent = 0.0f;
  }
  if (percent > 100.0f) {
    percent = 100.0f;
  }
  d.fillRect(x, y, w, h, TFT_DARKGREY);
  int fillW = static_cast<int>((percent / 100.0f) * w);
  if (fillW > 0) {
    d.fillRect(x, y, fillW, h, color);
  }
}

void MonitorUi::drawValueAt(int x, int y, int w, const String& text, uint16_t color, uint8_t textSize) {
  auto& d = M5Cardputer.Display;
  clearRegion(x, y, w, textSize == 1 ? 10 : 16);
  d.setTextSize(textSize);
  d.setTextColor(color, kPanelBg);
  d.drawString(text, x, y);
  d.setTextSize(1);
}

void MonitorUi::drawSparklineArea(int x, int y, int w, int h, const uint8_t* history, int historyIdx,
                                  bool historyFilled, uint16_t lineColor, uint16_t fillColor) {
  auto& d = M5Cardputer.Display;
  clearRegion(x, y, w, h);
  d.drawRect(x, y, w, h, kDimLine);

  for (int g = 1; g <= 3; ++g) {
    int gy = y + (h * g) / 4;
    d.drawFastHLine(x + 1, gy, w - 2, 0x1082);
  }

  int count = historyFilled ? kHistoryLen : historyIdx;
  if (count < 2) {
    return;
  }

  int step = max(1, w / kHistoryLen);
  int prevX = x;
  int prevY = y + h - 1;
  int baseline = y + h - 1;

  for (int i = 0; i < count; ++i) {
    int idx = historyFilled ? (historyIdx + i) % kHistoryLen : i;
    int px = x + i * step;
    if (px >= x + w - 1) {
      break;
    }
    int val = history[idx];
    if (val > 100) {
      val = 100;
    }
    int py = y + h - 1 - (val * (h - 2) / 100);
    if (i > 0) {
      d.drawLine(prevX, prevY, px, py, lineColor);
      d.drawLine(prevX, baseline, prevX, prevY, fillColor);
    }
    prevX = px;
    prevY = py;
  }
  d.drawLine(prevX, baseline, prevX, prevY, fillColor);
}

void MonitorUi::pushHistory(uint8_t* history, int& idx, bool& filled, float value) {
  int val = static_cast<int>(value);
  if (val < 0) {
    val = 0;
  }
  if (val > 100) {
    val = 100;
  }
  history[idx] = static_cast<uint8_t>(val);
  idx = (idx + 1) % kHistoryLen;
  if (idx == 0) {
    filled = true;
  }
}

void MonitorUi::clearRegion(int x, int y, int w, int h) {
  M5Cardputer.Display.fillRect(x, y, w, h, kPanelBg);
}

uint16_t MonitorUi::tempColor(float tempC) const {
  if (tempC < 0) {
    return TFT_DARKGREY;
  }
  if (tempC < 60.0f) {
    return TFT_GREEN;
  }
  if (tempC < 80.0f) {
    return TFT_YELLOW;
  }
  return TFT_RED;
}

uint16_t MonitorUi::accentForPage(MonitorPage page) const {
  switch (page) {
    case MONITOR_PAGE_RAM:
      return TFT_YELLOW;
    case MONITOR_PAGE_PROC:
      return TFT_ORANGE;
    case MONITOR_PAGE_IO:
      return TFT_MAGENTA;
    case MONITOR_PAGE_GPU:
      return TFT_GREEN;
    case MONITOR_PAGE_SETTINGS:
      return TFT_WHITE;
    case MONITOR_PAGE_CPU:
    default:
      return TFT_CYAN;
  }
}

String MonitorUi::formatTemp(float tempC) const {
  if (tempC < 0) {
    return "-- C";
  }
  return String(tempC, 0) + " C";
}

String MonitorUi::formatFreqMhz(float mhz) const {
  if (mhz <= 0) {
    return "-- GHz";
  }
  if (mhz >= 1000.0f) {
    return String(mhz / 1000.0f, 1) + " GHz";
  }
  return String(static_cast<int>(mhz)) + " MHz";
}

String MonitorUi::formatPercent(float value) const {
  if (value < 0.0f) {
    return "--%";
  }
  if (value < 10.0f) {
    return String(value, 1) + "%";
  }
  return String(value, 0) + "%";
}

String MonitorUi::formatDeviceBattery() const {
  // Cardputer-Adv uses ADC voltage only; USB power biases readings to ~100%.
  int rawPercent = M5Cardputer.Power.getBatteryLevel();
  int voltageMv = M5Cardputer.Power.getBatteryVoltage();

  if (rawPercent < 0 && voltageMv <= 0) {
    return "--%";
  }

  bool onExternalPower = voltageMv >= 4120 || (rawPercent >= 98 && voltageMv >= 4050);

  if (!onExternalPower) {
    if (rawPercent < 0) {
      rawPercent = 0;
    }
    if (smoothBattPercent_ < 0) {
      smoothBattPercent_ = rawPercent;
    } else {
      smoothBattPercent_ = static_cast<int>(smoothBattPercent_ * 0.75f + rawPercent * 0.25f + 0.5f);
    }
    lastBattOnBattery_ = smoothBattPercent_;
    return String(smoothBattPercent_) + "%";
  }

  int showPercent = lastBattOnBattery_ >= 0 ? lastBattOnBattery_ : rawPercent;
  if (showPercent < 0) {
    showPercent = 0;
  }
  if (showPercent > 100) {
    showPercent = 100;
  }
  return String(showPercent) + "%AC";
}

String MonitorUi::formatDateTime() const {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return "--/--/-- --:--:--";
  }
  char buf[20];
  strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &timeinfo);
  return String(buf);
}

void MonitorUi::drawTextLine(int y, const String& label, const String& value, uint16_t color) {
  auto& d = M5Cardputer.Display;
  d.setTextColor(TFT_DARKGREY, TFT_BLACK);
  d.drawString(label, kPad + 2, y);
  d.setTextColor(color, TFT_BLACK);
  d.drawString(value, kPad + 36, y);
}

String MonitorUi::truncateText(const String& value, size_t maxLength) const {
  if (value.length() <= maxLength) {
    return value;
  }
  if (maxLength <= 3) {
    return value.substring(0, maxLength);
  }
  return value.substring(0, maxLength - 3) + "...";
}

String MonitorUi::wifiStatusText() const {
  if (wifiManager_ == nullptr) {
    return "?";
  }
  if (wifiManager_->isConnected()) {
    return String(wifiManager_->getRSSI()) + "dBm";
  }
  if (wifiManager_->getStatus() == WIFI_AP_MODE) {
    return "AP";
  }
  if (wifiManager_->getStatus() == WIFI_CONNECTING) {
    return "...";
  }
  return "X";
}

String MonitorUi::linkStatusText(const PcStats& stats) const {
  if (stats.online) {
    return "LINK";
  }
  if (stats.error.indexOf("timeout") >= 0 || stats.error.indexOf("Timeout") >= 0) {
    return "SLOW";
  }
  if (stats.error.indexOf("offline") >= 0 || stats.error.indexOf("Offline") >= 0) {
    return "DOWN";
  }
  return "DOWN";
}

uint16_t MonitorUi::linkStatusColor(const PcStats& stats) const {
  if (stats.online) {
    return TFT_GREEN;
  }
  if (stats.error.indexOf("timeout") >= 0 || stats.error.indexOf("Timeout") >= 0) {
    return TFT_YELLOW;
  }
  return TFT_RED;
}

bool MonitorUi::keyWordContains(const Keyboard_Class::KeysState& keys, char expected) const {
  for (auto key : keys.word) {
    String keyText = String(key);
    if (keyText.length() > 0 && keyText.charAt(0) == expected) {
      return true;
    }
  }
  return false;
}
