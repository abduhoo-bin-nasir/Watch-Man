// Synth.cpp - the sound generators. Each function builds one sound with a few oscillators, noise and filters.
#include "Synth.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
const float TAU = 6.28318530718f;

struct Rng {                                         // small deterministic generator: the same sounds on every machine
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed * 2685821657736338717ULL + 88172645463325252ULL) {}
    uint32_t u32() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (uint32_t)(s >> 11); }
    float uni() { return (float)(u32() & 0xFFFFFF) / 16777216.0f; }          // 0..1
    float sgn() { return uni() * 2.0f - 1.0f; }                                // -1..1
    float range(float a, float b) { return a + (b - a) * uni(); }
};

float alphaFor(float fc) { return 1.0f - std::exp(-TAU * fc / (float)SR); }

struct LP {                                          // one-pole low-pass
    float a = 1, y = 0;
    explicit LP(float fc = 20000) : a(alphaFor(fc)) {}
    void setFc(float fc) { a = alphaFor(fc); }
    float operator()(float x) { y += a * (x - y); return y; }
};
struct HP {                                          // one-pole high-pass
    LP lp;
    explicit HP(float fc = 20) : lp(fc) {}
    float operator()(float x) { return x - lp(x); }
};
struct Pink {                                        // Paul Kellet's pink noise filter
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    float operator()(float w) {
        b0 = 0.99886f * b0 + w * 0.0555179f; b1 = 0.99332f * b1 + w * 0.0750759f; b2 = 0.96900f * b2 + w * 0.1538520f;
        b3 = 0.86650f * b3 + w * 0.3104856f; b4 = 0.55000f * b4 + w * 0.5329522f; b5 = -0.7616f * b5 - w * 0.0168980f;
        float o = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f; b6 = w * 0.115926f;
        return o * 0.11f;
    }
};
struct BP {                                          // resonant band-pass (RBJ biquad)
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    void set(float fc, float q) {
        fc = std::clamp(fc, 30.0f, SR * 0.45f);
        float w = TAU * fc / SR, al = std::sin(w) / (2 * q), a0 = 1 + al;
        b0 = al / a0; b1 = 0; b2 = -al / a0; a1 = -2 * std::cos(w) / a0; a2 = (1 - al) / a0;
    }
    float operator()(float x) {
        float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x; y2 = y1; y1 = y;
        return y;
    }
};

using Vec = std::vector<float>;
Vec zeros(float seconds) { return Vec((size_t)(seconds * SR), 0.0f); }

void normalize(Vec& v, float peak) {
    float m = 1e-9f;
    for (float x : v) m = std::max(m, std::fabs(x));
    float g = peak / m;
    for (float& x : v) x *= g;
}
// Makes a loop: 'v' holds the loop plus 'fade' extra seconds of the same signal; the extra part is mixed into the start so the end
// flows straight into the beginning.
Vec makeLoop(const Vec& v, float fadeSeconds) {
    size_t f = (size_t)(fadeSeconds * SR), n = v.size() - f;
    Vec out(v.begin(), v.begin() + (long)n);
    for (size_t i = 0; i < f; i++) {
        float w = (float)i / (float)f;
        out[i] = v[i] * w + v[n + i] * (1.0f - w);
    }
    return out;
}
Buf pack(Vec v) { return std::make_shared<const Vec>(std::move(v)); }

// add a decaying sine burst at sample 'at' (wraps around the end, so tails of loops flow into their start)
void addTone(Vec& out, long at, float freq, float amp, float decay, float seconds, bool wrap = false) {
    long n = (long)(seconds * SR), N = (long)out.size();
    for (long i = 0; i < n; i++) {
        long idx = at + i;
        if (idx >= N) { if (!wrap) break; idx %= N; }
        if (idx < 0) continue;
        float t = (float)i / SR;
        out[(size_t)idx] += amp * std::sin(TAU * freq * t) * std::exp(-t / decay);
    }
}

