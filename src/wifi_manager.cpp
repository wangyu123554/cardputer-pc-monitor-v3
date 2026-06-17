#include "wifi_manager.h"

namespace {
const char* kSetupApSsid = "PCMonitor-Setup";
const uint32_t kConnectTimeoutMs = 8000;
const uint32_t kReconnectDelayMs = 1000;
const uint32_t kReconnectIntervalMs = 10000;
}  // namespace

void WifiManager::begin() {
  WiFi.mode(WIFI_STA);
  status_ = WIFI_FAILED;
}

void WifiManager::begin(ConfigStore& configStore) {
  configStore_ = &configStore;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  connectSaved();
}

void WifiManager::loop() {
  if (portalRunning_) {
    server_.handleClient();
  }

  if (pendingReconnect_ && millis() >= reconnectAfterMs_) {
    pendingReconnect_ = false;
    portalRunning_ = false;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    connectSaved();
  }

  if (!portalRunning_ && status_ == WIFI_CONNECTED && WiFi.status() != WL_CONNECTED) {
    status_ = WIFI_FAILED;
    lastReconnectAttemptMs_ = millis();
  }

  if (!portalRunning_ && status_ == WIFI_FAILED &&
      millis() - lastReconnectAttemptMs_ >= kReconnectIntervalMs) {
    lastReconnectAttemptMs_ = millis();
    connectSaved();
  }
}

bool WifiManager::connectSaved() {
  if (configStore_ == nullptr) {
    status_ = WIFI_FAILED;
    return false;
  }

  DeviceConfig config = configStore_->load();
  if (config.wifiCount == 0) {
    lastReconnectAttemptMs_ = millis();
    startConfigPortal();
    return false;
  }

  status_ = WIFI_CONNECTING;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(100);

  int networkCount = WiFi.scanNetworks(false, true);
  if (networkCount <= 0) {
    status_ = WIFI_FAILED;
    lastReconnectAttemptMs_ = millis();
    startConfigPortal();
    return false;
  }

  size_t indexes[MAX_WIFI_CONFIGS] = {0};
  for (size_t i = 0; i < config.wifiCount && i < MAX_WIFI_CONFIGS; ++i) {
    indexes[i] = i;
  }
  sortSavedIndexesByPriority(indexes, config.wifiCount);

  for (size_t order = 0; order < config.wifiCount && order < MAX_WIFI_CONFIGS; ++order) {
    size_t index = indexes[order];
    WifiConfig wifiConfig = config.wifiConfigs[index];
    if (!isSavedSsidVisible(wifiConfig.ssid, networkCount)) {
      continue;
    }

    if (connectToWifi(wifiConfig, index)) {
      WiFi.scanDelete();
      return true;
    }
  }

  WiFi.scanDelete();
  status_ = WIFI_FAILED;
  lastReconnectAttemptMs_ = millis();
  startConfigPortal();
  return false;
}

void WifiManager::startConfigPortal() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kSetupApSsid);
  configurePortalRoutes();
  server_.begin();
  portalRunning_ = true;
  status_ = WIFI_AP_MODE;
}

bool WifiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getSSID() const {
  if (!isConnected()) {
    return "";
  }
  return WiFi.SSID();
}

int32_t WifiManager::getRSSI() const {
  if (!isConnected()) {
    return 0;
  }
  return WiFi.RSSI();
}

WifiManagerStatus WifiManager::getStatus() const {
  return status_;
}

String WifiManager::localIp() const {
  if (!isConnected()) {
    return "";
  }
  return WiFi.localIP().toString();
}

bool WifiManager::connectToWifi(const WifiConfig& wifiConfig, size_t savedIndex) {
  if (wifiConfig.ssid.length() == 0) {
    return false;
  }

  status_ = WIFI_CONNECTING;
  WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());

  uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < kConnectTimeoutMs) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    status_ = WIFI_CONNECTED;
    if (configStore_ != nullptr) {
      configStore_->updateLastConnected(savedIndex);
    }
    return true;
  }

  WiFi.disconnect(false, false);
  status_ = WIFI_FAILED;
  return false;
}

