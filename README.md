# ESP32-S3 Digital Loudspeaker Management System

**Firmware komplet dan siap compile 100% untuk ESP32-S3**

## Fitur Utama

### 1. Equalizer 10 Band (Biquad Filter)
- Frekuensi: 31, 63, 125, 250, 500, 1k, 2k, 4k, 8k, 16k Hz
- Gain range: -12 dB sampai +12 dB per band
- Filter tipe peaking dengan Q=1.0
- Real-time parameter adjustment

### 2. Crossover 3-Way
- Low channel: Frekuensi cut-off adjustable (default 250 Hz)
- Mid channel: Bandpass antara low-mid dan mid-high
- High channel: High-pass adjustable (default 2.5 kHz)
- Butterworth-style filter dengan Q=0.7071 (Sallen-Key)

### 3. Gain Control
- Master gain: -60 dB sampai +12 dB
- Channel gain: -12 dB sampai +12 dB per channel (low/mid/high)
- Real-time adjustment tanpa restart

### 4. Limiter / Peak Protection
- Soft-knee limiter dengan attack 2ms, release 50ms
- Ratio 4:1
- Threshold adjustable: -20 dB sampai 0 dB
- Proteksi dari clipping

### 5. Interface Opsi
- **Mode 1 - Menu Serial**: Kontrol via Serial Monitor (115200 baud)
- **Mode 2 - Demo**: Simulasi sinyal uji multi-frekuensi
- **Mode 3 - Real-time I2S**: I2S stereo audio processing

### 6. Penyimpanan Konfigurasi
- NVS (Non-Volatile Storage) di flash internal
- Save/load preset
- Persistensi data setelah power-off

## Hardware Setup

### ESP32-S3 Pinout (I2S Audio Interface)
```
GPIO 41 (TX1/D+) -> I2S BCLK (Bit Clock)
GPIO 42 (TX0/D-) -> I2S LRCK (Word Select)
GPIO 1           -> I2S DOUT (Data Out)
GPIO 2           -> I2S DIN  (Data In)

Optional:
GPIO 3           -> Button/Encoder A
GPIO 4           -> Button/Encoder B
GND              -> Ground
3V3              -> Power
```

### External Audio Hardware
- **I2S DAC/CODEC**: PCM5102, ES8388, SGTL5000, WM8978
- **Audio Amplifier**: Class D amplifier module untuk setiap channel
- **Loudspeaker Driver**: 
  - Woofer (low channel)
  - Midrange (mid channel)
  - Tweeter (high channel)

## Membangun & Upload

### Dengan PlatformIO
```bash
# Build firmware
pio run -e esp32-s3-devkitc-1

# Upload ke ESP32-S3
pio run -t upload -e esp32-s3-devkitc-1

# Monitor serial output
pio device monitor -b 115200 -e esp32-s3-devkitc-1
```

### Dengan Arduino IDE
1. Buka file `src/main.cpp`
2. Copy seluruh kode ke Arduino IDE
3. Tambahkan library yang diperlukan (built-in ESP32 board package)
4. Upload ke board ESP32-S3-DevKitC-1

## Penggunaan Menu Serial

### Startup
```
ESP32-S3 Digital Loudspeaker Management System
Firmware v1.0 - Complete DSP Implementation
Features: 10-band EQ, 3-way Crossover, Gain Control, Limiter

=== ESP32-S3 Digital Loudspeaker DSP ===
1. Tampilkan status
2. Set gain master
3. Set gain per channel (low/mid/high)
4. Atur equalizer 10 band
5. Atur crossover 3-way
6. Atur limiter
7. Simpan konfigurasi ke flash
8. Muat konfigurasi dari flash
9. Reset ke default
10. Mode demo (simulasi sinyal)
11. Aktifkan I2S real-time DSP
0. Keluar
```

### Contoh Penggunaan

**1. Lihat Status**
```
Pilih menu: 1
=== Loudspeaker DSP Status ===
Master Gain: 0.00 dB
Low Gain: 0.00 dB
Mid Gain: 0.00 dB
High Gain: 0.00 dB
Crossover Low-Mid: 250.0 Hz
Crossover Mid-High: 2500.0 Hz
Limiter: ON (Threshold: -3.0 dB)
Equalizer 10-band:
  Band 01 (31 Hz): +0.00 dB
  Band 02 (63 Hz): +0.00 dB
  ...
```

