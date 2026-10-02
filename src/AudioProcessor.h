#pragma once

#include <Arduino.h>
#include <array>
#include <Preferences.h>
#include <driver/i2s.h>

class BiquadFilter {
public:
  BiquadFilter();

  void setLowPass(float sampleRate, float cutoffHz, float q = 0.7071f);
  void setHighPass(float sampleRate, float cutoffHz, float q = 0.7071f);
  void setPeaking(float sampleRate, float centerHz, float gainDb, float q = 1.0f);
  float process(float sample);
  void reset();

private:
  float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
  float a1_ = 0.0f, a2_ = 0.0f;
  float x1_ = 0.0f, x2_ = 0.0f;
  float y1_ = 0.0f, y2_ = 0.0f;
};

class AudioProcessor {
public:
  static constexpr size_t kEqBandCount = 10;
  static constexpr float kSampleRate = 48000.0f;
  static constexpr float kEqBands[kEqBandCount] = {
      31.0f, 63.0f, 125.0f, 250.0f, 500.0f,
      1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};

  AudioProcessor();

  void begin();
  void resetDefaults();
  bool saveSettings();
  bool loadSettings();
  void rebuildFilters();

  void setMasterGain(float dB);
  void setChannelGain(float lowDb, float midDb, float highDb);
  void setEqBand(size_t index, float dB);
  void setCrossover(float lowMidHz, float midHighHz);
  void setLimiter(bool enable, float thresholdDb);
  
  float processSample(float input);
  void printStatus() const;
  float getBandFrequency(size_t index) const;

private:
  struct Config {
    float masterGainDb = 0.0f;
    float channelLowDb = 0.0f;
    float channelMidDb = 0.0f;
    float channelHighDb = 0.0f;
    float eqGainDb[kEqBandCount] = {0.0f};
    float crossoverLowMidHz = 250.0f;
    float crossoverMidHighHz = 2500.0f;
    bool limiterEnable = true;
    float limiterThresholdDb = -3.0f;
  };

  Config config_;
  std::array<BiquadFilter, kEqBandCount> eqFilters_;
  BiquadFilter lowPassFilter_;
  BiquadFilter highPassFilter_;
  BiquadFilter midBandPass_;
  Preferences prefs_;
  
  float peakLevel_ = 0.0f;
  float limiterAtk_ = 0.002f;  // 2ms attack
  float limiterRel_ = 0.05f;   // 50ms release
  float limiterGain_ = 1.0f;

  float dbToGain(float dB) const;
  float applyLimiter(float input);
};
