#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

static juce::AudioProcessor::BusesProperties makeBuses()
{
    auto b = juce::AudioProcessor::BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true);
    for (int i = 1; i <= MaxTracks; ++i)
        b = b.withOutput ("Out " + juce::String (i), juce::AudioChannelSet::stereo(), false);
    return b;
}

DrumForgeProcessor::DrumForgeProcessor() : AudioProcessor (makeBuses())
{
    fm.registerBasicFormats();
    genreIdx = 0;
    addTrack ("Kick", Kick);
    addTrack ("Snare", Snare);
    addTrack ("Closed Hat", Hat);
    addTrack ("Open Hat", OHat);
    addTrack ("Clap", Clap);
    applyGenre (0);
}

bool DrumForgeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet().isDisabled()) return false;
    for (auto& b : l.outputBuses)
        if (! b.isDisabled() && b != juce::AudioChannelSet::stereo()) return false;
    return true;
}

// ---------------- model ----------------
void DrumForgeProcessor::applyGenre (int gi)
{
    gi = juce::jlimit (0, (int) genres().size() - 1, gi);
    genreIdx = gi;
    bpm = genres()[(size_t) gi].bpm;
    swing = genres()[(size_t) gi].sw;
    generateAll();
}

void DrumForgeProcessor::generateTrack (int i)
{
    auto pat = generatePattern (genres()[(size_t) genreIdx.load()], tracks[i].type, density.load(), rng);
    for (int s = 0; s < 16; ++s) tracks[i].steps[s] = pat[(size_t) s];
}

void DrumForgeProcessor::generateAll()
{
    for (int i = 0; i < MaxTracks; ++i) if (tracks[i].active) generateTrack (i);
}

int DrumForgeProcessor::addTrack (const juce::String& name, int type)
{
    for (int i = 0; i < MaxTracks; ++i)
    {
        if (tracks[i].active) continue;
        int note = typeNote (type);
        auto used = [&] (int n) { for (auto& t : tracks) if (t.active && t.note == n) return true; return false; };
        if (used (note)) { note = 48; while (used (note)) ++note; }
        auto& t = tracks[i];
        t.name = name; t.type = type; t.note = note; t.vol = 0.85f; t.mute = false;
        clearSample (i);
        generateTrack (i);
        t.active = true;
        ++layoutVersion;
        return i;
    }
    return -1;
}

void DrumForgeProcessor::removeTrack (int i) { tracks[i].active = false; ++layoutVersion; }

void DrumForgeProcessor::clearSample (int i)
{
    { juce::SpinLock::ScopedLockType l (tracks[i].sampleLock); tracks[i].sample.reset(); }
    tracks[i].samplePath = {}; tracks[i].sampleName = {};
}

void DrumForgeProcessor::loadSample (int i, const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
    if (rd == nullptr) return;
    const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 10.0));
    const int ch = juce::jmin (2, (int) rd->numChannels);
    juce::AudioBuffer<float> tmp (ch, len);
    rd->read (&tmp, 0, len, 0, true, ch > 1);
    auto out = std::make_shared<juce::AudioBuffer<float>>();
    const double ratio = rd->sampleRate / sr;
    if (std::abs (ratio - 1.0) < 1e-4) *out = tmp;
    else
    {
        const int nl = (int) (len / ratio);
        out->setSize (ch, nl);
        for (int c = 0; c < ch; ++c) { juce::LagrangeInterpolator li; li.process (ratio, tmp.getReadPointer (c), out->getWritePointer (c), nl); }
    }
    { juce::SpinLock::ScopedLockType l (tracks[i].sampleLock); tracks[i].sample = out; }
    tracks[i].samplePath = f.getFullPathName();
    tracks[i].sampleName = f.getFileName();
}

