#include "AudioProcessor.h"
#include <cmath>

BiquadFilter::BiquadFilter() = default;

void BiquadFilter::reset() {
  x1_ = x2_ = y1_ = y2_ = 0.0f;
}

void BiquadFilter::setLowPass(float sampleRate, float cutoffHz, float q) {
  if (cutoffHz >= sampleRate * 0.5f) {
    b0_ = 1.0f;
    b1_ = b2_ = 0.0f;
    a1_ = a2_ = 0.0f;
    return;
  }

  const float w0 = 2.0f * M_PI * cutoffHz / sampleRate;
  const float sinW = sinf(w0);
  const float cosW = cosf(w0);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha;
  b0_ = (1.0f - cosW) / (2.0f * a0);
  b1_ = (1.0f - cosW) / a0;
  b2_ = (1.0f - cosW) / (2.0f * a0);
  a1_ = -(2.0f * cosW) / a0;
  a2_ = (1.0f - alpha) / a0;

  reset();
}

void BiquadFilter::setHighPass(float sampleRate, float cutoffHz, float q) {
  if (cutoffHz <= 0.0f) {
    b0_ = 1.0f;
    b1_ = b2_ = 0.0f;
    a1_ = a2_ = 0.0f;
    return;
  }

  if (cutoffHz >= sampleRate * 0.5f) {
    b0_ = 0.0f;
    b1_ = b2_ = 0.0f;
    a1_ = a2_ = 0.0f;
    return;
  }

  const float w0 = 2.0f * M_PI * cutoffHz / sampleRate;
  const float sinW = sinf(w0);
  const float cosW = cosf(w0);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha;
  b0_ = (1.0f + cosW) / (2.0f * a0);
  b1_ = -(1.0f + cosW) / a0;
  b2_ = (1.0f + cosW) / (2.0f * a0);
  a1_ = -(2.0f * cosW) / a0;
  a2_ = (1.0f - alpha) / a0;

  reset();
}

void BiquadFilter::setPeaking(float sampleRate, float centerHz, float gainDb, float q) {
  if (centerHz <= 0.0f || centerHz >= sampleRate * 0.5f) {
    b0_ = 1.0f;
    b1_ = b2_ = 0.0f;
    a1_ = a2_ = 0.0f;
    return;
  }

  const float w0 = 2.0f * M_PI * centerHz / sampleRate;
  const float sinW = sinf(w0);
  const float cosW = cosf(w0);
  const float A = powf(10.0f, gainDb / 40.0f);
  const float alpha = sinW / (2.0f * q);

  const float a0 = 1.0f + alpha / A;
  b0_ = (1.0f + alpha * A) / a0;
  b1_ = (-2.0f * cosW) / a0;
  b2_ = (1.0f - alpha * A) / a0;
  a1_ = (-2.0f * cosW) / a0;
  a2_ = (1.0f - alpha / A) / a0;

  reset();
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
  Serial.println("[Audio] Processor initialized");
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
  config_.limiterEnable = true;
  config_.limiterThresholdDb = -3.0f;
  limiterGain_ = 1.0f;

  rebuildFilters();
}

