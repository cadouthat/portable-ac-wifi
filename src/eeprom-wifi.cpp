#include "eeprom-wifi.h"

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

namespace {

constexpr int kSSIDMax = 32;
constexpr int kPSKMax = 64;
constexpr int kConfigSize = (kSSIDMax + kPSKMax);

constexpr unsigned long kConnectTimeoutMs = 30000;

bool connectToWifi(
    EEPROMClass& EEPROM, ESP8266WiFiClass& WiFi, HardwareSerial& Serial) {
  char ssid[kSSIDMax + 1] = {0};
  char psk[kPSKMax + 1] = {0};
  memcpy(ssid, &EEPROM[0], kSSIDMax);
  memcpy(psk, &EEPROM[kSSIDMax], kPSKMax);
  if (!ssid[0]) {
    Serial.println(F("No WiFi configuration found."));
    return false;
  }

  Serial.print(F("Connecting to SSID: "));
  Serial.println(ssid);
  WiFi.begin(ssid, psk);
  unsigned long connect_start_ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - connect_start_ms > kConnectTimeoutMs) {
      Serial.println(F("Connection failed!"));
      return false;
    }
    delay(500);
  }

  Serial.println(F("WiFi connected."));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Netmask: "));
  Serial.println(WiFi.subnetMask());
  Serial.print(F("Gateway: "));
  Serial.println(WiFi.gatewayIP());
  return true;
}

bool configureWifi(EEPROMClass& EEPROM, HardwareSerial& Serial) {
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);

  char ssid[kSSIDMax] = {0};
  char psk[kPSKMax] = {0};

  Serial.print(F("Enter WiFi SSID: "));
  Serial.setTimeout(10000);
  String ssid_s = Serial.readString();
  Serial.print(ssid_s);
  if (ssid_s.length() <= 1) {
    return false;
  }
  if (ssid_s.length() - 1 > kSSIDMax) {
    Serial.println(F("SSID is too long!"));
    return false;
  }
  // Truncate trailing newline character.
  memcpy(ssid, ssid_s.c_str(), ssid_s.length() - 1);
  
  Serial.print(F("Enter WiFi PSK: "));
  Serial.setTimeout(30000);
  String psk_s = Serial.readString();
  Serial.print(psk_s);
  if (psk_s.length() <= 1) {
    return false;
  }
  if (psk_s.length() - 1 > kPSKMax) {
    Serial.println(F("PSK is too long!"));
    return false;
  }
  // Truncate trailing newline character.
  memcpy(psk, psk_s.c_str(), psk_s.length() - 1);

  EEPROM.put(0, ssid);
  EEPROM.put(kSSIDMax, psk);
  return EEPROM.commit();
}

} // namespace

void initEEPROMWiFi(
    EEPROMClass& EEPROM, ESP8266WiFiClass& WiFi, HardwareSerial& Serial) {
  EEPROM.begin(kConfigSize);
  while (!connectToWifi(EEPROM, WiFi, Serial)) {
    if (!configureWifi(EEPROM, Serial)) {
      Serial.println(F("WiFi configuration failed!"));
      delay(1000);
    }
  }
}
