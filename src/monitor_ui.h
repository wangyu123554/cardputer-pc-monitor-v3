#pragma once

#include <Arduino.h>
#include <M5Cardputer.h>

#include "config_store.h"
#include "stats_client.h"
#include "wifi_manager.h"

enum MonitorPage {
  MONITOR_PAGE_CPU,
  MONITOR_PAGE_RAM,
  MONITOR_PAGE_PROC,
  MONITOR_PAGE_IO,
  MONITOR_PAGE_GPU,
  MONITOR_PAGE_SETTINGS
};

class MonitorUi {
 public:
  void begin();
  void begin(StatsClient& statsClient, WifiManager& wifiManager, ConfigStore& configStore);
  void showBootScreen();
  void update();

 private:
  void handleKeyboard();
  void switchPage(MonitorPage page);
  void renderPageFull();
  void renderPagePartial(const PcStats& stats);
  void drawPageChrome(MonitorPage page);
  void drawCpuData(const PcStats& stats);
  void drawRamData(const PcStats& stats);
  void drawProcData(const PcStats& stats);
  void drawIoData(const PcStats& stats);
  void drawGpuData(const PcStats& stats);
  void drawSettingsData();

  void drawHeader(MonitorPage page);
  void drawStatusBar(const PcStats* stats);
  void drawFooter();
  void drawSciFiPanel(int x, int y, int w, int h, uint16_t accent);
  void drawCornerBrackets(int x, int y, int w, int h, uint16_t color);
  void drawSegmentBar(int x, int y, int w, int h, float percent, uint16_t color);
  void drawMiniBar(int x, int y, int w, int h, float percent, uint16_t color);
  void drawValueAt(int x, int y, int w, const String& text, uint16_t color, uint8_t textSize = 1);
  void drawSparklineArea(int x, int y, int w, int h, const uint8_t* history, int historyIdx,
                         bool historyFilled, uint16_t lineColor, uint16_t fillColor);
  void pushHistory(uint8_t* history, int& idx, bool& filled, float value);
  void drawTextLine(int y, const String& label, const String& value, uint16_t color = TFT_WHITE);

  uint16_t tempColor(float tempC) const;
  uint16_t accentForPage(MonitorPage page) const;
  String truncateText(const String& value, size_t maxLength) const;
  String wifiStatusText() const;
  String linkStatusText(const PcStats& stats) const;
  uint16_t linkStatusColor(const PcStats& stats) const;
  String formatTemp(float tempC) const;
  String formatFreqMhz(float mhz) const;
  String formatPercent(float value) const;
  String formatDateTime() const;
  String formatDeviceBattery() const;
  bool keyWordContains(const Keyboard_Class::KeysState& keys, char expected) const;
  void clearRegion(int x, int y, int w, int h);

  StatsClient* statsClient_ = nullptr;
  WifiManager* wifiManager_ = nullptr;
  ConfigStore* configStore_ = nullptr;
  MonitorPage page_ = MONITOR_PAGE_CPU;
  MonitorPage chromePage_ = MONITOR_PAGE_CPU;
  bool chromeDrawn_ = false;
  bool dirty_ = true;

  static const int kHistoryLen = 48;
  uint8_t cpuHistory_[48] = {};
  uint8_t ramHistory_[48] = {};
  uint8_t procHistory_[48] = {};
  int cpuHistoryIdx_ = 0;
  int ramHistoryIdx_ = 0;
  int procHistoryIdx_ = 0;
  bool cpuHistoryFilled_ = false;
  bool ramHistoryFilled_ = false;
  bool procHistoryFilled_ = false;
  uint32_t lastStatusDrawMs_ = 0;
  String lastStatusText_;
  float displayTempC_ = -1.0f;
  mutable int smoothBattPercent_ = -1;
  mutable int lastBattOnBattery_ = -1;
};
