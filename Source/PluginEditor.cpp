#include "PluginEditor.h"
#include "Presets.h"
#include "BinaryData.h"

//==============================================================================
MainView::MainView (YehoshuaMasterProcessor& p) : proc (p), spectrum (p)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    pEQ     = { juce::String (juce::CharPointer_UTF8 ("EQUALIZER  \xe2\x80\xa2  SPECTRUM")),   { 16, 140, 880, 300 } };
    pMB     = { "MULTIBAND DYNAMICS",        { 16, 450, 560, 210 } };
    pSat    = { "ANALOG SATURATION",         { 586, 450, 310, 100 } };
    pExc    = { juce::String (juce::CharPointer_UTF8 ("EXCITER  \xe2\x80\xa2  SUB + AIR")),     { 586, 560, 310, 100 } };
    pImg    = { "STEREO IMAGE",              { 16, 670, 286, 136 } };
    pClip   = { "SOFT CLIPPER",              { 312, 670, 286, 136 } };
    pLim    = { juce::String (juce::CharPointer_UTF8 ("LIMITER  \xe2\x80\xa2  LOUDNESS")),      { 608, 670, 288, 136 } };
    pMeters = { juce::String (juce::CharPointer_UTF8 ("METERS  \xe2\x80\xa2  LUFS")),           { 908, 140, 356, 520 } };
    pIO     = { juce::String (juce::CharPointer_UTF8 ("INPUT  \xe2\x80\xa2  OUTPUT")),          { 908, 670, 356, 136 } };

    addAndMakeVisible (spectrum);

    // EQ
    power ("eqOn"); power ("hpfOn", "ON");
    knob ("hpfFreq", "Freq");
    knob ("lsFreq", "Freq"); knob ("lsGain", "Gain");
    for (auto b : { "b1", "b2", "b3" })
    {
        const juce::String s (b);
        knob (s + "Freq", "Freq"); knob (s + "Gain", "Gain"); knob (s + "Q", "Q");
    }
    knob ("hsFreq", "Freq"); knob ("hsGain", "Gain");

    // Multibande
    power ("mbOn");
    knob ("xLow", "X-Over Lo"); knob ("xHigh", "X-Over Hi"); knob ("mbMix", "Mix");
    for (auto b : { "lo", "mid", "hi" })
    {
        const juce::String s (b);
        knob (s + "Thr", "Thresh"); knob (s + "Ratio", "Ratio"); knob (s + "Gain", "Makeup");
        knob (s + "Att", "Attack"); knob (s + "Rel", "Release");
    }
    for (auto& m : bandGR) addAndMakeVisible (m);

    // Saturation / Exciter
    power ("satOn"); knob ("satDrive", "Drive"); knob ("satMix", "Mix");
    satType.addItemList ({ "Tape", "Tube" }, 1);
    addAndMakeVisible (satType);
    satTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "satType", satType);
    power ("excOn"); knob ("excSub", "Sub Harmonics"); knob ("excAir", "Air");

    // Stéréo / clipper / limiteur
    power ("imgOn"); knob ("imgWidth", "Width"); knob ("monoBassFreq", "Mono Below"); power ("monoBassOn", "MONO BASS");
    power ("clipOn"); knob ("clipDrive", "Drive"); knob ("clipCeiling", "Ceiling"); knob ("clipKnee", "Softness");
    power ("limOn"); knob ("limGain", "Boost"); knob ("limCeiling", "Ceiling"); knob ("limRelease", "Release");
    addAndMakeVisible (clipGR); addAndMakeVisible (limGR);
    for (auto* m : { &mComp, &mClip, &mLim }) addAndMakeVisible (m);

    // E/S
    knob ("inGain", "Input"); knob ("outGain", "Output");
    ditherBox.addItemList ({ "Off", "24-bit", "16-bit" }, 1);
    addAndMakeVisible (ditherBox);
    ditherAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "dither", ditherBox);

    for (auto* m : { &inL, &inR, &outL, &outR }) addAndMakeVisible (m);

    // Presets + A/B
    for (const auto& pr : getFactoryPresets()) presetBox.addItem (pr.name, presetBox.getNumItems() + 1);
    presetBox.setTextWhenNothingSelected ("Choisir un preset...");
    presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0) proc.applyPreset (id - 1); };
    addAndMakeVisible (presetBox);
    refreshPresetBox();

    auto step = [this] (int d)
    {
        const int n = (int) getFactoryPresets().size();
        const int idx = ((proc.getCurrentProgram() + d) % n + n) % n;
        proc.applyPreset (idx);
        refreshPresetBox();
    };
    prevBtn.onClick = [step] { step (-1); };
    nextBtn.onClick = [step] { step (1); };
    abBtn.setButtonText ("A");
    abBtn.setTooltip (juce::String (juce::CharPointer_UTF8 ("Comparer deux r\xc3\xa9glages A / B")));
    abBtn.onClick = [this] { proc.switchAB(); abBtn.setButtonText (proc.isOnB() ? "B" : "A"); abBtn.setToggleState (proc.isOnB(), juce::dontSendNotification); };
    copyBtn.setTooltip (juce::String (juce::CharPointer_UTF8 ("Copier le r\xc3\xa9glage actuel vers l'autre m\xc3\xa9moire (A/B)")));
    copyBtn.onClick = [this] { proc.copyCurrentToOther(); };
    resetLufs.setTooltip (juce::String (juce::CharPointer_UTF8 ("Remettre \xc3\xa0 z\xc3\xa9ro la mesure LUFS int\xc3\xa9gr\xc3\xa9" "e")));
    resetLufs.onClick = [this] { proc.resetLoudness(); };
    for (auto* b : { &prevBtn, &nextBtn, &abBtn, &copyBtn, &resetLufs }) addAndMakeVisible (b);

    setSize (W, H);
}

