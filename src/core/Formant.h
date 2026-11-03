// Stretch Cat: real formant shift (pure C++). Not a fake: the spectral envelope (the "vowel shape") of every frame is
// estimated, divided out of the spectrum, warped along the frequency axis and put back. Only the magnitudes change, so the
// pitch (the harmonic positions and phases) stays where it was and nothing can alias (no component is moved in frequency).
//
// Envelope estimate (v0.3): a "true envelope" (Roebel): the cepstral lifter is iterated so that the envelope passes through the
// harmonic peaks instead of averaging peaks and valleys. The lifter length follows the pitch of each frame (found as the
// strongest cepstral peak, about half a pitch period), so a high voice is not mistaken for a detailed envelope. That is what
// smeared the old fixed 2.5 ms lifter above about 400 Hz. Frames without a clear pitch (noise, dense mixes) use a fixed lifter.
// This is an offline pass inside the background render (no audio thread, no latency, allocation only before the loop).
// Limits: one pitch per frame, so dense mixes and chords get one common lifter; when the harmonics are further apart than the
// formants are wide (very high pitches) the envelope is simply not in the signal and cannot be recovered; shifts beyond +-12 st
// get artefacts. Measured on synthetic vowels (tests/test_formant.cpp), not judged by ear.
#pragma once
#include "StretchEngine.h"
#include <complex>