// ---------------------------------------------------------------- weather
Buf rainWindow() {
    Rng r(1); Pink pk; LP l1(2300), l2(2300); HP h(220);
    float len = 10.0f, fade = 1.0f; Vec v = zeros(len + fade);
    struct Drop { float ph = 0, f = 0, a = 0, d = 0; }; std::vector<Drop> drops;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        float base = h(l2(l1(pk(r.sgn())))) * (0.9f + 0.12f * std::sin(TAU * 1.0f * t / len * 1.0f) + 0.08f * std::sin(TAU * 3.0f * t / len));
        if (r.uni() < 15.0f / SR) { Drop d; d.f = r.range(2200, 4400); d.a = r.range(0.04f, 0.16f); d.d = r.range(0.002f, 0.005f); drops.push_back(d); }   // a tap on the glass
        float taps = 0;
        for (size_t k = 0; k < drops.size();) {
            Drop& d = drops[k]; d.ph += TAU * d.f / SR; d.a *= std::exp(-1.0f / (d.d * SR));
            taps += d.a * std::sin(d.ph);
            if (d.a < 0.0008f) { drops[k] = drops.back(); drops.pop_back(); } else k++;
        }
        v[i] = base + taps;
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.9f); return pack(o);
}
Buf rainOpen() {
    Rng r(2); HP h(1300); LP l(6500), l2(6500), rumble(700); Pink pk;
    float len = 10.0f, fade = 1.0f; Vec v = zeros(len + fade);
    struct Plink { float ph, f, a, d; }; std::vector<Plink> pl;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        float w = r.sgn();
        float hiss = l2(l(h(w))) * (0.85f + 0.15f * std::sin(TAU * 2.0f * t / len));
        float body = rumble(pk(r.sgn())) * 0.9f;
        if (r.uni() < 32.0f / SR) pl.push_back({0, r.range(500, 2400), r.range(0.03f, 0.12f), r.range(0.006f, 0.02f)});     // puddles
        float p = 0;
        for (size_t k = 0; k < pl.size();) {
            Plink& d = pl[k]; d.ph += TAU * d.f / SR; d.a *= std::exp(-1.0f / (d.d * SR)); p += d.a * std::sin(d.ph);
            if (d.a < 0.0008f) { pl[k] = pl.back(); pl.pop_back(); } else k++;
        }
        v[i] = hiss * 0.8f + body * 0.9f + p;
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.9f); return pack(o);
}
Buf windSound() {
    Rng r(3); BP bp; LP lp(1800);
    float len = 12.0f, fade = 1.5f; Vec v = zeros(len + fade);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        if (i % 64 == 0) bp.set(380.0f + 260.0f * std::sin(TAU * 1.0f * t / len) + 120.0f * std::sin(TAU * 3.0f * t / len + 1.0f), 1.8f);
        float gust = 0.55f + 0.45f * std::sin(TAU * 2.0f * t / len + 0.6f) * std::sin(TAU * 1.0f * t / len + 2.0f);
        v[i] = lp(bp(r.sgn())) * (0.35f + 0.65f * std::max(0.0f, gust)) * 3.0f;
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.8f); return pack(o);
}
Buf thunder(int variant) {
    Rng r((uint64_t)(10 + variant)); float len = 4.6f + 0.7f * variant; Vec v = zeros(len);
    HP crackHp(900); LP rum(400); float brown = 0;
    float bumps[5] = {0.12f + 0.05f * variant, 0.55f, 1.15f + 0.1f * variant, 1.95f, 2.9f}, amps[5] = {1.0f, 0.8f, 0.65f, 0.45f, 0.3f};
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        float w = r.sgn();
        float crack = crackHp(w) * std::exp(-t / (0.035f + 0.01f * variant)) * (variant == 0 ? 1.0f : 0.7f);        // the sharp part
        brown = brown * 0.996f + w * 0.05f;
        rum.setFc(std::max(55.0f, 520.0f * std::exp(-t * 0.9f)));                                                     // the rumble darkens as it rolls away
        float env = 0;
        for (int k = 0; k < 5; k++) { float d = (t - bumps[k]) / 0.35f; env += amps[k] * std::exp(-d * d); }
        float body = rum(brown) * 7.0f * env * std::exp(-t * 0.22f);
        float sub = std::sin(TAU * 38.0f * t) * 0.25f * env * std::exp(-t * 0.35f);
        v[i] = crack * 0.9f + body + sub;
    }
    // fade out the last 0.3 s so it never ends on a click
    for (size_t i = 0; i < (size_t)(0.3f * SR); i++) v[v.size() - 1 - i] *= (float)i / (0.3f * SR);
    normalize(v, 0.95f); return pack(v);
}

