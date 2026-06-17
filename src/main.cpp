#include <Arduino.h>
#include <M5Cardputer.h>
#include <time.h>

#include "config_store.h"
#include "monitor_ui.h"
#include "stats_client.h"
#include "wifi_manager.h"

ConfigStore configStore;
WifiManager wifiManager;
StatsClient statsClient;
MonitorUi ui;

bool ntpConfigured = false;

void ensureNtpSync() {
  if (ntpConfigured || !wifiManager.isConnected()) {
    return;
  }
  configTime(8 * 3600, 0, "pool.ntp.org", "ntp.aliyun.com");
  ntpConfigured = true;
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);
  M5Cardputer.Display.setRotation(1);

  Serial.begin(115200);
  delay(100);

  configStore.begin();
  wifiManager.begin();
  statsClient.begin(configStore, wifiManager);
  ui.begin(statsClient, wifiManager, configStore);
  ui.showBootScreen();

  delay(300);
  wifiManager.begin(configStore);
}

void loop() {
  M5Cardputer.update();
  wifiManager.loop();
  ensureNtpSync();
  ui.update();
  statsClient.loop();
  delay(10);
}
