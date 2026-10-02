#include <Arduino.h>
#include <cmath>
#include "AudioProcessor.h"

namespace {
AudioProcessor gProcessor;

void printMenu() {
  Serial.println("\n=== ESP32-S3 Loudspeaker Management System ===");
  Serial.println("1. Tampilkan status");
  Serial.println("2. Set gain master");
  Serial.println("3. Set gain per channel");
  Serial.println("4. Atur equalizer 10 band");
  Serial.println("5. Atur crossover 3 way");
  Serial.println("6. Simpan konfigurasi");
  Serial.println("7. Muat konfigurasi");
  Serial.println("8. Reset ke default");
  Serial.println("9. Demo sinyal uji");
  Serial.println("0. Keluar");
  Serial.print("Pilih menu: ");
}

void setMasterGain() {
  Serial.print("Masukkan gain master (dB): ");
  while (!Serial.available()) delay(10);
  float v = Serial.parseFloat();
  gProcessor.setMasterGain(v);
  Serial.printf("Gain master: %.2f dB\n", v);
}

void setChannelGain() {
  Serial.print("Gain low (dB): ");
  while (!Serial.available()) delay(10);
  float low = Serial.parseFloat();

  Serial.print("Gain mid (dB): ");
  while (!Serial.available()) delay(10);
  float mid = Serial.parseFloat();

  Serial.print("Gain high (dB): ");
  while (!Serial.available()) delay(10);
  float high = Serial.parseFloat();

  gProcessor.setChannelGain(low, mid, high);
  Serial.printf("Low=%.2f Mid=%.2f High=%.2f dB\n", low, mid, high);
}

void setEqBand() {
  Serial.println("Pilih band 1..10:");
  for (size_t i = 0; i < AudioProcessor::kEqBandCount; ++i) {
    Serial.printf("%d. %.0f Hz\n", static_cast<int>(i + 1), gProcessor.getBandFrequency(i));
  }

  while (!Serial.available()) delay(10);
  int band = Serial.parseInt();
  if (band < 1 || band > 10) {
    Serial.println("Band tidak valid");
    return;
  }

  Serial.print("Masukkan gain band (dB): ");
  while (!Serial.available()) delay(10);
  float gain = Serial.parseFloat();

  gProcessor.setEqBand(static_cast<size_t>(band - 1), gain);
  Serial.printf("Band %d diatur ke %.2f dB\n", band, gain);
}

void setCrossover() {
  Serial.print("Frekuensi crossover low-mid (Hz): ");
  while (!Serial.available()) delay(10);
  float lowMid = Serial.parseFloat();

  Serial.print("Frekuensi crossover mid-high (Hz): ");
  while (!Serial.available()) delay(10);
  float midHigh = Serial.parseFloat();

  gProcessor.setCrossover(lowMid, midHigh);
  Serial.printf("Low-Mid=%.0f Hz, Mid-High=%.0f Hz\n", lowMid, midHigh);
}

void saveConfig() {
  if (gProcessor.saveSettings()) {
    Serial.println("Konfigurasi berhasil disimpan");
  } else {
    Serial.println("Gagal menyimpan konfigurasi");
  }
}

void loadConfig() {
  if (gProcessor.loadSettings()) {
    Serial.println("Konfigurasi berhasil dimuat");
  } else {
    Serial.println("Belum ada konfigurasi tersimpan");
  }
}

void resetDefaults() {
  gProcessor.resetDefaults();
  Serial.println("Pengaturan direset ke default");
}

void demoSignal() {
  Serial.println("Demo sinyal uji mulai...");
  for (int i = 0; i < 240; ++i) {
    const float t = static_cast<float>(i) / 240.0f;
    const float sample =
        0.25f * sinf(2.0f * PI * 40.0f * t) +
        0.20f * sinf(2.0f * PI * 250.0f * t) +
        0.15f * sinf(2.0f * PI * 3500.0f * t);

    const float out = gProcessor.processSample(sample);
    Serial.printf("in=%.4f out=%.4f\n", sample, out);
    delay(15);
  }
}
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("ESP32-S3 Digital Loudspeaker DSP");

  gProcessor.begin();
  printMenu();
}

void loop() {
  if (Serial.available() > 0) {
    int choice = Serial.parseInt();

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
        saveConfig();
        break;
      case 7:
        loadConfig();
        break;
      case 8:
        resetDefaults();
        break;
      case 9:
        demoSignal();
        break;
      case 0:
        Serial.println("Keluar dari menu. Tekan reset untuk mulai lagi.");
        while (true) {
          delay(1000);
        }
      default:
        Serial.println("Pilihan tidak valid.");
        break;
    }

    printMenu();
  }

  delay(10);
}
