#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "PluginProcessor.h"

//==============================================================================
// Bouton rotatif avec nom + valeur, attaché à un paramètre
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& st, const juce::String& paramID, const juce::String& title)
        : name (title)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 14);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setVelocityBasedMode (false);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, paramID, slider);
        if (auto* prm = st.getParameter (paramID))
        {
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            slider.setTooltip (prm->getName (64) + juce::String (juce::CharPointer_UTF8 ("  (double-clic = valeur par d\xc3\xa9" "faut, Ctrl+glisser = r\xc3\xa9glage fin)")));
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Pal::gold);
        g.setFont (ymFont (11.0f, true));
        g.drawFittedText (name.toUpperCase(), getLocalBounds().removeFromTop (13), juce::Justification::centred, 1);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        r.removeFromTop (13);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, r.getWidth(), 14);
        slider.setBounds (r);
    }

    juce::Slider slider;

private:
    juce::String name;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
class PowerButton : public juce::ToggleButton
{
public:
    PowerButton (juce::AudioProcessorValueTreeState& st, const juce::String& paramID, const juce::String& caption = {})
    {
        setButtonText (caption);
        setTooltip (juce::String (juce::CharPointer_UTF8 ("Activer / d\xc3\xa9sactiver")));
        att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, paramID, *this);
    }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

//==============================================================================
// Barre de mesure verticale (niveau ou réduction de gain)
class MeterBar : public juce::Component
{
public:
    MeterBar (juce::String lbl, bool isGR) : label (std::move (lbl)), gr (isGR) {}

    // valeur en dB (niveau : -60..+3 ; GR : 0..-24)
    void push (float db, float dt)
    {
        if (gr)
        {
            value = db < value ? db : juce::jmin (0.0f, value + 30.0f * dt);
            hold = juce::jmin (hold, value); holdT += dt;
            if (holdT > 1.5f) { hold = value; holdT = 0; }
        }
        else
        {
            value = db > value ? db : juce::jmax (-100.0f, value - 22.0f * dt);
            if (value >= hold) { hold = value; holdT = 0; }
            else if ((holdT += dt) > 1.5f) hold = juce::jmax (-100.0f, hold - 30.0f * dt);
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto lab = r.removeFromBottom (14);
        g.setColour (Pal::textDim);
        g.setFont (ymFont (9.5f, true));
        g.drawFittedText (label, lab.toNearestInt().expanded (6, 0), juce::Justification::centred, 1, 0.6f);

        g.setColour (juce::Colour (0xff070504));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (Pal::goldDark.withAlpha (0.7f));
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
        auto inner = r.reduced (2.0f);

        if (gr)
        {
            const float frac = juce::jlimit (0.0f, 1.0f, -value / 24.0f);
            auto bar = inner.withHeight (inner.getHeight() * frac);
            juce::ColourGradient cg (Pal::goldBright, 0, inner.getY(), Pal::orangeDeep, 0, inner.getBottom(), false);
            g.setGradientFill (cg);
            g.fillRect (bar);
            const float hy = inner.getY() + inner.getHeight() * juce::jlimit (0.0f, 1.0f, -hold / 24.0f);
            g.setColour (Pal::goldBright);
            g.fillRect (inner.getX(), hy - 1, inner.getWidth(), 2.0f);
        }
        else
        {
            auto yOf = [&] (float db) { return inner.getBottom() - inner.getHeight() * juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 63.0f); };
            const float y = yOf (value);
            juce::ColourGradient cg (Pal::red, 0, yOf (3.0f), Pal::goldDark, 0, inner.getBottom(), false);
            cg.addColour (0.143, Pal::orange);   // -6 dB
            cg.addColour (0.33, Pal::goldBright); // -18 dB
            g.setGradientFill (cg);
            g.fillRect (juce::Rectangle<float> (inner.getX(), y, inner.getWidth(), inner.getBottom() - y));
            g.setColour (hold > -0.05f ? Pal::red : Pal::goldBright);
            g.fillRect (inner.getX(), yOf (hold) - 1, inner.getWidth(), 2.0f);
        }
    }