// ---------------------------------------------------------------- doors and steps
Buf creak(int variant, bool closing) {
    Rng r((uint64_t)(20 + variant + (closing ? 10 : 0))); float len = closing ? 0.85f : 1.5f; Vec v = zeros(len);
    BP formant; float phase = 0, f0 = 135.0f + 25.0f * variant, slipPh = 0;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR, u = t / len;
        float f = f0 * (1.0f + 0.7f * u * (closing ? -0.5f : 1.0f) + 0.05f * std::sin(TAU * 3.1f * t));
        phase += f / SR; phase -= std::floor(phase);
        float saw = phase * 2 - 1;
        float slipRate = 28.0f + 22.0f * std::sin(TAU * 1.7f * t + variant) + 12.0f * r.sgn() * 0.1f;           // stick-slip: the hinge grabs and lets go
        slipPh += slipRate / SR; slipPh -= std::floor(slipPh);
        float slip = std::pow(1.0f - slipPh, 2.5f);
        if (i % 32 == 0) formant.set(520.0f + 600.0f * u + 150.0f * std::sin(TAU * 2.3f * t), 5.0f);
        float env = std::min(1.0f, t / 0.12f) * std::min(1.0f, (len - t) / 0.25f) * (0.7f + 0.3f * std::sin(TAU * 4.3f * t));
        v[i] = (formant(saw * slip) * 6.0f + r.sgn() * 0.05f * slip) * env;
    }
    if (closing) {                                                               // the latch, right at the end
        long at = (long)v.size() - (long)(0.07f * SR);
        for (long k = 0; k < (long)(0.05f * SR); k++) {
            float t = (float)k / SR; size_t idx = (size_t)(at + k);
            if (idx < v.size()) v[idx] += (r.sgn() * std::exp(-t / 0.003f) * 0.7f + std::sin(TAU * 110.0f * t) * std::exp(-t / 0.03f) * 0.8f);
        }
    }
    normalize(v, 0.8f); return pack(v);
}
Buf knock() {
    Rng r(30); Vec v = zeros(1.0f); LP lp(1300);
    float at[3] = {0.02f, 0.24f, 0.43f};
    for (int k = 0; k < 3; k++) {
        long s0 = (long)(at[k] * SR);
        for (long i = 0; i < (long)(0.2f * SR); i++) {
            float t = (float)i / SR; size_t idx = (size_t)(s0 + i); if (idx >= v.size()) break;
            v[idx] += std::sin(TAU * (96.0f - 30.0f * std::min(t * 8, 1.0f)) * t) * std::exp(-t / 0.05f) * 0.9f + lp(r.sgn()) * std::exp(-t / 0.012f) * 0.7f;
        }
    }
    normalize(v, 0.9f); return pack(v);
}
Buf latch() {
    Rng r(31); Vec v = zeros(0.2f);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        v[i] = r.sgn() * std::exp(-t / 0.002f) * 0.8f + std::sin(TAU * 140.0f * t) * std::exp(-t / 0.035f) * 0.7f;
    }
    normalize(v, 0.8f); return pack(v);
}
Buf foot(int variant, int surface) {   // 0 tile, 1 stairs, 2 wet roof
    Rng r((uint64_t)(40 + variant + surface * 10)); float len = surface == 1 ? 0.45f : 0.32f; Vec v = zeros(len);
    float f0 = 68.0f + 9.0f * variant; HP clickHp(1800); LP clickLp(5500), body(600);
    HP sp(2800); LP spl(7500);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR, w = r.sgn();
        float thump = std::sin(TAU * f0 * t * (1.0f - 0.25f * std::min(t * 12, 1.0f))) * std::exp(-t / 0.05f) * 1.0f + body(w) * std::exp(-t / 0.02f) * 0.5f;
        float click = clickLp(clickHp(w)) * std::exp(-t / (0.0035f + 0.0005f * variant)) * (0.5f + 0.1f * variant);
        float extra = 0;
        if (surface == 1) extra = std::sin(TAU * (205.0f + 12 * variant) * t) * std::exp(-t / 0.13f) * 0.35f;                 // hollow stair
        if (surface == 2) extra = spl(sp(w)) * std::min(1.0f, t / 0.006f) * std::exp(-t / 0.06f) * 0.7f;                      // splash
        v[i] = thump + click + extra;
    }
    normalize(v, 0.8f); return pack(v);
}
Buf slamSound() {
    Rng r(50); Vec v = zeros(1.0f); LP lp(600);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        v[i] = std::sin(TAU * 70.0f * t) * std::exp(-t / 0.12f) + lp(r.sgn()) * std::exp(-t / 0.08f) * 1.4f + std::sin(TAU * 190.0f * t) * std::exp(-t / 0.2f) * 0.3f;
    }
    normalize(v, 0.85f); return pack(v);
}
Buf pipeKnock() {
    Vec v = zeros(1.6f);
    float at[3] = {0.05f, 0.40f, 0.72f};
    for (int k = 0; k < 3; k++) { addTone(v, (long)(at[k] * SR), 310.0f + 12 * k, 0.8f, 0.18f, 0.8f); addTone(v, (long)(at[k] * SR), 640.0f, 0.3f, 0.09f, 0.4f); }
    normalize(v, 0.8f); return pack(v);
}
Buf lensSound() {                                   // the little brass cover on a peephole sliding aside
    Rng r(51); Vec v = zeros(0.25f); BP bp;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR; if (i % 16 == 0) bp.set(1800.0f + 2500.0f * t, 4.0f);
        v[i] = bp(r.sgn()) * std::min(1.0f, t / 0.01f) * std::exp(-t / 0.07f) * 3.0f + std::sin(TAU * 3200.0f * t) * std::exp(-t / 0.004f) * 0.3f;
    }
    normalize(v, 0.5f); return pack(v);
}

