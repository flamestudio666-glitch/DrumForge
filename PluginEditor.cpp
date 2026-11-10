#include "PluginEditor.h"

static const juce::Colour bg (0xff14141a);

// ---------------- TrackRow ----------------
TrackRow::TrackRow (DrumForgeProcessor& p, int i) : proc (p), idx (i)
{
    for (int s = 0; s < 16; ++s) { steps[s] = std::make_unique<StepButton> (p, i, s); addAndMakeVisible (*steps[s]); }
    addAndMakeVisible (gen); addAndMakeVisible (loadB); addAndMakeVisible (delB); addAndMakeVisible (muteB);
    addAndMakeVisible (vol); addAndMakeVisible (handle);

    gen.onClick = [this] { proc.generateTrack (idx); repaint(); };
    delB.onClick = [this] { proc.removeTrack (idx); };
    muteB.setClickingTogglesState (true);
    muteB.setToggleState (proc.tracks[idx].mute.load(), juce::dontSendNotification);
    muteB.onClick = [this] { proc.tracks[idx].mute = muteB.getToggleState(); };
    vol.setSliderStyle (juce::Slider::LinearHorizontal); vol.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    vol.setRange (0.0, 1.0, 0.01); vol.setValue (proc.tracks[idx].vol.load(), juce::dontSendNotification);
    vol.onValueChange = [this] { proc.tracks[idx].vol = (float) vol.getValue(); };
    loadB.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a drum one-shot", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { auto f = fc.getResult(); if (f.existsAsFile()) proc.loadSample (idx, f); repaint(); });
    };
    handle.label = "MIDI";
    handle.makeFile = [this] { return proc.exportMidi ({ idx }, proc.tracks[idx].name); };
}

void TrackRow::paint (juce::Graphics& g)
{
    auto& t = proc.tracks[idx];
    g.setColour (juce::Colours::white); g.setFont (juce::Font (juce::FontOptions (14.f, juce::Font::bold)));
    g.drawText (t.name, 0, 2, 120, 16, juce::Justification::left);
    g.setColour (juce::Colours::grey); g.setFont (11.f);
    g.drawText ("Out " + juce::String (idx + 1) + (t.sampleName.isNotEmpty() ? "  |  " + t.sampleName : "  |  synth"), 0, 17, 120, 13, juce::Justification::left);
}

void TrackRow::resized()
{
    for (int s = 0; s < 16; ++s) steps[s]->setBounds (124 + s * 28, 0, 28, 32);
    int x = 124 + 16 * 28 + 8;
    gen.setBounds (x, 3, 44, 26);        x += 48;
    muteB.setBounds (x, 3, 28, 26);      x += 32;
    vol.setBounds (x, 3, 80, 26);        x += 84;
    loadB.setBounds (x, 3, 48, 26);      x += 52;
    handle.setBounds (x, 3, 52, 26);     x += 56;
    delB.setBounds (x, 3, 26, 26);
}

bool TrackRow::isInterestedInFileDrag (const juce::StringArray& f)
{
    return f.size() > 0 && juce::File (f[0]).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg");
}
void TrackRow::filesDropped (const juce::StringArray& f, int, int) { proc.loadSample (idx, juce::File (f[0])); repaint(); }