Knob& MainView::knob (const juce::String& id, const juce::String& title)
{
    auto* k = knobs.add (new Knob (proc.apvts, id, title));
    K[id] = k;
    addAndMakeVisible (k);
    return *k;
}

PowerButton& MainView::power (const juce::String& id, const juce::String& text)
{
    auto* b = powers.add (new PowerButton (proc.apvts, id, text));
    P[id] = b;
    addAndMakeVisible (b);
    return *b;
}

void MainView::refreshPresetBox()
{
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

//==============================================================================
void MainView::resized()
{
    auto headerPower = [] (const Panel& pn) { return juce::Rectangle<int> (pn.r.getRight() - 30, pn.r.getY() + 5, 22, 22); };

    // En-tête
    presetBox.setBounds (900, 30, 268, 30);
    prevBtn.setBounds (1174, 30, 42, 30);
    nextBtn.setBounds (1222, 30, 42, 30);
    abBtn.setBounds (900, 70, 60, 26);
    copyBtn.setBounds (966, 70, 110, 26);

    // ---- EQ
    P["eqOn"]->setBounds (headerPower (pEQ));
    auto eq = pEQ.r.reduced (12, 0);
    spectrum.setBounds (eq.getX(), pEQ.r.getY() + 32, eq.getWidth(), 158);
    {
        const juce::StringArray cols[] = { { "hpfFreq" }, { "lsFreq", "lsGain" }, { "b1Freq", "b1Gain", "b1Q" },
                                           { "b2Freq", "b2Gain", "b2Q" }, { "b3Freq", "b3Gain", "b3Q" }, { "hsFreq", "hsGain" } };
        const float colW = eq.getWidth() / 6.0f;
        const int y = pEQ.r.getY() + 210, h = 84;
        for (int c = 0; c < 6; ++c)
        {
            auto col = juce::Rectangle<int> ((int) (eq.getX() + c * colW), y, (int) colW, h).reduced (4, 0);
            if (c == 0)
            {
                P["hpfOn"]->setBounds (col.getX() + 4, y + 32, 46, 20);
                K["hpfFreq"]->setBounds (col.withTrimmedLeft (54));
                continue;
            }
            const int n = cols[c].size();
            const int kw = col.getWidth() / n;
            for (int i = 0; i < n; ++i)
                K[cols[c][i]]->setBounds (col.getX() + i * kw, y, kw, h);
        }
    }

    // ---- Multibande
    P["mbOn"]->setBounds (headerPower (pMB));
    {
        const int x0 = pMB.r.getX() + 10, y0 = pMB.r.getY() + 30;
        K["xLow"]->setBounds (x0, y0, 90, 58);
        K["xHigh"]->setBounds (x0, y0 + 59, 90, 58);
        K["mbMix"]->setBounds (x0, y0 + 118, 90, 58);
        const char* bands[] = { "lo", "mid", "hi" };
        const int bw = 146;
        for (int b = 0; b < 3; ++b)
        {
            const juce::String s (bands[b]);
            const int bx = pMB.r.getX() + 110 + b * (bw + 2);
            const int kw = (bw - 26) / 3;
            K[s + "Thr"]->setBounds (bx, y0 + 18, kw, 76);
            K[s + "Ratio"]->setBounds (bx + kw, y0 + 18, kw, 76);
            K[s + "Gain"]->setBounds (bx + 2 * kw, y0 + 18, kw, 76);
            K[s + "Att"]->setBounds (bx + 4, y0 + 98, (bw - 26) / 2 - 4, 76);
            K[s + "Rel"]->setBounds (bx + (bw - 26) / 2, y0 + 98, (bw - 26) / 2 - 4, 76);
            bandGR[b].setBounds (bx + bw - 22, y0 + 20, 18, 154);
        }
    }

    // ---- Saturation
    P["satOn"]->setBounds (headerPower (pSat));
    satType.setBounds (pSat.r.getX() + 14, pSat.r.getY() + 52, 84, 26);
    K["satDrive"]->setBounds (pSat.r.getX() + 108, pSat.r.getY() + 28, 94, 68);
    K["satMix"]->setBounds (pSat.r.getX() + 206, pSat.r.getY() + 28, 94, 68);

    // ---- Exciter
    P["excOn"]->setBounds (headerPower (pExc));
    K["excSub"]->setBounds (pExc.r.getX() + 20, pExc.r.getY() + 28, 130, 68);
    K["excAir"]->setBounds (pExc.r.getX() + 160, pExc.r.getY() + 28, 130, 68);

    // ---- Stéréo
    P["imgOn"]->setBounds (headerPower (pImg));
    K["imgWidth"]->setBounds (pImg.r.getX() + 8, pImg.r.getY() + 30, 92, 100);
    K["monoBassFreq"]->setBounds (pImg.r.getX() + 100, pImg.r.getY() + 30, 92, 100);
    P["monoBassOn"]->setBounds (pImg.r.getX() + 192, pImg.r.getY() + 66, 88, 24);

    // ---- Clipper
    P["clipOn"]->setBounds (headerPower (pClip));
    K["clipDrive"]->setBounds (pClip.r.getX() + 6, pClip.r.getY() + 30, 82, 100);
    K["clipCeiling"]->setBounds (pClip.r.getX() + 88, pClip.r.getY() + 30, 82, 100);
    K["clipKnee"]->setBounds (pClip.r.getX() + 170, pClip.r.getY() + 30, 82, 100);
    clipGR.setBounds (pClip.r.getX() + 256, pClip.r.getY() + 32, 22, 98);

    // ---- Limiteur
    P["limOn"]->setBounds (headerPower (pLim));
    K["limGain"]->setBounds (pLim.r.getX() + 4, pLim.r.getY() + 28, 96, 104);
    K["limCeiling"]->setBounds (pLim.r.getX() + 100, pLim.r.getY() + 34, 76, 96);
    K["limRelease"]->setBounds (pLim.r.getX() + 176, pLim.r.getY() + 34, 76, 96);
    limGR.setBounds (pLim.r.getX() + 258, pLim.r.getY() + 32, 22, 98);

    // ---- Mesures
    resetLufs.setBounds (pMeters.r.getRight() - 70, pMeters.r.getY() + 5, 60, 22);
    {
        const int x = pMeters.r.getX(), y = pMeters.r.getY() + 66, h = 272;
        inL.setBounds (x + 22, y, 24, h);   inR.setBounds (x + 50, y, 24, h);
        mComp.setBounds (x + 132, y, 28, h); mClip.setBounds (x + 166, y, 28, h); mLim.setBounds (x + 200, y, 28, h);
        outL.setBounds (x + 282, y, 24, h); outR.setBounds (x + 310, y, 24, h);
        lufsArea = { x + 12, pMeters.r.getY() + 346, 332, 112 };
        corrArea = { x + 12, pMeters.r.getY() + 466, 332, 44 };
    }

    // ---- E/S
    K["inGain"]->setBounds (pIO.r.getX() + 8, pIO.r.getY() + 30, 100, 100);
    K["outGain"]->setBounds (pIO.r.getX() + 112, pIO.r.getY() + 30, 100, 100);
    ditherBox.setBounds (pIO.r.getX() + 226, pIO.r.getY() + 62, 116, 26);
}

//==============================================================================
void MainView::drawPanel (juce::Graphics& g, const Panel& pn)
{
    auto r = pn.r.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0, 3), 8.0f);
    g.setGradientFill (juce::ColourGradient (Pal::panelHi, 0, r.getY(), Pal::panel, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Pal::goldDark);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.2f);
    g.setColour (Pal::gold.withAlpha (0.12f));
    g.drawRoundedRectangle (r.reduced (3.5f), 6.0f, 1.0f);

    g.setColour (Pal::gold);
    g.setFont (ymFont (13.0f, true));
    g.drawText (pn.title, (int) r.getX() + 14, (int) r.getY() + 7, (int) r.getWidth() - 60, 18, juce::Justification::left);
    g.setColour (Pal::orange);
    g.fillRect (r.getX() + 14, r.getY() + 26, 34.0f, 2.0f);
}