**2. Set Master Gain**
```
Pilih menu: 2
Masukkan gain master (dB, -60 sampai +12): 3.0
>> Gain master: 3.00 dB
```

**3. Atur Equalizer Band**
```
Pilih menu: 4
Pilih band equalizer (1-10):
 1. 31 Hz
 2. 63 Hz
...
Masukkan nomor band: 1
Masukkan gain band (dB, -12 sampai +12): +6.0
>> Band 1 diatur ke +6.00 dB
```

**4. Demo Mode**
```
Pilih menu: 10
=== DEMO MODE: Simulasi Sinyal ===
Menjalankan loop sinyal uji selama 10 detik...
[    0 ms] in=0.0000 out=0.0000
[   50 ms] in=0.0250 out=0.0250
[  100 ms] in=0.0499 out=0.0499
... (tekan 'q' untuk keluar)
```

**5. Real-time I2S Mode**
```
Pilih menu: 11
=== I2S Real-Time DSP Mode ===
Inisialisasi I2S...
[I2S] Audio interface initialized
I2S aktif. Tekan 'q' untuk keluar mode real-time.
[I2S] Frames processed: 100
[I2S] Frames processed: 200
... (tekan 'q' untuk keluar)
>> I2S mode dihentikan
```

## Spesifikasi DSP

### Sample Rate
- 48 kHz (fixed, dapat diubah di `AudioProcessor.h`)

### Filter Quality
- 10-band EQ: Biquad peaking filter, Q=1.0
- Crossover: Butterworth style, Q=0.7071
- Limiter: Soft-knee dengan attack/release adjustable

### Processing Chain
```
Input Audio
    ↓
[Master Gain]
    ↓
[10-band EQ (10x Biquad Filter)]
    ↓
[3-way Crossover]
    ├→ [Low-pass @ 250Hz] → [Low Channel Gain]
    ├→ [Band-pass Mid] → [Mid Channel Gain]
    └→ [High-pass @ 2.5kHz] → [High Channel Gain]
    ↓
[Limiter/Protection]
    ↓
Output Audio
```

### Memory Usage (Estimated)
- Flash: ~250 KB (code + data)
- RAM: ~50 KB (audio buffers + filter states)

## Catatan Teknis

### Biquad Filter Implementation
Menggunakan standard direct form II transposed biquad:
```
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
```

### Crossover Calculation
- Low pass: Butterworth LPF @ crossoverLowMidHz
- High pass: Butterworth HPF @ crossoverMidHighHz
- Mid band: Derived dari input - lowpass + lowpass - highpass

### Limiter Logic
- Peak detection dengan decay 0.5% per sample
- Attack: 2ms exponential ramp
- Release: 50ms exponential ramp
- Ratio: 4:1 soft-knee

## Troubleshooting

### Tidak ada output di Serial Monitor
- Pastikan baud rate = 115200
- Periksa kabel USB dan driver CH340/CP2102

### I2S Mode Timeout
- Pastikan DAC/CODEC sudah terhubung dan dikonfigurasi
- Cek pin connections GPIO 1, 2, 41, 42
- Adjust timeout di `i2sRealTimeMode()` jika perlu

### Filter Tidak Bekerja
- Pastikan tidak ada audio input yang overload (> ±1.0)
- Periksa limiter threshold tidak terlalu rendah
- Validasi gain channel dalam range yang tepat

## Roadmap Pengembangan

- [ ] UI berbasis web (WiFi + AsyncWebServer)
- [ ] Bluetooth control (BLE)
- [ ] Hardware LCD display (I2C 16x2)
- [ ] Rotary encoder control
- [ ] Preset manager dengan SD card
- [ ] Real-time spectrum analyzer via Serial
- [ ] Multi-zone amplifier control
- [ ] Remote WiFi control app

## License

Open source untuk keperluan edukatif dan komersial dengan referensi source code.

## Support & Documentation

Untuk pertanyaan atau development lebih lanjut, silakan check:
- ESP32-S3 Technical Reference: https://www.espressif.com/
- I2S Audio Processing: https://www.dsprelated.com/
- Biquad Filter Design: https://www.earlevel.com/

---

**Version**: 1.0  
**Last Updated**: 2024  
**Status**: Production Ready
