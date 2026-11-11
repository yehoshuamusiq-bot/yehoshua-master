/*
    YEHOSHU'A MUSIQ — MASTER
    Moteur DSP (C++ pur, sans dépendance JUCE) : EQ, multibande, saturation,
    exciter, imageur stéréo, soft clipper, limiteur look-ahead, LUFS, dither.
*/
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ym
{
constexpr double kPi = 3.14159265358979323846;

inline double dbToGain (double db)  { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g)   { return g > 1.0e-9 ? 20.0 * std::log10 (g) : -180.0; }

//==============================================================================
// Biquad (RBJ cookbook), Transposed Direct Form II, double precision
struct Biquad
{
    enum class Type { LowPass, HighPass, Peak, LowShelf, HighShelf, AllPass };

    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    void set (Type t, double sr, double freq, double q, double gainDb = 0.0)
    {
        freq = std::clamp (freq, 5.0, sr * 0.49);
        q    = std::max (q, 0.025);
        const double A  = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * freq / sr;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / (2.0 * q);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;

        switch (t)
        {
            case Type::LowPass:
                B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = (1 - cw) / 2;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::HighPass:
                B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = (1 + cw) / 2;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Type::Peak:
                B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A;
                A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case Type::LowShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) - (A - 1) * cw + s);
                B1 = 2 * A * ((A - 1) - (A + 1) * cw);
                B2 = A * ((A + 1) - (A - 1) * cw - s);
                A0 = (A + 1) + (A - 1) * cw + s;
                A1 = -2 * ((A - 1) + (A + 1) * cw);
                A2 = (A + 1) + (A - 1) * cw - s; break;
            }
            case Type::HighShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + s);
                B1 = -2 * A * ((A - 1) + (A + 1) * cw);
                B2 = A * ((A + 1) + (A - 1) * cw - s);
                A0 = (A + 1) - (A - 1) * cw + s;
                A1 = 2 * ((A - 1) - (A + 1) * cw);
                A2 = (A + 1) - (A - 1) * cw - s; break;
            }
            case Type::AllPass:
                B0 = 1 - alpha; B1 = -2 * cw; B2 = 1 + alpha;
                A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    // magnitude (linéaire) à une fréquence donnée — pour dessiner la courbe d'EQ
    double magnitude (double freq, double sr) const
    {
        const double w = 2.0 * kPi * freq / sr;
        const double c1 = std::cos (w), s1 = std::sin (w);
        const double c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2);
        const double dr = 1 + a1 * c1 + a2 * c2,  di = -(a1 * s1 + a2 * s2);
        return std::sqrt ((nr * nr + ni * ni) / std::max (1e-30, dr * dr + di * di));
    }
};

//==============================================================================
// Filtre Linkwitz-Riley 4e ordre (2 Butterworth en cascade)
struct LR4
{
    Biquad s1, s2;
    void set (bool highPass, double sr, double f)
    {
        const auto t = highPass ? Biquad::Type::HighPass : Biquad::Type::LowPass;
        s1.set (t, sr, f, 0.70710678);
        s2.set (t, sr, f, 0.70710678);
    }
    void reset() { s1.reset(); s2.reset(); }
    inline double process (double x) { return s2.process (s1.process (x)); }
};

// Séparateur 3 bandes à phase cohérente (somme = passe-tout, magnitude plate)
struct ThreeBandSplitter
{
    LR4 lp1, hp1, lp2, hp2, apLowLP, apLowHP;

    void set (double sr, double f1, double f2)
    {
        lp1.set (false, sr, f1); hp1.set (true, sr, f1);
        lp2.set (false, sr, f2); hp2.set (true, sr, f2);
        apLowLP.set (false, sr, f2); apLowHP.set (true, sr, f2);
    }
    void reset() { lp1.reset(); hp1.reset(); lp2.reset(); hp2.reset(); apLowLP.reset(); apLowHP.reset(); }

    inline void process (double x, double& lo, double& mid, double& hi)
    {
        const double l = lp1.process (x);
        const double r = hp1.process (x);
        lo  = apLowLP.process (l) + apLowHP.process (l);   // compensation de phase de f2
        mid = lp2.process (r);
        hi  = hp2.process (r);
    }
};

//==============================================================================
// Compresseur feed-forward, stéréo lié, genou doux, détection RMS/peak mix
struct Compressor
{
    double sr = 44100;
    double threshold = -18, ratio = 2, knee = 6, makeup = 0;
    double attCoef = 0, relCoef = 0;
    double envDb = 0;          // réduction de gain lissée (dB, <= 0)
    double rmsState = 0, rmsCoef = 0;

