#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include "DrumVoice.h"

// Pattern strings: 16 chars, one per 16th step.  x = always, 1-9 = probability (x/10), . = silent
// An EMPTY string means "random sparse pattern".
struct Genre
{
    juce::String name, group;
    float bpm = 100.f, sw = 0.f;
    std::array<juce::String, NumTypes> p;
};

inline std::vector<Genre> buildGenres()
{
    std::vector<Genre> v;
    const char* E = "................";
    using P = std::initializer_list<std::pair<int, const char*>>;
    auto find = [&] (const juce::String& n) -> const Genre* { for (auto& g : v) if (g.name == n) return &g; return nullptr; };
    auto mk = [&] (const char* n, float bpm, float sw, P pats)
    { Genre g; g.name = n; g.bpm = bpm; g.sw = sw; for (auto& s : g.p) s = E;
      for (auto& q : pats) g.p[(size_t) q.first] = q.second; v.push_back (g); };
    auto al = [&] (const char* n, const char* base, float bpm, float sw, P pats)
    { Genre g; if (auto* b = find (base)) g = *b; g.name = n; g.bpm = bpm; g.sw = sw;
      for (auto& q : pats) g.p[(size_t) q.first] = q.second; v.push_back (g); };

    // ---- base genres ----
    mk ("Hip-Hop", 90, .14f, {{Kick,"x......4..5....2"},{Snare,"....x.......x..."},{Hat,"x3x3x3x3x3x3x3x4"},{OHat,"..............4."},{Clap,"....5.......5..."},{Rim,"..3....3..3....."},{Shaker,"4343434343434343"},{Perc,".2..2..2..2..2.."},{Sub,"x......4..5....2"},{Tom,"............2.3."}});
    mk ("Trap", 140, 0, {{Kick,"x..3..3..2.4...2"},{Snare,"........x......."},{Hat,"xx3xxx3xxx4xxx5x"},{OHat,"......3.......4."},{Clap,"........x......."},{Rim,".......2.......3"},{Shaker,"3.3.3.3.3.3.3.3."},{Perc,"..2...2...2...2."},{Sub,"x..3..3..2.4...2"},{Tom,"..............3."}});
    mk ("House", 124, .05f, {{Kick,"x...x...x...x..."},{Snare,"............2..."},{Hat,"3.x.3.x.3.x.3.x."},{OHat,"..x...x...x...x."},{Clap,"....x.......x..."},{Rim,".2..3..2.3..2..."},{Shaker,"x3x3x3x3x3x3x3x3"},{Perc,".3..2.3...2..3.."},{Sub,"..x...x...x...x."},{Tom,".......2......3."}});
    mk ("Techno", 132, 0, {{Kick,"x...x...x...x..."},{Snare,"....3.......3..."},{Hat,"..x...x...x...x."},{OHat,"..4...5...4...5."},{Clap,"....5.......5..."},{Rim,".3.3..3..3.3..3."},{Shaker,"5555555555555555"},{Perc,".4.2.4.3.4.2.4.3"},{Sub,"..x...x...x...x."},{Tom,"..3...2...3...4."}});
    mk ("Drum & Bass", 174, 0, {{Kick,"x.........5....2"},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{OHat,"..4...4...4...5."},{Clap,"....3.......3..."},{Rim,".2..2..2..2..2.."},{Shaker,"3333333333333333"},{Perc,".3..2..3..2..3.."},{Sub,"x.........4....."},{Tom,".........2....3."}});
    mk ("Reggaeton", 95, 0, {{Kick,"x...x...x...x..."},{Snare,"...x..x....x..x."},{Hat,"x.x.x.x.x.x.x.x."},{OHat,".......3.......4"},{Clap,"...3..3....3..3."},{Rim,"..2...2...2...2."},{Shaker,"4.4.4.4.4.4.4.4."},{Perc,".3..2..3..2..3.."},{Sub,"x..2x...x..2x..."},{Tom,"..............3."}});
    mk ("Lo-Fi", 78, .22f, {{Kick,"x.....3...x3...."},{Snare,"....x.......x..."},{Hat,"x.4.x.4.x.4.x.4."},{OHat,"......3.......3."},{Clap,"....3.......3..."},{Rim,"..3.....3......."},{Shaker,"3.3.3.3.3.3.3.3."},{Perc,".2.....2....2..."},{Sub,"x.....3...x3...."},{Tom,"...........3...."}});
    mk ("Afrobeats", 102, .08f, {{Kick,"x..3..4...5.3..."},{Snare,"....x.......x..."},{Hat,"x.xxx.xxx.xxx.xx"},{OHat,"..4...4...4...4."},{Clap,"....4.......4..."},{Rim,"..x..x..x.x..x.."},{Shaker,"x3x3x3x3x3x3x3x3"},{Perc,".3.x..3..x.3..x."},{Sub,"x..3..4...5.3..."},{Tom,"..2...2...3....."}});
    mk ("Baila", 112, .06f, {{Kick,"x.....x.x.....x."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{OHat,"..3...3...3...3."},{Clap,"....4.......4..."},{Rim,"..x.3.x.3.x.3.x."},{Shaker,"xxxxxxxxxxxxxxxx"},{Perc,".3.x.3.x.3.x.3.x"},{Sub,"x.....x.x.....x."},{Tom,".......2......3."}});
    mk ("Rock", 110, 0, {{Kick,"x.....3.x.3....."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{OHat,"..............3."},{Tom,"............2.3."}});
    mk ("Metal", 160, 0, {{Kick,"xx.xx.x.xx.xx.x."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."}});
    mk ("Punk", 180, 0, {{Kick,"x.x...x.x.x...x."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."}});
    mk ("Jazz", 120, .3f, {{Kick,"2.....2.....2..."},{Snare,"..3...3...2...3."},{Hat,"x..xx..xx..xx..x"},{OHat,"....3.......3..."}});
    mk ("Blues", 82, .3f, {{Kick,"x.....3.x......."},{Snare,"....x.......x..."},{Hat,"x.xxx.xxx.xxx.xx"}});
    mk ("Funk", 105, .1f, {{Kick,"x..x..3..x.x..3."},{Snare,"....x..3.3..x..3"},{Hat,"xxxxxxxxxxxxxxxx"},{OHat,"..4...4...4...4."}});
    mk ("Reggae", 76, .1f, {{Kick,"........x......."},{Snare,"........x......."},{Rim,"........x......."},{Hat,"x.x.x.x.x.x.x.x."}});
    mk ("Ska", 165, 0, {{Kick,"x...x...x...x..."},{Snare,"....x.......x..."},{Hat,"..x...x...x...x."}});
    mk ("Country", 100, .05f, {{Kick,"x.......x......."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{Rim,"..x...x...x...x."}});
    mk ("Folk", 92, .05f, {{Kick,"x.......x......."},{Snare,"....3.......3..."},{Shaker,"x.x.x.x.x.x.x.x."}});
    mk ("Gospel", 90, .2f, {{Kick,"x.....3.x.3...3."},{Snare,"....x.......x..."},{Clap,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."}});
    mk ("Dubstep", 140, 0, {{Kick,"x.........3....."},{Snare,"........x......."},{Hat,"x.x.x.x.x.x.x.x."},{Sub,"x.........3....."}});
    mk ("Ambient", 70, 0, {{Kick,"3..............."},{Hat,"..2...2...2...2."},{Perc,".2.....2....2..."},{Tom,"..............2."}});
    mk ("Downtempo", 90, .15f, {{Kick,"x.....3...x....."},{Snare,"........x......."},{Hat,"x.3.x.3.x.3.x.3."},{Shaker,"3.3.3.3.3.3.3.3."}});
    mk ("Electro", 128, 0, {{Kick,"x..x..x...x...x."},{Snare,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{Clap,"....x.......x..."}});
    mk ("Synthwave", 100, 0, {{Kick,"x...x...x...x..."},{Snare,"....x.......x..."},{Clap,"....x.......x..."},{Hat,"x.x.x.x.x.x.x.x."},{Tom,"..........3.3.3."}});
    mk ("Breakbeat", 130, .05f, {{Kick,"x.....x...x...3."},{Snare,"....x.......x..3"},{Hat,"x.x.x.x.x.x.x.x."},{OHat,"......3.......3."}});
    mk ("Orchestral", 90, 0, {{Kick,"x.......2......."},{Tom,"x.......3.....3."},{Snare,"..............3."},{Perc,".2.....2.....2.."}});
    mk ("Tribal", 105, .05f, {{Tom,"x.x..x.x..x.x..."},{Kick,"x.......x......."},{Perc,".x.x.x..x.x.x.x."},{Shaker,"x3x3x3x3x3x3x3x3"}});
    mk ("Cinematic", 85, 0, {{Kick,"x.......x......."},{Snare,"........x......."},{Tom,"x..3....x.3....."},{Perc,".......3.......3"}});

    // ---- aliases / variations ----
    al ("Pop", "Rock", 108, 0, {{Clap,"....5.......5..."},{OHat,"......3.......3."}});
    al ("Rap", "Hip-Hop", 95, .08f, {});
    al ("R&B", "Hip-Hop", 70, .15f, {{Kick,"x.....3..x3....."},{Clap,"....4.......4..."},{Hat,"x.x.xxx.x.x.x.xx"}});
    al ("Soul", "Funk", 92, .15f, {{Hat,"x.x.x.x.x.x.x.x."},{OHat,"..3...3...3...3."}});
    al ("Disco", "House", 118, .05f, {{Hat,"x.x.x.x.x.x.x.x."},{Clap,"....x.......x..."}});
    al ("Indie", "Rock", 122, .03f, {});
    al ("Alternative", "Rock", 112, 0, {{Kick,"x...x...x.3.x..."}});
    al ("Grunge", "Rock", 96, 0, {{Kick,"x.....x.x...3..."}});
    al ("EDM", "House", 128, 0, {{Clap,"....x.......x..."}});
    al ("Deep House", "House", 120, .08f, {{Hat,"3.3.3.3.3.3.3.3."}});
    al ("Progressive House", "House", 126, 0, {});
    al ("Trance", "House", 138, 0, {{Hat,"x.x.x.x.x.x.x.x."}});
    al ("Psytrance", "House", 145, 0, {{Sub,".xxx.xxx.xxx.xxx"}});
    al ("Future Bass", "Dubstep", 150, 0, {{Kick,"x.....3...x.3..."},{Snare,E},{Clap,"........x......."}});
    al ("Hardstyle", "House", 150, 0, {{Sub,".xxx.xxx.xxx.xxx"}});
    al ("Hardcore", "House", 180, 0, {{Kick,"x..3x..3x..3x..3"}});
    al ("Industrial", "Techno", 130, 0, {{Kick,"x...x...x...x..3"},{Perc,".3.3.3.3.3.3.3.3"}});
    al ("Minimal Techno", "Techno", 126, 0, {{Hat,"..x...x...x...x."},{Perc,".3..3..2.3..2..3"}});
    al ("Melodic Techno", "Techno", 122, 0, {{Hat,"..x...x...x...x."}});
    al ("Soundtrack", "Cinematic", 95, 0, {});
    al ("Opera", "Orchestral", 70, 0, {{Kick,"x..............."}});
    al ("Musical Theatre", "Jazz", 120, .2f, {{Kick,"x...x...x...x..."},{Hat,"x.xxx.xxx.xxx.xx"}});
    al ("Choral", "Orchestral", 60, 0, {{Kick,"x..............."},{Tom,"........2......."}});
    al ("Acapella", "Hip-Hop", 100, .1f, {});
    al ("Experimental", "", 100, 0, {});            // empty patterns = random
    al ("New Age", "Ambient", 80, 0, {});
    al ("Meditation Music", "Ambient", 60, 0, {});
    al ("Spiritual Music", "Tribal", 80, 0, {});
    al ("World Fusion", "Afrobeats", 108, .1f, {});
    al ("New Wave", "Rock", 128, 0, {{Kick,"x...x...x...x..."},{Tom,"..............3."}});

    struct Grp { const char* name; std::vector<const char*> items; };
    const std::vector<Grp> groups = {
        { "Popular", { "Pop","Hip-Hop","Rap","R&B","Rock","Metal","Jazz","Blues","Soul","Funk","Reggae","Country","Folk","Gospel","Disco","Punk","Indie","Alternative","Grunge","Ska","Reggaeton","Afrobeats","Baila" } },
        { "Electronic", { "EDM","Techno","House","Deep House","Progressive House","Trance","Psytrance","Drum & Bass","Dubstep","Future Bass","Hardstyle","Hardcore","Trap","Ambient","Downtempo","Electro","Synthwave","Industrial","Minimal Techno","Melodic Techno","Breakbeat" } },
        { "Other", { "Orchestral","Cinematic","Soundtrack","Opera","Musical Theatre","Choral","Acapella","Experimental","New Age","Meditation Music","Spiritual Music","World Fusion","Tribal","Lo-Fi","New Wave" } } };

    std::vector<Genre> out;
    for (auto& gr : groups)
        for (auto n : gr.items)
            for (auto& g : v)
                if (g.name == n) { Genre c = g; c.group = gr.name; out.push_back (c); break; }
    return out;
}

inline const std::vector<Genre>& genres() { static const std::vector<Genre> g = buildGenres(); return g; }

inline std::array<float, 16> generatePattern (const Genre& g, int type, float density, juce::Random& r)
{
    std::array<float, 16> out {};
    const juce::String& s = g.p[(size_t) type];
    for (int i = 0; i < 16; ++i)
    {
        if (s.length() < 16) { out[(size_t) i] = r.nextFloat() < 0.12f * density ? 0.85f + r.nextFloat() * 0.15f : 0.f; continue; }
        const juce::juce_wchar c = s[i];
        if (c == 'x') out[(size_t) i] = 0.85f + r.nextFloat() * 0.15f;
        else if (c >= '1' && c <= '9')
            out[(size_t) i] = r.nextFloat() < juce::jmin (1.f, (float) (c - '0') / 10.f * density) ? 0.55f + r.nextFloat() * 0.35f : 0.f;
    }
    return out;
}
