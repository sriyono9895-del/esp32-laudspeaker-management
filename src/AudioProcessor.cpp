#include "AudioProcessor.h"

namespace {
constexpr float kSampleRate = 48000.0f;
constexpr float kEqBands[AudioProcessor::kEqBandCount] = {
    31.0f, 63.0f, 125.0f, 250.0f, 500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
}

BiquadFilter::BiquadFilter() = default;

void BiquadFilter::setLowPass(float sampleRate, float cutoffHz, float q) {
  const float omega = 2.0f * PI * cutoffHz / sampleRate;
  const float sinW = sinf(omega);
  const float cosW = cosf(omega);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha;
  b0_ = (1.0f - cosW) / (2.0f * a0);
  b1_ = (1.0f - cosW) / a0;
  b2_ = (1.0f - cosW) / (2.0f * a0);
  a1_ = -(2.0f * cosW) / a0;
  a2_ = (1.0f - alpha) / a0;

  x1_ = x2_ = y1_ = y2_ = 0.0f;
}

void BiquadFilter::setHighPass(float sampleRate, float cutoffHz, float q) {
  const float omega = 2.0f * PI * cutoffHz / sampleRate;
  const float sinW = sinf(omega);
  const float cosW = cosf(omega);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha;
  b0_ = (1.0f + cosW) / (2.0f * a0);
  b1_ = -(1.0f + cosW) / a0;
  b2_ = (1.0f + cosW) / (2.0f * a0);
  a1_ = -(2.0f * cosW) / a0;
  a2_ = (1.0f - alpha) / a0;

  x1_ = x2_ = y1_ = y2_ = 0.0f;
}

void BiquadFilter::setPeaking(float sampleRate, float centerHz, float gainDb, float q) {
  const float omega = 2.0f * PI * centerHz / sampleRate;
  const float sinW = sinf(omega);
  const float cosW = cosf(omega);
  const float gain = powf(10.0f, gainDb / 40.0f);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha / gain;
  b0_ = 1.0f + alpha * gain;
  b1_ = -2.0f * cosW;
  b2_ = 1.0f - alpha * gain;
  a1_ = -2.0f * cosW;
  a2_ = 1.0f - alpha / gain;

  b0_ /= a0;
  b1_ /= a0;
  b2_ /= a0;
  a1_ /= a0;
  a2_ /= a0;

  x1_ = x2_ = y1_ = y2_ = 0.0f;
}

float BiquadFilter::process(float sample) {
  const float y = b0_ * sample + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
  x2_ = x1_;
  x1_ = sample;
  y2_ = y1_;
  y1_ = y;
  return y;
}

AudioProcessor::AudioProcessor() {
  resetDefaults();
}

void AudioProcessor::begin() {
  prefs_.begin("laudspk", false);
  if (!loadSettings()) {
    resetDefaults();
  }
  rebuildFilters();
}

void AudioProcessor::resetDefaults() {
  config_.masterGainDb = 0.0f;
  config_.channelLowDb = 0.0f;
  config_.channelMidDb = 0.0f;
  config_.channelHighDb = 0.0f;

  for (size_t i = 0; i < kEqBandCount; ++i) {
    config_.eqGainDb[i] = 0.0f;
  }

  config_.crossoverLowMidHz = 250.0f;
  config_.crossoverMidHighHz = 2500.0f;

  rebuildFilters();
}

bool AudioProcessor::saveSettings() {
  prefs_.putFloat("masterGain", config_.masterGainDb);
  prefs_.putFloat("lowGain", config_.channelLowDb);
  prefs_.putFloat("midGain", config_.channelMidDb);
  prefs_.putFloat("highGain", config_.channelHighDb);
  prefs_.putFloat("lowMid", config_.crossoverLowMidHz);
  prefs_.putFloat("midHigh", config_.crossoverMidHighHz);

  for (size_t i = 0; i < kEqBandCount; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "eq_%02d", static_cast<int>(i));
    prefs_.putFloat(key, config_.eqGainDb[i]);
  }

  return true;
}