    void prepare (double sampleRate) { sr = sampleRate; rmsCoef = std::exp (-1.0 / (0.005 * sr)); reset(); }
    void reset() { envDb = 0; rmsState = 0; }

    void setParams (double thrDb, double rat, double attackMs, double releaseMs, double makeupDb)
    {
        threshold = thrDb; ratio = std::max (1.0, rat); makeup = makeupDb;
        attCoef = std::exp (-1.0 / (std::max (0.05, attackMs) * 0.001 * sr));
        relCoef = std::exp (-1.0 / (std::max (1.0, releaseMs) * 0.001 * sr));
    }

    inline double computeGainDb (double inDb) const
    {
        const double over = inDb - threshold;
        if (2 * over < -knee) return 0.0;
        if (2 * std::abs (over) <= knee)
        {
            const double t = over + knee / 2;
            return (1.0 / ratio - 1.0) * t * t / (2 * knee);
        }
        return (1.0 / ratio - 1.0) * over;
    }

    // renvoie le gain linéaire à appliquer (incluant makeup)
    inline double process (double l, double r)
    {
        const double pk = std::max (std::abs (l), std::abs (r));
        rmsState = rmsCoef * rmsState + (1 - rmsCoef) * pk * pk;
        const double det = 0.5 * pk + 0.5 * std::sqrt (rmsState) * 1.41421356;
        const double target = computeGainDb (gainToDb (det));
        const double c = target < envDb ? attCoef : relCoef;
        envDb = target + c * (envDb - target);
        return dbToGain (envDb + makeup);
    }
};

//==============================================================================
// Saturation analogique (Tape symétrique / Tube asymétrique), gain petit-signal = 1
struct Saturator
{
    Biquad dcL, dcR;
    void prepare (double sr) { dcL.set (Biquad::Type::HighPass, sr, 8.0, 0.707); dcR = dcL; reset(); }
    void reset() { dcL.reset(); dcR.reset(); }

    static inline double tape (double x, double g) { return std::tanh (g * x) / g; }
    static inline double tube (double x, double g)
    {
        constexpr double b = 0.25;
        const double tb = std::tanh (g * b);
        return (std::tanh (g * (x + b)) - tb) / (g * (1.0 - tb * tb));
    }

    // drive en dB (0..24), type 0 = Tape, 1 = Tube, mix 0..1
    inline void process (double& l, double& r, double driveDb, int type, double mix)
    {
        const double g = std::max (0.05, dbToGain (driveDb) * 0.5);
        double wl, wr;
        if (type == 0) { wl = tape (l, g); wr = tape (r, g); }
        else           { wl = dcL.process (tube (l, g)); wr = dcR.process (tube (r, g)); }
        l = l + mix * (wl - l);
        r = r + mix * (wr - r);
    }
};

//==============================================================================
// Exciter : harmoniques de sub (punch des basses / 808) + air (brillance)
struct Exciter
{
    Biquad subLP[2], subLP2[2], subHarmBP[2], airHP[2], airHP2[2];

    void prepare (double sr)
    {
        for (int c = 0; c < 2; ++c)
        {
            subLP[c].set  (Biquad::Type::LowPass, sr, 110.0, 0.707);
            subLP2[c].set (Biquad::Type::LowPass, sr, 110.0, 0.707);
            subHarmBP[c].set (Biquad::Type::HighPass, sr, 140.0, 0.707);
            airHP[c].set  (Biquad::Type::HighPass, sr, 7500.0, 0.707);
            airHP2[c].set (Biquad::Type::HighPass, sr, 7500.0, 0.707);
        }
        reset();
    }
    void reset() { for (int c = 0; c < 2; ++c) { subLP[c].reset(); subLP2[c].reset(); subHarmBP[c].reset(); airHP[c].reset(); airHP2[c].reset(); } }

    inline double processOne (int c, double x, double subAmt, double airAmt)
    {
        double out = x;
        if (subAmt > 0)
        {
            const double lo = subLP2[c].process (subLP[c].process (x));
            // redresseur doux + saturation => harmoniques paires et impaires audibles sur petits HP
            const double h = std::tanh (4.0 * lo) * 0.6 + 0.4 * std::abs (std::tanh (3.0 * lo));
            out += subAmt * 0.9 * subHarmBP[c].process (h) + subAmt * 0.35 * lo;
        }
        if (airAmt > 0)
        {
            const double hi = airHP2[c].process (airHP[c].process (x));
            out += airAmt * (0.5 * hi + 0.4 * std::tanh (3.0 * hi));
        }
        return out;
    }
};

