#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <memory>

enum DrumType { Kick = 0, Snare, Hat, OHat, Clap, Rim, Tom, Perc, Shaker, Sub, NumTypes };

inline const char* typeName(int t)
{
    static const char* n[] = { "Kick", "Snare", "Closed Hat", "Open Hat", "Clap", "Rim", "Tom", "Perc", "Shaker", "808 Sub" };
    return n[t];
}
inline int typeNote(int t) // General MIDI drum map
{
    static const int n[] = { 36, 38, 42, 46, 39, 37, 45, 56, 70, 35 };
    return n[t];
}

// One voice per track. Built-in synth drums, or a loaded one-shot sample.
struct DrumVoice
{
    bool active = false;
    int type = 0, age = 0, pos = 0;
    double sr = 44100.0;
    float vel = 1.f, phase = 0.f, phase2 = 0.f, hp = 0.f, hpA = 0.f;
    uint32_t rng = 22222u;
    std::shared_ptr<juce::AudioBuffer<float>> smp;

    void trigger (int t, float v, double rate, std::shared_ptr<juce::AudioBuffer<float>> s)
    {
        type = t; vel = v; sr = rate; age = 0; pos = 0;
        phase = phase2 = hp = 0.f;
        smp = std::move (s);
        active = true;
        float fc = 0.f;
        switch (t) { case Snare: fc = 1800.f; break; case Clap: fc = 1500.f; break; case Hat: fc = 7500.f; break;
                     case OHat: fc = 7000.f; break; case Rim: fc = 3000.f; break; case Shaker: fc = 5500.f; break; default: break; }
        hpA = fc > 0.f ? 1.f - std::exp (-6.2831853f * fc / (float) rate) : 0.f;
    }

    float noise() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (float) rng / 2147483648.f - 1.f; }
    float hpf (float x) { hp += hpA * (x - hp); return x - hp; }

    bool next (float& l, float& r)
    {
        if (! active) return false;
        if (smp != nullptr)
        {
            const int n = smp->getNumSamples();
            if (pos >= n) { active = false; smp.reset(); return false; }
            l = smp->getSample (0, pos) * vel;
            r = (smp->getNumChannels() > 1 ? smp->getSample (1, pos) : smp->getSample (0, pos)) * vel;
            ++pos;
            return true;
        }
        const float t = (float) age / (float) sr, tp = 6.2831853f, fs = (float) sr;
        float o = 0.f, dur = 0.5f;
        switch (type)
        {
            case Kick:  { phase += tp * (45.f + 115.f * std::exp (-t * 40.f)) / fs; o = std::sin (phase) * std::exp (-t * 8.f); dur = 0.6f; break; }
            case Sub:   { phase += tp * (42.f + 28.f * std::exp (-t * 20.f)) / fs; o = std::sin (phase) * std::exp (-t * 3.2f); dur = 1.4f; break; }
            case Snare: { phase += tp * (150.f + 70.f * std::exp (-t * 30.f)) / fs;
                          o = hpf (noise()) * std::exp (-t * 22.f) * 0.8f + std::sin (phase) * std::exp (-t * 35.f) * 0.7f; dur = 0.35f; break; }
            case Clap:  { float e = 0.f; const float b[3] = { 0.f, 0.012f, 0.024f };
                          for (float bb : b) if (t >= bb) e += std::exp (-(t - bb) * 130.f);
                          if (t >= 0.036f) e += 0.8f * std::exp (-(t - 0.036f) * 18.f);
                          o = hpf (noise()) * e * 0.6f; dur = 0.4f; break; }
            case Hat:   { o = hpf (noise()) * std::exp (-t * 55.f) * 0.6f; dur = 0.15f; break; }
            case OHat:  { o = hpf (noise()) * std::exp (-t * 9.f) * 0.55f; dur = 0.6f; break; }
            case Rim:   { phase += tp * (500.f + 320.f * std::exp (-t * 120.f)) / fs;
                          o = std::sin (phase) * std::exp (-t * 110.f) * 0.6f + hpf (noise()) * std::exp (-t * 150.f) * 0.4f; dur = 0.12f; break; }
            case Tom:   { phase += tp * (95.f + 125.f * std::exp (-t * 18.f)) / fs; o = std::sin (phase) * std::exp (-t * 9.f); dur = 0.6f; break; }
            case Shaker:{ const float e = t < 0.012f ? t / 0.012f : std::exp (-(t - 0.012f) * 35.f); o = hpf (noise()) * e * 0.4f; dur = 0.25f; break; }
            default:    { phase += tp * 540.f / fs; phase2 += tp * 800.f / fs;
                          o = (std::sin (phase) * 0.6f + std::sin (phase2) * 0.2f) * std::exp (-t * 14.f); dur = 0.35f; break; }
        }
        if (t > dur) { active = false; return false; }
        ++age;
        l = r = o * vel;
        return true;
    }
};
