#ifndef __TESTING_EEPROM_H
#define __TESTING_EEPROM_H

#include <stdint.h>
#include <string.h>

#include "gmock/gmock.h"

class FakeEEPROM {
 public:
  std::vector<uint8_t> fake_data;

  void begin(size_t size) {
    fake_data.resize(size);
  }
  void reset() {
    fake_data.clear();
  }
  uint8_t& operator [](int address) {
    return fake_data.at(address);
  }
  void put(int address, const char data[]) {
    memcpy(&(*this)[address], data, sizeof(data));
  }

  MOCK_METHOD0(commit, bool());
};

typedef FakeEEPROM EEPROMClass;
extern EEPROMClass EEPROM;

#endif
