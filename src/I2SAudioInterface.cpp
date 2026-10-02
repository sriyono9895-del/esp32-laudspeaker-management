#include "I2SAudioInterface.h"

I2SAudioInterface::I2SAudioInterface() = default;

bool I2SAudioInterface::init() {
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
      .sample_rate = kSampleRate,
      .bits_per_sample = (i2s_bits_per_sample_t)kBitsPerSample,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format =
          (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = kBufferSize,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  i2s_pin_config_t pin_config = {
      .bck_io_num = kPinBCLK,
      .ws_io_num = kPinWS,
      .data_out_num = kPinDOUT,
      .data_in_num = kPinDIN,
      .mck_io_num = kPinMCLK};

  esp_err_t err = i2s_driver_install(kI2sPort, &i2s_config, 0, nullptr);
  if (err != ESP_OK) {
    Serial.printf("[I2S] Install driver failed: %d\n", err);
    return false;
  }

  err = i2s_set_pin(kI2sPort, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("[I2S] Set pins failed: %d\n", err);
    i2s_driver_uninstall(kI2sPort);
    return false;
  }

  initialized_ = true;
  Serial.println("[I2S] Audio interface initialized");
  return true;
}

bool I2SAudioInterface::deinit() {
  if (!initialized_) {
    return false;
  }
  esp_err_t err = i2s_driver_uninstall(kI2sPort);
  if (err == ESP_OK) {
    initialized_ = false;
    Serial.println("[I2S] Audio interface deinitialized");
    return true;
  }
  return false;
}

bool I2SAudioInterface::write(const int32_t *buffer, size_t frames) {
  if (!initialized_) {
    return false;
  }
  size_t bytes_written = 0;
  esp_err_t err =
      i2s_write(kI2sPort, buffer, frames * sizeof(int32_t), &bytes_written, 10);
  return err == ESP_OK && bytes_written == frames * sizeof(int32_t);
}

bool I2SAudioInterface::read(int32_t *buffer, size_t frames) {
  if (!initialized_) {
    return false;
  }
  size_t bytes_read = 0;
  esp_err_t err =
      i2s_read(kI2sPort, buffer, frames * sizeof(int32_t), &bytes_read, 10);
  return err == ESP_OK && bytes_read == frames * sizeof(int32_t);
}