juce::File DrumForgeProcessor::exportMidi (const std::vector<int>& idx, const juce::String& fileName)
{
    const int bars = exportBars.load(), stepT = 120;      // 480 PPQ, 120 ticks per 16th
    const double sw = swing.load();
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm.load())), 0.0);
    for (int b = 0; b < bars; ++b)
        for (int s = 0; s < 16; ++s)
            for (int i : idx)
            {
                const float v = tracks[i].steps[s].load();
                if (v <= 0.f) continue;
                const double on = (b * 16 + s) * stepT + ((s & 1) ? sw * stepT : 0.0);
                seq.addEvent (juce::MidiMessage::noteOn (10, tracks[i].note, v), on);
                seq.addEvent (juce::MidiMessage::noteOff (10, tracks[i].note), on + 90.0);
            }
    seq.sort(); seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (480); mf.addTrack (seq);
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DrumForge")
                 .getChildFile (juce::File::createLegalFileName (fileName) + ".mid");
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (os.openedOk()) mf.writeTo (os);
    return f;
}

// ---------------- audio ----------------
void DrumForgeProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int N = buf.getNumSamples();
    buf.clear();

    struct Trig { int off, trk; float vel; };
    Trig trigs[512]; int nt = 0;
    auto push = [&] (int off, int trk, float vel) { if (nt < 512) trigs[nt++] = { off, trk, vel }; };

    // MIDI in: play a pad by sending the track's note
    for (const auto m : midi)
    {
        auto msg = m.getMessage();
        if (msg.isNoteOn())
            for (int i = 0; i < MaxTracks; ++i)
                if (tracks[i].active && tracks[i].note == msg.getNoteNumber()) push (juce::jlimit (0, N - 1, m.samplePosition), i, msg.getFloatVelocity());
    }

    // sequencer: follow host transport, or the internal Play button
    double bpmNow = bpm.load(), ppq0 = 0.0;
    bool hostPlaying = false, run = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getIsPlaying())
                if (auto p = pos->getPpqPosition())
                {
                    hostPlaying = run = true; ppq0 = *p;
                    if (auto b = pos->getBpm()) bpmNow = *b;
                }
    const bool internal = ! hostPlaying && internalPlay.load();
    if (internal) { run = true; ppq0 = internalPpq; }

    if (run)
    {
        const double perSamp = bpmNow / (60.0 * sr), ppq1 = ppq0 + N * perSamp;
        const double sw = swing.load();
        for (long n = (long) std::floor (ppq0 * 4.0) - 1; n <= (long) std::ceil (ppq1 * 4.0); ++n)
        {
            const double tp = n * 0.25 + ((n & 1) ? sw * 0.25 : 0.0);
            if (tp < ppq0 || tp >= ppq1) continue;
            const int off = juce::jlimit (0, N - 1, (int) ((tp - ppq0) / perSamp));
            const int s = (int) (((n % 16) + 16) % 16);
            currentStep = s;
            for (int i = 0; i < MaxTracks; ++i)
                if (tracks[i].active && ! tracks[i].mute)
                {
                    const float v = tracks[i].steps[s].load();
                    if (v > 0.f) push (off, i, v);
                }
        }
        if (internal) internalPpq += N * perSamp;
    }
    else { currentStep = -1; if (! internalPlay.load()) internalPpq = 0.0; }

    std::sort (trigs, trigs + nt, [] (const Trig& a, const Trig& b) { return a.off < b.off; });

    // per-track output routing: own bus if enabled in the host, otherwise Main
    float* oL[MaxTracks]; float* oR[MaxTracks]; float gain[MaxTracks];
    auto mainB = getBusBuffer (buf, false, 0);
    for (int i = 0; i < MaxTracks; ++i)
    {
        const bool own = (i + 1) < getBusCount (false) && getBus (false, i + 1)->isEnabled();
        auto b = own ? getBusBuffer (buf, false, i + 1) : mainB;
        oL[i] = b.getWritePointer (0);
        oR[i] = b.getNumChannels() > 1 ? b.getWritePointer (1) : b.getWritePointer (0);
        gain[i] = tracks[i].vol.load();
    }

    auto render = [&] (int a, int b)
    {
        for (int i = 0; i < MaxTracks; ++i)
        {
            auto& v = voices[i];
            if (! v.active) continue;
            for (int s = a; s < b; ++s)
            {
                float l, r;
                if (! v.next (l, r)) break;
                oL[i][s] += l * gain[i];
                oR[i][s] += r * gain[i];
            }
        }
    };

    int pos = 0;
    for (int k = 0; k < nt; ++k)
    {
        render (pos, trigs[k].off); pos = trigs[k].off;
        auto& tr = tracks[trigs[k].trk];
        if (tr.type == Hat)                      // closed hat chokes open hats
            for (int j = 0; j < MaxTracks; ++j) if (tracks[j].type == OHat) voices[j].active = false;
        std::shared_ptr<juce::AudioBuffer<float>> smp;
        { juce::SpinLock::ScopedTryLockType l (tr.sampleLock); if (l.isLocked()) smp = tr.sample; }
        voices[trigs[k].trk].trigger (tr.type, trigs[k].vel, sr, smp);
    }
    render (pos, N);
}

