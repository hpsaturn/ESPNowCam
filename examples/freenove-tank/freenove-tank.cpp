/**************************************************
 * ESP32Cam Freenove ESPNow Transmitter
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESP32S3 camera tests project:
 * https://github.com/hpsaturn/esp32s3-cam
**************************************************/

#include <Arduino.h>
#include <ESP32WifiCLI.hpp>
#include "CamFreenove.h"
#include "ESPNowCam.h"
#include "comm.pb.h"
#include <ESP32Servo.h>
#include "ConfigApp.hpp"
#include <Utils.h>

#define BUILTINLED 2

CamFreenove Camera;
ESPNowCam radio;

// buffer for control commands from joystick
uint8_t *recv_buff;
// Joystick meessage for store the commands
JoystickMessage jm = JoystickMessage_init_zero;

Servo servoLeft;
Servo servoRight;
int servoLeftPin = 47;
int servoRightPin = 21;

bool running, fire, debug;
uint32_t count = 0;

int spanLeft, offsetLeft, degreesCenterL;
int degreesMinL, degreesMaxL;

int deathBand;

int spanRight, offsetRight, degreesCenterR;
int degreesMinR, degreesMaxR;

bool setup_mode = false;
int setup_time = 10000;

bool cam_stopped = false;

void attachServoLeft() {
  if (!servoLeft.attached()) servoLeft.attach(servoLeftPin);
}

void attachServoRight() {
  if (!servoRight.attached()) servoRight.attach(servoRightPin);
}

void detachServos() {
  servoLeft.detach();
  servoRight.detach();
}

/**
 * @param Vty Joystick left stick, Y axis (forward/backward)
 * @param Wt Joystick right stick, X asis (Left/Right) 
*/
void setSpeed(int16_t Vy, int16_t Wt) {
  int16_t Vtx = constrain(-Wt, -100, 100);
  int16_t Vty = constrain(Vy, -100, 100);

  if (abs(Vtx) < deathBand && abs(Vty) < deathBand) {
    Vtx = 0;
    Vty = 0;
  }
 
  // Mixer
  int spdL = Vty + Vtx;   //motorDelanteroIzquierdo
  int spdR = -Vty + Vtx;  //motorDelanteroDerecho

  // Servo output
  spdL = map(spdL, -100, 100, degreesMinL, degreesMaxL);
  spdR = map(spdR, -100, 100, degreesMinR, degreesMaxR);

  // if (spdL != deathBand) {
  if (abs(spdL - degreesCenterL) > deathBand) {
    attachServoLeft();
    servoLeft.write(spdL);
  } else {
    servoLeft.detach();
  }

  // if (spdR != degreesCenterR) {
  if (abs(spdR - degreesCenterR) > deathBand) {
    attachServoRight();
    servoRight.write(spdR);
  } else {
    servoRight.detach();
  }

  analogWrite(BUILTINLED, abs(Vty));

  // Debugging
  if (debug && (spdL !=degreesCenterL || spdR != degreesCenterR))
    Serial.printf("[spdR:%04d spdL:%04d]\r\n", spdR, spdL);
}

// main method to frames send
void processFrame() {
  if (!cam_stopped && Camera.get()) {
    uint8_t *out_jpg = NULL;
    size_t out_jpg_len = 0;
    frame2jpg(Camera.fb, 12, &out_jpg, &out_jpg_len);
    radio.sendData(out_jpg, out_jpg_len);
    free(out_jpg);
    Camera.free();
    // printFPS("CAM:");
  }
}

bool decodeMsg(uint16_t message_length) {
  pb_istream_t stream = pb_istream_from_buffer(recv_buff, message_length);
  bool status = pb_decode(&stream, JoystickMessage_fields, &jm);
  if (!status) {
    Serial.printf("Decoding msg failed: %s\r\n", PB_GET_ERROR(&stream));
    return false;
  }
  if (jm.ck == 0x01) {
    static uint_least32_t speedStamp = 0;
    if (millis() - speedStamp > 20) {
      speedStamp = millis();
      setSpeed(jm.ay - 100, jm.az - 100);
      running = true;
    }
  }
  return true;
}

