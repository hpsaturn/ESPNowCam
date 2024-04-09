/**************************************************
 * ESP32Cam
 * ========
 * Config utlility used for general purpose
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include "ConfigApp.hpp"

#define X(kname, kreal, ktype) kreal, 
char const *keys[] = { CONFIG_KEYS_LIST };
#undef X

#define X(kname, kreal, ktype) ktype, 
int keys_type[] = { CONFIG_KEYS_LIST };
#undef X

void ConfigApp::init(const char app_name[]) {
    _app_name = new char[strlen(app_name) + 1];
    strcpy(_app_name, app_name);
    chipid = ESP.getEfuseMac();
    deviceId = getDeviceId();
    // override with debug INFO level (>=3)
#if CORE_DEBUG_LEVEL >= 1
    devmode = true;
#endif
    if (devmode) Serial.println("-->[CONF] debug is enable.");
}

void ConfigApp::saveString(String key, String value){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RW_MODE);
    preferences.putString(key.c_str(), value.c_str());
    preferences.end();
}

void ConfigApp::saveString(CONFKEYS key, String value){
    saveString(getKey(key),value);
}

String ConfigApp::getString(String key, String defaultValue){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    String out = preferences.getString(key.c_str(), defaultValue);
    preferences.end();
    return out;
}

String ConfigApp::getString(CONFKEYS key, String defaultValue){
    return getString(getKey(key),defaultValue);
}

void ConfigApp::saveInt(String key, int value){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RW_MODE);
    preferences.putInt(key.c_str(), value);
    preferences.end();
}

void ConfigApp::saveInt(CONFKEYS key, int value){
    saveInt(getKey(key),value);
}

int32_t ConfigApp::getInt(String key, int defaultValue){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    int32_t out = preferences.getInt(key.c_str(), defaultValue);
    preferences.end();
    return out;
}

int32_t ConfigApp::getInt(CONFKEYS key, int defaultValue){ 
    return getInt(getKey(key),defaultValue);
}

void ConfigApp::saveBool(String key, bool value){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RW_MODE);
    preferences.putBool(key.c_str(), value);
    preferences.end();
}

void ConfigApp::saveBool(CONFKEYS key, bool value){
    saveBool(getKey(key),value);
}

bool ConfigApp::getBool(String key, bool defaultValue){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    bool out = preferences.getBool(key.c_str(), defaultValue);
    preferences.end();
    return out;
}

bool ConfigApp::getBool(CONFKEYS key, bool defaultValue){
    return getBool(getKey(key),defaultValue);
}

void ConfigApp::saveFloat(String key, float value){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RW_MODE);
    preferences.putFloat(key.c_str(), value);
    preferences.end();
}

void ConfigApp::saveFloat(CONFKEYS key, float value){
    saveFloat(getKey(key),value);
}

float ConfigApp::getFloat(String key, float defaultValue){
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    float out = preferences.getFloat(key.c_str(), defaultValue);
    preferences.end();
    return out;
}

float ConfigApp::getFloat(CONFKEYS key, float defaultValue){
    return getFloat(getKey(key),defaultValue);
}

PreferenceType ConfigApp::keyType(String key) {
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    PreferenceType type = preferences.getType(key.c_str());
    preferences.end();
    return type;
}

bool ConfigApp::isKey(String key) {
    std::lock_guard<std::mutex> lck(config_mtx);
    preferences.begin(_app_name, RO_MODE);
    bool iskey = preferences.isKey(key.c_str());
    preferences.end();
    return iskey;
}

bool ConfigApp::isKey(CONFKEYS key) {
    return isKey(getKey(key));
}

String ConfigApp::getKey(CONFKEYS key) {
  if (key < 0 || key > CONFKEYS::KCOUNT) return "";
  return String(keys[key]);
}

ConfKeyType ConfigApp::getKeyType(CONFKEYS key) {
  if (key < 0 || key > CONFKEYS::KCOUNT) return ConfKeyType::UNKNOWN;
  return (ConfKeyType)keys_type[key];
}

ConfKeyType ConfigApp::getKeyType(String key) {
  for (int i = 0; i < KCOUNT; i++) {
    if (key.equals(keys[i])) return (ConfKeyType)keys_type[i];
  }
  return ConfKeyType::UNKNOWN;
}

String ConfigApp::getValue(String key) {
  ConfKeyType type = cfg.getKeyType(key);
  if (type == ConfKeyType::BOOL) return cfg.getBool(key, false) ? "true" : "false";
  if (type == ConfKeyType::FLOAT) return String(cfg.getFloat(key, false));
  if (type == ConfKeyType::INT) return String(cfg.getInt(key, false));
  if (type == ConfKeyType::STRING) return cfg.getString(key, "");
  return "";
}

String ConfigApp::getDeviceId() {
    uint8_t baseMac[6];
    // Get MAC address for WiFi station
    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
    char baseMacChr[18] = {0};
    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]+2);
    return String(baseMacChr);
}

String ConfigApp::getDeviceIdShort() {
    String devId = getDeviceId();
    devId = devId.substring(13);
    devId.replace(":","");
    return devId;
}

void ConfigApp::DEBUG(const char *text, const char *textb) {
    if (devmode) {
        _debugPort.print(text);
        if (textb) {
            _debugPort.print(" ");
            _debugPort.print(textb);
        }
        _debugPort.println();
    }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CFGHANDLER)
ConfigApp cfg;
#endif