// ---------------------------------------------------------------- lift, page, bells
Buf liftDing() {
    Vec v = zeros(1.5f);
    addTone(v, 0, 1175.0f, 0.7f, 0.35f, 1.0f); addTone(v, 0, 2350.0f, 0.15f, 0.2f, 0.6f);
    addTone(v, (long)(0.42f * SR), 880.0f, 0.7f, 0.45f, 1.0f); addTone(v, (long)(0.42f * SR), 1760.0f, 0.15f, 0.25f, 0.6f);
    normalize(v, 0.6f); return pack(v);
}
Buf doorSlide() {
    Rng r(52); Vec v = zeros(1.1f); BP bp; float ph = 0;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR, u = t / 1.1f; if (i % 32 == 0) bp.set(420.0f + 900.0f * std::sin(u * 3.14159f), 1.6f);
        ph += (170.0f + 40.0f * u) / SR;
        float env = std::sin(u * 3.14159f);
        v[i] = (bp(r.sgn()) * 2.0f + std::sin(TAU * ph) * 0.18f) * env;
    }
    normalize(v, 0.5f); return pack(v);
}
Buf pageTurn() {
    Rng r(53); Vec v = zeros(0.4f); BP bp;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR; if (i % 16 == 0) bp.set(2600.0f + 2400.0f * std::sin(t * 7.0f), 1.2f);
        float env = std::min(1.0f, t / 0.03f) * std::exp(-t / 0.12f) * (0.6f + 0.4f * std::sin(t * 31.0f));
        v[i] = bp(r.sgn()) * env * 3.0f;
    }
    normalize(v, 0.45f); return pack(v);
}
Buf cluePing() {
    Vec v = zeros(1.4f);
    addTone(v, 0, 660.0f, 0.6f, 0.5f, 1.3f); addTone(v, 0, 1325.0f, 0.2f, 0.3f, 1.0f); addTone(v, 0, 1990.0f, 0.06f, 0.15f, 0.6f);
    normalize(v, 0.35f); return pack(v);
}

// ---------------------------------------------------------------- hums and machines (loops)
Buf lampBuzz() {
    Rng r(60); float len = 2.0f, fade = 0.25f; Vec v = zeros(len + fade); LP lp(5000); HP h(60);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR, s = 0;
        for (int k = 1; k <= 9; k++) s += std::sin(TAU * 100.0f * k * t) / (float)(k * k * 0.5f + 0.5f);
        v[i] = h(lp(s * 0.5f)) * (0.9f + 0.1f * std::sin(TAU * 6.0f * t));
    }
    for (int k = 0; k < 5; k++) { long at = (long)(r.uni() * (len - 0.1f) * SR); for (int j = 0; j < 80; j++) v[(size_t)(at + j)] += r.sgn() * 0.5f * std::exp(-j / 20.0f); }   // a few crackles
    Vec o = makeLoop(v, fade); normalize(o, 0.8f); return pack(o);
}
Buf liftHum() {
    Rng r(61); float len = 4.0f, fade = 0.5f; Vec v = zeros(len + fade); LP lp(500); Pink pk;
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        v[i] = (std::sin(TAU * 48.0f * t) + 0.5f * std::sin(TAU * 96.0f * t) + 0.25f * std::sin(TAU * 144.0f * t)) * (0.85f + 0.15f * std::sin(TAU * 6.0f * t)) + lp(pk(r.sgn())) * 1.2f;
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.7f); return pack(o);
}
Buf fanHum() {
    Rng r(62); float len = 4.0f, fade = 0.5f; Vec v = zeros(len + fade); LP lp(260);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        v[i] = (std::sin(TAU * 72.0f * t) * 0.8f + std::sin(TAU * 144.0f * t) * 0.25f) * (0.85f + 0.15f * std::sin(TAU * 12.0f * t)) + lp(r.sgn()) * 1.5f;
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.6f); return pack(o);
}
Buf crtHum() {
    float len = 2.0f; Vec v = zeros(len);
    for (size_t i = 0; i < v.size(); i++) { float t = (float)i / SR; v[i] = std::sin(TAU * 120.0f * t) + 0.4f * std::sin(TAU * 240.0f * t) + 0.15f * std::sin(TAU * 360.0f * t); }
    normalize(v, 0.4f); return pack(v);
}
Buf purr() {
    Rng r(63); float len = 4.0f, fade = 0.5f; Vec v = zeros(len + fade); BP bp; Pink pk; LP post(520);
    bp.set(210.0f, 0.9f);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR;
        float am = 0.5f + 0.5f * std::sin(TAU * 24.0f * t);                           // the purr is a 24 Hz rumble...
        float breath = 0.65f + 0.35f * std::sin(TAU * 0.5f * t);                      // ...that swells with each breath
        v[i] = post(bp(pk(r.sgn()) * 6.0f) * std::pow(am, 1.4f) * breath * 2.0f + std::sin(TAU * 48.0f * t) * 0.05f * am);
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.8f); return pack(o);
}
Buf meow(int variant) {
    Rng r((uint64_t)(70 + variant)); float len = 0.8f + 0.1f * variant; Vec v = zeros(len); float phase = 0, f0 = 470.0f + 55.0f * variant; BP f1, f2; HP h(300);
    for (size_t i = 0; i < v.size(); i++) {
        float t = (float)i / SR, u = t / len;
        float contour = 1.0f + 0.55f * std::sin(std::min(u * 1.7f, 1.0f) * 1.5708f) - 0.35f * std::max(0.0f, u - 0.55f);
        float f = f0 * contour * (1.0f + 0.012f * std::sin(TAU * 6.0f * t));
        phase += f / SR; phase -= std::floor(phase);
        float saw = phase * 2 - 1;
        if (i % 16 == 0) { f1.set(780.0f + 450.0f * contour, 5.0f); f2.set(2300.0f + 500.0f * contour, 6.0f); }
        float env = std::min(1.0f, t / 0.07f) * std::pow(std::max(0.0f, std::sin(u * 3.14159f)), 0.7f);
        v[i] = (f1(saw) * 3.0f + f2(saw) * 2.0f + h(r.sgn()) * 0.04f) * env;
    }
    normalize(v, 0.8f); return pack(v);
}

