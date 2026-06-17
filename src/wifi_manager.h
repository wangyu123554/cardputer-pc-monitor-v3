#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config_store.h"

enum WifiManagerStatus {
  WIFI_CONNECTED,
  WIFI_CONNECTING,
  WIFI_AP_MODE,
  WIFI_FAILED
};

class WifiManager {
 public:
  void begin();
  void begin(ConfigStore& configStore);
  void loop();
  bool connectSaved();
  void startConfigPortal();
  bool isConnected() const;
  String getSSID() const;
  int32_t getRSSI() const;
  WifiManagerStatus getStatus() const;
  String localIp() const;

 private:
  bool connectToWifi(const WifiConfig& wifiConfig, size_t savedIndex);
  bool isSavedSsidVisible(const String& ssid, int networkCount) const;
  void sortSavedIndexesByPriority(size_t* indexes, size_t count);
  void configurePortalRoutes();
  void handlePortalRoot();
  void handlePortalSave();
  String buildPortalPage() const;
  String htmlEscape(const String& value) const;

  WebServer server_{80};
  ConfigStore* configStore_ = nullptr;
  WifiManagerStatus status_ = WIFI_FAILED;
  bool portalRunning_ = false;
  bool pendingReconnect_ = false;
  uint32_t reconnectAfterMs_ = 0;
  uint32_t lastReconnectAttemptMs_ = 0;
};
