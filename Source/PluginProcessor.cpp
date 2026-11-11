#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;
using NR  = juce::NormalisableRange<float>;

namespace
{
NR freqRange (float lo, float hi)
{
    NR r (lo, hi, 0.01f);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

juce::String formatValue (float v, const juce::String& unit)
{
    if (unit == "Hz")  return v >= 1000.f ? juce::String (v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    if (unit == "dB")  return (v > 0.004f ? "+" : "") + juce::String (v, 1) + " dB";
    if (unit == "%")   return juce::String (juce::roundToInt (v)) + " %";
    if (unit == "ms")  return v < 10.f ? juce::String (v, 1) + " ms" : juce::String (juce::roundToInt (v)) + " ms";
    if (unit == ":1")  return juce::String (v, 1) + ":1";
    return juce::String (v, 2);
}

std::unique_ptr<APF> fp (const juce::String& id, const juce::String& name, NR range, float def, const juce::String& unit)
{
    return std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, def,
                                  juce::AudioParameterFloatAttributes()
                                      .withLabel (unit)
                                      .withStringFromValueFunction ([unit] (float v, int) { return formatValue (v, unit); })
                                      .withValueFromStringFunction ([] (const juce::String& t) { return t.retainCharacters ("-0123456789.").getFloatValue() * (t.containsIgnoreCase ("k") ? 1000.f : 1.f); }));
}
std::unique_ptr<APB> bp (const char* id, const char* name, bool def)
{
    return std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def);
}
} // namespace

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout YehoshuaMasterProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;

    // ----- Entrée / sortie
    L.add (fp ("inGain",  "Input Gain",  NR (-24.f, 24.f, 0.01f), 0.f, "dB"));
    L.add (fp ("outGain", "Output Gain", NR (-24.f, 0.f, 0.01f), 0.f, "dB"));

    // ----- EQ
    L.add (bp ("eqOn",  "EQ On", true));
    L.add (bp ("hpfOn", "Low Cut On", false));
    L.add (fp ("hpfFreq", "Low Cut Freq", freqRange (10.f, 300.f), 25.f, "Hz"));
    L.add (fp ("lsFreq",  "Low Shelf Freq", freqRange (20.f, 500.f), 80.f, "Hz"));
    L.add (fp ("lsGain",  "Low Shelf Gain", NR (-12.f, 12.f, 0.01f), 0.f, "dB"));
    L.add (fp ("b1Freq",  "Low-Mid Freq", freqRange (40.f, 1200.f), 250.f, "Hz"));
    L.add (fp ("b1Gain",  "Low-Mid Gain", NR (-12.f, 12.f, 0.01f), 0.f, "dB"));
    L.add (fp ("b1Q",     "Low-Mid Q",    freqRange (0.2f, 6.f), 1.0f, ""));
    L.add (fp ("b2Freq",  "Mid Freq",     freqRange (200.f, 6000.f), 1200.f, "Hz"));
    L.add (fp ("b2Gain",  "Mid Gain",     NR (-12.f, 12.f, 0.01f), 0.f, "dB"));
    L.add (fp ("b2Q",     "Mid Q",        freqRange (0.2f, 6.f), 1.0f, ""));
    L.add (fp ("b3Freq",  "Presence Freq", freqRange (1000.f, 16000.f), 4500.f, "Hz"));
    L.add (fp ("b3Gain",  "Presence Gain", NR (-12.f, 12.f, 0.01f), 0.f, "dB"));
    L.add (fp ("b3Q",     "Presence Q",    freqRange (0.2f, 6.f), 1.0f, ""));
    L.add (fp ("hsFreq",  "High Shelf Freq", freqRange (2000.f, 20000.f), 10000.f, "Hz"));
    L.add (fp ("hsGain",  "High Shelf Gain", NR (-12.f, 12.f, 0.01f), 0.f, "dB"));

    // ----- Compresseur multibande
    L.add (bp ("mbOn", "Multiband On", false));
    L.add (fp ("xLow",  "Crossover Low",  freqRange (40.f, 600.f), 120.f, "Hz"));
    L.add (fp ("xHigh", "Crossover High", freqRange (1000.f, 10000.f), 2500.f, "Hz"));
    L.add (fp ("mbMix", "Multiband Mix", NR (0.f, 100.f, 0.1f), 100.f, "%"));
    const char* bands[] = { "lo", "mid", "hi" };
    const char* bandNames[] = { "Low", "Mid", "High" };
    const float defAtt[] = { 20.f, 10.f, 5.f }, defRel[] = { 200.f, 150.f, 100.f };
    for (int b = 0; b < 3; ++b)
    {
        const juce::String id (bands[b]), nm (bandNames[b]);
        L.add (fp (id + "Thr",   nm + " Threshold", NR (-60.f, 0.f, 0.01f), -18.f, "dB"));
        L.add (fp (id + "Ratio", nm + " Ratio",     freqRange (1.f, 20.f), 2.f, ":1"));
        L.add (fp (id + "Att",   nm + " Attack",    freqRange (0.1f, 200.f), defAtt[b], "ms"));
        L.add (fp (id + "Rel",   nm + " Release",   freqRange (10.f, 2000.f), defRel[b], "ms"));
        L.add (fp (id + "Gain",  nm + " Makeup",    NR (-12.f, 12.f, 0.01f), 0.f, "dB"));
    }

    // ----- Saturation / Exciter
    L.add (bp ("satOn", "Saturation On", false));
    L.add (std::make_unique<APC> (juce::ParameterID { "satType", 1 }, "Saturation Type", juce::StringArray { "Tape", "Tube" }, 0));
    L.add (fp ("satDrive", "Saturation Drive", NR (0.f, 24.f, 0.01f), 6.f, "dB"));
    L.add (fp ("satMix",   "Saturation Mix",   NR (0.f, 100.f, 0.1f), 50.f, "%"));
    L.add (bp ("excOn", "Exciter On", false));
    L.add (fp ("excSub", "Sub Harmonics", NR (0.f, 100.f, 0.1f), 25.f, "%"));
    L.add (fp ("excAir", "Air",           NR (0.f, 100.f, 0.1f), 20.f, "%"));

    // ----- Image stéréo
    L.add (bp ("imgOn", "Stereo On", true));
    L.add (fp ("imgWidth", "Stereo Width", NR (0.f, 200.f, 0.1f), 100.f, "%"));
    L.add (bp ("monoBassOn", "Mono Bass On", false));
    L.add (fp ("monoBassFreq", "Mono Bass Freq", freqRange (40.f, 300.f), 120.f, "Hz"));

    // ----- Soft clipper
    L.add (bp ("clipOn", "Clipper On", false));
    L.add (fp ("clipDrive",   "Clipper Drive",   NR (0.f, 18.f, 0.01f), 0.f, "dB"));
    L.add (fp ("clipCeiling", "Clipper Ceiling", NR (-12.f, 0.f, 0.01f), -0.5f, "dB"));
    L.add (fp ("clipKnee",    "Clipper Softness", NR (0.f, 100.f, 0.1f), 50.f, "%"));

    // ----- Limiteur
    L.add (bp ("limOn", "Limiter On", true));
    L.add (fp ("limGain",    "Limiter Gain",    NR (0.f, 24.f, 0.01f), 0.f, "dB"));
    L.add (fp ("limCeiling", "Limiter Ceiling", NR (-6.f, 0.f, 0.01f), -0.3f, "dB"));
    L.add (fp ("limRelease", "Limiter Release", freqRange (1.f, 1000.f), 80.f, "ms"));

    L.add (std::make_unique<APC> (juce::ParameterID { "dither", 1 }, "Dither", juce::StringArray { "Off", "24-bit", "16-bit" }, 0));
    return L;
}