// control commands data
void onDataReady(uint32_t lenght) {
  // Serial.printf("onDataReady: len %i\r\n",lenght);
  decodeMsg(lenght);
}

void wcli_reboot(String opts){
  ESP.restart();
}

void saveInteger(String key, String v) {
  int32_t value = v.toInt();
  cfg.saveInt(key, value);
  Serial.printf("saved: %s:%i\r\n",key.c_str(),value);
}

void saveFloat(String key, String v) {
  float value = v.toFloat();
  cfg.saveFloat(key, value);
  Serial.printf("saved: %s:%.5f\r\n",key.c_str(),value);
}

void saveBoolean(String key, String v) {
  v.toLowerCase();
  cfg.saveBool(key, v.equals("on") || v.equals("1") || v.equals("enable") || v.equals("true"));
  Serial.printf("saved: %s:%s\r\n", key.c_str(), cfg.getBool(key, false) ? "true" : "false");
}

void saveString(String key, String v) {
  cfg.saveString(key, v);
  Serial.printf("saved: %s:%s\r\n",key.c_str(),v.c_str());
}

bool isValidKey(String key) {
  for (int i = 0; i < KCOUNT; i++) {
    if (key.equals(cfg.getKey((CONFKEYS)i))) return true;
  }
  return false;
}

void wcli_kset(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String key = operands.first();
  String v = operands.second();
  if(isValidKey(key)){
    if(cfg.getKeyType(key) == ConfKeyType::BOOL) saveBoolean(key,v);
    else if(cfg.getKeyType(key) == ConfKeyType::FLOAT) saveFloat(key,v);
    else if(cfg.getKeyType(key) == ConfKeyType::INT) saveInteger(key,v);
    else if(cfg.getKeyType(key) == ConfKeyType::STRING) saveString(key,v);
    else Serial.println("Invalid key action for: " + key);
  }
  else {
    Serial.printf("invalid key: %s\r\nPlease see the valid keys with klist command.\r\n",key.c_str());
  }
}

