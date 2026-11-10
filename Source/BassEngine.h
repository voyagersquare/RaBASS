#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace rabass
{
constexpr double pi = 3.14159265358979323846;
struct Parameters
{
    bool poly = false;
    float mix = 0.35f, detune = 7.0f, sub = 0.4f;
    float sawLevel = 0.75f, triangleLevel = 0.2f, sineLevel = 0.15f;
    float cutoff = 700.0f, resonance = 0.2f, filterEnv = 3.0f, filterDecay = 0.25f;
    float attack = 0.005f, decay = 0.25f, sustain = 0.65f, release = 0.15f;
    float drive = 0.2f, glide = 0.07f, outputDb = -9.0f;
};

class Envelope
{
public:
    enum class Stage { off, attack, decay, sustain, release };
    void reset() noexcept { value = 0; stage = Stage::off; }
    void on(bool restart) noexcept { if (restart) value = 0; stage = Stage::attack; }
    void off(float seconds, double rate) noexcept
    {
        if (stage == Stage::off) return;
        releaseStep = value / std::max(1.0, seconds * rate);
        stage = Stage::release;
    }
    float next(float attack, float decay, float sustain, double rate) noexcept
    {
        switch (stage)
        {
            case Stage::attack:
                value += static_cast<float>(1.0 / std::max(1.0, attack * rate));
                if (value >= 1) { value = 1; stage = Stage::decay; }
                break;
            case Stage::decay:
                value -= static_cast<float>((1.0 - sustain) / std::max(1.0, decay * rate));
                if (value <= sustain) { value = sustain; stage = Stage::sustain; }
                break;
            case Stage::sustain: value = sustain; break;
            case Stage::release:
                value -= releaseStep;
                if (value <= 0) reset();
                break;
            case Stage::off: break;
        }
        return value;
    }
    bool active() const noexcept { return stage != Stage::off; }
    bool releasing() const noexcept { return stage == Stage::release; }
private:
    Stage stage = Stage::off;
    float value = 0, releaseStep = 0;
};

class BassEngine
{
public:
    void prepare(double rate) noexcept
    {
        sampleRate = std::clamp(rate, 8000.0, 384000.0);
        smoothCoefficient = static_cast<float>(1 - std::exp(-1 / (0.01 * sampleRate)));
        reset();
    }
    void reset() noexcept
    {
        for (auto& v : voices) v = Voice{};
        for (auto& channel : held) channel.fill(Held{});
        pedals.fill(false); bends.fill(0); serial = 0;
        dcInput = dcOutput = 0;
        smooth = target;
    }
    void setParameters(const Parameters& p) noexcept
    {
        if (p.poly != target.poly) reset();
        target = p;
    }
    void noteOn(int channel, int note, float velocity) noexcept
    {
        if (!valid(channel, note)) return;
        if (velocity <= 0) { noteOff(channel, note); return; }
        auto& h = held[static_cast<size_t>(channel - 1)][static_cast<size_t>(note)];
        h = {true, std::clamp(velocity, 0.0f, 1.0f), ++serial};
        if (!target.poly)
        {
            auto& v = voices[0];
            const bool legato = v.amp.active() && !v.amp.releasing();
            start(v, channel, note, velocity, legato);
            return;
        }
        Voice* selected = nullptr;
        for (auto& v : voices)
        {
            if (v.note == note && v.channel == channel && v.amp.active()) { selected = &v; break; }
            if (!v.amp.active()) selected = &v;
        }
        if (selected == nullptr)
        {
            // Release tails are stolen first, then the oldest held voice.
            selected = &voices[0];
            for (auto& v : voices)
                if ((v.amp.releasing() && !selected->amp.releasing()) ||
                    (v.amp.releasing() == selected->amp.releasing() && v.age < selected->age)) selected = &v;
        }
        start(*selected, channel, note, velocity, false);
    }
    void noteOff(int channel, int note) noexcept
    {
        if (!valid(channel, note)) return;
        held[static_cast<size_t>(channel - 1)][static_cast<size_t>(note)].down = false;
        if (!target.poly)
        {
            auto& v = voices[0];
            if (v.note != note || v.channel != channel) return;
            int nextChannel = 0, nextNote = 0;
            if (lastHeld(nextChannel, nextNote))
                start(v, nextChannel, nextNote, held[static_cast<size_t>(nextChannel-1)][static_cast<size_t>(nextNote)].velocity, true);
            else if (!pedals[static_cast<size_t>(channel - 1)]) releaseVoice(v);
            return;
        }
        if (!pedals[static_cast<size_t>(channel - 1)])
            for (auto& v : voices) if (v.note == note && v.channel == channel) releaseVoice(v);
    }
    void sustainPedal(int channel, bool down) noexcept
    {
        if (channel < 1 || channel > 16) return;
        pedals[static_cast<size_t>(channel - 1)] = down;
        if (!down)
            for (auto& v : voices)
                if (v.channel == channel && v.note >= 0 && !held[static_cast<size_t>(channel - 1)][static_cast<size_t>(v.note)].down)
                    releaseVoice(v);
    }
    void pitchWheel(int channel, int value) noexcept
    {
        if (channel >= 1 && channel <= 16)
            bends[static_cast<size_t>(channel - 1)] = 2.0f * static_cast<float>(std::clamp(value,0,16383) - 8192) / 8192.0f;
    }
    void allNotesOff(int channel, bool immediate) noexcept
    {
        if (channel < 1 || channel > 16) return;
        held[static_cast<size_t>(channel - 1)].fill(Held{});
        pedals[static_cast<size_t>(channel - 1)] = false;
        for (auto& v : voices)
            if (v.channel == channel) { if (immediate) v = Voice{}; else releaseVoice(v); }
        if (!target.poly && !immediate)
        {
            int c = 0, n = 0;
            if (lastHeld(c,n)) start(voices[0], c,n,held[static_cast<size_t>(c-1)][static_cast<size_t>(n)].velocity,true);
        }
    }
    void render(float* left, float* right, int count) noexcept
    {
        const double internalRate = sampleRate * 2.0;
        const auto dcPole = static_cast<float>(std::exp(-2 * pi * 15 / sampleRate));
        for (int i=0; i<count; ++i)
        {
            smoothParameter(smooth.cutoff, target.cutoff);
            smoothParameter(smooth.resonance, target.resonance);
            smoothParameter(smooth.mix, target.mix);
            smoothParameter(smooth.detune, target.detune);
            smoothParameter(smooth.sub, target.sub);
            smoothParameter(smooth.sawLevel, target.sawLevel);
            smoothParameter(smooth.triangleLevel, target.triangleLevel);
            smoothParameter(smooth.sineLevel, target.sineLevel);
            smoothParameter(smooth.drive, target.drive);
            smoothParameter(smooth.outputDb, target.outputDb);
            float sum = 0;
            for (auto& v : voices)
            {
                if (!v.amp.active()) continue;
                const float glideStep = target.glide > 0.001f ? static_cast<float>(1 - std::exp(-1 / (target.glide * sampleRate))) : 1.0f;
                v.pitch += (static_cast<float>(v.note) - v.pitch) * glideStep;
                const double hz = 440 * std::exp2((v.pitch + bends[static_cast<size_t>(v.channel - 1)] - 69) / 12.0);
                const double d1 = std::min(0.45, hz / internalRate);
                const double d2 = std::min(0.45, d1 * std::exp2(smooth.detune / 1200.0));
                const float amp = v.amp.next(target.attack, target.decay, target.sustain, sampleRate);
                const float env = v.filter.next(0.001f, target.filterDecay, 0.0f, sampleRate);
                const double cutoff = std::clamp<double>(smooth.cutoff * std::exp2(target.filterEnv * env), 20.0, std::min(18000.0, sampleRate * 0.4));
                const double g = std::tan(pi * cutoff / internalRate);
                const double k = 2.0 - 1.8 * smooth.resonance;
                const double a = 1 / (1 + g * (g + k));
                float filtered = 0;
                for (int oversample = 0; oversample < 2; ++oversample)
                {
                    const double levelScale = 1.0 / std::max(1.0f, smooth.sawLevel + smooth.triangleLevel + smooth.sineLevel);
                    const double osc = levelScale * (oscillatorMix(v.phase1,d1,smooth) * (1 - smooth.mix)
                        + oscillatorMix(v.phase2,d2,smooth) * smooth.mix);
                    const double raw = (osc + smooth.sub * std::sin(2*pi*v.subPhase)) / (1+smooth.sub);
                    const double driven = std::tanh(raw * (1 + 7 * smooth.drive));
                    const double band = a * (v.ic1 + g * (driven - v.ic2));
                    const double low = v.ic2 + g * band;
                    v.ic1 = 2 * band - v.ic1;
                    v.ic2 = 2 * low - v.ic2;
                    filtered += static_cast<float>(low * 0.5);
                    advance(v.phase1,d1); advance(v.phase2,d2); advance(v.subPhase,d1*0.5);
                }
                sum += filtered * amp * v.velocity * 0.7f;
                if (!v.amp.active()) v = Voice{};
            }
            const float blocked = sum - dcInput + dcPole * dcOutput;
            dcInput = sum; dcOutput = std::abs(blocked) < 1.0e-20f ? 0 : blocked;
            const auto output = static_cast<float>(std::tanh(blocked * std::pow(10.0, smooth.outputDb / 20.0)));
            left[i] += output;
            if (right != nullptr) right[i] += output;
        }
    }
private:
    struct Held { bool down = false; float velocity = 0; uint64_t order = 0; };
    struct Voice
    {
        int note = -1, channel = 1;
        uint64_t age = 0;
        float pitch = 0, velocity = 0;
        double phase1 = 0, phase2 = 0.23, subPhase = 0, ic1 = 0, ic2 = 0;
        Envelope amp, filter;
    };
    static bool valid(int c, int n) noexcept { return c>=1 && c<=16 && n>=0 && n<128; }
    void smoothParameter(float& value, float destination) const noexcept { value += smoothCoefficient * (destination-value); }
    static void advance(double& p, double increment) noexcept { p += increment; if (p>=1) p-=1; }
    static double blep(double t, double dt) noexcept
    {
        if (t<dt) { t/=dt; return t+t-t*t-1; }
        if (t>1-dt) { t=(t-1)/dt; return t*t+t+t+1; }
        return 0;
    }
    static double oscillatorMix(double phase, double dt, const Parameters& p) noexcept
    {
        const double saw = 2*phase-1-blep(phase,dt);
        const double triangle = 1-4*std::abs(phase-0.5);
        const double sine = std::sin(2*pi*phase);
        return p.sawLevel*saw + p.triangleLevel*triangle + p.sineLevel*sine;
    }
    bool lastHeld(int& channel, int& note) const noexcept
    {
        uint64_t newest = 0;
        for (size_t c=0;c<held.size();++c)
            for (size_t n=0;n<held[c].size();++n)
                if (held[c][n].down && held[c][n].order>newest)
                { newest=held[c][n].order; channel=static_cast<int>(c)+1; note=static_cast<int>(n); }
        return newest!=0;
    }
    void start(Voice& v, int channel, int note, float velocity, bool legato) noexcept
    {
        if (!legato)
        {
            v = Voice{};
            v.pitch = static_cast<float>(note);
            v.amp.on(true); v.filter.on(true);
        }
        v.note=note; v.channel=channel; v.velocity=velocity; v.age=serial;
    }
    void releaseVoice(Voice& v) noexcept
    {
        if (!v.amp.releasing())
        { v.amp.off(target.release,sampleRate); v.filter.off(target.release,sampleRate); }
    }
    std::array<Voice,8> voices{};
    std::array<std::array<Held,128>,16> held{};
    std::array<bool,16> pedals{};
    std::array<float,16> bends{};
    Parameters target{}, smooth{};
    uint64_t serial = 0;
    double sampleRate = 44100;
    float smoothCoefficient = 0.002f, dcInput = 0, dcOutput = 0;
};
}
