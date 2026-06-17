#include "config_store.h"

namespace {
const char* kNamespace = "pc-monitor";
const char* kAgentIpKey = "agent_ip";
const char* kAgentPortKey = "agent_port";
const char* kApiTokenKey = "api_token";
const char* kWifiListKey = "wifi_json";
const size_t kWifiJsonCapacity = 1024;

WifiConfig emptyWifiConfig() {
  WifiConfig config;
  return config;
}
}  // namespace

void ConfigStore::begin() {
  if (begun_) {
    return;
  }
  preferences_.begin(kNamespace, false);
  begun_ = true;
  load();
}

DeviceConfig ConfigStore::load() {
  if (!begun_) {
    begin();
    return config_;
  }

  config_.agentIp = preferences_.getString(kAgentIpKey, "");
  config_.agentPort = preferences_.getUShort(kAgentPortKey, DEFAULT_AGENT_PORT);
  if (config_.agentPort == 0) {
    config_.agentPort = DEFAULT_AGENT_PORT;
  }
  config_.apiToken = preferences_.getString(kApiTokenKey, "");
  loadWifiList();
  return config_;
}

bool ConfigStore::save() {
  if (!begun_) {
    begin();
  }

  preferences_.putString(kAgentIpKey, config_.agentIp);
  preferences_.putUShort(kAgentPortKey, config_.agentPort);
  preferences_.putString(kApiTokenKey, config_.apiToken);
  return saveWifiList();
}

bool ConfigStore::save(const DeviceConfig& config) {
  if (!begun_) {
    begin();
  }
  config_ = config;
  if (config_.wifiCount > MAX_WIFI_CONFIGS) {
    config_.wifiCount = MAX_WIFI_CONFIGS;
  }
  if (config_.agentPort == 0) {
    config_.agentPort = DEFAULT_AGENT_PORT;
  }
  return save();
}

void ConfigStore::clearAll() {
  if (!begun_) {
    begin();
  }
  preferences_.clear();
  config_ = DeviceConfig{};
}

String ConfigStore::getAgentIp() const {
  return config_.agentIp;
}

bool ConfigStore::setAgentIp(const String& agentIp) {
  if (!begun_) {
    begin();
  }
  config_.agentIp = agentIp;
  return save();
}

uint16_t ConfigStore::getAgentPort() const {
  return config_.agentPort;
}

bool ConfigStore::setAgentPort(uint16_t agentPort) {
  if (!begun_) {
    begin();
  }
  if (agentPort == 0) {
    return false;
  }
  config_.agentPort = agentPort;
  return save();
}

String ConfigStore::getApiToken() const {
  return config_.apiToken;
}

bool ConfigStore::setApiToken(const String& apiToken) {
  if (!begun_) {
    begin();
  }
  config_.apiToken = apiToken;
  return save();
}

bool ConfigStore::addWifi(const String& ssid, const String& password) {
  if (!begun_) {
    begin();
  }
  if (ssid.length() == 0) {
    return false;
  }

  for (size_t i = 0; i < config_.wifiCount; ++i) {
    if (config_.wifiConfigs[i].ssid == ssid) {
      config_.wifiConfigs[i].password = password;
      return saveWifiList();
    }
  }

  if (config_.wifiCount >= MAX_WIFI_CONFIGS) {
    for (size_t i = 1; i < MAX_WIFI_CONFIGS; ++i) {
      config_.wifiConfigs[i - 1] = config_.wifiConfigs[i];
    }
    config_.wifiCount = MAX_WIFI_CONFIGS - 1;
  }

  config_.wifiConfigs[config_.wifiCount].ssid = ssid;
  config_.wifiConfigs[config_.wifiCount].password = password;
  config_.wifiConfigs[config_.wifiCount].lastConnected = 0;
  ++config_.wifiCount;
  return saveWifiList();
}

bool ConfigStore::removeWifi(size_t index) {
  if (!begun_) {
    begin();
  }
  if (index >= config_.wifiCount) {
    return false;
  }

  for (size_t i = index + 1; i < config_.wifiCount; ++i) {
    config_.wifiConfigs[i - 1] = config_.wifiConfigs[i];
  }
  --config_.wifiCount;
  config_.wifiConfigs[config_.wifiCount] = emptyWifiConfig();
  return saveWifiList();
}

size_t ConfigStore::getWifiCount() const {
  return config_.wifiCount;
}

WifiConfig ConfigStore::getWifi(size_t index) const {
  if (index >= config_.wifiCount) {
    return emptyWifiConfig();
  }
  return config_.wifiConfigs[index];
}

bool ConfigStore::updateLastConnected(size_t index) {
  if (!begun_) {
    begin();
  }
  if (index >= config_.wifiCount) {
    return false;
  }

  uint32_t latest = 0;
  for (size_t i = 0; i < config_.wifiCount; ++i) {
    if (config_.wifiConfigs[i].lastConnected > latest) {
      latest = config_.wifiConfigs[i].lastConnected;
    }
  }
  config_.wifiConfigs[index].lastConnected = latest + 1;
  return saveWifiList();
}

bool ConfigStore::saveWifiList() {
  if (!begun_) {
    begin();
  }

  JsonDocument doc;
  JsonArray wifiArray = doc["wifi"].to<JsonArray>();
  for (size_t i = 0; i < config_.wifiCount && i < MAX_WIFI_CONFIGS; ++i) {
    JsonObject item = wifiArray.add<JsonObject>();
    item["ssid"] = config_.wifiConfigs[i].ssid;
    item["password"] = config_.wifiConfigs[i].password;
    item["last_connected"] = config_.wifiConfigs[i].lastConnected;
  }

  String json;
  serializeJson(doc, json);
  preferences_.putString(kWifiListKey, json);
  return true;
}

void ConfigStore::loadWifiList() {
  config_.wifiCount = 0;
  for (size_t i = 0; i < MAX_WIFI_CONFIGS; ++i) {
    config_.wifiConfigs[i] = emptyWifiConfig();
  }

  String json = preferences_.getString(kWifiListKey, "");
  if (json.length() == 0) {
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    preferences_.remove(kWifiListKey);
    return;
  }

  JsonArray wifiArray = doc["wifi"].as<JsonArray>();
  for (JsonObject item : wifiArray) {
    if (config_.wifiCount >= MAX_WIFI_CONFIGS) {
      break;
    }
    config_.wifiConfigs[config_.wifiCount].ssid = item["ssid"] | "";
    config_.wifiConfigs[config_.wifiCount].password = item["password"] | "";
    config_.wifiConfigs[config_.wifiCount].lastConnected = item["last_connected"] | 0;
    if (config_.wifiConfigs[config_.wifiCount].ssid.length() > 0) {
      ++config_.wifiCount;
    }
  }
}