void wcli_klist(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  String opt = operands.first();
  int key_count = KCOUNT;                       // Show all keys to configure 
  if (opt.equals("basic")) key_count = KBASIC; // Only show the basic keys to configure
  Serial.printf("\n%11s \t%s \t%s \r\n", "KEYNAME", "DEFINED", "VALUE");
  Serial.printf("\n%11s \t%s \t%s \r\n", "=======", "=======", "=====");

  for (int i = 0; i < key_count; i++) {
    if(i==KBASIC) continue;
    String key = cfg.getKey((CONFKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = cfg.getValue(key);
    Serial.printf("%11s \t%s \t%s \r\n", key, defined.c_str(), value.c_str());
  }
}

void wcli_setup(String opts) {
  setup_mode = true;
  Serial.println("\r\nSetup Mode Enable (fail-safe mode)\r\n");
}

void wcli_exit(String opts) {
  setup_time = 0;
  setup_mode = false;
}

void wcli_debug(String opts) {
  debug = !debug;
  cfg.saveBool(CONFKEYS::KDEBUG, debug);
}

void wcli_servoL(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  attachServoLeft();
  servoLeft.write(operands.first().toInt());
}

void wcli_servoR(String opts) {
  maschinendeck::Pair<String, String> operands = maschinendeck::SerialTerminal::ParseCommand(opts);
  attachServoRight();
  servoRight.write(operands.first().toInt());
}

void wcli_pauseCam(String opts){
  cam_stopped = !cam_stopped;
  Serial.printf("camera streaming %s\r\n", cam_stopped ? "stopped" : "resumed");
}

void loadVariables() {
  debug = cfg.getBool(CONFKEYS::KDEBUG, false);

  spanLeft = cfg.getInt(CONFKEYS::KLPAN, 18);
  offsetLeft = cfg.getInt(CONFKEYS::KLOFST, 0);
  degreesCenterL = cfg.getInt(CONFKEYS::KLCENT, 97);

  spanRight = cfg.getInt(CONFKEYS::KRPAN, 18);
  offsetRight = cfg.getInt(CONFKEYS::KROFST, 0);
  degreesCenterR = cfg.getInt(CONFKEYS::KRCENT, 100);

  deathBand = cfg.getInt(CONFKEYS::KDBAND, 2);

  degreesMinL = degreesCenterL - spanLeft + offsetLeft;
  degreesMaxL = degreesCenterL + spanLeft + offsetLeft;

  degreesMinR = degreesCenterR - spanRight + offsetRight;
  degreesMaxR = degreesCenterR + spanRight + offsetRight;  
}

void wcli_print(String opts) {
  loadVariables();
  Serial.printf("LEFT => span: %i offset: %i center: %i\r\n", spanLeft, offsetLeft, degreesCenterL);
  Serial.printf("LEFT => degreesMinL: %i degreesMaxL: %i\r\n\n", degreesMinL, degreesMaxL);

  Serial.printf("RIGHT => span: %i offset: %i center: %i\r\n", spanRight, offsetRight, degreesCenterR);
  Serial.printf("RIGHT => degreesMinR: %i degreesMaxR: %i\r\n\n", degreesMinR, degreesMaxR);
  
  Serial.printf("Others => deathBand: %i periodHertz: %i\r\n", deathBand,cfg.getInt(CONFKEYS::KPHERTZ, 50));
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  cfg.init("espnowcam");

  wcli.disableConnectInBoot();
  wcli.setSilentMode(true);  
  wcli.begin();              

  wcli.term->add("reboot", &wcli_reboot,   "\tperform a ESP32 reboot");
  wcli.term->add("setup", &wcli_setup,"\tTYPE THIS WORD to start to configure the device :D\n");
  wcli.term->add("exit", &wcli_exit, "\texit of the setup mode. AUTO EXIT in 10 seg! :)");
  wcli.term->add("klist", &wcli_klist, "\tlist valid preference keys");
  wcli.term->add("kset", &wcli_kset, "\tset preference key (e.g on/off or 1/0 or text)");
  wcli.term->add("print", &wcli_print, "\tprint current variables");
  wcli.term->add("servoL", &wcli_servoL, "\tset value on servo L");
  wcli.term->add("servoR", &wcli_servoR, "\tset value on servo R");
  wcli.term->add("pauseCam", &wcli_pauseCam, "\tstop/resume camera stream");
  wcli.term->add("debug", &wcli_debug, "\tdebugging flag toggle");

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoLeft.setPeriodHertz(cfg.getInt(CONFKEYS::KPHERTZ, 50));      // Standard 50hz servo
  servoRight.setPeriodHertz(cfg.getInt(CONFKEYS::KPHERTZ, 50));      // Standard 50hz servo

  attachServoLeft();
  attachServoRight();

  loadVariables();

  detachServos();

  // Configuration loop in setup:
  // 10 seconds for reconfiguration (first use case or fail-safe mode for example)
  uint32_t start = millis();
  while (setup_mode || (millis() - start < setup_time)) wcli.loop();
  Serial.println();

  loadVariables();

  if(psramFound()){
    size_t psram_size = esp_spiram_get_size() / 1048576;
    Serial.printf("PSRAM size: %dMb\r\n", psram_size);
  }
  
  if (Camera.begin()) Serial.println("Camera Init Success");
  else Serial.println("Camera Init Failed");
  
  delay(1000);

  // BE CAREFUL WITH IT, IF JPG LEVEL CHANGES, INCREASE IT
  recv_buff = (uint8_t*)  ps_malloc(100* sizeof( uint8_t ) ) ;
  radio.setRecvBuffer(recv_buff);
  radio.setRecvCallback(onDataReady);

  uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.init(244);
  
}

void loop() {
  processFrame();
  wcli.loop();
}