//==============================================================================
// Imageur stéréo M/S + basses en mono
struct StereoImager
{
    LR4 sideHP;
    void prepare (double sr, double monoFreq) { sideHP.set (true, sr, monoFreq); sideHP.reset(); }
    void setMonoFreq (double sr, double f) { sideHP.set (true, sr, f); }
    void reset() { sideHP.reset(); }

    inline void process (double& l, double& r, double width, bool monoBass)
    {
        const double m = 0.5 * (l + r);
        double s = 0.5 * (l - r) * width;
        if (monoBass) s = sideHP.process (s);
        l = m + s; r = m - s;
    }
};

//==============================================================================
// Soft clipper à genou réglable (0 = hard clip, 1 = très doux)
struct SoftClipper
{
    static inline double clip (double x, double ceiling, double knee)
    {
        const double ax = std::abs (x);
        const double a = ceiling * (1.0 - std::clamp (knee, 0.0, 0.999));
        if (ax <= a) return x;
        const double range = ceiling - a;
        double y = (range <= 1e-12) ? ceiling : a + range * std::tanh ((ax - a) / range);
        y = std::min (y, ceiling);
        return x < 0 ? -y : y;
    }
};

//==============================================================================
// Limiteur brickwall look-ahead stéréo lié (hold + moyenne glissante => zéro dépassement)
class Limiter
{
public:
    void prepare (double sampleRate, double lookaheadMs)
    {
        prepareWithLength (sampleRate, std::max (1, (int) std::round (lookaheadMs * 0.001 * sampleRate)));
    }

    void prepareWithLength (double sampleRate, int lengthSamples)
    {
        sr = sampleRate;
        L = std::max (1, lengthSamples);
        delayL.assign ((size_t) L, 0.0); delayR.assign ((size_t) L, 0.0);
        dqIdx.assign ((size_t) L + 1, 0); dqVal.assign ((size_t) L + 1, 1.0);
        box.assign ((size_t) L, 1.0);
        reset();
    }

    void reset()
    {
        std::fill (delayL.begin(), delayL.end(), 0.0);
        std::fill (delayR.begin(), delayR.end(), 0.0);
        std::fill (box.begin(), box.end(), 1.0);
        boxSum = (double) L; pos = 0; n = 0; dqHead = dqTail = 0; env = 1.0; resync = 0;
    }

    int getLatency() const { return L - 1; }

    void setParams (double ceilingDb, double releaseMs)
    {
        ceiling = dbToGain (ceilingDb);
        relCoef = std::exp (-1.0 / (std::max (0.5, releaseMs) * 0.001 * sr));
    }

    // traite un échantillon stéréo, renvoie le gain appliqué
    inline double process (double& l, double& r)
    {
        const double pk = std::max (std::abs (l), std::abs (r));
        const double req = pk > ceiling ? ceiling / pk : 1.0;

        // min glissant sur L échantillons (deque monotone)
        const int cap = L + 1;
        while (dqHead != dqTail && dqVal[(size_t) ((dqTail - 1 + cap) % cap)] >= req)
            dqTail = (dqTail - 1 + cap) % cap;
        dqIdx[(size_t) dqTail] = n; dqVal[(size_t) dqTail] = req; dqTail = (dqTail + 1) % cap;
        while (dqIdx[(size_t) dqHead] <= n - L) dqHead = (dqHead + 1) % cap;
        const double held = dqVal[(size_t) dqHead];

        // relâchement exponentiel (ne remonte jamais au-dessus de 'held')
        env = held < env ? held : held + relCoef * (env - held);

        // moyenne glissante sur L
        boxSum += env - box[(size_t) pos];
        box[(size_t) pos] = env;
        if (++resync >= 8192) { resync = 0; boxSum = 0; for (double v : box) boxSum += v; }
        const double g = std::min (1.0, boxSum / (double) L);

        // ligne de retard de L-1 échantillons
        const int rd = (pos + 1) % L;   // plus ancien = L-1 échantillons de retard après écriture
        delayL[(size_t) pos] = l; delayR[(size_t) pos] = r;
        double ol = (L == 1) ? l : delayL[(size_t) rd];
        double or_ = (L == 1) ? r : delayR[(size_t) rd];
        pos = (pos + 1) % L; ++n;

        ol *= g; or_ *= g;
        // sécurité absolue
        ol = std::clamp (ol, -ceiling, ceiling);
        or_ = std::clamp (or_, -ceiling, ceiling);
        l = ol; r = or_;
        return g;
    }

private:
    double sr = 44100, ceiling = 1.0, relCoef = 0.999, env = 1.0, boxSum = 0;
    int L = 1, pos = 0, dqHead = 0, dqTail = 0, resync = 0;
    long long n = 0;
    std::vector<double> delayL, delayR, box, dqVal;
    std::vector<long long> dqIdx;
};