    float getHold() const { return hold; }

private:
    juce::String label;
    bool gr;
    float value = -100, hold = -100, holdT = 0;
};

//==============================================================================
// Analyseur de spectre + courbe d'EQ
class SpectrumEQ : public juce::Component
{
public:
    explicit SpectrumEQ (YehoshuaMasterProcessor& p) : proc (p)
    {
        ring.assign ((size_t) N, 0.0f);
        fftData.assign ((size_t) N * 2, 0.0f);
        smooth.assign ((size_t) N / 2, -120.0f);
    }

    void update()
    {
        auto& fifo = proc.specFifo;
        const int ready = fifo.getNumReady();
        if (ready > 0)
        {
            int s1, n1, s2, n2;
            fifo.prepareToRead (ready, s1, n1, s2, n2);
            auto push = [this] (const float* src, int n)
            {
                for (int i = 0; i < n; ++i) { ring[(size_t) ringPos] = src[i]; ringPos = (ringPos + 1) % N; }
            };
            push (proc.specBuffer.data() + s1, n1);
            push (proc.specBuffer.data() + s2, n2);
            fifo.finishedRead (n1 + n2);
        }

        for (int i = 0; i < N; ++i) fftData[(size_t) i] = ring[(size_t) ((ringPos + i) % N)];
        std::fill (fftData.begin() + N, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), (size_t) N);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        const float norm = 2.0f / (float) N * 2.0f;   // compensation fenêtre de Hann
        for (int b = 0; b < N / 2; ++b)
        {
            const float db = juce::Decibels::gainToDecibels (fftData[(size_t) b] * norm, -120.0f);
            float& s = smooth[(size_t) b];
            s = db > s ? s + 0.6f * (db - s) : s + 0.12f * (db - s);
        }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff070504));
        g.fillRoundedRectangle (r, 4.0f);

        const float w = r.getWidth(), h = r.getHeight();
        auto xOf = [&] (double f) { return r.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * w; };
        auto fOf = [&] (float x) { return 20.0 * std::pow (1000.0, (double) (x - r.getX()) / w); };

        // grille
        g.setFont (ymFont (9.5f));
        for (double f : { 30.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            const float x = xOf (f);
            g.setColour (Pal::goldDark.withAlpha (0.22f));
            g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            g.setColour (Pal::textDim.withAlpha (0.8f));
            g.drawText (f >= 1000 ? juce::String (f / 1000.0, 0) + "k" : juce::String ((int) f), (int) x + 2, (int) r.getBottom() - 12, 30, 11, juce::Justification::left);
        }
        auto yEq = [&] (double db) { return r.getCentreY() - (float) (db / 15.0) * (h * 0.5f); };
        for (double db : { -12.0, -6.0, 0.0, 6.0, 12.0 })
        {
            g.setColour (Pal::goldDark.withAlpha (db == 0 ? 0.45f : 0.15f));
            g.drawHorizontalLine ((int) yEq (db), r.getX(), r.getRight());
            g.setColour (Pal::textDim.withAlpha (0.7f));
            g.drawText ((db > 0 ? "+" : "") + juce::String ((int) db), (int) r.getRight() - 26, (int) yEq (db) - 6, 22, 11, juce::Justification::right);
        }

        // spectre (pente +4.5 dB/oct pour une lecture musicale)
        const double sr = proc.getCurrentSampleRate();
        juce::Path spec;
        bool started = false;
        for (float x = r.getX(); x <= r.getRight(); x += 1.5f)
        {
            const double f = fOf (x);
            const double binF = f / sr * N;
            const int b0 = juce::jlimit (1, N / 2 - 2, (int) binF);
            const float fr = (float) (binF - b0);
            float db = smooth[(size_t) b0] * (1 - fr) + smooth[(size_t) b0 + 1] * fr;
            db += 4.5f * (float) std::log2 (f / 1000.0);
            const float y = r.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 84.0f) / 84.0f) * h;
            if (! started) { spec.startNewSubPath (x, r.getBottom()); started = true; }
            spec.lineTo (x, y);
        }
        spec.lineTo (r.getRight(), r.getBottom());
        spec.closeSubPath();
        g.setGradientFill (juce::ColourGradient (Pal::orange.withAlpha (0.55f), 0, r.getY(), Pal::orangeDeep.withAlpha (0.05f), 0, r.getBottom(), false));
        g.fillPath (spec);
        g.setColour (Pal::orange.withAlpha (0.9f));
        g.strokePath (spec, juce::PathStrokeType (1.0f));

        // courbe d'EQ calculée à partir des paramètres actuels
        auto& st = proc.apvts;
        auto v = [&] (const char* id) { return (double) st.getRawParameterValue (id)->load(); };
        if (v ("eqOn") > 0.5)
        {
            ym::Biquad f[7];
            f[0].set (ym::Biquad::Type::HighPass, sr, v ("hpfFreq"), 0.5411961);
            f[1].set (ym::Biquad::Type::HighPass, sr, v ("hpfFreq"), 1.3065630);
            f[2].set (ym::Biquad::Type::LowShelf, sr, v ("lsFreq"), 0.707, v ("lsGain"));
            f[3].set (ym::Biquad::Type::Peak, sr, v ("b1Freq"), v ("b1Q"), v ("b1Gain"));
            f[4].set (ym::Biquad::Type::Peak, sr, v ("b2Freq"), v ("b2Q"), v ("b2Gain"));
            f[5].set (ym::Biquad::Type::Peak, sr, v ("b3Freq"), v ("b3Q"), v ("b3Gain"));
            f[6].set (ym::Biquad::Type::HighShelf, sr, v ("hsFreq"), 0.707, v ("hsGain"));
            const bool hp = v ("hpfOn") > 0.5;

            juce::Path eq;
            for (float x = r.getX(); x <= r.getRight(); x += 2.0f)
            {
                const double fq = fOf (x);
                double mag = 1.0;
                for (int i = hp ? 0 : 2; i < 7; ++i) mag *= f[i].magnitude (fq, sr);
                const float y = juce::jlimit (r.getY(), r.getBottom(), yEq (ym::gainToDb (mag)));
                if (x == r.getX()) eq.startNewSubPath (x, y); else eq.lineTo (x, y);
            }
            g.setColour (Pal::goldBright.withAlpha (0.25f));
            g.strokePath (eq, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
            g.setColour (Pal::goldBright);
            g.strokePath (eq, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

            const std::pair<const char*, const char*> nodes[] = { { "lsFreq", "lsGain" }, { "b1Freq", "b1Gain" }, { "b2Freq", "b2Gain" }, { "b3Freq", "b3Gain" }, { "hsFreq", "hsGain" } };
            int idx = 1;
            for (auto& [fid, gid] : nodes)
            {
                auto c = juce::Point<float> (xOf (v (fid)), yEq (v (gid)));
                g.setColour (Pal::bg); g.fillEllipse (juce::Rectangle<float> (14, 14).withCentre (c));
                g.setColour (Pal::orange); g.drawEllipse (juce::Rectangle<float> (14, 14).withCentre (c), 1.6f);
                g.setColour (Pal::goldBright); g.setFont (ymFont (9.0f, true));
                g.drawText (juce::String (idx++), juce::Rectangle<float> (14, 14).withCentre (c), juce::Justification::centred);
            }
        }

        g.setColour (Pal::goldDark);
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    }

private:
    YehoshuaMasterProcessor& proc;
    static constexpr int order = 12, N = 1 << order;
    juce::dsp::FFT fft { order };
    juce::dsp::WindowingFunction<float> window { (size_t) N, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> ring, fftData, smooth;
    int ringPos = 0;
};