void MainView::paint (juce::Graphics& g)
{
    // fond
    g.fillAll (Pal::bg);
    g.setGradientFill (juce::ColourGradient (Pal::orangeDeep.withAlpha (0.20f), 640, 30, Pal::bg.withAlpha (0.0f), 640, 330, true));
    g.fillRect (0, 0, W, 300);

    // barres d'égaliseur décoratives (comme le logo)
    {
        juce::Random rnd (79);
        for (int i = 0; i < 64; ++i)
        {
            const float x = 440.0f + i * 7.0f;
            const float hgt = 14.0f + rnd.nextFloat() * 70.0f * std::sin ((float) i / 64.0f * juce::MathConstants<float>::pi);
            for (float y = 128.0f - hgt; y < 126.0f; y += 4.0f)
            {
                g.setColour (Pal::orange.withAlpha (0.05f + 0.10f * (1.0f - (126.0f - y) / 90.0f)));
                g.fillRect (x, y, 5.0f, 2.5f);
            }
        }
    }

    // logo + titre
    g.setOpacity (1.0f);
    if (logo.isValid())
        g.drawImage (logo, juce::Rectangle<float> (16, 4, 128, 128), juce::RectanglePlacement::centred);

    {
        juce::ColourGradient tg (Pal::goldBright, 0, 22, Pal::orangeDeep, 0, 72, false);
        tg.addColour (0.5, Pal::gold);
        g.setGradientFill (tg);
        g.setFont (ymTitleFont (46.0f));
        g.drawText ("YEHOSHU'A MUSIQ", 160, 16, 700, 56, juce::Justification::left);
        g.setColour (Pal::orange);
        g.setFont (ymFont (19.0f, true));
        g.drawText ("M A S T E R    -    M A S T E R I N G   S U I T E", 162, 72, 700, 24, juce::Justification::left);
        g.setColour (Pal::textDim);
        g.setFont (ymFont (12.0f, true));
        g.drawText (juce::CharPointer_UTF8 ("BEATS \xe2\x80\xa2 VISION \xe2\x80\xa2 PURPOSE      |      PRODUCE \xe2\x80\xa2 CREATE \xe2\x80\xa2 ELEVATE"),
                    162, 100, 700, 18, juce::Justification::left);
    }

    g.setColour (Pal::textDim);
    g.setFont (ymFont (11.0f, true));
    g.drawText ("PRESET", 900, 14, 100, 14, juce::Justification::left);
    g.drawText ("v1.0  |  4x OS  |  latence " + juce::String (proc.getLatencySamples()) + " smp",
                1080, 70, 184, 26, juce::Justification::centredRight);

    // ligne dorée
    g.setGradientFill (juce::ColourGradient (Pal::gold.withAlpha (0.0f), 0, 0, Pal::gold.withAlpha (0.0f), (float) W, 0, false));
    {
        juce::ColourGradient lg (Pal::gold.withAlpha (0.0f), 0, 0, Pal::gold.withAlpha (0.0f), (float) W, 0, false);
        lg.addColour (0.5, Pal::goldBright);
        g.setGradientFill (lg);
        g.fillRect (0, 132, W, 1);
    }

    for (auto* pn : { &pEQ, &pMB, &pSat, &pExc, &pImg, &pClip, &pLim, &pMeters, &pIO })
        drawPanel (g, *pn);

    // en-têtes de colonnes EQ
    {
        auto eq = pEQ.r.reduced (12, 0);
        const char* names[] = { "LOW CUT", "1  LOW SHELF", "2  LOW-MID", "3  MID", "4  PRESENCE", "5  HIGH SHELF" };
        const float colW = eq.getWidth() / 6.0f;
        for (int c = 0; c < 6; ++c)
        {
            auto col = juce::Rectangle<float> (eq.getX() + c * colW, (float) pEQ.r.getY() + 194, colW, 14).reduced (6, 0);
            g.setColour (Pal::orange);
            g.setFont (ymFont (11.0f, true));
            g.drawText (names[c], col, juce::Justification::centred);
            if (c > 0)
            {
                g.setColour (Pal::goldDark.withAlpha (0.5f));
                g.drawVerticalLine ((int) (eq.getX() + c * colW), (float) pEQ.r.getY() + 196, (float) pEQ.r.getBottom() - 8);
            }
        }
    }
    // en-têtes multibande
    {
        const char* bn[] = { "LOW BAND", "MID BAND", "HIGH BAND" };
        for (int b = 0; b < 3; ++b)
        {
            const int bx = pMB.r.getX() + 110 + b * 148;
            g.setColour (Pal::goldDark.withAlpha (0.5f));
            g.drawVerticalLine (bx - 4, (float) pMB.r.getY() + 32, (float) pMB.r.getBottom() - 8);
            g.setColour (Pal::orange);
            g.setFont (ymFont (11.0f, true));
            g.drawText (bn[b], bx, pMB.r.getY() + 30, 120, 14, juce::Justification::centred);
        }
    }
    g.setColour (Pal::gold);
    g.setFont (ymFont (11.0f, true));
    g.drawText ("TYPE", pSat.r.getX() + 14, pSat.r.getY() + 34, 84, 14, juce::Justification::centred);
    g.drawText ("DITHER", pIO.r.getX() + 226, pIO.r.getY() + 44, 116, 14, juce::Justification::centred);
    g.setColour (Pal::textDim);
    g.setFont (ymFont (10.0f));
    g.drawText ("16-bit = export CD/MP3", pIO.r.getX() + 222, pIO.r.getY() + 94, 124, 14, juce::Justification::centred);

    // ---- Mesures : échelles et valeurs
    {
        const int x = pMeters.r.getX(), y = pMeters.r.getY() + 66, h = 272 - 14;
        g.setFont (ymFont (9.5f));
        for (int db : { 0, -3, -6, -12, -18, -24, -36, -48, -60 })
        {
            const float yy = y + 2 + (h - 4) * (1.0f - (db + 60.0f) / 63.0f);
            g.setColour (Pal::textDim);
            g.drawText (juce::String (db), x + 76, (int) yy - 6, 24, 12, juce::Justification::centredLeft);
            g.drawText (juce::String (db), x + 252, (int) yy - 6, 26, 12, juce::Justification::centredRight);
        }
        for (int db : { 0, -6, -12, -18, -24 })
        {
            const float yy = y + 2 + (h - 4) * (-db / 24.0f);
            g.setColour (Pal::textDim);
            g.drawText (juce::String (db), x + 104, (int) yy - 6, 24, 12, juce::Justification::centredRight);
        }
        g.setColour (Pal::gold);
        g.setFont (ymFont (10.5f, true));
        g.drawText ("INPUT", x + 18, y - 32, 60, 14, juce::Justification::centred);
        g.drawText ("GAIN REDUCTION", x + 120, y - 32, 120, 14, juce::Justification::centred);
        g.drawText ("OUTPUT", x + 278, y - 32, 60, 14, juce::Justification::centred);

        const float op = outL.getHold() > outR.getHold() ? outL.getHold() : outR.getHold();
        g.setColour (op > -0.05f ? Pal::red : Pal::goldBright);
        g.setFont (ymFont (12.0f, true));
        g.drawText (op <= -99 ? juce::String ("-inf") : juce::String (op, 1) + " dB", x + 270, y - 18, 76, 14, juce::Justification::centred);
    }

    // LUFS
    {
        auto a = lufsArea.toFloat();
        g.setColour (juce::Colour (0xff070504));
        g.fillRoundedRectangle (a, 6.0f);
        g.setColour (Pal::goldDark);
        g.drawRoundedRectangle (a.reduced (0.5f), 6.0f, 1.0f);
        auto fmt = [] (float v) { return v <= -99.0f ? juce::String ("--.-") : juce::String (v, 1); };
        const std::pair<const char*, float> vals[] = { { "MOMENTARY", lufsM }, { "SHORT-TERM", lufsS }, { "INTEGRATED", lufsI } };
        const float cw = a.getWidth() / 3.0f;
        for (int i = 0; i < 3; ++i)
        {
            auto c = juce::Rectangle<float> (a.getX() + i * cw, a.getY() + 8, cw, 70);
            g.setColour (Pal::textDim);
            g.setFont (ymFont (10.5f, true));
            g.drawText (vals[i].first, c.removeFromTop (14), juce::Justification::centred);
            g.setColour (i == 2 ? Pal::goldBright : Pal::gold);
            g.setFont (ymFont (i == 2 ? 30.0f : 24.0f, true));
            g.drawText (fmt (vals[i].second), c.removeFromTop (36), juce::Justification::centred);
            g.setColour (Pal::orange);
            g.setFont (ymFont (10.0f, true));
            g.drawText ("LUFS", c, juce::Justification::centredTop);
        }
        g.setColour (Pal::textDim);
        g.setFont (ymFont (10.0f));
        g.drawText (juce::CharPointer_UTF8 ("Cibles : Spotify/YouTube -14 \xe2\x80\xa2 Apple -16 \xe2\x80\xa2 Club/Rap fort -9 \xc3\xa0 -7"),
                    a.withTrimmedTop (86).withTrimmedBottom (4), juce::Justification::centred);
    }

    // corrélation
    {
        auto a = corrArea.toFloat();
        g.setColour (Pal::gold);
        g.setFont (ymFont (10.5f, true));
        g.drawText ("PHASE CORRELATION", a.removeFromTop (14), juce::Justification::left);
        auto bar = a.removeFromTop (14);
        g.setColour (juce::Colour (0xff070504));
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (Pal::goldDark);
        g.drawRoundedRectangle (bar, 3.0f, 1.0f);
        const float cx = bar.getCentreX();
        const float vx = cx + juce::jlimit (-1.0f, 1.0f, corr) * bar.getWidth() * 0.5f;
        g.setColour (corr < 0 ? Pal::red : Pal::orange);
        g.fillRect (juce::Rectangle<float> (juce::jmin (cx, vx), bar.getY() + 2, std::abs (vx - cx), bar.getHeight() - 4));
        g.setColour (Pal::goldBright);
        g.fillRect (cx - 0.5f, bar.getY(), 1.0f, bar.getHeight());
        g.setColour (Pal::textDim);
        g.setFont (ymFont (9.5f));
        g.drawText ("-1", a.withHeight (12), juce::Justification::left);
        g.drawText ("0", a.withHeight (12), juce::Justification::centred);
        g.drawText ("+1", a.withHeight (12), juce::Justification::right);
    }
}

