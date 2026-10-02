#pragma once

#include <Arduino.h>
#include <array>
#include <Preferences.h>

class BiquadFilter {
public:
  BiquadFilter();

  void setLowPass(float sampleRate, float cutoffHz, float q = 0.7071f);
  void setHighPass(float sampleRate, float cutoffHz, float q = 0.7071f);
  void setPeaking(float sampleRate, float centerHz, float gainDb, float q = 1.0f);
  float process(float sample);

private:
  float b0_ = 1.0f;
  float b1_ = 0.0f;
  float b2_ = 0.0f;
  float a1_ = 0.0f;
  float a2_ = 0.0f;
  float x1_ = 0.0f;
  float x2_ = 0.0f;
  float y1_ = 0.0f;
  float y2_ = 0.0f;
};

class AudioProcessor {
public:
  static constexpr size_t kEqBandCount = 10;

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
  float processSample(float input);
  void printStatus() const;
  float getBandFrequency(size_t index) const;

private:
  struct Config {
    float masterGainDb = 0.0f;
    float channelLowDb = 0.0f;
    float channelMidDb = 0.0f;
    float channelHighDb = 0.0f;
    float eqGainDb[kEqBandCount] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                   0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float crossoverLowMidHz = 250.0f;
    float crossoverMidHighHz = 2500.0f;
  };

  Config config_;
  std::array<BiquadFilter, kEqBandCount> eqFilters_;
  BiquadFilter lowFilter_;
  BiquadFilter highFilter_;
  Preferences prefs_;

  float dbToGain(float dB) const;
};