// ---------------------------------------------------------------- speech-like murmur (a TV commentator, a chess radio)
// Not words: a buzzing voice pitched around 'pitch' Hz that is switched on and off in syllables, each syllable shaped by two vowel
// formants. At a distance, through a wall, it is exactly what a TV sounds like.
void murmur(Vec& v, Rng& r, float pitch, float lo, float hi, float gain, float excitedFrom, float excitedTo) {
    static const float V[6][2] = {{700, 1100}, {500, 1700}, {400, 2200}, {450, 850}, {300, 2300}, {600, 1300}};
    float phase = 0, t = 0; size_t i = 0; BP a, b; HP h(lo); LP l(hi);
    while (i < v.size()) {
        float syl = r.range(0.08f, 0.22f), gap = r.uni() < 0.15f ? r.range(0.4f, 0.9f) : r.range(0.02f, 0.2f);
        const float* fm = V[r.u32() % 6]; a.set(fm[0], 6.0f); b.set(fm[1], 7.0f);
        float contour = r.range(-12.0f, 12.0f);
        size_t n = (size_t)(syl * SR);
        for (size_t k = 0; k < n && i < v.size(); k++, i++, t += 1.0f / SR) {
            float u = (float)k / (float)n, ex = (t > excitedFrom && t < excitedTo) ? 1.35f : 1.0f;
            float f = (pitch + contour * (u - 0.5f) + 8.0f * std::sin(TAU * 0.8f * t)) * ex;
            phase += f / SR; phase -= std::floor(phase);
            float saw = phase * 2 - 1, env = std::sin(u * 3.14159f);
            v[i] += l(h(a(saw) * 2.5f + b(saw) * 1.6f)) * env * gain * (ex > 1 ? 1.5f : 1.0f);
        }
        size_t g = (size_t)(gap * SR); i += g; t += gap;
    }
}
Buf tvMatch() {
    Rng r(80); float len = 12.0f, fade = 1.0f; Vec v = zeros(len + fade); Pink pk; LP lp(2400); HP hp(160);
    for (size_t i = 0; i < v.size(); i++) {                                       // the crowd
        float t = (float)i / SR, swell = 0.5f + 0.18f * std::sin(TAU * 1.0f * t / len * 2.0f) + 0.1f * std::sin(TAU * 7.0f * t / len);
        float cheer = 0; for (float c0 : {3.2f, 8.6f}) { float d = (t - c0) / 0.9f; cheer += std::exp(-d * d); }
        v[i] = hp(lp(pk(r.sgn()))) * (swell + 1.1f * cheer) * 1.5f;
    }
    Vec voice = zeros(len + fade); murmur(voice, r, 118.0f, 180.0f, 3800.0f, 0.55f, 3.0f, 4.8f);
    for (size_t i = 0; i < v.size(); i++) v[i] += voice[i] * 1.7f;
    for (size_t i = (size_t)(6.4f * SR); i < (size_t)(6.9f * SR); i++) {          // referee's whistle
        float t = (float)i / SR; v[i] += (std::sin(TAU * 2850.0f * t) + std::sin(TAU * 3200.0f * t)) * 0.18f * (0.6f + 0.4f * std::sin(TAU * 30.0f * t)) * std::min(1.0f, (6.9f - t) / 0.1f);
    }
    Vec o = makeLoop(v, fade); normalize(o, 0.85f); return pack(o);
}
Buf radioChess() {
    Rng r(81); float len = 14.0f, fade = 1.0f; Vec v = zeros(len + fade); HP h(60); LP l(3200);
    murmur(v, r, 104.0f, 300.0f, 2800.0f, 0.6f, 99.0f, 99.0f);
    for (size_t i = 0; i < v.size(); i++) { float s = r.sgn(); v[i] += l(h(s)) * 0.07f * (0.5f + 0.5f * std::sin(TAU * 0.3f * i / SR)); }       // static
    for (float at : {2.0f, 5.5f, 9.0f, 12.2f}) addTone(v, (long)(at * SR), 880.0f, 0.22f, 0.07f, 0.2f);                                        // time signal pips
    Vec o = makeLoop(v, fade); normalize(o, 0.8f); return pack(o);
}