namespace stretchcat {

#ifndef SC_FORMANT_ITERS
#define SC_FORMANT_ITERS 30
#endif
#ifndef SC_FORMANT_TOL
#define SC_FORMANT_TOL 0.1
#endif
#ifndef SC_FORMANT_LIFTER_FRAC
#define SC_FORMANT_LIFTER_FRAC 0.5
#endif
constexpr double kLifterFrac = SC_FORMANT_LIFTER_FRAC;     // lifter length as a share of the pitch period (tuned on tests/test_formant.cpp)

class Fft {
public:
    explicit Fft(int n) : N(n), cs((size_t) n / 2), sn((size_t) n / 2), rev((size_t) n) {
        for (int i = 0; i < N / 2; ++i) { cs[(size_t) i] = std::cos(2 * kPi * i / N); sn[(size_t) i] = std::sin(2 * kPi * i / N); }
        int bits = 0; while ((1 << bits) < N) ++bits;
        for (int i = 0; i < N; ++i) { int r = 0; for (int b = 0; b < bits; ++b) if (i & (1 << b)) r |= 1 << (bits - 1 - b); rev[(size_t) i] = r; }
    }
    int size() const { return N; }
    // in place; inverse = true gives the unscaled inverse (divide by N yourself)
    void run(std::vector<double>& re, std::vector<double>& im, bool inverse) const {
        for (int i = 0; i < N; ++i) { const int j = rev[(size_t) i]; if (j > i) { std::swap(re[(size_t) i], re[(size_t) j]); std::swap(im[(size_t) i], im[(size_t) j]); } }
        for (int len = 2; len <= N; len <<= 1) {
            const int half = len / 2, step = N / len;
            for (int i = 0; i < N; i += len) for (int k = 0; k < half; ++k) {
                const double wr = cs[(size_t) (k * step)], wi = (inverse ? 1.0 : -1.0) * sn[(size_t) (k * step)];
                const size_t u = (size_t) (i + k), v = (size_t) (i + k + half);
                const double tr = re[v] * wr - im[v] * wi, ti = re[v] * wi + im[v] * wr;
                re[v] = re[u] - tr; im[v] = im[u] - ti; re[u] += tr; im[u] += ti;
            }
        }
    }
private:
    int N; std::vector<double> cs, sn; std::vector<int> rev;
};

// Shifts the formants of `s` by semisAt(f) semitones, f = position in the result 0..1. Returns false when cancelled (s untouched).
// Frames where the shift is under 0.02 st are copied unchanged. Output has the same length.
inline constexpr int formantLatencySamples() { return 0; }   // offline pass inside the background render

inline bool applyFormantShift(Stereo& s, double sr, const std::function<double(double)>& semisAt, const std::atomic<bool>* cancel = nullptr) {
    const int N = 2048, H = N / 4, half = N / 2; const size_t n = s.L.size(); if (n < (size_t) N) return true;
    static const Fft fft(2048);
    const int fixedLifter = std::max(8, (int) std::lround(0.0025 * sr));                // frames without a clear pitch
    const int maxLifter = std::max(fixedLifter, (int) std::lround(0.008 * sr));          // frames with a pitch below about 60 Hz are capped here
    std::vector<double> win((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5 - 0.5 * std::cos(2 * kPi * i / N);
    const double norm = 1.0 / 1.5;                               // sum of squared 75 percent overlapped Hann windows
    std::vector<float>* ch[2] = { &s.L, &s.R };
    std::vector<std::vector<float>> out(2, std::vector<float>(n, 0.f));
    std::vector<double> sre[2] = { std::vector<double>((size_t) N), std::vector<double>((size_t) N) }, sim[2] = { std::vector<double>((size_t) N), std::vector<double>((size_t) N) };
    std::vector<double> A[2] = { std::vector<double>((size_t) N), std::vector<double>((size_t) N) };   // log magnitudes (the peaks the envelope must reach)
    std::vector<double> E[2] = { std::vector<double>((size_t) N), std::vector<double>((size_t) N) };   // envelopes
    std::vector<double> cr((size_t) N), ci((size_t) N), w((size_t) N), gain((size_t) half + 1);
    int curL = -1;
    auto setLifter = [&](int L) {      // flat to L/2, raised cosine to L (a real even weight, also carrying the 1/N of the inverse FFT)
        if (L == curL) return;
        curL = L; const int h = L / 2;
        for (int i = 0; i < N; ++i) { const int q = std::min(i, N - i); w[(size_t) i] = (q < L ? (q > h ? 0.5 + 0.5 * std::cos(kPi * (q - h) / (L - h)) : 1.0) : 0.0) / N; }
    };
    // smooths both channels at once: two real even log spectra packed as real and imaginary part give two real even cepstra
    auto smooth = [&]() {
        for (int i = 0; i < N; ++i) { cr[(size_t) i] = A[0][(size_t) i]; ci[(size_t) i] = A[1][(size_t) i]; }
        fft.run(cr, ci, false);
        for (int i = 0; i < N; ++i) { cr[(size_t) i] *= w[(size_t) i]; ci[(size_t) i] *= w[(size_t) i]; }
        fft.run(cr, ci, true);
        for (int i = 0; i < N; ++i) { E[0][(size_t) i] = cr[(size_t) i]; E[1][(size_t) i] = ci[(size_t) i]; }
    };
    const int qLo = std::max(8, (int) (sr / 1400.0)), qHi = std::min(half - 1, (int) (sr / 70.0));
    bool any = false;
    for (long long pos = -(long long) (N - H); pos < (long long) n; pos += H) {
        if (cancel && cancel->load(std::memory_order_relaxed)) return false;
        const double centre = (double) std::max(0LL, std::min((long long) n - 1, pos + half)) / (double) n, st = semisAt(centre);
        const bool shift = std::fabs(st) > 0.02; any = any || shift;
        const double ratio = std::pow(2.0, st / 12.0);
        for (int c = 0; c < 2; ++c) { const auto& in = *ch[c]; for (int i = 0; i < N; ++i) { const long long k = pos + i; sre[c][(size_t) i] = (k >= 0 && k < (long long) n ? (double) in[(size_t) k] : 0.0) * win[(size_t) i]; sim[c][(size_t) i] = 0.0; } }
        if (shift) {
            double peak = 0;
            for (int c = 0; c < 2; ++c) { fft.run(sre[c], sim[c], false); for (int i = 0; i < N; ++i) { const double m = std::sqrt(sre[c][(size_t) i] * sre[c][(size_t) i] + sim[c][(size_t) i] * sim[c][(size_t) i]); A[c][(size_t) i] = m; peak = std::max(peak, m); } }
            const double floorM = peak * 1e-5 + 1e-12;                             // 100 dB under the strongest bin: deeper valleys carry no envelope
            for (int c = 0; c < 2; ++c) for (int i = 0; i < N; ++i) A[c][(size_t) i] = std::log(std::max(A[c][(size_t) i], floorM));
            // pitch of this frame: strongest cepstral peak (first one within 75 percent of it, so octaves down are not taken)
            for (int i = 0; i < N; ++i) { cr[(size_t) i] = 0.5 * (A[0][(size_t) i] + A[1][(size_t) i]); ci[(size_t) i] = 0.0; }
            fft.run(cr, ci, false);                                                // cepstrum (times N), real even
            double best = 0; for (int q = qLo; q <= qHi; ++q) best = std::max(best, cr[(size_t) q] / N);
            int qp = 0; if (best > 0.08) for (int q = qLo + 1; q < qHi; ++q) { const double v = cr[(size_t) q] / N; if (v >= 0.75 * best && v >= cr[(size_t) q - 1] / N && v >= cr[(size_t) q + 1] / N) { qp = q; break; } }
            const int L = qp > 0 ? std::max(6, std::min(maxLifter, (int) std::lround(kLifterFrac * qp))) : fixedLifter;
            setLifter(L);
            smooth();
            for (int it = 0; it < SC_FORMANT_ITERS; ++it) {                                      // true envelope: raise the log spectrum to the envelope where it is below, smooth again
                double worst = 0;
                for (int c = 0; c < 2; ++c) for (int i = 0; i <= half; ++i) { const double d = A[c][(size_t) i] - E[c][(size_t) i]; if (d < 0) { A[c][(size_t) i] = E[c][(size_t) i]; A[c][(size_t) (N - i) % N] = E[c][(size_t) i]; } else worst = std::max(worst, d); }
                if (worst < SC_FORMANT_TOL) break;
                smooth();
            }
            for (int c = 0; c < 2; ++c) {
                const auto& env = E[c];
                for (int k = 0; k <= half; ++k) {
                    const double src = (double) k / ratio; double e2;
                    if (src >= (double) half) e2 = env[(size_t) half]; else { const int i0 = (int) src; const double f = src - i0; e2 = env[(size_t) i0] * (1 - f) + env[(size_t) std::min(half, i0 + 1)] * f; }
                    gain[(size_t) k] = std::exp(std::max(-3.0, std::min(2.0, e2 - env[(size_t) k])));     // -26 dB .. +17 dB
                }
                for (int k = 0; k <= half; ++k) { sre[c][(size_t) k] *= gain[(size_t) k]; sim[c][(size_t) k] *= gain[(size_t) k]; if (k > 0 && k < half) { sre[c][(size_t) (N - k)] *= gain[(size_t) k]; sim[c][(size_t) (N - k)] *= gain[(size_t) k]; } }
                fft.run(sre[c], sim[c], true);
                for (int i = 0; i < N; ++i) sre[c][(size_t) i] /= N;
            }
        }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < N; ++i) { const long long k = pos + i; if (k >= 0 && k < (long long) n) out[(size_t) c][(size_t) k] += (float) (sre[c][(size_t) i] * win[(size_t) i] * norm); }
    }
    if (!any) return true;                                          // nothing was shifted: keep the input bit exact
    s.L = std::move(out[0]); s.R = std::move(out[1]);
    return true;
}
}
