# Drum Forge (VST3 drum generator)

Genre-based drum pattern generator with synth drums, per-instrument multi-outputs
(Out 1..15 + Main), MIDI export by drag & drop, and drop-in one-shot samples.

## Build (Windows)
1. Install: Visual Studio 2022 (workload "Desktop development with C++"), CMake, Git.
2. In this folder:
   ```
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
   (first run downloads JUCE, takes a few minutes)
3. Output: `build\DrumForge_artefacts\Release\VST3\Drum Forge.vst3`
   and `...\Standalone\Drum Forge.exe` (use the Standalone to test quickly).
4. Copy the `.vst3` folder to `C:\Program Files\Common Files\VST3\`.

## FL Studio
Options > Manage plugins > Find installed plugins (verify scan folder).
Add "Drum Forge" in the Channel Rack, open the plugin wrapper settings (gear icon)
and enable/assign the extra outputs to mixer inserts (menu names differ per FL version).
Out 1 = Kick, Out 2 = Snare, Out 3 = Closed Hat ... (slot order). A track whose output
is not enabled in the host plays on Main.

## Notes
- Sequencer follows the host transport. The Play button in the plugin plays without the host.
- Send the track's MIDI note to play the pad live (Kick 36, Snare 38, Hat 42, Open Hat 46, Clap 39 ...).
- Closed hat chokes open hat.