// ---------------------------------------------------------------- piano and voice
void pianoNote(Vec& out, float t0, float freq, float amp, float dur) {
    long at = (long)(t0 * SR), n = (long)(dur * SR), N = (long)out.size();
    for (long i = 0; i < n; i++) {
        float t = (float)i / SR, s = 0;
        for (int k = 1; k <= 7; k++) {
            float fk = freq * k * (1.0f + 0.0004f * k * k);
            if (fk > 9000) break;
            s += std::sin(TAU * fk * t) * (1.0f / std::pow((float)k, 1.15f)) * std::exp(-t / (2.4f / (1.0f + 0.5f * k)));
        }
        float hammer = std::exp(-t / 0.006f) * ((float)((i * 2654435761u) & 255) / 255.0f - 0.5f) * 0.25f;
        float env = std::min(1.0f, t / 0.004f);
        out[(size_t)((at + i) % N)] += (s + hammer) * env * amp;
    }
}
Buf pianoLoop() {
    Rng r(90); float len = 24.0f; Vec v = zeros(len);
    const int deg[] = {0, 2, 4, 2, 1, 3, 5, 3, 2, 4, 6, 4, 3, 5, 7, 5, 4, 2, 0, -1};     // an exercise: thirds up the scale, then back
    const int scale[] = {0, 2, 4, 5, 7, 9, 11};
    float t = 0.5f; int count = (int)(sizeof(deg) / sizeof(deg[0]));
    for (int pass = 0; pass < 3; pass++)
        for (int k = 0; k < count; k++) {
            int d = deg[k]; int oct = d >= 7 ? 1 : (d < 0 ? -1 : 0); int dd = ((d % 7) + 7) % 7;
            int semi = scale[dd] + 12 * oct + (pass == 1 && k == 9 ? 1 : 0);          // one wrong note on the second pass...
            float f = 261.63f * std::pow(2.0f, (float)semi / 12.0f);
            pianoNote(v, t, f, 0.22f + 0.05f * r.sgn(), 2.2f);
            if (pass == 1 && k == 9) { t += 0.55f; pianoNote(v, t, 261.63f * std::pow(2.0f, (float)(scale[dd] + 12 * oct) / 12.0f), 0.26f, 2.2f); }   // ...and the correction
            t += (k % 8 == 7 ? 0.62f : 0.36f) + 0.04f * r.sgn();
        }
    normalize(v, 0.8f); return pack(v);
}
Buf operaHum() {
    Rng r(91); float len = 10.0f; Vec v = zeros(len); BP a, b, c; a.set(300, 7); b.set(1000, 8); c.set(2800, 9); float ph = 0;
    struct N { float f, t0, dur; } notes[] = {{440, 0.3f, 1.7f}, {523, 2.1f, 1.2f}, {494, 3.4f, 0.8f}, {440, 4.3f, 2.2f}, {330, 6.7f, 2.6f}};
    for (const N& n : notes) {
        long s0 = (long)(n.t0 * SR), cnt = (long)(n.dur * SR); ph = 0;
        for (long i = 0; i < cnt; i++) {
            float t = (float)i / SR, env = std::min(1.0f, t / 0.25f) * std::min(1.0f, (n.dur - t) / 0.45f);
            float vib = 1.0f + 0.014f * std::sin(TAU * 5.4f * t) * std::min(1.0f, t / 0.6f);
            ph += n.f * vib / SR; ph -= std::floor(ph);
            float saw = ph * 2 - 1;
            v[(size_t)((s0 + i) % (long)v.size())] += (a(saw) * 2.0f + b(saw) * 1.2f + c(saw) * 0.5f) * env;
        }
    }
    normalize(v, 0.7f); (void)r; return pack(v);
}
Buf keyboardLoop() {
    Rng r(92); float len = 8.0f; Vec v = zeros(len); HP h(2200);
    float t = 0.3f;
    while (t < len - 1.0f) {
        int clicks = 6 + (int)(r.uni() * 9); float rate = r.range(7.0f, 12.0f);
        for (int k = 0; k < clicks; k++) {
            long at = (long)(t * SR);
            for (long i = 0; i < (long)(0.03f * SR); i++) { float tt = (float)i / SR; if ((size_t)(at + i) < v.size()) v[(size_t)(at + i)] += h(r.sgn()) * std::exp(-tt / 0.004f) * 0.7f + std::sin(TAU * 420.0f * tt) * std::exp(-tt / 0.012f) * 0.4f; }
            t += 1.0f / rate + r.range(-0.02f, 0.03f);
        }
        t += r.range(0.6f, 1.6f);
    }
    normalize(v, 0.6f); return pack(v);
}

