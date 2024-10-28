/**************************************************
 * ESP32Cam Freenove Tank Transmitter and Receiver
 * by @hpsaturn Copyright (C) 2024
 * This file is part ESPNowCam project:
 * https://github.com/hpsaturn/ESPNowCam
**************************************************/

#include <Arduino.h>
#include <ESP32WifiCLI.hpp>
#include <ESPNowCam.h>
#include <common/comm.pb.h>
#include <EasyPreferences.hpp> 
#include <drivers/CamFreenove.h>
#include <ESP32Servo.h>
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

void wcli_reboot(char *args, Stream *response){
  ESP.restart();
}

void wcli_kset(char *args, Stream *response) {
  Pair<String, String> operands = wcli.parseCommand(args);
  String key = operands.first();
  String v = operands.second();
  cfg.saveAuto(key,v);
}

void wcli_klist(char *args, Stream *response) {
  Pair<String, String> operands = wcli.parseCommand(args);
  String opt = operands.first();
  Serial.printf("\n%11s \t%s \t%s \r\n", "KEYNAME", "DEFINED", "VALUE");
  Serial.printf("\n%11s \t%s \t%s \r\n", "=======", "=======", "=====");

  for (int i = 0; i < KCOUNT; i++) {
    String key = cfg.getKey((PKEYS)i);
    bool isDefined = cfg.isKey(key);
    String defined = isDefined ? "custom " : "default";
    String value = "";
    if (isDefined) value = cfg.getValue(key);
    Serial.printf("%11s \t%s \t%s \r\n", key.c_str(), defined.c_str(), value.c_str());
  }
}

void wcli_setup(char *args, Stream *response) {
  setup_mode = true;
  Serial.println("\r\nSetup Mode Enable (fail-safe mode)\r\n");
}

void wcli_exit(char *args, Stream *response) {
  setup_time = 0;
  setup_mode = false;
}

void wcli_debug(char *args, Stream *response) {
  debug = !debug;
  cfg.saveBool(PKEYS::KDEBUG, debug);
}

void wcli_servoL(char *args, Stream *response) {
  Pair<String, String> operands = wcli.parseCommand(args);
  attachServoLeft();
  servoLeft.write(operands.first().toInt());
}

void wcli_servoR(char *args, Stream *response) {
  Pair<String, String> operands = wcli.parseCommand(args);
  attachServoRight();
  servoRight.write(operands.first().toInt());
}

void wcli_pauseCam(char *args, Stream *response){
  cam_stopped = !cam_stopped;
  Serial.printf("camera streaming %s\r\n", cam_stopped ? "stopped" : "resumed");
}

void loadVariables() {
  debug = cfg.getBool(PKEYS::KDEBUG, false);

  spanLeft = cfg.getInt(PKEYS::KLPAN, 18);
  offsetLeft = cfg.getInt(PKEYS::KLOFST, 0);
  degreesCenterL = cfg.getInt(PKEYS::KLCENT, 97);

  spanRight = cfg.getInt(PKEYS::KRPAN, 18);
  offsetRight = cfg.getInt(PKEYS::KROFST, 0);
  degreesCenterR = cfg.getInt(PKEYS::KRCENT, 100);

  deathBand = cfg.getInt(PKEYS::KDBAND, 2);

  degreesMinL = degreesCenterL - spanLeft + offsetLeft;
  degreesMaxL = degreesCenterL + spanLeft + offsetLeft;

  degreesMinR = degreesCenterR - spanRight + offsetRight;
  degreesMaxR = degreesCenterR + spanRight + offsetRight;  
}

void wcli_print(char *args, Stream *response) {
  loadVariables();
  Serial.printf("LEFT => span: %i offset: %i center: %i\r\n", spanLeft, offsetLeft, degreesCenterL);
  Serial.printf("LEFT => degreesMinL: %i degreesMaxL: %i\r\n\n", degreesMinL, degreesMaxL);

  Serial.printf("RIGHT => span: %i offset: %i center: %i\r\n", spanRight, offsetRight, degreesCenterR);
  Serial.printf("RIGHT => degreesMinR: %i degreesMaxR: %i\r\n\n", degreesMinR, degreesMaxR);
  
  Serial.printf("Others => deathBand: %i periodHertz: %i\r\n", deathBand,cfg.getInt(PKEYS::KPHERTZ, 50));
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  delay(1000);

  cfg.init("espnowcam");

  wcli.setSilentMode(true);  

  wcli.add("reboot", &wcli_reboot,   "\tperform a ESP32 reboot");
  wcli.add("setup", &wcli_setup,"\tTYPE THIS WORD to start to configure the device :D\n");
  wcli.add("exit", &wcli_exit, "\texit of the setup mode. AUTO EXIT in 10 seg! :)");
  wcli.add("klist", &wcli_klist, "\tlist valid preference keys");
  wcli.add("kset", &wcli_kset, "\tset preference key (e.g on/off or 1/0 or text)");
  wcli.add("print", &wcli_print, "\tprint current variables");
  wcli.add("servoL", &wcli_servoL, "\tset value on servo L");
  wcli.add("servoR", &wcli_servoR, "\tset value on servo R");
  wcli.add("pauseCam", &wcli_pauseCam, "\tstop/resume camera stream");
  wcli.add("debug", &wcli_debug, "\tdebugging flag toggle");
  
  wcli.begin();              

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  servoLeft.setPeriodHertz(cfg.getInt(PKEYS::KPHERTZ, 50));      // Standard 50hz servo
  servoRight.setPeriodHertz(cfg.getInt(PKEYS::KPHERTZ, 50));      // Standard 50hz servo

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
  recv_buff = static_cast<uint8_t*>(ps_malloc(100 * sizeof(uint8_t)));
  radio.setRecvBuffer(recv_buff);
  radio.setRecvCallback(onDataReady);

  const uint8_t macRecv[6] = {0xB8,0xF0,0x09,0xC6,0x0E,0xCC};
  radio.setTarget(macRecv);
  radio.init(244);
  
}

void loop() {
  processFrame();
  wcli.loop();
}