bool AudioProcessor::saveSettings() {
  prefs_.putFloat("masterGain", config_.masterGainDb);
  prefs_.putFloat("lowGain", config_.channelLowDb);
  prefs_.putFloat("midGain", config_.channelMidDb);
  prefs_.putFloat("highGain", config_.channelHighDb);
  prefs_.putFloat("lowMid", config_.crossoverLowMidHz);
  prefs_.putFloat("midHigh", config_.crossoverMidHighHz);
  prefs_.putUChar("limiterEn", config_.limiterEnable ? 1 : 0);
  prefs_.putFloat("limiterTh", config_.limiterThresholdDb);

  for (size_t i = 0; i < kEqBandCount; ++i) {
    char key[12];
    snprintf(key, sizeof(key), "eq_%02zu", i);
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
  config_.limiterEnable = prefs_.getUChar("limiterEn", 1) != 0;
  config_.limiterThresholdDb = prefs_.getFloat("limiterTh", -3.0f);

  for (size_t i = 0; i < kEqBandCount; ++i) {
    char key[12];
    snprintf(key, sizeof(key), "eq_%02zu", i);
    config_.eqGainDb[i] = prefs_.getFloat(key, 0.0f);
  }

  return true;
}

void AudioProcessor::rebuildFilters() {
  for (size_t i = 0; i < kEqBandCount; ++i) {
    eqFilters_[i].setPeaking(kSampleRate, kEqBands[i], config_.eqGainDb[i], 1.0f);
  }

  lowPassFilter_.setLowPass(kSampleRate, config_.crossoverLowMidHz, 0.7071f);
  highPassFilter_.setHighPass(kSampleRate, config_.crossoverMidHighHz, 0.7071f);
}

float AudioProcessor::dbToGain(float dB) const {
  return powf(10.0f, dB / 20.0f);
}

float AudioProcessor::applyLimiter(float input) {
  if (!config_.limiterEnable) {
    return input;
  }

  const float level = fabsf(input);
  peakLevel_ = fmaxf(level, peakLevel_ * 0.995f);

  const float thresholdLin = dbToGain(config_.limiterThresholdDb);

  if (peakLevel_ > thresholdLin) {
    const float ratio = 4.0f;
    float targetGain = (peakLevel_ > 0.0f) ? (thresholdLin + (peakLevel_ - thresholdLin) / ratio) / peakLevel_ : 1.0f;
    targetGain = fmaxf(targetGain, 0.1f);

    if (targetGain < limiterGain_) {
      limiterGain_ += (targetGain - limiterGain_) * limiterAtk_;
    } else {
      limiterGain_ += (targetGain - limiterGain_) * limiterRel_;
    }
  } else {
    limiterGain_ += (1.0f - limiterGain_) * limiterRel_;
  }

  return input * limiterGain_;
}

float AudioProcessor::processSample(float input) {
  // Apply master gain
  float sample = input * dbToGain(config_.masterGainDb);

  // Apply 10-band EQ
  for (size_t i = 0; i < kEqBandCount; ++i) {
    sample = eqFilters_[i].process(sample);
  }

  // 3-way crossover
  float low = lowPassFilter_.process(sample) * dbToGain(config_.channelLowDb);
  float high = highPassFilter_.process(sample) * dbToGain(config_.channelHighDb);
  float mid = (sample - low + low - high) * dbToGain(config_.channelMidDb);

  float output = low + mid + high;

  // Apply limiter/protection
  output = applyLimiter(output);

  return output;
}

void AudioProcessor::setMasterGain(float dB) {
  config_.masterGainDb = fmaxf(-60.0f, fminf(dB, 12.0f));
}

void AudioProcessor::setChannelGain(float lowDb, float midDb, float highDb) {
  config_.channelLowDb = fmaxf(-12.0f, fminf(lowDb, 12.0f));
  config_.channelMidDb = fmaxf(-12.0f, fminf(midDb, 12.0f));
  config_.channelHighDb = fmaxf(-12.0f, fminf(highDb, 12.0f));
}

void AudioProcessor::setEqBand(size_t index, float dB) {
  if (index < kEqBandCount) {
    config_.eqGainDb[index] = fmaxf(-12.0f, fminf(dB, 12.0f));
    eqFilters_[index].setPeaking(kSampleRate, kEqBands[index], config_.eqGainDb[index], 1.0f);
  }
}

void AudioProcessor::setCrossover(float lowMidHz, float midHighHz) {
  config_.crossoverLowMidHz = fmaxf(50.0f, fminf(lowMidHz, 5000.0f));
  config_.crossoverMidHighHz = fmaxf(500.0f, fminf(midHighHz, 20000.0f));
  if (config_.crossoverLowMidHz >= config_.crossoverMidHighHz) {
    config_.crossoverMidHighHz = config_.crossoverLowMidHz * 2.0f;
  }
  lowPassFilter_.setLowPass(kSampleRate, config_.crossoverLowMidHz, 0.7071f);
  highPassFilter_.setHighPass(kSampleRate, config_.crossoverMidHighHz, 0.7071f);
}

void AudioProcessor::setLimiter(bool enable, float thresholdDb) {
  config_.limiterEnable = enable;
  config_.limiterThresholdDb = fmaxf(-20.0f, fminf(thresholdDb, 0.0f));
}

void AudioProcessor::printStatus() const {
  Serial.println("\n=== Loudspeaker DSP Status ===");
  Serial.printf("Master Gain: %.2f dB\n", config_.masterGainDb);
  Serial.printf("Low Gain: %.2f dB\n", config_.channelLowDb);
  Serial.printf("Mid Gain: %.2f dB\n", config_.channelMidDb);
  Serial.printf("High Gain: %.2f dB\n", config_.channelHighDb);
  Serial.printf("Crossover Low-Mid: %.1f Hz\n", config_.crossoverLowMidHz);
  Serial.printf("Crossover Mid-High: %.1f Hz\n", config_.crossoverMidHighHz);
  Serial.printf("Limiter: %s (Threshold: %.1f dB)\n",
                config_.limiterEnable ? "ON" : "OFF",
                config_.limiterThresholdDb);

  Serial.println("Equalizer 10-band:");
  for (size_t i = 0; i < kEqBandCount; ++i) {
    Serial.printf("  Band %02zu (%.0f Hz): %+.2f dB\n",
                  i + 1,
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
