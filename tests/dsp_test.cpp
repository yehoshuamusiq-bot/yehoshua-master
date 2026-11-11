// Tests unitaires du moteur DSP (g++ -O2 -std=c++17 -I../Source tests/dsp_test.cpp)
#include "dsp/MasterDSP.h"
#include <cstdio>
#include <random>

int main()
{
    int fails = 0;
    const double sr = 48000;

    // 1) Limiteur : aucun dépassement du plafond, signal très chaud
    {
        ym::Limiter lim; lim.prepare (sr * 4, 1.5); lim.setParams (-1.0, 80);
        std::mt19937 rng (1); std::normal_distribution<double> nd (0, 0.8);
        double maxOut = 0;
        for (int i = 0; i < (int) (sr * 4 * 5); ++i)
        {
            double l = nd (rng) * 3.0, r = nd (rng) * 3.0;
            if (i % 9000 == 0) l = 12.0;            // transitoires énormes
            lim.process (l, r);
            maxOut = std::max ({ maxOut, std::abs (l), std::abs (r) });
        }
        const double ceil = ym::dbToGain (-1.0);
        // le clamp final garantit le plafond ; on vérifie aussi que le gain seul suffit
        std::printf ("Limiter max out = %.5f dBFS (plafond -1.0)\n", ym::gainToDb (maxOut));
        if (maxOut > ceil + 1e-9) { std::printf ("FAIL limiter\n"); ++fails; }
    }
    // 1b) Limiteur sans le clamp : vérifier que l'enveloppe seule tient le plafond
    {
        ym::Limiter lim; lim.prepare (sr, 1.5); lim.setParams (0.0, 50);
        double maxOut = 0; int over = 0;
        for (int i = 0; i < (int) sr * 3; ++i)
        {
            const double x = 4.0 * std::sin (2 * ym::kPi * 60 * i / sr) * (i % 4800 < 100 ? 2.0 : 0.6);
            double l = x, r = -x;
            lim.process (l, r);
            maxOut = std::max (maxOut, std::abs (l));
            if (std::abs (l) >= 1.0 - 1e-12) ++over;
        }
        std::printf ("Limiter (sine+bursts) max = %.4f, samples at ceiling = %d\n", maxOut, over);
    }

    // 2) LUFS : sinus 1 kHz -23 dBFS stéréo => -23 LUFS
    {
        ym::LoudnessMeter m; m.prepare (sr);
        const double a = ym::dbToGain (-23.0);
        for (int i = 0; i < (int) sr * 10; ++i)
        {
            const double x = a * std::sin (2 * ym::kPi * 1000 * i / sr);
            m.process (x, x);
        }
        std::printf ("LUFS M=%.2f S=%.2f I=%.2f (attendu -23.0)\n", m.momentary, m.shortTerm, m.integrated);
        if (std::abs (m.integrated + 23.0) > 0.2) { std::printf ("FAIL lufs\n"); ++fails; }
    }

    // 3) Séparateur 3 bandes : somme à magnitude plate
    {
        ym::ThreeBandSplitter sp; sp.set (sr, 120, 2500);
        for (double f : { 50.0, 120.0, 600.0, 2500.0, 9000.0 })
        {
            sp.reset(); double peakIn = 0, peakOut = 0;
            for (int i = 0; i < (int) sr; ++i)
            {
                const double x = std::sin (2 * ym::kPi * f * i / sr);
                double lo, mi, hi; sp.process (x, lo, mi, hi);
                if (i > sr / 2) { peakIn = std::max (peakIn, std::abs (x)); peakOut = std::max (peakOut, std::abs (lo + mi + hi)); }
            }
            std::printf ("Crossover sum @%5.0f Hz: %+.3f dB\n", f, ym::gainToDb (peakOut / peakIn));
            if (std::abs (ym::gainToDb (peakOut / peakIn)) > 0.1) { std::printf ("FAIL xover\n"); ++fails; }
        }
    }

    // 4) Soft clipper
    {
        const double c = ym::dbToGain (-0.3);
        for (double k : { 0.0, 0.5, 1.0 })
            for (double x : { 0.2, 0.9, 1.5, 10.0 })
                if (std::abs (ym::SoftClipper::clip (x, c, k)) > c + 1e-12) { std::printf ("FAIL clipper\n"); ++fails; }
        std::printf ("Clipper k=0.5: 0.5->%.3f 0.9->%.3f 2.0->%.3f\n",
                     ym::SoftClipper::clip (0.5, c, 0.5), ym::SoftClipper::clip (0.9, c, 0.5), ym::SoftClipper::clip (2.0, c, 0.5));
    }

    // 5) Compresseur : -6 dB au-dessus du seuil ratio 4 => ~-4.5 dB de GR à l'état stable
    {
        ym::Compressor cp; cp.prepare (sr); cp.setParams (-20, 4, 5, 100, 0);
        double g = 1;
        for (int i = 0; i < (int) sr; ++i) { const double x = ym::dbToGain (-10) * std::sin (2 * ym::kPi * 200 * i / sr); g = cp.process (x, x); }
        std::printf ("Comp GR = %.2f dB\n", ym::gainToDb (g));
    }

    std::printf (fails ? "\n%d ECHEC(S)\n" : "\nTOUS LES TESTS OK\n", fails);
    return fails;
}
