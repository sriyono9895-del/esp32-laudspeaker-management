#pragma once

#include <Arduino.h>
#include <driver/i2s.h>
#include <soc/i2s_reg.h>

class I2SAudioInterface {
public:
  static constexpr size_t kBufferSize = 512;
  static constexpr int kSampleRate = 48000;
  static constexpr int kBitsPerSample = 32;
  static constexpr i2s_port_t kI2sPort = I2S_NUM_0;

  // Pin configuration
  static constexpr int kPinBCLK = 41;    // GPIO41: Bit Clock
  static constexpr int kPinWS = 42;      // GPIO42: Word Select (LRCK)
  static constexpr int kPinDIN = 2;      // GPIO2: Data In (RX)
  static constexpr int kPinDOUT = 1;     // GPIO1: Data Out (TX)
  static constexpr int kPinMCLK = -1;    // Not used in ESP32-S3

  I2SAudioInterface();
  bool init();
  bool deinit();
  bool write(const int32_t *buffer, size_t frames);
  bool read(int32_t *buffer, size_t frames);

private:
  bool initialized_ = false;
};