// ---------------- Editor ----------------
DrumForgeEditor::DrumForgeEditor (DrumForgeProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setSize (960, 650);

    juce::String lastGroup;
    const auto& gl = genres();
    for (size_t i = 0; i < gl.size(); ++i)
    {
        if (gl[i].group != lastGroup) { genre.addSectionHeading (gl[i].group); lastGroup = gl[i].group; }
        genre.addItem (gl[i].name, (int) i + 1);
    }
    genre.onChange = [this]
    {
        const int gi = genre.getSelectedId() - 1;
        if (gi < 0) return;
        proc.applyGenre (gi);
        bpm.setValue (proc.bpm.load(), juce::dontSendNotification);
        swing.setValue (proc.swing.load(), juce::dontSendNotification);
        repaint();
    };

    auto setup = [this] (juce::Slider& s, juce::Label& l, const juce::String& txt, double lo, double hi, double step)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal); s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
        s.setRange (lo, hi, step); l.setText (txt, juce::dontSendNotification); l.setFont (12.f);
        l.attachToComponent (&s, false); addAndMakeVisible (s);
    };
    setup (bpm, bpmL, "Preview BPM (DAW tempo is used in host)", 60, 200, 1);
    setup (swing, swL, "Swing", 0, 0.45, 0.01);
    setup (density, denL, "Density", 0.3, 1.7, 0.05);
    bpm.onValueChange = [this] { proc.bpm = (float) bpm.getValue(); };
    swing.onValueChange = [this] { proc.swing = (float) swing.getValue(); };
    density.onValueChange = [this] { proc.density = (float) density.getValue(); };
    density.onDragEnd = [this] { proc.generateAll(); };

    genAll.onClick = [this] { proc.generateAll(); repaint(); };
    playB.setClickingTogglesState (true);
    playB.onClick = [this] { proc.internalPlay = playB.getToggleState(); playB.setButtonText (playB.getToggleState() ? "Stop" : "Play"); };

    bars.addItem ("1 bar", 1); bars.addItem ("2 bars", 2); bars.addItem ("4 bars", 3); bars.addItem ("8 bars", 4);
    bars.setSelectedId (3, juce::dontSendNotification);
    bars.onChange = [this] { static const int b[] = { 1, 2, 4, 8 }; proc.exportBars = b[juce::jlimit (1, 4, bars.getSelectedId()) - 1]; };

    allHandle.label = "Drag all MIDI";
    allHandle.makeFile = [this]
    {
        std::vector<int> idx;
        for (int i = 0; i < MaxTracks; ++i) if (proc.tracks[i].active) idx.push_back (i);
        return proc.exportMidi (idx, "DrumForge_" + genres()[(size_t) proc.genreIdx.load()].name);
    };

    for (int t = 0; t < NumTypes; ++t) newType.addItem (typeName (t), t + 1);
    newType.setSelectedId (1, juce::dontSendNotification);
    newName.setTextToShowWhenEmpty ("Instrument name", juce::Colours::grey);
    addB.onClick = [this]
    {
        const int t = newType.getSelectedId() - 1;
        auto n = newName.getText().trim();
        proc.addTrack (n.isEmpty() ? juce::String (typeName (t)) : n, t);
        newName.clear();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &genre, &genAll, &playB, &bars, &allHandle, &newName, &newType, &addB })
        addAndMakeVisible (c);

    startTimerHz (30);
}

void DrumForgeEditor::rebuildRows()
{
    rows.clear();
    for (int i = 0; i < MaxTracks; ++i)
        if (proc.tracks[i].active) { auto* r = new TrackRow (proc, i); rows.add (r); addAndMakeVisible (r); }
    genre.setSelectedId (proc.genreIdx.load() + 1, juce::dontSendNotification);
    bpm.setValue (proc.bpm.load(), juce::dontSendNotification);
    swing.setValue (proc.swing.load(), juce::dontSendNotification);
    density.setValue (proc.density.load(), juce::dontSendNotification);
    resized();
}

void DrumForgeEditor::timerCallback()
{
    if (lastVersion != proc.layoutVersion.load()) { lastVersion = proc.layoutVersion.load(); rebuildRows(); }
    repaint();
}

void DrumForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (juce::Colour (0xffff5d73)); g.setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
    g.drawText ("DRUM FORGE", 20, 4, 200, 16, juce::Justification::left);
    g.setColour (juce::Colours::grey); g.setFont (11.f);
    g.drawText ("Each instrument has its own output (Out 1, Out 2 ...). In FL Studio route them to separate mixer inserts via the wrapper settings.  Drop a wav onto a row to replace its sound.",
                20, 626, 920, 16, juce::Justification::left);
}

void DrumForgeEditor::resized()
{
    genre.setBounds (20, 26, 170, 26);
    bpm.setBounds (200, 40, 150, 22);
    swing.setBounds (365, 40, 120, 22);
    density.setBounds (500, 40, 120, 22);
    genAll.setBounds (632, 26, 100, 26);
    playB.setBounds (738, 26, 56, 26);
    bars.setBounds (800, 26, 64, 26);
    allHandle.setBounds (870, 26, 70, 26);
    for (int i = 0; i < rows.size(); ++i) rows[i]->setBounds (20, 78 + i * 36, 920, 32);
    newName.setBounds (20, 600, 170, 24);
    newType.setBounds (196, 600, 120, 24);
    addB.setBounds (322, 600, 120, 24);
}
