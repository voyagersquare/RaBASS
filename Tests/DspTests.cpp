#include "BassEngine.h"
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace
{
void require(bool ok,const char* message)
{
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
std::vector<float> render(rabass::BassEngine& engine,int samples)
{
    std::vector<float> left(static_cast<size_t>(samples)),right(static_cast<size_t>(samples));
    engine.render(left.data(),right.data(),samples);
    for (size_t i=0;i<left.size();++i)
    {
        require(std::isfinite(left[i]) && std::abs(left[i])<=1.0f,"finite bounded output");
        require(left[i]==right[i],"phase coherent stereo bass");
    }
    return left;
}
double energy(const std::vector<float>& v)
{
    double e=0; for (auto x:v) e+=x*x; return e/v.size();
}
double frequency(const std::vector<float>& data,double rate)
{
    int crossings=0;
    for (size_t i=1;i<data.size();++i) if (data[i-1]<=0 && data[i]>0) ++crossings;
    return crossings*rate/static_cast<double>(data.size());
}
}
int main()
{
    rabass::Parameters p;
    rabass::BassEngine a,b;
    a.prepare(48000); b.prepare(48000);
    require(energy(render(a,1000))==0,"silence before MIDI");
    a.noteOn(1,36,1);
    require(energy(render(a,48000))>.0001,"MIDI generates bass");
    a.noteOff(1,36); render(a,48000);
    require(energy(render(a,4096))<1e-12,"release ends without stuck note");

    a.reset(); b.reset();
    a.noteOn(1,48,.8f); b.noteOn(1,48,.8f); render(a,4000); render(b,4000);
    a.noteOn(1,55,.9f); b.noteOn(1,55,.9f); render(a,4000); render(b,4000);
    a.noteOff(1,55); b.noteOn(1,48,.8f);
    require(render(a,12000)==render(b,12000),"mono returns to last held note with legato glide");

    a.reset(); b.reset(); a.noteOn(1,40,1); b.noteOn(1,40,1);
    a.sustainPedal(1,true); a.noteOff(1,40);
    require(render(a,24000)==render(b,24000),"pedal holds released note");
    a.sustainPedal(1,false); render(a,48000);
    require(energy(render(a,4096))<1e-12,"pedal release clears sustained note");

    a.reset(); b.reset(); a.noteOn(2,40,1); b.noteOn(2,40,1);
    a.noteOff(1,40); a.allNotesOff(1,true);
    require(render(a,12000)==render(b,12000),"MIDI channels are isolated");
    a.allNotesOff(2,true); render(a,48000);
    require(energy(render(a,4096))<1e-12,"all sound off clears voices");

    p.sawLevel=0; p.triangleLevel=1; p.sineLevel=0;
    p.sub=0; p.mix=0; p.detune=0; p.cutoff=18000; p.filterEnv=0;
    p.resonance=0; p.drive=0; p.outputDb=-3;
    a.setParameters(p); a.prepare(48000); a.noteOn(1,45,1); render(a,48000);
    require(std::abs(frequency(render(a,48000),48000)-110)<2,"concert tuning A2 110 Hz");
    a.pitchWheel(1,16383); render(a,4800);
    require(std::abs(frequency(render(a,48000),48000)-123.47)<2,"pitch bend plus two semitones");

    a.prepare(48000); b.setParameters(p); b.prepare(48000);
    a.noteOn(1,45,1); b.noteOn(1,45,1);
    const auto contiguous=render(a,4096);
    std::vector<float> fragmented;
    for (int i=0;i<64;++i) { const auto part=render(b,64); fragmented.insert(fragmented.end(),part.begin(),part.end()); }
    require(contiguous==fragmented,"render independent of host block size");

    // Each waveform must work alone; the mixture must change the signal.
    std::array<std::vector<float>,3> solo;
    for (size_t shape=0;shape<solo.size();++shape)
    {
        p.sawLevel=shape==0 ? 1.0f : 0.0f;
        p.triangleLevel=shape==1 ? 1.0f : 0.0f;
        p.sineLevel=shape==2 ? 1.0f : 0.0f;
        a.setParameters(p); a.prepare(48000); a.noteOn(1,45,1);
        render(a,48000); solo[shape]=render(a,48000);
        require(energy(solo[shape])>.0001,"each waveform produces sound alone");
        require(std::abs(frequency(solo[shape],48000)-110)<2,"each waveform has the same fundamental pitch");
    }
    require(solo[0]!=solo[1] && solo[0]!=solo[2] && solo[1]!=solo[2],"waveforms have distinct signals");
    p.sawLevel=.7f; p.triangleLevel=.4f; p.sineLevel=.3f;
    a.setParameters(p); a.prepare(48000); a.noteOn(1,45,1); render(a,48000);
    const auto mixed=render(a,48000);
    require(energy(mixed)>.0001 && mixed!=solo[0] && mixed!=solo[1] && mixed!=solo[2],"three-wave mixer combines all levels");
    p.sawLevel=0; p.triangleLevel=0; p.sineLevel=0;
    a.setParameters(p); a.prepare(48000); a.noteOn(1,45,1);
    require(energy(render(a,4800))==0,"all oscillator levels zero mutes the main signal");
    p.sub=.6f; a.setParameters(p); a.prepare(48000); a.noteOn(1,45,1);
    require(energy(render(a,48000))>.0001,"original sub remains independent of the three-wave mixer");

    std::mt19937 random(0x524142);
    for (double rate : {8000.0,44100.0,48000.0,96000.0})
    {
        p.poly=true; p.cutoff=18000; p.resonance=1; p.filterEnv=6; p.drive=1; p.outputDb=6;
        p.attack=.001f; p.release=.01f; a.setParameters(p); a.prepare(rate);
        for (int event=0;event<250;++event)
        {
            const int channel=1+static_cast<int>(random()%16), note=static_cast<int>(random()%128);
            switch (random()%5)
            {
                case 0: a.noteOff(channel,note); break;
                case 1: a.pitchWheel(channel,static_cast<int>(random()%16384)); break;
                case 2: a.sustainPedal(channel,(random()%2)!=0); break;
                default: a.noteOn(channel,note,.8f); break;
            }
            p.sawLevel=static_cast<float>(random()%101)/100;
            p.triangleLevel=static_cast<float>(random()%101)/100;
            p.sineLevel=static_cast<float>(random()%101)/100;
            a.setParameters(p); render(a,128);
        }
        for (int c=1;c<=16;++c) a.allNotesOff(c,true);
        render(a,static_cast<int>(rate));
        require(energy(render(a,4096))<1e-12,"poly voice stealing and panic leave no stuck notes");
    }
    std::cout << "PASS: tuning, MIDI, mono priority, pedal, release, polyphony, block sizes and sample rates\n";
}
