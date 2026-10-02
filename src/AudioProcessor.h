#include <Arduino.h>
#include <Preferences.h>
#include <cmath>
#include "AudioProcessor.h"

namespace {

AudioProcessor gProcessor;

void printMenu() {
  Serial.println("\n=== ESP32-S3 Loudspeaker Management ===");
  Serial.println("1. Tampilkan status audio");
  Serial.println("2. Set gain master");
  Serial.println("3. Set gain low/mid/high");
  Serial.println("4. Edit equalizer 10 band");
  Serial.println("5. Edit crossover 3 way");
  Serial.println("6. Simpan konfigurasi ke flash");
  Serial.println("7. Muat konfigurasi dari flash");
  Serial.println("8. Reset ke default");
  Serial.println("9. Jalankan demo sinyal uji");
  Serial.println("0. Keluar");
  Serial.print("Pilih menu: ");
}

void setMasterGain() {
  Serial.print("Masukkan gain master (dB, contoh: 0.0): ");
  while (!Serial.available()) {
    delay(10);
  }
  float value = Serial.parseFloat();
  gProcessor.setMasterGain(value);
  Serial.printf("Gain master diatur ke %.2f dB\n", value);
}

void setChannelGain() {
  Serial.print("Masukkan gain low (dB): ");
  while (!Serial.available()) {
    delay(10);
  }
  float low = Serial.parseFloat();

  Serial.print("Masukkan gain mid (dB): ");
  while (!Serial.available()) {
    delay(10);
  }
  float mid = Serial.parseFloat();

  Serial.print("Masukkan gain high (dB): ");
  while (!Serial.available()) {
    delay(10);
  }
  float high = Serial.parseFloat();

  gProcessor.setChannelGain(low, mid, high);
  Serial.printf("Gain low=%.2f, mid=%.2f, high=%.2f dB\n", low, mid, high);
}

void editEqBand() {
  Serial.println("Pilih band [1..10]:");
  for (int i = 0; i < 10; ++i) {
    Serial.printf("%d. %.0f Hz\n", i + 1, gProcessor.getBandFrequency(i));
  }

  while (!Serial.available()) {
    delay(10);
  }
  int index = Serial.parseInt() - 1;

  if (index < 0 || index >= 10) {
    Serial.println("Indeks band tidak valid.");
    return;
  }

  Serial.print("Masukkan gain band (dB, contoh: +3.5): ");
  while (!Serial.available()) {
    delay(10);
  }
  float gain = Serial.parseFloat();
  gProcessor.setEqBand(index, gain);
  Serial.printf("Band %d diatur ke %.2f dB\n", index + 1, gain);
}

void editCrossover() {
  Serial.print("Masukkan frekuensi crossover low-mid (Hz): ");
  while (!Serial.available()) {
    delay(10);
  }
  float lowMid = Serial.parseFloat();

  Serial.print("Masukkan frekuensi crossover mid-high (Hz): ");
  while (!Serial.available()) {
    delay(10);
  }
  float midHigh = Serial.parseFloat();

  gProcessor.setCrossover(lowMid, midHigh);
  Serial.printf("Crossover diatur: low-mid=%.0f Hz, mid-high=%.0f Hz\n", lowMid, midHigh);
}

void printStatus() {
  gProcessor.printStatus();
}

void saveConfig() {
  if (gProcessor.saveSettings()) {
    Serial.println("Konfigurasi berhasil disimpan ke flash.");
  } else {
    Serial.println("Gagal menyimpan konfigurasi.");
  }
}

void loadConfig() {
  if (gProcessor.loadSettings()) {
    Serial.println("Konfigurasi berhasil dimuat dari flash.");
  } else {
    Serial.println("Tidak ada konfigurasi tersimpan atau gagal dibaca.");
  }
}

void resetDefaults() {
  gProcessor.resetDefaults();
  Serial.println("Konfigurasi direset ke default.");
}

void demoSignal() {
  Serial.println("Demo sinyal uji berjalan...");
  for (int i = 0; i < 200; ++i) {
    float t = static_cast<float>(i) / 200.0f;
    float tone = sin(2.0f * PI * 40.0f * t) + 0.5f * sin(2.0f * PI * 200.0f * t) + 0.25f * sin(2.0f * PI * 4000.0f * t);
    float sample = tone * 0.25f;
    float processed = gProcessor.processSample(sample);
    Serial.printf("sample=%0.4f processed=%0.4f\n", sample, processed);
    delay(20);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("ESP32-S3 Loudspeaker Management System");

  gProcessor.begin();
  printMenu();
}

void loop() {
  if (Serial.available() > 0) {
    int choice = Serial.parseInt();

    switch (choice) {
      case 1:
        printStatus();
        break;
      case 2:
        setMasterGain();
        break;
      case 3:
        setChannelGain();
        break;
      case 4:
        editEqBand();
        break;
      case 5:
        editCrossover();
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
        Serial.println("Menu ditutup. Tekan tombol reset untuk memulai ulang.");
        while (true) {
          delay(1000);
        }
      default:
        Serial.println("Pilihan tidak valid.");
        break;
    }

    printMenu();
  }

  delay(50);
}
