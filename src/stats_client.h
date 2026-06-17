#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#include "config_store.h"

class WifiManager;

static const int kMaxTopProcesses = 5;
static const int kMaxDisks = 4;

struct ProcessInfo {
  String name;
  float cpuPercent = 0.0f;
  float memMb = 0.0f;
};

struct DiskInfo {
  String letter;
  float usage = 0.0f;
  float usedGb = 0.0f;
  float totalGb = 0.0f;
};

struct PcStats {
  String status = "offline";
  String hostname;
  String time;
  float cpuUsage = 0.0f;
  float cpuCoreMax = 0.0f;
  float cpuFreqMhz = 0.0f;
  float cpuTempC = -1.0f;
  float ramUsage = 0.0f;
  float ramUsedGb = 0.0f;
  float ramTotalGb = 0.0f;
  float swapPercent = 0.0f;
  float swapUsedGb = 0.0f;
  float swapTotalGb = 0.0f;
  float diskUsage = 0.0f;
  float diskUsedGb = 0.0f;
  float diskTotalGb = 0.0f;
  float diskReadMbps = 0.0f;
  float diskWriteMbps = 0.0f;
  float uploadKbps = 0.0f;
  float downloadKbps = 0.0f;
  float pingMs = -1.0f;
  bool gpuAvailable = false;
  String gpuName;
  float gpuUsage = 0.0f;
  float gpuTempC = -1.0f;
  int gpuMemUsedMb = 0;
  int gpuMemTotalMb = 0;
  float gpuMemPercent = 0.0f;
  float gpuPowerW = -1.0f;
  int batteryPercent = -1;
  bool batteryPlugged = false;
  float uptimeH = 0.0f;
  int processCount = 0;
  ProcessInfo topProcesses[kMaxTopProcesses];
  int topProcessCount = 0;
  DiskInfo disks[kMaxDisks];
  int diskCount = 0;
  bool online = false;
  String error;
};

class StatsClient {
 public:
  void begin();
  void begin(ConfigStore& configStore, WifiManager& wifiManager);
  void loop();
  PcStats fetchNow();
  PcStats currentStats() const;
  bool consumeUpdated();

 private:
  String buildBaseUrl() const;
  bool hasAgentConfig() const;
  bool requestStats(PcStats& outStats);
  void setOffline(const String& error);
  void parseStatsJson(const String& payload, PcStats& outStats);

  ConfigStore* configStore_ = nullptr;
  WifiManager* wifiManager_ = nullptr;
  PcStats stats_;
  uint32_t lastRequestMs_ = 0;
  uint32_t lastSuccessMs_ = 0;
  int failCount_ = 0;
  bool statsUpdated_ = false;

  static const int kMaxFailBeforeOffline = 4;
  static const uint32_t kStaleGraceMs = 20000;
};