//==============================================================================
// Loudness meter ITU-R BS.1770 / EBU R128 (Momentary, Short-term, Integrated)
class LoudnessMeter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        blockLen = std::max (1, (int) std::round (sr * 0.1));
        for (int c = 0; c < 2; ++c)
        {
            // coefficients K-weighting exacts (BS.1770-4, conception bilinéaire à tout sr)
            {
                const double K = std::tan (kPi * 1681.974450955533 / sr), Q = 0.7071752369554196;
                const double Vh = std::pow (10.0, 3.999843853973347 / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
                const double a0 = 1.0 + K / Q + K * K;
                pre[c].b0 = (Vh + Vb * K / Q + K * K) / a0; pre[c].b1 = 2.0 * (K * K - Vh) / a0;
                pre[c].b2 = (Vh - Vb * K / Q + K * K) / a0; pre[c].a1 = 2.0 * (K * K - 1.0) / a0;
                pre[c].a2 = (1.0 - K / Q + K * K) / a0;
            }
            {
                const double K = std::tan (kPi * 38.13547087602444 / sr), Q = 0.5003270373238773;
                const double a0 = 1.0 + K / Q + K * K;
                rlb[c].b0 = 1.0; rlb[c].b1 = -2.0; rlb[c].b2 = 1.0;
                rlb[c].a1 = 2.0 * (K * K - 1.0) / a0; rlb[c].a2 = (1.0 - K / Q + K * K) / a0;
            }
        }
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c) { pre[c].reset(); rlb[c].reset(); }
        sub.fill (0.0); subPos = 0; subCount = 0; acc = 0; accN = 0;
        histCount.fill (0); histEnergy.fill (0.0);
        momentary = shortTerm = integrated = -100.0;
    }

    inline void process (double l, double r)
    {
        const double kl = rlb[0].process (pre[0].process (l));
        const double kr = rlb[1].process (pre[1].process (r));
        acc += kl * kl + kr * kr;
        if (++accN >= blockLen)
        {
            sub[(size_t) subPos] = acc / (double) accN;
            subPos = (subPos + 1) % 30; subCount = std::min (subCount + 1, 30);
            acc = 0; accN = 0;
            update();
        }
    }

    double momentary = -100, shortTerm = -100, integrated = -100;

private:
    static double toLufs (double e) { return e > 1e-12 ? -0.691 + 10.0 * std::log10 (e) : -100.0; }

    double meanLast (int count) const
    {
        count = std::min (count, subCount);
        if (count == 0) return 0;
        double s = 0;
        for (int i = 1; i <= count; ++i) s += sub[(size_t) ((subPos - i + 30) % 30)];
        return s / count;
    }

    void update()
    {
        const double eM = meanLast (4);
        momentary = toLufs (eM);
        shortTerm = toLufs (meanLast (30));

        if (subCount >= 4 && momentary > -70.0)
        {
            const int bin = std::clamp ((int) ((momentary + 70.0) * 10.0), 0, kBins - 1);
            histCount[(size_t) bin] += 1; histEnergy[(size_t) bin] += eM;
        }

        // gating
        double sumE = 0; long long cnt = 0;
        for (int b = 0; b < kBins; ++b) { sumE += histEnergy[(size_t) b]; cnt += histCount[(size_t) b]; }
        if (cnt == 0) { integrated = -100; return; }
        const double rel = toLufs (sumE / (double) cnt) - 10.0;
        const int relBin = std::clamp ((int) std::ceil ((rel + 70.0) * 10.0), 0, kBins - 1);
        sumE = 0; cnt = 0;
        for (int b = relBin; b < kBins; ++b) { sumE += histEnergy[(size_t) b]; cnt += histCount[(size_t) b]; }
        integrated = cnt > 0 ? toLufs (sumE / (double) cnt) : -100.0;
    }

    static constexpr int kBins = 900; // -70 .. +20 LUFS par 0.1 LU
    double sr = 44100;
    Biquad pre[2], rlb[2];
    std::array<double, 30> sub {};
    int subPos = 0, subCount = 0, blockLen = 4410, accN = 0;
    double acc = 0;
    std::array<long long, kBins> histCount {};
    std::array<double, kBins> histEnergy {};
};

//==============================================================================
// Dither TPDF
struct Dither
{
    uint32_t s1 = 0x12345678u, s2 = 0x9abcdef1u;
    inline double rnd (uint32_t& s) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (double) s / 4294967296.0; }
    inline double process (double x, int bits)
    {
        if (bits <= 0) return x;
        const double q = 1.0 / (double) (1u << (bits - 1));
        const double d = (rnd (s1) - rnd (s2)) * q;
        return std::round ((x + d) / q) * q;
    }
};

} // namespace ym