//==============================================================================
YehoshuaMasterProcessor::YehoshuaMasterProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, osFactorLog2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
}

bool YehoshuaMasterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void YehoshuaMasterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSR = sampleRate;
    const double osSR = sampleRate * osFactor;

    oversampler->reset();
    oversampler->initProcessing ((size_t) samplesPerBlock);

    for (int c = 0; c < 2; ++c)
    {
        hpf[c][0].reset(); hpf[c][1].reset(); ls[c].reset(); b1[c].reset(); b2[c].reset(); b3[c].reset(); hs[c].reset();
        split[c].reset();
    }
    for (auto& cp : comp) cp.prepare (sampleRate);
    exciter.prepare (sampleRate);
    imager.prepare (sampleRate, p ("monoBassFreq"));
    lastMonoFreq = p ("monoBassFreq");
    sat.prepare (osSR);

    const int k = juce::jmax (1, (int) std::round (0.0015 * sampleRate));   // look-ahead 1.5 ms
    limiter.prepareWithLength (osSR, k * osFactor + 1);
    loudness.prepare (sampleRate);

    inGainS.reset (sampleRate, 0.05);  inGainS.setCurrentAndTargetValue (ym::dbToGain (p ("inGain")));
    outGainS.reset (sampleRate, 0.05); outGainS.setCurrentAndTargetValue (ym::dbToGain (p ("outGain")));
    widthS.reset (sampleRate, 0.05);   widthS.setCurrentAndTargetValue (p ("imgWidth") * 0.01);
    limGainS.reset (osSR, 0.05);       limGainS.setCurrentAndTargetValue (ym::dbToGain (p ("limGain")));
    clipDriveS.reset (osSR, 0.05);     clipDriveS.setCurrentAndTargetValue (ym::dbToGain (p ("clipDrive")));

    corrLR = corrLL = corrRR = 0;
    setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()) + k);
}

