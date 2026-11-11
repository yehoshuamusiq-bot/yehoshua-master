#pragma once

#include <JuceHeader.h>
#include "dsp/MasterDSP.h"

//==============================================================================
// Mesures partagées audio -> interface (sans verrou)
struct MeterData
{
    std::atomic<float> inPeakL { -100 }, inPeakR { -100 }, outPeakL { -100 }, outPeakR { -100 };
    std::atomic<float> limGR { 0 }, clipGR { 0 }, compGR[3] { { 0 }, { 0 }, { 0 } };
    std::atomic<float> lufsM { -100 }, lufsS { -100 }, lufsI { -100 };
    std::atomic<float> correlation { 1 };
};

//==============================================================================
class YehoshuaMasterProcessor : public juce::AudioProcessor
{
public:
    YehoshuaMasterProcessor();
    ~YehoshuaMasterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}
    void applyPreset (int index);

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // A/B
    void switchAB();
    void copyCurrentToOther();
    bool isOnB() const { return onB; }

    void resetLoudness() { resetLufsFlag.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    MeterData meters;

    // Analyseur de spectre : FIFO mono post-traitement
    static constexpr int fftOrder = 12, fftSize = 1 << fftOrder;
    juce::AbstractFifo specFifo { fftSize * 2 };
    std::vector<float> specBuffer = std::vector<float> ((size_t) fftSize * 2, 0.0f);

    double getCurrentSampleRate() const { return currentSR; }

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool  pb (const char* id) const { return apvts.getRawParameterValue (id)->load() > 0.5f; }

    void updateFilters();

    double currentSR = 44100.0;
    int currentProgram = 0;
    bool onB = false;
    juce::ValueTree stateA, stateB;
    std::atomic<bool> resetLufsFlag { false };

    static constexpr int osFactorLog2 = 2;              // 4x
    static constexpr int osFactor = 1 << osFactorLog2;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    // DSP
    ym::Biquad hpf[2][2], ls[2], b1[2], b2[2], b3[2], hs[2];
    ym::ThreeBandSplitter split[2];
    ym::Compressor comp[3];
    ym::Exciter exciter;
    ym::StereoImager imager;
    ym::Saturator sat;
    ym::Limiter limiter;
    ym::LoudnessMeter loudness;
    ym::Dither dither[2];

    juce::SmoothedValue<double> inGainS, outGainS, limGainS, clipDriveS, widthS;
    double corrLR = 0, corrLL = 0, corrRR = 0;
    float lastMonoFreq = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YehoshuaMasterProcessor)
};
