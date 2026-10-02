#include <Arduino.h>
#include <cmath>
#include "AudioProcessor.h"
#include "I2SAudioInterface.h"

namespace {

AudioProcessor gProcessor;
I2SAudioInterface gI2sInterface;
bool gI2sActive = false;
bool gDemoMode = false;

void printMenu() {
  Serial.println("\n=== ESP32-S3 Digital Loudspeaker DSP ===");
  Serial.println("1. Tampilkan status");
  Serial.println("2. Set gain master");
  Serial.println("3. Set gain per channel (low/mid/high)");
  Serial.println("4. Atur equalizer 10 band");
  Serial.println("5. Atur crossover 3-way");
  Serial.println("6. Atur limiter");
  Serial.println("7. Simpan konfigurasi ke flash");
  Serial.println("8. Muat konfigurasi dari flash");
  Serial.println("9. Reset ke default");
  Serial.println("10. Mode demo (simulasi sinyal)");
  Serial.println("11. Aktifkan I2S real-time DSP");
  Serial.println("0. Keluar");
  Serial.print("Pilih menu: ");
}

void setMasterGain() {
  Serial.print("Masukkan gain master (dB, -60 sampai +12): ");
  while (!Serial.available()) {
    delay(10);
  }
  float v = Serial.parseFloat();
  gProcessor.setMasterGain(v);
  Serial.printf(">> Gain master: %.2f dB\n", v);
}

void setChannelGain() {
  Serial.print("Gain low (dB, -12 sampai +12): ");
  while (!Serial.available()) {
    delay(10);
  }
  float low = Serial.parseFloat();

  Serial.print("Gain mid (dB, -12 sampai +12): ");
  while (!Serial.available()) {
    delay(10);
  }
  float mid = Serial.parseFloat();

  Serial.print("Gain high (dB, -12 sampai +12): ");
  while (!Serial.available()) {
    delay(10);
  }
  float high = Serial.parseFloat();

  gProcessor.setChannelGain(low, mid, high);
  Serial.printf(">> Low=%.2f dB, Mid=%.2f dB, High=%.2f dB\n", low, mid, high);
}

void setEqBand() {
  Serial.println("Pilih band equalizer (1-10):");
  for (size_t i = 0; i < AudioProcessor::kEqBandCount; ++i) {
    Serial.printf("%2zu. %.0f Hz\n", i + 1, gProcessor.getBandFrequency(i));
  }

  while (!Serial.available()) {
    delay(10);
  }
  int band = Serial.parseInt();
  if (band < 1 || band > 10) {
    Serial.println("Band tidak valid");
    return;
  }

  Serial.print("Masukkan gain band (dB, -12 sampai +12): ");
  while (!Serial.available()) {
    delay(10);
  }
  float gain = Serial.parseFloat();

  gProcessor.setEqBand(static_cast<size_t>(band - 1), gain);
  Serial.printf(">> Band %d diatur ke %.2f dB\n", band, gain);
}

void setCrossover() {
  Serial.print("Frekuensi crossover low-mid (Hz, 50-5000): ");
  while (!Serial.available()) {
    delay(10);
  }
  float lowMid = Serial.parseFloat();

  Serial.print("Frekuensi crossover mid-high (Hz, 500-20000): ");
  while (!Serial.available()) {
    delay(10);
  }
  float midHigh = Serial.parseFloat();

  gProcessor.setCrossover(lowMid, midHigh);
  Serial.printf(">> Low-Mid=%.0f Hz, Mid-High=%.0f Hz\n", lowMid, midHigh);
}

void setLimiter() {
  Serial.print("Aktifkan limiter? (1=yes, 0=no): ");
  while (!Serial.available()) {
    delay(10);
  }
  int enable = Serial.parseInt();

  Serial.print("Limiter threshold (dB, -20 sampai 0): ");
  while (!Serial.available()) {
    delay(10);
  }
  float threshold = Serial.parseFloat();

  gProcessor.setLimiter(enable != 0, threshold);
  Serial.printf(">> Limiter %s, Threshold: %.1f dB\n",
                enable ? "ON" : "OFF", threshold);
}

void saveConfig() {
  if (gProcessor.saveSettings()) {
    Serial.println(">> Konfigurasi berhasil disimpan ke flash");
  } else {
    Serial.println("[ERROR] Gagal menyimpan konfigurasi");
  }
}

void loadConfig() {
  if (gProcessor.loadSettings()) {
    Serial.println(">> Konfigurasi berhasil dimuat dari flash");
  } else {
    Serial.println("[WARNING] Belum ada konfigurasi tersimpan");
  }
}

void resetDefaults() {
  gProcessor.resetDefaults();
  Serial.println(">> Semua pengaturan direset ke default");
}

void demoMode() {
  gDemoMode = true;
  Serial.println("\n=== DEMO MODE: Simulasi Sinyal ===");
  Serial.println("Menjalankan loop sinyal uji selama 10 detik...");

  const int durationMs = 10000;
  const int updateMs = 50;
  int elapsed = 0;

  while (gDemoMode && elapsed < durationMs) {
    const float t = static_cast<float>(elapsed) / 1000.0f;

    // Multi-frequency test signal
    const float sig1 = 0.25f * sinf(2.0f * M_PI * 50.0f * t);    // 50 Hz
    const float sig2 = 0.15f * sinf(2.0f * M_PI * 250.0f * t);   // 250 Hz
    const float sig3 = 0.10f * sinf(2.0f * M_PI * 2000.0f * t);  // 2k Hz
    const float sig4 = 0.08f * sinf(2.0f * M_PI * 8000.0f * t);  // 8k Hz

    const float inputSample = sig1 + sig2 + sig3 + sig4;
    const float outputSample = gProcessor.processSample(inputSample);

    Serial.printf("[%5d ms] in=%.4f out=%.4f\n", elapsed, inputSample, outputSample);

    elapsed += updateMs;
    delay(updateMs);

    if (Serial.available() > 0) {
      if (Serial.read() == 'q') {
        gDemoMode = false;
      }
    }
  }

  gDemoMode = false;
  Serial.println(">> Demo mode selesai. Ketik angka untuk menu selanjutnya.");
}

void i2sRealTimeMode() {
  Serial.println("\n=== I2S Real-Time DSP Mode ===");
  Serial.println("Inisialisasi I2S...");

  if (!gI2sInterface.init()) {
    Serial.println("[ERROR] Gagal inisialisasi I2S interface");
    return;
  }

  gI2sActive = true;
  Serial.println("I2S aktif. Tekan 'q' untuk keluar mode real-time.");
  Serial.println("Tunggu audio dari I2S... (timeout: 30 detik)");

  uint32_t startTime = millis();
  const uint32_t timeoutMs = 30000;

  int32_t i2sBuffer[I2SAudioInterface::kBufferSize];
  int frameCount = 0;

  while (gI2sActive && (millis() - startTime) < timeoutMs) {
    if (gI2sInterface.read(i2sBuffer, I2SAudioInterface::kBufferSize / 2)) {
      for (size_t i = 0; i < I2SAudioInterface::kBufferSize / 2; ++i) {
        // Convert from 32-bit integer to float [-1, 1]
        float inputSample = static_cast<float>(i2sBuffer[i]) / 2147483648.0f;

        // Process through DSP
        float outputSample = gProcessor.processSample(inputSample);

        // Convert back to 32-bit integer
        int32_t outputInt = static_cast<int32_t>(outputSample * 2147483647.0f);
        i2sBuffer[i] = outputInt;
      }

      if (!gI2sInterface.write(i2sBuffer, I2SAudioInterface::kBufferSize / 2)) {
        Serial.println("[WARNING] I2S write failed");
      }

      frameCount++;
      if ((frameCount % 100) == 0) {
        Serial.printf("[I2S] Frames processed: %d\n", frameCount);
      }
    } else {
      Serial.println("[WARNING] I2S read timeout");
      delay(100);
    }

    if (Serial.available() > 0) {
      int c = Serial.read();
      if (c == 'q' || c == 'Q') {
        gI2sActive = false;
      }
    }
  }

  gI2sInterface.deinit();
  Serial.println("\n>> I2S mode dihentikan");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32-S3 Digital Loudspeaker Management System ===");
  Serial.println("Firmware v1.0 - Complete DSP Implementation");
  Serial.println("Features: 10-band EQ, 3-way Crossover, Gain Control, Limiter");

  gProcessor.begin();

  Serial.println("System ready. Ketik angka untuk pilihan menu.");
  printMenu();
}

void loop() {
  if (Serial.available() > 0) {
    int choice = Serial.parseInt();
    // Clear any remaining characters in the buffer
    while (Serial.available() > 0) {
      Serial.read();
    }

    switch (choice) {
      case 1:
        gProcessor.printStatus();
        break;
      case 2:
        setMasterGain();
        break;
      case 3:
        setChannelGain();
        break;
      case 4:
        setEqBand();
        break;
      case 5:
        setCrossover();
        break;
      case 6:
        setLimiter();
        break;
      case 7:
        saveConfig();
        break;
      case 8:
        loadConfig();
        break;
      case 9:
        resetDefaults();
        break;
      case 10:
        demoMode();
        break;
      case 11:
        i2sRealTimeMode();
        break;
      case 0:
        Serial.println(">> Program dihentikan. Tekan reset untuk mulai lagi.");
        while (true) {
          delay(1000);
        }
        break;
      default:
        Serial.println("[ERROR] Pilihan tidak valid.");
        break;
    }

    printMenu();
  }

  delay(100);
}