void YehoshuaMasterProcessor::updateFilters()
{
    const double sr = currentSR;
    for (int c = 0; c < 2; ++c)
    {
        hpf[c][0].set (ym::Biquad::Type::HighPass, sr, p ("hpfFreq"), 0.5411961);
        hpf[c][1].set (ym::Biquad::Type::HighPass, sr, p ("hpfFreq"), 1.3065630);
        ls[c].set (ym::Biquad::Type::LowShelf,  sr, p ("lsFreq"), 0.707, p ("lsGain"));
        b1[c].set (ym::Biquad::Type::Peak,      sr, p ("b1Freq"), p ("b1Q"), p ("b1Gain"));
        b2[c].set (ym::Biquad::Type::Peak,      sr, p ("b2Freq"), p ("b2Q"), p ("b2Gain"));
        b3[c].set (ym::Biquad::Type::Peak,      sr, p ("b3Freq"), p ("b3Q"), p ("b3Gain"));
        hs[c].set (ym::Biquad::Type::HighShelf, sr, p ("hsFreq"), 0.707, p ("hsGain"));

        const double xl = p ("xLow");
        const double xh = juce::jmax ((double) p ("xHigh"), xl * 1.5);
        split[c].set (sr, xl, xh);
    }
    const char* bands[] = { "lo", "mid", "hi" };
    for (int b = 0; b < 3; ++b)
    {
        const juce::String id (bands[b]);
        comp[b].setParams (apvts.getRawParameterValue (id + "Thr")->load(),
                           apvts.getRawParameterValue (id + "Ratio")->load(),
                           apvts.getRawParameterValue (id + "Att")->load(),
                           apvts.getRawParameterValue (id + "Rel")->load(),
                           apvts.getRawParameterValue (id + "Gain")->load());
    }
    const float mf = p ("monoBassFreq");
    if (mf != lastMonoFreq) { imager.setMonoFreq (sr, mf); lastMonoFreq = mf; }
}

//==============================================================================
static inline void atomicMax (std::atomic<float>& a, float v) { if (v > a.load (std::memory_order_relaxed)) a.store (v, std::memory_order_relaxed); }
static inline void atomicMin (std::atomic<float>& a, float v) { if (v < a.load (std::memory_order_relaxed)) a.store (v, std::memory_order_relaxed); }

void YehoshuaMasterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int N = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, N);
    if (buffer.getNumChannels() < 2 || N == 0) return;

    if (resetLufsFlag.exchange (false)) loudness.reset();
    updateFilters();

    const bool eqOn = pb ("eqOn"), hpfOn = pb ("hpfOn"), mbOn = pb ("mbOn"), satOn = pb ("satOn"),
               excOn = pb ("excOn"), imgOn = pb ("imgOn"), monoBass = pb ("monoBassOn"),
               clipOn = pb ("clipOn"), limOn = pb ("limOn");
    const double mbMix = p ("mbMix") * 0.01, excSub = p ("excSub") * 0.01, excAir = p ("excAir") * 0.01;
    const double satDrive = p ("satDrive"), satMix = p ("satMix") * 0.01;
    const int satType = (int) p ("satType");
    const double clipCeil = ym::dbToGain (p ("clipCeiling")), clipKnee = p ("clipKnee") * 0.01;
    const double limCeilDb = p ("limCeiling");
    const int ditherBits = (int) p ("dither") == 1 ? 24 : ((int) p ("dither") == 2 ? 16 : 0);

    inGainS.setTargetValue (ym::dbToGain (p ("inGain")));
    outGainS.setTargetValue (ym::dbToGain (p ("outGain")));
    widthS.setTargetValue (p ("imgWidth") * 0.01);
    limGainS.setTargetValue (ym::dbToGain (p ("limGain")));
    clipDriveS.setTargetValue (ym::dbToGain (p ("clipDrive")));
    // limiteur désactivé => plafond très haut : il reste une simple ligne de retard (latence constante)
    limiter.setParams (limOn ? limCeilDb : 60.0, p ("limRelease"));

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // ---------------------------------------------------------- étage 1 (fréquence native)
    double inPkL = 0, inPkR = 0, minEnv[3] = { 0, 0, 0 };
    for (int i = 0; i < N; ++i)
    {
        const double g = inGainS.getNextValue();
        double l = L[i] * g, r = R[i] * g;
        inPkL = std::max (inPkL, std::abs (l)); inPkR = std::max (inPkR, std::abs (r));

        if (eqOn)
        {
            double* s[2] = { &l, &r };
            for (int c = 0; c < 2; ++c)
            {
                double x = *s[c];
                if (hpfOn) x = hpf[c][1].process (hpf[c][0].process (x));
                x = hs[c].process (b3[c].process (b2[c].process (b1[c].process (ls[c].process (x)))));
                *s[c] = x;
            }
        }

        if (mbOn)
        {
            double bl[3], br[3];
            split[0].process (l, bl[0], bl[1], bl[2]);
            split[1].process (r, br[0], br[1], br[2]);
            double wl = 0, wr = 0;
            for (int b = 0; b < 3; ++b)
            {
                const double cg = comp[b].process (bl[b], br[b]);
                wl += bl[b] * cg; wr += br[b] * cg;
                minEnv[b] = std::min (minEnv[b], comp[b].envDb);
            }
            const double dl = bl[0] + bl[1] + bl[2], dr = br[0] + br[1] + br[2];
            l = dl + mbMix * (wl - dl);
            r = dr + mbMix * (wr - dr);
        }

        if (excOn)
        {
            l = exciter.processOne (0, l, excSub, excAir);
            r = exciter.processOne (1, r, excSub, excAir);
        }

        const double w = widthS.getNextValue();
        if (imgOn) imager.process (l, r, w, monoBass);

        L[i] = (float) l; R[i] = (float) r;
    }

    // ---------------------------------------------------------- étage 2 (suréchantillonné 4x)
    juce::dsp::AudioBlock<float> block (buffer);
    auto stereoBlock = block.getSubsetChannelBlock (0, 2);
    auto up = oversampler->processSamplesUp (stereoBlock);
    const int M = (int) up.getNumSamples();
    float* uL = up.getChannelPointer (0);
    float* uR = up.getChannelPointer (1);

    double minClip = 1.0, minLim = 1.0;
    for (int j = 0; j < M; ++j)
    {
        double l = uL[j], r = uR[j];

        if (satOn) sat.process (l, r, satDrive, satType, satMix);

        const double cd = clipDriveS.getNextValue();
        if (clipOn)
        {
            l *= cd; r *= cd;
            const double pk = std::max (std::abs (l), std::abs (r));
            l = ym::SoftClipper::clip (l, clipCeil, clipKnee);
            r = ym::SoftClipper::clip (r, clipCeil, clipKnee);
            const double pk2 = std::max (std::abs (l), std::abs (r));
            if (pk > 1e-9) minClip = std::min (minClip, pk2 / pk);
        }

        const double lg = limGainS.getNextValue();
        if (limOn) { l *= lg; r *= lg; }
        minLim = std::min (minLim, limiter.process (l, r));

        uL[j] = (float) l; uR[j] = (float) r;
    }
    oversampler->processSamplesDown (stereoBlock);

    // ---------------------------------------------------------- sortie + mesures
    const float safety = limOn ? (float) ym::dbToGain (limCeilDb) : 4.0f;
    double outPkL = 0, outPkR = 0;
    int written = 0;
    int start1, size1, start2, size2;
    specFifo.prepareToWrite (N, start1, size1, start2, size2);
    for (int i = 0; i < N; ++i)
    {
        const double og = outGainS.getNextValue();
        double l = juce::jlimit (-safety, safety, L[i]) * og;
        double r = juce::jlimit (-safety, safety, R[i]) * og;
        if (ditherBits > 0) { l = dither[0].process (l, ditherBits); r = dither[1].process (r, ditherBits); }
        L[i] = (float) l; R[i] = (float) r;

        outPkL = std::max (outPkL, std::abs (l)); outPkR = std::max (outPkR, std::abs (r));
        loudness.process (l, r);

        constexpr double cc = 0.9995;
        corrLR = cc * corrLR + (1 - cc) * l * r;
        corrLL = cc * corrLL + (1 - cc) * l * l;
        corrRR = cc * corrRR + (1 - cc) * r * r;

        const float mono = (float) (0.5 * (l + r));
        if (i < size1) specBuffer[(size_t) (start1 + i)] = mono, ++written;
        else if (i - size1 < size2) specBuffer[(size_t) (start2 + i - size1)] = mono, ++written;
    }
    specFifo.finishedWrite (written);

    atomicMax (meters.inPeakL,  (float) ym::gainToDb (inPkL));
    atomicMax (meters.inPeakR,  (float) ym::gainToDb (inPkR));
    atomicMax (meters.outPeakL, (float) ym::gainToDb (outPkL));
    atomicMax (meters.outPeakR, (float) ym::gainToDb (outPkR));
    atomicMin (meters.limGR,  (float) ym::gainToDb (minLim));
    atomicMin (meters.clipGR, (float) ym::gainToDb (minClip));
    for (int b = 0; b < 3; ++b) atomicMin (meters.compGR[b], (float) minEnv[b]);
    meters.lufsM.store ((float) loudness.momentary);
    meters.lufsS.store ((float) loudness.shortTerm);
    meters.lufsI.store ((float) loudness.integrated);
    const double den = std::sqrt (corrLL * corrRR);
    meters.correlation.store (den > 1e-12 ? (float) (corrLR / den) : 1.0f);
}

