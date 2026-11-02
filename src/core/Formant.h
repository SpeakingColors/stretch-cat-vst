// Stretch Cat: real formant shift (pure C++). Not a fake: the spectral envelope (the "vowel shape") of every frame is
// estimated with a cepstral lifter, divided out of the spectrum, warped along the frequency axis and put back. Only the
// magnitudes change, so the pitch (the harmonic positions and phases) stays where it was.
// Limits: the envelope is estimated per frame without pitch tracking, so it is cleanest on voices and single lead sounds,
// and it smears when the pitch is above about 400 Hz or on dense mixes. Shifts beyond +-12 st get artefacts.
#pragma once
#include "StretchEngine.h"
#include <complex>

namespace stretchcat {

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
inline bool applyFormantShift(Stereo& s, double sr, const std::function<double(double)>& semisAt, const std::atomic<bool>* cancel = nullptr) {
    const int N = 2048, H = N / 4, half = N / 2; const size_t n = s.L.size(); if (n < (size_t) N) return true;
    static const Fft fft(2048);
    const int lifter = std::max(8, (int) std::lround(0.0025 * sr));
    std::vector<double> win((size_t) N); for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5 - 0.5 * std::cos(2 * kPi * i / N);
    const double norm = 1.0 / 1.5;                               // sum of squared 75 percent overlapped Hann windows
    std::vector<float>* ch[2] = { &s.L, &s.R };
    std::vector<std::vector<float>> out(2, std::vector<float>(n, 0.f));
    std::vector<double> re((size_t) N), im((size_t) N), lm((size_t) N), lim((size_t) N), env((size_t) half + 1), gain((size_t) half + 1);
    bool any = false;
    for (long long pos = -(long long) (N - H); pos < (long long) n; pos += H) {
        if (cancel && cancel->load(std::memory_order_relaxed)) return false;
        const double centre = (double) std::max(0LL, std::min((long long) n - 1, pos + half)) / (double) n, st = semisAt(centre);
        const bool shift = std::fabs(st) > 0.02; any = any || shift;
        const double ratio = std::pow(2.0, st / 12.0);
        for (int c = 0; c < 2; ++c) {
            const auto& in = *ch[c];
            for (int i = 0; i < N; ++i) { const long long k = pos + i; re[(size_t) i] = (k >= 0 && k < (long long) n ? (double) in[(size_t) k] : 0.0) * win[(size_t) i]; im[(size_t) i] = 0.0; }
            if (shift) {
                fft.run(re, im, false);
                for (int i = 0; i < N; ++i) { const double mag = std::sqrt(re[(size_t) i] * re[(size_t) i] + im[(size_t) i] * im[(size_t) i]); lm[(size_t) i] = std::log(mag + 1e-9); lim[(size_t) i] = 0.0; }
                std::vector<double> cr = lm, ci(lim);
                fft.run(cr, ci, true);                                     // cepstrum (unscaled)
                for (int i = 0; i < N; ++i) { const int q = std::min(i, N - i); const double w = q < lifter ? (q > lifter / 2 ? 0.5 + 0.5 * std::cos(kPi * (q - lifter / 2) / (lifter - lifter / 2)) : 1.0) : 0.0; cr[(size_t) i] *= w / N; ci[(size_t) i] = 0.0; }
                fft.run(cr, ci, false);                                    // smoothed log envelope
                for (int k = 0; k <= half; ++k) env[(size_t) k] = cr[(size_t) k];
                for (int k = 0; k <= half; ++k) {
                    const double src = (double) k / ratio; double e2;
                    if (src >= (double) half) e2 = env[(size_t) half]; else { const int i0 = (int) src; const double f = src - i0; e2 = env[(size_t) i0] * (1 - f) + env[(size_t) std::min(half, i0 + 1)] * f; }
                    gain[(size_t) k] = std::exp(std::max(-3.0, std::min(2.0, e2 - env[(size_t) k])));     // -26 dB .. +17 dB
                }
                for (int k = 0; k <= half; ++k) { re[(size_t) k] *= gain[(size_t) k]; im[(size_t) k] *= gain[(size_t) k]; if (k > 0 && k < half) { re[(size_t) (N - k)] *= gain[(size_t) k]; im[(size_t) (N - k)] *= gain[(size_t) k]; } }
                fft.run(re, im, true);
                for (int i = 0; i < N; ++i) re[(size_t) i] /= N;
            } else {
                // the frame passes through the same window pair so the overlap-add stays a perfect sum
            }
            for (int i = 0; i < N; ++i) { const long long k = pos + i; if (k >= 0 && k < (long long) n) out[(size_t) c][(size_t) k] += (float) (re[(size_t) i] * win[(size_t) i] * norm); }
        }
    }
    if (!any) return true;                                          // nothing was shifted: keep the input bit exact
    s.L = std::move(out[0]); s.R = std::move(out[1]);
    return true;
}
}
