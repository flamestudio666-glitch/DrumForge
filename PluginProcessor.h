#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <vector>
#include "DrumVoice.h"
#include "Genres.h"

constexpr int MaxTracks = 15;   // track slot i -> output bus i+1 ("Out i+1"); bus 0 = Main

struct Track
{
    Track() { for (auto& s : steps) s = 0.f; }
    juce::String name;                       // GUI thread only
    int type = 0, note = 36;
    std::atomic<float> steps[16];
    std::atomic<float> vol { 0.85f };
    std::atomic<bool> mute { false }, active { false };
    juce::SpinLock sampleLock;
    std::shared_ptr<juce::AudioBuffer<float>> sample;
    juce::String samplePath, sampleName;     // GUI thread only
};

class DrumForgeProcessor : public juce::AudioProcessor
{
public:
    DrumForgeProcessor();

    void prepareToPlay (double sampleRate, int) override { sr = sampleRate; }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Drum Forge"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 1.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- model (shared with editor) ----
    Track tracks[MaxTracks];
    std::atomic<float> bpm { 90.f }, swing { 0.14f }, density { 1.f };
    std::atomic<int> genreIdx { 0 }, currentStep { -1 }, layoutVersion { 0 }, exportBars { 4 };
    std::atomic<bool> internalPlay { false };

    void applyGenre (int gi);
    void generateTrack (int i);
    void generateAll();
    int  addTrack (const juce::String& name, int type);   // returns slot or -1
    void removeTrack (int i);
    void loadSample (int i, const juce::File& f);
    void clearSample (int i);
    juce::File exportMidi (const std::vector<int>& idx, const juce::String& fileName);

private:
    DrumVoice voices[MaxTracks];
    double sr = 44100.0, internalPpq = 0.0;
    juce::AudioFormatManager fm;
    juce::Random rng;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumForgeProcessor)
};
