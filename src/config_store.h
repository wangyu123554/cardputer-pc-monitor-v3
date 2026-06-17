#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

constexpr size_t MAX_WIFI_CONFIGS = 5;
constexpr uint16_t DEFAULT_AGENT_PORT = 8765;

struct WifiConfig {
  String ssid;
  String password;
  uint32_t lastConnected = 0;
};

struct DeviceConfig {
  WifiConfig wifiConfigs[MAX_WIFI_CONFIGS];
  size_t wifiCount = 0;
  String agentIp = "";
  uint16_t agentPort = DEFAULT_AGENT_PORT;
  String apiToken;
};

class ConfigStore {
 public:
  void begin();
  DeviceConfig load();
  bool save();
  bool save(const DeviceConfig& config);
  void clearAll();

  String getAgentIp() const;
  bool setAgentIp(const String& agentIp);

  uint16_t getAgentPort() const;
  bool setAgentPort(uint16_t agentPort);

  String getApiToken() const;
  bool setApiToken(const String& apiToken);

  bool addWifi(const String& ssid, const String& password);
  bool removeWifi(size_t index);
  size_t getWifiCount() const;
  WifiConfig getWifi(size_t index) const;
  bool updateLastConnected(size_t index);

 private:
  bool saveWifiList();
  void loadWifiList();

  Preferences preferences_;
  DeviceConfig config_;
  bool begun_ = false;
};
