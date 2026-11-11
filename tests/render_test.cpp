// Banc d'essai : traite un "beat" synthétique avec chaque preset, vérifie le plafond,
// mesure le LUFS, écrit des WAV avant/après et une capture PNG de l'interface.
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

static juce::AudioBuffer<float> makeBeat (double sr, double seconds)
{
    const int n = (int) (sr * seconds);
    juce::AudioBuffer<float> b (2, n);
    juce::Random rnd (42);
    const double bpm = 140, beat = 60.0 / bpm;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        const double tb = std::fmod (t, beat), tbar = std::fmod (t, beat * 4);
        // kick
        const double kf = 45 + 120 * std::exp (-tb * 30);
        double kick = std::sin (2 * juce::MathConstants<double>::pi * kf * tb) * std::exp (-tb * 6) * 0.9;
        // 808
        const double bass = std::sin (2 * juce::MathConstants<double>::pi * 49 * t) * std::exp (-tbar * 1.2) * 0.5;
        // snare (temps 2 et 4)
        const double ts = std::fmod (t + beat, beat * 2);
        double snare = (rnd.nextDouble() * 2 - 1) * std::exp (-ts * 18) * 0.35 + std::sin (2 * juce::MathConstants<double>::pi * 190 * ts) * std::exp (-ts * 25) * 0.2;
        // hats
        const double th = std::fmod (t, beat / 2);
        double hat = (rnd.nextDouble() * 2 - 1) * std::exp (-th * 80) * 0.12;
        // accords
        double pad = 0;
        for (double f : { 220.0, 261.63, 329.63, 392.0 }) pad += std::sin (2 * juce::MathConstants<double>::pi * f * t + f) * 0.05;
        const double l = kick + bass + snare + hat * 1.2 + pad * 1.1;
        const double r = kick + bass + snare + hat * 0.8 + pad * 0.9;
        b.setSample (0, i, (float) (l * 0.5));
        b.setSample (1, i, (float) (r * 0.5));
    }
    return b;
}

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (w) { os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp/ymtest");
    outDir.createDirectory();

    const double sr = 48000; const int bs = 512;
    auto src = makeBeat (sr, 12.0);
    int fails = 0;

    YehoshuaMasterProcessor proc;
    proc.setPlayConfigDetails (2, 2, sr, bs);
    proc.prepareToPlay (sr, bs);
    std::printf ("Latence rapportée : %d échantillons (%.2f ms)\n", proc.getLatencySamples(), proc.getLatencySamples() * 1000.0 / sr);

    YMLookAndFeel laf;
    laf.setDefaultSansSerifTypefaceName ("DejaVu Sans");
    juce::LookAndFeel::setDefaultLookAndFeel (&laf);
    std::unique_ptr<MainView> view;

    const auto& presets = getFactoryPresets();
    for (int pi = 0; pi < (int) presets.size(); ++pi)
    {
        proc.applyPreset (pi);
        proc.prepareToPlay (sr, bs);
        proc.resetLoudness();
        juce::AudioBuffer<float> buf (src);
        juce::MidiBuffer midi;
        const bool snap = (pi == 1);
        if (snap) { view = std::make_unique<MainView> (proc); view->setLookAndFeel (&laf); }

        for (int pos = 0; pos < buf.getNumSamples(); pos += bs)
        {
            const int n = juce::jmin (bs, buf.getNumSamples() - pos);
            juce::AudioBuffer<float> blk (buf.getArrayOfWritePointers(), 2, pos, n);
            proc.processBlock (blk, midi);
            if (snap && (pos / bs) % 3 == 0) view->tick (0.032f);
        }

        float peak = 0; bool nan = false;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < buf.getNumSamples(); ++i)
            {
                const float v = buf.getSample (c, i);
                if (! std::isfinite (v)) nan = true;
                peak = std::max (peak, std::abs (v));
            }
        const float ceilDb = proc.apvts.getRawParameterValue ("limCeiling")->load();
        const float peakDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        const bool ok = ! nan && peakDb <= ceilDb + 0.01f;
        if (! ok) ++fails;
        std::printf ("%-32s  pic %6.2f dBFS (plafond %5.2f)  LUFS-I %6.2f  GR-lim %5.1f dB  %s\n",
                     presets[(size_t) pi].name, peakDb, ceilDb, proc.meters.lufsI.load(),
                     proc.meters.limGR.load(), ok ? "OK" : "ECHEC");
        if (pi == 1) writeWav (outDir.getChildFile ("after_hiphop_loud.wav"), buf, sr);
        if (snap)
        {
            auto img = view->createComponentSnapshot (view->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            juce::FileOutputStream fos (outDir.getChildFile ("gui.png"));
            fos.setPosition (0); fos.truncate();
            png.writeImageToStream (img, fos);
            view->setLookAndFeel (nullptr);
            view.reset();
        }
    }
    writeWav (outDir.getChildFile ("before.wav"), src, sr);

    // Mesure du LUFS de la source
    {
        proc.applyPreset (0);
        auto* lim = proc.apvts.getParameter ("limOn"); lim->setValueNotifyingHost (0.0f);
        proc.prepareToPlay (sr, bs);
        juce::AudioBuffer<float> buf (src); juce::MidiBuffer midi;
        for (int pos = 0; pos < buf.getNumSamples(); pos += bs)
        {
            const int n = juce::jmin (bs, buf.getNumSamples() - pos);
            juce::AudioBuffer<float> blk (buf.getArrayOfWritePointers(), 2, pos, n);
            proc.processBlock (blk, midi);
        }
        std::printf ("Source brute (tout désactivé) : LUFS-I %.2f\n", proc.meters.lufsI.load());
    }

    std::printf (fails ? "\n%d ECHEC(S)\n" : "\nTOUS LES PRESETS OK\n", fails);
    return fails;
}
