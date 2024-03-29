#include "settings-handler.h"

#include <string>
#include <ESP8266WebServer.h>

#include "ac-settings-encoder.h"
#include "target-cooling.h"

namespace {

constexpr char kJsonContentType[] = "application/json";
constexpr char kCoolString[] = "cool";
constexpr char kDehumString[] = "dehum";
constexpr char kFanOnlyString[] = "fan_only";
constexpr char kLowString[] = "low";
constexpr char kMediumString[] = "medium";
constexpr char kHighString[] = "high";
constexpr char kTrueString[] = "true";
constexpr char kFalseString[] = "false";

const char* modeToString(ACSettingsEncoder::Mode mode) {
  switch (mode) {
    case ACSettingsEncoder::kCool:
      return kCoolString;
    case ACSettingsEncoder::kDehum:
      return kDehumString;
    case ACSettingsEncoder::kFanOnly:
      return kFanOnlyString;
  }
  return "";
}

ACSettingsEncoder::Mode stringToMode(const char* mode) {
  if (!strcmp(mode, kDehumString)) {
    return ACSettingsEncoder::kDehum;
  }
  if (!strcmp(mode, kFanOnlyString)) {
    return ACSettingsEncoder::kFanOnly;
  }
  return ACSettingsEncoder::kCool;
}

const char* fanSpeedToString(ACSettingsEncoder::FanSpeed fan) {
  switch (fan) {
    case ACSettingsEncoder::kLow:
      return kLowString;
    case ACSettingsEncoder::kMedium:
      return kMediumString;
    case ACSettingsEncoder::kHigh:
      return kHighString;
  }
  return "";
}

ACSettingsEncoder::FanSpeed stringToFanSpeed(const char* fan) {
  if (!strcmp(fan, kLowString)) {
    return ACSettingsEncoder::kLow;
  }
  if (!strcmp(fan, kMediumString)) {
    return ACSettingsEncoder::kMedium;
  }
  return ACSettingsEncoder::kHigh;
}

void handleSettingsUpdate(ESP8266WebServer& server, ACSettingsEncoder& ac) {
  ac.setFanSpeed(stringToFanSpeed(server.arg("fan").c_str()));
  ac.setMode(stringToMode(server.arg("mode").c_str()));
  if (server.arg("timer").equals(kTrueString)) {
    ac.timerOn();
  } else {
    ac.timerOff();
  }
  if (server.arg("power").equals(kTrueString)) {
    ac.powerOn();
  } else {
    ac.powerOff();
  }
  int thermostatInF = server.arg("thermostatInF").toInt();
  thermostatInF = min(thermostatInF, kThermostatMaxF);
  thermostatInF = max(thermostatInF, kThermostatMinF);
  ac.setThermostatInF(thermostatInF);
  ac.send();
}

void handleTargetCoolingUpdate(ESP8266WebServer& server, TargetCooling& tc) {
  float thermostatInF = server.arg("thermostatInF").toFloat();
  tc.setThermostatInF(thermostatInF);
  bool isEnabled = server.arg("enabled").equals(kTrueString);
  tc.setEnabled(isEnabled);
}

} // namespace

void handleSettings(
  ESP8266WebServer& server, ACSettingsEncoder& ac, float ambient_temp_f) {
  if (server.method() == HTTP_POST) {
    handleSettingsUpdate(server, ac);
  }
  char json[128];
  snprintf(json, sizeof(json),
            "{"
            R"("ambient":"%f",)"
            R"("fan":"%s",)"
            R"("mode":"%s",)"
            R"("timer":%s,)"
            R"("power":%s,)"
            R"("thermostatInF":%d)"
            "}",
            ambient_temp_f,
            fanSpeedToString(ac.getFanSpeed()),
            modeToString(ac.getMode()),
            ac.isTimerOn() ? kTrueString : kFalseString,
            ac.isPowerOn() ? kTrueString : kFalseString,
            ac.getThermostatInF());
  server.send(kHTTPOK, kJsonContentType, json);
}

void handleTargetCooling(
  ESP8266WebServer& server, TargetCooling& tc, float ambient_temp_f) {
  if (server.method() == HTTP_POST) {
    handleTargetCoolingUpdate(server, tc);
  }
  char json[128];
  snprintf(json, sizeof(json),
            "{"
            R"("ambient":"%f",)"
            R"("enabled":%s,)"
            R"("thermostatInF":"%f")"
            "}",
            ambient_temp_f,
            tc.isEnabled() ? kTrueString : kFalseString,
            tc.getThermostatInF());
  server.send(kHTTPOK, kJsonContentType, json);
}