void MainView::tick (float dt)
{
    auto& m = proc.meters;
    inL.push (m.inPeakL.exchange (-100.0f), dt);
    inR.push (m.inPeakR.exchange (-100.0f), dt);
    outL.push (m.outPeakL.exchange (-100.0f), dt);
    outR.push (m.outPeakR.exchange (-100.0f), dt);
    const float cg = m.clipGR.exchange (0.0f), lg = m.limGR.exchange (0.0f);
    clipGR.push (cg, dt); mClip.push (cg, dt);
    limGR.push (lg, dt);  mLim.push (lg, dt);
    float worst = 0.0f;
    for (int b = 0; b < 3; ++b) { const float v = m.compGR[b].exchange (0.0f); bandGR[b].push (v, dt); worst = juce::jmin (worst, v); }
    mComp.push (worst, dt);
    lufsM = m.lufsM.load(); lufsS = m.lufsS.load(); lufsI = m.lufsI.load();
    corr += 0.2f * (m.correlation.load() - corr);

    spectrum.update();
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1) refreshPresetBox();
    repaint (lufsArea.expanded (2));
    repaint (corrArea.expanded (2));
    repaint (pMeters.r.getX() + 266, pMeters.r.getY() + 46, 84, 18);
}

//==============================================================================
YehoshuaMasterEditor::YehoshuaMasterEditor (YehoshuaMasterProcessor& p)
    : AudioProcessorEditor (&p), view (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (view);
    setResizable (true, true);
    setResizeLimits (768, 492, 1920, 1230);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) MainView::W / (double) MainView::H);
    setSize (1120, 718);
    lastTime = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (30);
}

YehoshuaMasterEditor::~YehoshuaMasterEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void YehoshuaMasterEditor::paint (juce::Graphics& g) { g.fillAll (Pal::bg); }

void YehoshuaMasterEditor::resized()
{
    const float s = (float) getWidth() / (float) MainView::W;
    view.setTransform (juce::AffineTransform::scale (s));
    view.setBounds (0, 0, MainView::W, MainView::H);
}

void YehoshuaMasterEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) juce::jlimit (0.001, 0.2, (now - lastTime) * 0.001);
    lastTime = now;
    view.tick (dt);
}