//==============================================================================
int YehoshuaMasterProcessor::getNumPrograms() { return (int) getFactoryPresets().size(); }

const juce::String YehoshuaMasterProcessor::getProgramName (int index)
{
    const auto& ps = getFactoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? juce::String (ps[(size_t) index].name) : juce::String();
}

void YehoshuaMasterProcessor::setCurrentProgram (int index)
{
    if (index != currentProgram) applyPreset (index);
}

void YehoshuaMasterProcessor::applyPreset (int index)
{
    const auto& ps = getFactoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    currentProgram = index;

    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, v] : ps[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

//==============================================================================
void YehoshuaMasterProcessor::switchAB()
{
    auto current = apvts.copyState();
    if (! onB)
    {
        stateA = current;
        if (! stateB.isValid()) stateB = current.createCopy();
        apvts.replaceState (stateB.createCopy());
    }
    else
    {
        stateB = current;
        if (! stateA.isValid()) stateA = current.createCopy();
        apvts.replaceState (stateA.createCopy());
    }
    onB = ! onB;
}

void YehoshuaMasterProcessor::copyCurrentToOther()
{
    if (onB) stateA = apvts.copyState();
    else     stateB = apvts.copyState();
}

void YehoshuaMasterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void YehoshuaMasterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) vt.getProperty ("program", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* YehoshuaMasterProcessor::createEditor() { return new YehoshuaMasterEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new YehoshuaMasterProcessor(); }
