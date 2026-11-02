// Stretch Cat: slice markers (pure C++). Markers are fractions 0..1 of the source file, kept sorted inside the selection.
// Up to kMax slices (kMax - 1 markers), one pad and one MIDI note per slice.
#pragma once
#include "StretchEngine.h"
#include <string>
#include <sstream>

namespace stretchcat {

struct SliceSet {
    static constexpr int kMax = 8;
    std::vector<double> marks;                       // sorted, strictly between the selection edges
    int count() const { return (int) marks.size() + 1; }
    bool operator==(const SliceSet& o) const { return marks == o.marks; }

    void clear() { marks.clear(); }
    // Even slices over the selection [a, b].
    void even(double a, double b, int n = kMax) { marks.clear(); n = std::max(1, std::min(kMax, n)); for (int i = 1; i < n; ++i) marks.push_back(a + (b - a) * i / n); }
    // Marks on the strongest hits of the file inside [a, b] (fractions of the file).
    void byTransient(const std::vector<float>& L, double a, double b, double sr) {
        const long long n = (long long) L.size(); marks.clear(); if (n < 1024) return;
        marks = transientSlices(L, (long long) (a * (double) n), (long long) (b * (double) n), sr, kMax - 1);
        sanitise(a, b);
    }
    // Adds a marker; returns its index, or -1 when the set is full or the spot is too close to another marker or an edge.
    int add(double f, double a, double b, double minGap = 0.002) {
        if ((int) marks.size() >= kMax - 1 || f <= a + minGap || f >= b - minGap) return -1;
        for (double m : marks) if (std::fabs(m - f) < minGap) return -1;
        marks.push_back(f); std::sort(marks.begin(), marks.end());
        return (int) (std::find(marks.begin(), marks.end(), f) - marks.begin());
    }
    int nearest(double f, double within) const { int bi = -1; double bd = within; for (size_t i = 0; i < marks.size(); ++i) if (std::fabs(marks[i] - f) <= bd) { bd = std::fabs(marks[i] - f); bi = (int) i; } return bi; }
    void remove(int i) { if (i >= 0 && i < (int) marks.size()) marks.erase(marks.begin() + i); }
    // Moves marker i, never past its neighbours or the selection edges.
    void move(int i, double f, double a, double b, double minGap = 0.002) {
        if (i < 0 || i >= (int) marks.size()) return;
        const double lo = (i == 0 ? a : marks[(size_t) i - 1]) + minGap, hi = (i + 1 == (int) marks.size() ? b : marks[(size_t) i + 1]) - minGap;
        marks[(size_t) i] = std::max(lo, std::min(hi, f));
    }
    void sanitise(double a, double b) {
        std::vector<double> r; std::sort(marks.begin(), marks.end());
        for (double m : marks) if (std::isfinite(m) && m > a && m < b && (r.empty() || m - r.back() > 0.001) && (int) r.size() < kMax - 1) r.push_back(m);
        marks = r;
    }
    std::string toString() const { std::ostringstream o; for (size_t i = 0; i < marks.size(); ++i) o << (i ? "," : "") << marks[i]; return o.str(); }
    static SliceSet fromString(const std::string& s) { SliceSet r; std::istringstream in(s); std::string t; while (std::getline(in, t, ',')) { try { r.marks.push_back(std::stod(t)); } catch (...) {} } r.sanitise(0.0, 1.0); return r; }
};

// Where source sample `s` lands in the render (map: rising (output, source) points). Clamped to the map ends.
inline double sourceToOut(const std::vector<std::pair<double, double>>& map, double s) {
    if (map.empty()) return 0.0;
    if (s <= map.front().second) return map.front().first;
    size_t lo = 0, hi = map.size() - 1; if (s >= map[hi].second) return map[hi].first;
    while (hi - lo > 1) { const size_t mid = (lo + hi) / 2; if (map[mid].second <= s) lo = mid; else hi = mid; }
    const double d = map[hi].second - map[lo].second; return d <= 0 ? map[lo].first : map[lo].first + (map[hi].first - map[lo].first) * (s - map[lo].second) / d;
}
inline double outToSource(const std::vector<std::pair<double, double>>& map, double o) {
    if (map.empty()) return 0.0;
    if (o <= map.front().first) return map.front().second;
    size_t lo = 0, hi = map.size() - 1; if (o >= map[hi].first) return map[hi].second;
    while (hi - lo > 1) { const size_t mid = (lo + hi) / 2; if (map[mid].first <= o) lo = mid; else hi = mid; }
    const double d = map[hi].first - map[lo].first; return d <= 0 ? map[lo].second : map[lo].second + (map[hi].second - map[lo].second) * (o - map[lo].first) / d;
}
// Slice boundaries in the rendered buffer: count + 1 output positions, first 0 and last = length.
inline std::vector<long long> sliceBounds(const SliceSet& s, const std::vector<std::pair<double, double>>& map, double srcLen, long long outLen) {
    std::vector<long long> r; r.push_back(0);
    for (double m : s.marks) { const long long o = std::max(r.back() + 1, std::min(outLen - 1, (long long) std::llround(sourceToOut(map, m * srcLen)))); r.push_back(o); }
    r.push_back(outLen);
    return r;
}
}
