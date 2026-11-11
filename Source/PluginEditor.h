#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components.h"

//==============================================================================
// Vue principale dessinée à 1280 x 820, mise à l'échelle par l'éditeur
class MainView : public juce::Component
{
public:
    static constexpr int W = 1280, H = 820;
    explicit MainView (YehoshuaMasterProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick (float dt);

private:
    struct Panel { juce::String title; juce::Rectangle<int> r; };
    void drawPanel (juce::Graphics&, const Panel&);
    Knob& knob (const juce::String& id, const juce::String& title);
    PowerButton& power (const juce::String& id, const juce::String& text = {});
    void refreshPresetBox();

    YehoshuaMasterProcessor& proc;
    juce::Image logo;
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<PowerButton> powers;
    std::map<juce::String, Knob*> K;
    std::map<juce::String, PowerButton*> P;

    Panel pEQ, pMB, pSat, pExc, pImg, pClip, pLim, pMeters, pIO;

    SpectrumEQ spectrum;
    juce::ComboBox satType, ditherBox, presetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> satTypeAtt, ditherAtt;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, abBtn { "A / B" }, copyBtn { "COPY A>B" }, resetLufs { "RESET" };

    MeterBar inL { "IN L", false }, inR { "IN R", false }, outL { "OUT L", false }, outR { "OUT R", false };
    MeterBar clipGR { "CLIP", true }, limGR { "LIMIT", true };
    MeterBar mComp { "COMP", true }, mClip { "CLIP", true }, mLim { "LIMIT", true };
    MeterBar bandGR[3] { { "GR", true }, { "GR", true }, { "GR", true } };

    float lufsM = -100, lufsS = -100, lufsI = -100, corr = 1, truePeakHold = -100;
    juce::Rectangle<int> lufsArea, corrArea;
};

//==============================================================================
class YehoshuaMasterEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit YehoshuaMasterEditor (YehoshuaMasterProcessor&);
    ~YehoshuaMasterEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    YMLookAndFeel laf;
    MainView view;
    juce::TooltipWindow tooltips { this, 600 };
    double lastTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YehoshuaMasterEditor)
};