// ---------------- state ----------------
void DrumForgeProcessor::getStateInformation (juce::MemoryBlock& d)
{
    juce::XmlElement x ("DrumForge");
    x.setAttribute ("genre", genreIdx.load()); x.setAttribute ("bpm", (double) bpm.load());
    x.setAttribute ("swing", (double) swing.load()); x.setAttribute ("density", (double) density.load());
    for (int i = 0; i < MaxTracks; ++i)
    {
        auto& t = tracks[i];
        if (! t.active) continue;
        auto* e = x.createNewChildElement ("T");
        e->setAttribute ("i", i); e->setAttribute ("name", t.name); e->setAttribute ("type", t.type);
        e->setAttribute ("note", t.note); e->setAttribute ("vol", (double) t.vol.load());
        e->setAttribute ("mute", t.mute.load() ? 1 : 0); e->setAttribute ("path", t.samplePath);
        juce::StringArray sa; for (int s = 0; s < 16; ++s) sa.add (juce::String (t.steps[s].load(), 2));
        e->setAttribute ("steps", sa.joinIntoString (","));
    }
    copyXmlToBinary (x, d);
}

void DrumForgeProcessor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size);
    if (x == nullptr || ! x->hasTagName ("DrumForge")) return;
    for (int i = 0; i < MaxTracks; ++i) { tracks[i].active = false; clearSample (i); }
    genreIdx = x->getIntAttribute ("genre", 0);
    bpm = (float) x->getDoubleAttribute ("bpm", 90.0); swing = (float) x->getDoubleAttribute ("swing", 0.1);
    density = (float) x->getDoubleAttribute ("density", 1.0);
    for (auto* e : x->getChildIterator())
    {
        const int i = e->getIntAttribute ("i", -1);
        if (i < 0 || i >= MaxTracks) continue;
        auto& t = tracks[i];
        t.name = e->getStringAttribute ("name"); t.type = e->getIntAttribute ("type");
        t.note = e->getIntAttribute ("note", 36); t.vol = (float) e->getDoubleAttribute ("vol", 0.85);
        t.mute = e->getIntAttribute ("mute") != 0;
        juce::StringArray sa; sa.addTokens (e->getStringAttribute ("steps"), ",", "");
        for (int s = 0; s < 16; ++s) t.steps[s] = s < sa.size() ? sa[s].getFloatValue() : 0.f;
        auto p = e->getStringAttribute ("path");
        if (p.isNotEmpty() && juce::File (p).existsAsFile()) loadSample (i, juce::File (p));
        t.active = true;
    }
    ++layoutVersion;
}

juce::AudioProcessorEditor* DrumForgeProcessor::createEditor() { return new DrumForgeEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DrumForgeProcessor(); }