// ---------------------------------------------------------------- ambient music (not songs: slow pads that drift)
// Notes are written with wrap-around so a chord's long release flows into the start of the loop.
void padNote(Vec& out, float t0, float dur, float freq, float amp, float attack, float release, float cutoff) {
    long at = (long)(t0 * SR), n = (long)((dur + release) * SR), N = (long)out.size(); LP lp(cutoff), lp2(cutoff);
    float p1 = 0, p2 = 0, p3 = 0;
    for (long i = 0; i < n; i++) {
        float t = (float)i / SR;
        float env = std::min(1.0f, t / attack) * (t < dur ? 1.0f : std::max(0.0f, 1.0f - (t - dur) / release));
        env = env * env * (3 - 2 * env);                                                   // smooth
        p1 += freq * 1.003f / SR; p2 += freq * 0.997f / SR; p3 += freq / SR;
        p1 -= std::floor(p1); p2 -= std::floor(p2); p3 -= std::floor(p3);
        float s = (p1 * 2 - 1) * 0.35f + (p2 * 2 - 1) * 0.35f + std::sin(TAU * p3) * 0.5f;
        s = lp2(lp(s)) * (0.8f + 0.2f * std::sin(TAU * 0.23f * t + freq));
        out[(size_t)((at + i) % N)] += s * env * amp;
    }
}
void bellNote(Vec& out, float t0, float freq, float amp) {
    long at = (long)(t0 * SR), n = (long)(5.0f * SR), N = (long)out.size();
    for (long i = 0; i < n; i++) {
        float t = (float)i / SR;
        float s = std::sin(TAU * freq * t) * std::exp(-t / 1.6f) + 0.3f * std::sin(TAU * freq * 2.76f * t) * std::exp(-t / 0.7f) + 0.12f * std::sin(TAU * freq * 5.4f * t) * std::exp(-t / 0.3f);
        out[(size_t)((at + i) % N)] += s * std::min(1.0f, t / 0.006f) * amp;
    }
}
Buf musicHall() {
    Rng r(100); float len = 32.0f; Vec v = zeros(len);
    struct Chord { float notes[5]; int count; };
    Chord ch[4] = {{{110.00f, 164.81f, 220.00f, 261.63f, 493.88f}, 5},       // A minor add9 (low)
                   {{87.31f, 130.81f, 220.00f, 329.63f, 0}, 4},               // F major 7
                   {{73.42f, 110.00f, 174.61f, 261.63f, 329.63f}, 5},         // D minor 9
                   {{82.41f, 123.47f, 164.81f, 220.00f, 0}, 4}};              // E (open)
    for (int c = 0; c < 4; c++)
        for (int k = 0; k < ch[c].count; k++) padNote(v, c * 8.0f + 0.15f * k, 8.2f, ch[c].notes[k], 0.20f / (1.0f + 0.15f * k), 2.6f, 3.0f, 650.0f + 120.0f * k);
    for (size_t i = 0; i < v.size(); i++) v[i] += std::sin(TAU * 55.0f * i / SR) * 0.07f;                    // sub drone on A1
    const float bells[] = {659.25f, 880.0f, 1046.5f, 1174.7f, 783.99f};
    float t = 3.0f; while (t < len - 2.0f) { bellNote(v, t, bells[r.u32() % 5], 0.045f); t += r.range(5.0f, 9.0f); }
    normalize(v, 0.85f); return pack(v);
}
Buf musicRoof() {
    Rng r(101); float len = 32.0f; Vec v = zeros(len);
    padNote(v, 0.0f, 15.5f, 73.42f, 0.28f, 3.5f, 4.0f, 420.0f); padNote(v, 0.2f, 15.5f, 110.0f, 0.2f, 3.5f, 4.0f, 420.0f);
    padNote(v, 16.0f, 15.5f, 65.41f, 0.28f, 3.5f, 4.0f, 420.0f); padNote(v, 16.2f, 15.5f, 98.0f, 0.2f, 3.5f, 4.0f, 420.0f);
    for (float f : {587.33f, 698.46f, 880.0f}) {                                                             // cold shimmer high above
        for (size_t i = 0; i < v.size(); i++) { float t = (float)i / SR; v[i] += std::sin(TAU * f * t + 0.5f * std::sin(TAU * 0.5f * t)) * 0.012f * (0.5f + 0.5f * std::sin(TAU * (f / 587.33f) * 2.0f * t / len * 1.0f * 4.0f)); }
    }
    for (float t0 : {2.0f, 10.0f, 18.0f, 26.0f}) pianoNote(v, t0, 36.7f, 0.5f, 5.0f);                                // a deep note, now and then
    bellNote(v, 13.5f, 587.33f, 0.04f); bellNote(v, 28.0f, 466.16f, 0.04f); (void)r;
    normalize(v, 0.85f); return pack(v);
}
}  // namespace