bool AudioProcessor::loadSettings() {
  if (!prefs_.isKey("masterGain")) {
    return false;
  }

  config_.masterGainDb = prefs_.getFloat("masterGain", 0.0f);
  config_.channelLowDb = prefs_.getFloat("lowGain", 0.0f);
  config_.channelMidDb = prefs_.getFloat("midGain", 0.0f);
  config_.channelHighDb = prefs_.getFloat("highGain", 0.0f);
  config_.crossoverLowMidHz = prefs_.getFloat("lowMid", 250.0f);
  config_.crossoverMidHighHz = prefs_.getFloat("midHigh", 2500.0f);

  for (size_t i = 0; i < kEqBandCount; ++i) {
    char key[8];
    snprintf(key, sizeof(key), "eq_%02d", static_cast<int>(i));
    config_.eqGainDb[i] = prefs_.getFloat(key, 0.0f);
  }

  return true;
}

void AudioProcessor::rebuildFilters() {
  for (size_t i = 0; i < kEqBandCount; ++i) {
    eqFilters_[i].setPeaking(kSampleRate, kEqBands[i], config_.eqGainDb[i], 1.0f);
  }

  lowFilter_.setLowPass(kSampleRate, config_.crossoverLowMidHz, 0.7071f);
  highFilter_.setHighPass(kSampleRate, config_.crossoverMidHighHz, 0.7071f);
}

float AudioProcessor::dbToGain(float dB) const {
  return powf(10.0f, dB / 20.0f);
}

float AudioProcessor::processSample(float input) {
  float sample = input * dbToGain(config_.masterGainDb);

  for (size_t i = 0; i < kEqBandCount; ++i) {
    sample = eqFilters_[i].process(sample);
  }

  const float low = lowFilter_.process(sample) * dbToGain(config_.channelLowDb);
  const float high = highFilter_.process(sample) * dbToGain(config_.channelHighDb);
  const float mid = (sample - low - high) * dbToGain(config_.channelMidDb);

  return low + mid + high;
}

void AudioProcessor::setMasterGain(float dB) {
  config_.masterGainDb = dB;
}

void AudioProcessor::setChannelGain(float lowDb, float midDb, float highDb) {
  config_.channelLowDb = lowDb;
  config_.channelMidDb = midDb;
  config_.channelHighDb = highDb;
}

void AudioProcessor::setEqBand(size_t index, float dB) {
  if (index < kEqBandCount) {
    config_.eqGainDb[index] = dB;
    eqFilters_[index].setPeaking(kSampleRate, kEqBands[index], dB, 1.0f);
  }
}

void AudioProcessor::setCrossover(float lowMidHz, float midHighHz) {
  config_.crossoverLowMidHz = lowMidHz;
  config_.crossoverMidHighHz = midHighHz;
  lowFilter_.setLowPass(kSampleRate, lowMidHz, 0.7071f);
  highFilter_.setHighPass(kSampleRate, midHighHz, 0.7071f);
}

void AudioProcessor::printStatus() const {
  Serial.println("\n=== Loudspeaker DSP Status ===");
  Serial.printf("Master Gain: %.2f dB\n", config_.masterGainDb);
  Serial.printf("Low Gain: %.2f dB\n", config_.channelLowDb);
  Serial.printf("Mid Gain: %.2f dB\n", config_.channelMidDb);
  Serial.printf("High Gain: %.2f dB\n", config_.channelHighDb);
  Serial.printf("Crossover Low-Mid: %.0f Hz\n", config_.crossoverLowMidHz);
  Serial.printf("Crossover Mid-High: %.0f Hz\n", config_.crossoverMidHighHz);

  Serial.println("Equalizer 10-band:");
  for (size_t i = 0; i < kEqBandCount; ++i) {
    Serial.printf("  Band %02d (%0.0f Hz): %.2f dB\n",
                  static_cast<int>(i + 1),
                  getBandFrequency(i),
                  config_.eqGainDb[i]);
  }
}

float AudioProcessor::getBandFrequency(size_t index) const {
  if (index >= kEqBandCount) {
    return 0.0f;
  }
  return kEqBands[index];
}
