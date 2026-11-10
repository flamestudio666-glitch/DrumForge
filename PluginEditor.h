#pragma once
#include "PluginProcessor.h"

// Small button you can DRAG out of the plugin window into a DAW (piano roll / playlist).
class MidiHandle : public juce::Component
{
public:
    std::function<juce::File()> makeFile;
    juce::String label;
    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff3a3a4d)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.f);
        g.setColour (juce::Colours::white); g.setFont (12.f); g.drawText (label, getLocalBounds(), juce::Justification::centred);
    }
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        dragging = true;
        auto f = makeFile();
        juce::DragAndDropContainer::performExternalDragDropOfFiles (juce::StringArray { f.getFullPathName() }, false, this, [this] { dragging = false; });
    }
private:
    bool dragging = false;
};

class StepButton : public juce::Component
{
public:
    StepButton (DrumForgeProcessor& p, int t, int s) : proc (p), trk (t), step (s) {}
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (1.f);
        const float v = proc.tracks[trk].steps[step].load();
        g.setColour ((step / 4) % 2 == 0 ? juce::Colour (0xff363647) : juce::Colour (0xff2b2b38)); g.fillRoundedRectangle (b, 4.f);
        if (v > 0.f) { g.setColour (juce::Colour (0xffff5d73).withAlpha (0.45f + v * 0.55f)); g.fillRoundedRectangle (b, 4.f); }
        if (proc.currentStep.load() == step) { g.setColour (juce::Colour (0xffffb347)); g.drawRoundedRectangle (b, 4.f, 2.f); }
    }
    void mouseDown (const juce::MouseEvent&) override
    {
        auto& a = proc.tracks[trk].steps[step];
        a = a.load() > 0.f ? 0.f : 0.9f;
        repaint();
    }
private:
    DrumForgeProcessor& proc; int trk, step;
};

class TrackRow : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    TrackRow (DrumForgeProcessor& p, int i);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& f) override;
    void filesDropped (const juce::StringArray& f, int, int) override;
private:
    DrumForgeProcessor& proc; int idx;
    std::unique_ptr<StepButton> steps[16];
    juce::TextButton gen { "Gen" }, loadB { "Load" }, delB { "X" }, muteB { "M" };
    juce::Slider vol;
    MidiHandle handle;
    std::unique_ptr<juce::FileChooser> chooser;
};

class DrumForgeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DrumForgeEditor (DrumForgeProcessor&);
    ~DrumForgeEditor() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void rebuildRows();
    DrumForgeProcessor& proc;
    juce::ComboBox genre, bars, newType;
    juce::Slider bpm, swing, density;
    juce::Label bpmL, swL, denL;
    juce::TextButton genAll { "Generate all" }, playB { "Play" }, addB { "Add instrument" };
    juce::TextEditor newName;
    MidiHandle allHandle;
    juce::OwnedArray<TrackRow> rows;
    int lastVersion = -1;
};
