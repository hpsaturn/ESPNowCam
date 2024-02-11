/**************************************************
 * ESP32Cam
 * ========
 * Config utlility used for general purpose
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#ifndef CanfigApp_hpp
#define CanfigApp_hpp

#include <Preferences.h>
#include <mutex>

#define RW_MODE false
#define RO_MODE true

typedef enum {
    INT, BOOL, FLOAT, STRING, UNKNOWN
} ConfKeyType;

#define CONFIG_KEYS_LIST         \
  X(KLPAN, "lSpan", INT)      \
  X(KLOFST, "lOffset", INT)   \
  X(KLCENT, "lCenter", INT)   \
  X(KRPAN, "rSpan", INT)     \
  X(KROFST, "rOffset", INT)  \
  X(KRCENT, "rCenter", INT)  \
  X(KDBAND, "deathBand", INT)    \
  X(KPHERTZ, "periodHertz", INT) \
  X(KDEBUG, "debug", BOOL)      \
  X(KBASIC, "-----", UNKNOWN)    \
  X(KCOUNT, "KCOUNT", UNKNOWN)

#define X(kname, kreal, ktype) kname,
typedef enum CONFKEYS : size_t { CONFIG_KEYS_LIST } CONFKEYS; 
#undef X

class ConfigApp {
   public:
    uint64_t chipid;
    String deviceId;
    String dname;
    bool devmode;
    
    void init(const char app_name[]);

    void saveInt(String key, int value);
    void saveInt(CONFKEYS key, int value);

    int32_t getInt(String key, int defaultValue);
    int32_t getInt(CONFKEYS key, int defaultValue);
    
    bool getBool(String key, bool defaultValue);
    bool getBool(CONFKEYS key, bool defaultValue);

    void saveBool(String key, bool value);
    void saveBool(CONFKEYS key, bool value);

    float getFloat(String key, float defaultValue = 0.0);
    float getFloat(CONFKEYS key, float defaultValue = 0.0);

    void saveFloat(String key, float value);
    void saveFloat(CONFKEYS key, float value);

    void saveString(String key, String value);
    void saveString(CONFKEYS key, String value);

    String getString(String key, String defaultValue);
    String getString(CONFKEYS key, String defaultValue);

    String getValue(String key);

    String getDeviceId();

    String getDeviceIdShort();

    PreferenceType keyType(String key);

    bool isKey(String key);
    
    bool isKey(CONFKEYS key);

    String getKey(CONFKEYS key);

    ConfKeyType getKeyType(String key);

    ConfKeyType getKeyType(CONFKEYS key);
 
   private:  
    /// mutex for R/W actions
    std::mutex config_mtx;
    ///preferences main key
    char* _app_name;
    ///ESP32 preferences abstraction
    Preferences preferences;

    void DEBUG(const char* text, const char* textb = "");

    // @todo use DEBUG_ESP_PORT ?
#ifdef WM_DEBUG_PORT
    Stream& _debugPort = WM_DEBUG_PORT;
#else
    Stream& _debugPort = Serial;  // debug output stream ref
#endif
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_CFGHANDLER)
extern ConfigApp cfg;
#endif

#endif