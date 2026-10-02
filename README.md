# ESP32-S3 Digital Loudspeaker Management System

Project ini berisi implementasi firmware dasar untuk sistem manajemen loudspeaker digital berbasis ESP32-S3 dengan fitur:

- Equalizer 10 band
- Crossover 3 way
- Pengatur gain per channel
- Menu interaktif via Serial Monitor
- Penyimpanan pengaturan ke NVS/Flash

## Fitur utama

1. Equalizer 10 band
   - Frekuensi: 31, 63, 125, 250, 500, 1k, 2k, 4k, 8k, 16k Hz
   - Gain tiap band dapat diatur dari -12 dB sampai +12 dB

2. Crossover 3 way
   - Low / Mid / High
   - Frekuensi crossover dapat diatur untuk membagi sinyal ke driver low, mid, dan tweeter

3. Gain control
   - Gain master
   - Gain channel low, mid, high
   - Pengaturan dB untuk optimasi output audio

4. Menu serial
   - Buka Serial Monitor pada baud 115200
   - Ketik angka sesuai menu

## Struktur project

- `src/main.cpp` : firmware utama dan menu serial
- `src/AudioProcessor.h` : definisi class processor audio
- `src/AudioProcessor.cpp` : logika equalizer, crossover, gain, NVS
- `platformio.ini` : konfigurasi build PlatformIO

## Persiapan hardware

- ESP32-S3 DevKit
- I2S DAC / CODEC audio
- Amplifier power stage untuk low/mid/high
- Optional: potensiometer atau encoder untuk kontrol manual

## Build

```bash
pio run
```

## Upload

```bash
pio run --target upload
```

## Penggunaan

1. Hubungkan ESP32-S3 ke komputer.
2. Buka Serial Monitor dengan baud `115200`.
3. Pilih menu sesuai kebutuhan.
4. Jika ingin menyalakan demo sinyal uji, gunakan menu `9`.

## Catatan

Firmware ini merupakan fondasi DSP loudspeaker management yang siap dikembangkan ke level produksi. Untuk aplikasi nyata dengan output audio I2S yang presisi, biasanya akan ditambah:

- filtered IIR biquad per band
- crossover Butterworth/Linkwitz-Riley
- limiter dan protection
- ADC input untuk sensing
- web UI / BLE / WiFi control

