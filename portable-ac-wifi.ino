#include <Adafruit_MCP9808.h>
#include <ESP8266WebServer.h>
#include <eeprom-wifi.h>

#include "src/ac-settings-encoder.h"
#include "src/index.html.h"
#include "src/settings-handler.h"
#include "src/target-cooling.h"

// Default MCP9808 breakout address, no address pins connected.
#define TEMP_SENSOR_I2C_ADDR 0x18
// The most precise resolution (0.0625 deg), but longest sample time (250ms).
#define TEMP_SENSOR_RESOLUTION_MODE 3
#define IR_OUTPUT_PIN 14
#define IR_OUTPUT_INACTIVE LOW
#define IR_FREQ 38000
#define SERVER_HTTP_PORT 80
#define SERIAL_BAUD 115200

// Averages 100 samples at 6 seconds apart = 10 min
constexpr unsigned long kTargetCoolingInterval = 6000;

Adafruit_MCP9808 temp_sensor;
ACSettingsEncoder ac(IR_OUTPUT_PIN, IR_OUTPUT_INACTIVE);
TargetCooling targetCooling(ac);
ESP8266WebServer server(SERVER_HTTP_PORT);

float sampleTempF() {
  // Resume temperature sampling.
  temp_sensor.wake();
  float f = temp_sensor.readTempF();
  // Stop temperature sampling to reduce power consumption.
  temp_sensor.shutdown();
  return f;
}

void handleSettingsCallback() {
  handleSettings(server, ac, sampleTempF());
}

void handleTargetCoolingCallback() {
  handleTargetCooling(server, targetCooling, sampleTempF());
}

void initWebServer() {
  server.on("/settings", handleSettingsCallback);
  server.on("/targetCooling", handleTargetCoolingCallback);
  server.on("/", []() {
    server.send(kHTTPOK, kIndexContentType, FPSTR(kIndexHtml));
  });
  server.begin();
}

void setup() {
  // The built-in LED is blinked to indicate WiFi configuration is needed.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Make sure the infrared LED is off for now, but configure frequency
  // that will be used for PWM later.
  pinMode(IR_OUTPUT_PIN, OUTPUT);
  digitalWrite(IR_OUTPUT_PIN, IR_OUTPUT_INACTIVE);
  analogWriteFreq(IR_FREQ);

  // Serial is used for initial WiFi configuration.
  Serial.begin(SERIAL_BAUD);
  delay(500);

  if (!temp_sensor.begin(TEMP_SENSOR_I2C_ADDR)) {
    // Ambient temperature is optional, log warning and carry on.
    Serial.println(F("Couldn't find MCP9808!"));
  }
  temp_sensor.setResolution(TEMP_SENSOR_RESOLUTION_MODE);

  // Get the A/C state in sync with internal state.
  ac.send();

  EEPROM.begin(kWiFiConfigSize);
  // Block until WiFi is connected.
  while (!connectToWiFi()) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    // Prompt for new configuration via serial.
    if (!configureWiFi()) {
      Serial.println(F("WiFi configuration failed!"));
      delay(1000);
    }
  }

  initWebServer();
}

void loop() {
  server.handleClient();

  static unsigned long last_ms = 0;
  unsigned long now_ms = millis();
  if (now_ms - last_ms >= kTargetCoolingInterval) {
    last_ms = now_ms;
    targetCooling.process(sampleTempF());
  }
}