bool WifiManager::isSavedSsidVisible(const String& ssid, int networkCount) const {
  for (int i = 0; i < networkCount; ++i) {
    if (WiFi.SSID(i) == ssid) {
      return true;
    }
  }
  return false;
}

void WifiManager::sortSavedIndexesByPriority(size_t* indexes, size_t count) {
  if (configStore_ == nullptr) {
    return;
  }

  DeviceConfig config = configStore_->load();
  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      uint32_t left = config.wifiConfigs[indexes[i]].lastConnected;
      uint32_t right = config.wifiConfigs[indexes[j]].lastConnected;
      if (right > left) {
        size_t temp = indexes[i];
        indexes[i] = indexes[j];
        indexes[j] = temp;
      }
    }
  }
}

void WifiManager::configurePortalRoutes() {
  server_.on("/", HTTP_GET, [this]() { handlePortalRoot(); });
  server_.on("/save", HTTP_POST, [this]() { handlePortalSave(); });
  server_.onNotFound([this]() { handlePortalRoot(); });
}

void WifiManager::handlePortalRoot() {
  server_.send(200, "text/html", buildPortalPage());
}

void WifiManager::handlePortalSave() {
  if (configStore_ == nullptr) {
    server_.send(500, "text/plain", "Config store unavailable");
    return;
  }

  String ssid = server_.arg("ssid");
  String password = server_.arg("password");
  String agentIp = server_.arg("agent_ip");
  uint16_t agentPort = static_cast<uint16_t>(server_.arg("agent_port").toInt());

  if (ssid.length() == 0) {
    server_.send(400, "text/plain", "Wi-Fi SSID is required");
    return;
  }

  if (agentPort == 0) {
    agentPort = DEFAULT_AGENT_PORT;
  }

  configStore_->addWifi(ssid, password);
  configStore_->setAgentIp(agentIp);
  configStore_->setAgentPort(agentPort);

  server_.send(
      200,
      "text/html",
      "<!doctype html><html><head><meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<title>PC Monitor</title></head><body>"
      "<h1>Saved</h1><p>PC Monitor will reconnect now.</p>"
      "</body></html>");

  pendingReconnect_ = true;
  reconnectAfterMs_ = millis() + kReconnectDelayMs;
}

String WifiManager::buildPortalPage() const {
  String agentIp = "";
  uint16_t agentPort = DEFAULT_AGENT_PORT;
  if (configStore_ != nullptr) {
    agentIp = configStore_->getAgentIp();
    agentPort = configStore_->getAgentPort();
  }

  String html;
  html.reserve(1800);
  html += "<!doctype html><html><head><meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>PC Monitor Setup</title>";
  html += "<style>body{font-family:Arial,sans-serif;margin:24px;max-width:520px}";
  html += "label{display:block;margin-top:14px;font-weight:600}";
  html += "input{box-sizing:border-box;width:100%;padding:10px;margin-top:6px}";
  html += "button{margin-top:18px;padding:10px 14px;width:100%}</style>";
  html += "</head><body><h1>PC Monitor Setup</h1>";
  html += "<p>Connect Cardputer to Wi-Fi and set PC Agent IP (port 8765).</p>";
  html += "<form method=\"post\" action=\"/save\">";
  html += "<label>Wi-Fi SSID<input name=\"ssid\" required></label>";
  html += "<label>Wi-Fi Password<input name=\"password\" type=\"password\"></label>";
  html += "<label>Windows Agent IP<input name=\"agent_ip\" value=\"";
  html += htmlEscape(agentIp);
  html += "\"></label>";
  html += "<label>Windows Agent Port<input name=\"agent_port\" type=\"number\" min=\"1\" max=\"65535\" value=\"";
  html += String(agentPort);
  html += "\"></label>";
  html += "<button type=\"submit\">Save and Connect</button>";
  html += "</form></body></html>";
  return html;
}

String WifiManager::htmlEscape(const String& value) const {
  String escaped;
  escaped.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    char c = value.charAt(i);
    if (c == '&') {
      escaped += "&amp;";
    } else if (c == '<') {
      escaped += "&lt;";
    } else if (c == '>') {
      escaped += "&gt;";
    } else if (c == '"') {
      escaped += "&quot;";
    } else {
      escaped += c;
    }
  }
  return escaped;
}