// ================================================================== public
Sounds generateSounds() {
    Sounds s;
    s.rainWindow = rainWindow(); s.rainOpen = rainOpen(); s.wind = windSound();
    s.musicHall = musicHall(); s.musicRoof = musicRoof();
    s.lampBuzz = lampBuzz(); s.liftHum = liftHum(); s.purr = purr();
    s.tvMatch = tvMatch(); s.radioChess = radioChess(); s.piano = pianoLoop(); s.opera = operaHum(); s.keyboard = keyboardLoop(); s.fan = fanHum(); s.crtHum = crtHum();
    for (int i = 0; i < 3; i++) { s.thunder.push_back(thunder(i)); s.creakOpen.push_back(creak(i, false)); s.creakClose.push_back(creak(i, true)); s.meow.push_back(meow(i)); }
    for (int i = 0; i < 4; i++) { s.footTile.push_back(foot(i, 0)); s.footStairs.push_back(foot(i, 1)); s.footWet.push_back(foot(i, 2)); }
    s.knock = knock(); s.latch = latch(); s.liftDing = liftDing(); s.doorSlide = doorSlide(); s.pageTurn = pageTurn(); s.cluePing = cluePing();
    s.pipeKnock = pipeKnock(); s.slam = slamSound(); s.lens = lensSound();
    return s;
}

Buf Sounds::byName(const std::string& n) const {
    for (const auto& p : allSoundsNamed(*this)) if (p.first == n) return p.second;
    return nullptr;
}

bool isLoopSound(const std::string& n) {
    static const char* loops[] = {"rainWindow", "rainOpen", "wind", "musicHall", "musicRoof", "lampBuzz", "liftHum", "purr", "tvMatch", "radioChess", "piano", "opera", "keyboard", "fan", "crtHum"};
    for (const char* l : loops) if (n == l) return true;
    return false;
}

std::vector<std::pair<std::string, Buf>> allSoundsNamed(const Sounds& s) {
    std::vector<std::pair<std::string, Buf>> o = {
        {"rainWindow", s.rainWindow}, {"rainOpen", s.rainOpen}, {"wind", s.wind}, {"musicHall", s.musicHall}, {"musicRoof", s.musicRoof},
        {"lampBuzz", s.lampBuzz}, {"liftHum", s.liftHum}, {"purr", s.purr}, {"tvMatch", s.tvMatch}, {"radioChess", s.radioChess},
        {"piano", s.piano}, {"opera", s.opera}, {"keyboard", s.keyboard}, {"fan", s.fan}, {"crtHum", s.crtHum},
        {"knock", s.knock}, {"latch", s.latch}, {"liftDing", s.liftDing}, {"doorSlide", s.doorSlide}, {"pageTurn", s.pageTurn},
        {"cluePing", s.cluePing}, {"pipeKnock", s.pipeKnock}, {"slam", s.slam}, {"lens", s.lens}};
    for (size_t i = 0; i < s.thunder.size(); i++) o.push_back({"thunder" + std::to_string(i), s.thunder[i]});
    for (size_t i = 0; i < s.creakOpen.size(); i++) o.push_back({"creakOpen" + std::to_string(i), s.creakOpen[i]});
    for (size_t i = 0; i < s.creakClose.size(); i++) o.push_back({"creakClose" + std::to_string(i), s.creakClose[i]});
    for (size_t i = 0; i < s.meow.size(); i++) o.push_back({"meow" + std::to_string(i), s.meow[i]});
    for (size_t i = 0; i < s.footTile.size(); i++) o.push_back({"footTile" + std::to_string(i), s.footTile[i]});
    for (size_t i = 0; i < s.footStairs.size(); i++) o.push_back({"footStairs" + std::to_string(i), s.footStairs[i]});
    for (size_t i = 0; i < s.footWet.size(); i++) o.push_back({"footWet" + std::to_string(i), s.footWet[i]});
    return o;
}
