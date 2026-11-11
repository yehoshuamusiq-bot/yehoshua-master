#pragma once
#include <JuceHeader.h>

// Palette tirée du logo YEHOSHU'A MUSIQ
namespace Pal
{
    const juce::Colour bg         { 0xff0b0806 };
    const juce::Colour panel      { 0xff16100a };
    const juce::Colour panelHi    { 0xff21170d };
    const juce::Colour gold       { 0xffd9a646 };
    const juce::Colour goldBright { 0xfff6d27a };
    const juce::Colour goldDark   { 0xff7a5a22 };
    const juce::Colour orange     { 0xffff7b1c };
    const juce::Colour orangeDeep { 0xffc8460a };
    const juce::Colour red        { 0xffe8361f };
    const juce::Colour text       { 0xffead9b5 };
    const juce::Colour textDim    { 0xff9c8763 };
}

inline juce::Font ymFont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}
inline juce::Font ymTitleFont (float h)
{
    // police à empattements style "logo" : Georgia (Windows/macOS), sinon repli sûr
    static const juce::String face = []
    {
        const auto names = juce::Font::findAllTypefaceNames();
        for (auto* n : { "Georgia", "Times New Roman", "DejaVu Serif", "Liberation Serif" })
            if (names.contains (n)) return juce::String (n);
        return juce::String();
    }();
    if (face.isEmpty()) return ymFont (h, true);
    return juce::Font (juce::FontOptions (face, h, juce::Font::bold));
}

//==============================================================================
class YMLookAndFeel : public juce::LookAndFeel_V4
{
public:
    YMLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, Pal::text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, Pal::orange.withAlpha (0.4f));
        setColour (juce::Label::textColourId, Pal::text);
        setColour (juce::TextEditor::textColourId, Pal::goldBright);
        setColour (juce::TextEditor::backgroundColourId, Pal::bg);
        setColour (juce::TextEditor::highlightColourId, Pal::orange.withAlpha (0.4f));
        setColour (juce::CaretComponent::caretColourId, Pal::orange);
        setColour (juce::ComboBox::backgroundColourId, Pal::bg);
        setColour (juce::ComboBox::textColourId, Pal::goldBright);
        setColour (juce::ComboBox::outlineColourId, Pal::goldDark);
        setColour (juce::ComboBox::arrowColourId, Pal::orange);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff120d08));
        setColour (juce::PopupMenu::textColourId, Pal::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Pal::orangeDeep);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TextButton::buttonColourId, Pal::panelHi);
        setColour (juce::TextButton::buttonOnColourId, Pal::orangeDeep);
        setColour (juce::TextButton::textColourOffId, Pal::gold);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::BubbleComponent::backgroundColourId, Pal::bg);
        setColour (juce::BubbleComponent::outlineColourId, Pal::gold);
        setColour (juce::TooltipWindow::backgroundColourId, Pal::bg);
        setColour (juce::TooltipWindow::textColourId, Pal::text);
        setColour (juce::TooltipWindow::outlineColourId, Pal::gold);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float size = juce::jmin (b.getWidth(), b.getHeight());
        auto r = b.withSizeKeepingCentre (size, size);
        const auto c = r.getCentre();
        const float radius = size * 0.5f;
        const float arcW = juce::jmax (2.0f, radius * 0.13f);
        const float arcR = radius - arcW * 0.5f;

        // piste
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0, startA, endA, true);
        g.setColour (juce::Colour (0xff2a1f12));
        g.strokePath (track, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // arc de valeur (bipolaire si la plage traverse 0)
        const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float zeroPos = bipolar ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
        const float a0 = startA + zeroPos * (endA - startA);
        const float a1 = startA + pos * (endA - startA);
        if (std::abs (a1 - a0) > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (c.x, c.y, arcR, arcR, 0, juce::jmin (a0, a1), juce::jmax (a0, a1), true);
            juce::ColourGradient grad (Pal::orangeDeep, c.x - radius, c.y + radius, Pal::goldBright, c.x + radius, c.y - radius, false);
            g.setGradientFill (grad);
            g.strokePath (val, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (Pal::orange.withAlpha (0.18f));
            g.strokePath (val, juce::PathStrokeType (arcW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // corps du bouton (métal noir + anneau doré)
        const float bodyR = radius - arcW * 1.9f;
        auto body = juce::Rectangle<float> (bodyR * 2, bodyR * 2).withCentre (c);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (body.translated (0, bodyR * 0.12f));
        juce::ColourGradient metal (Pal::goldBright, body.getX(), body.getY(), Pal::goldDark, body.getRight(), body.getBottom(), false);
        g.setGradientFill (metal);
        g.fillEllipse (body);
        auto inner = body.reduced (juce::jmax (1.5f, bodyR * 0.14f));
        juce::ColourGradient dark (juce::Colour (0xff3a2c1a), inner.getX(), inner.getY(), juce::Colour (0xff0c0805), inner.getRight(), inner.getBottom(), false);
        g.setGradientFill (dark);
        g.fillEllipse (inner);

        // aiguille
        const float pr0 = bodyR * 0.25f, pr1 = bodyR * 0.82f;
        const float sinA = std::sin (a1), cosA = -std::cos (a1);
        g.setColour (s.isEnabled() ? Pal::goldBright : Pal::textDim);
        g.drawLine (c.x + sinA * pr0, c.y + cosA * pr0, c.x + sinA * pr1, c.y + cosA * pr1, juce::jmax (1.5f, bodyR * 0.13f));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();

        if (b.getButtonText().isEmpty())   // bouton "power"
        {
            const float d = juce::jmin (r.getWidth(), r.getHeight());
            auto cr = r.withSizeKeepingCentre (d, d);
            if (on)
            {
                g.setColour (Pal::orange.withAlpha (0.25f));
                g.fillEllipse (cr.expanded (1.5f));
            }
            g.setColour (on ? Pal::orange : juce::Colour (0xff2a2016));
            g.fillEllipse (cr.reduced (1.5f));
            g.setColour (on ? Pal::goldBright : (hl ? Pal::gold : Pal::goldDark));
            g.drawEllipse (cr.reduced (1.5f), 1.2f);
            auto ic = cr.reduced (d * 0.3f);
            juce::Path pw;
            pw.addCentredArc (ic.getCentreX(), ic.getCentreY(), ic.getWidth() * 0.5f, ic.getHeight() * 0.5f, 0,
                              juce::MathConstants<float>::pi * 0.22f, juce::MathConstants<float>::pi * 1.78f, true);
            pw.startNewSubPath (ic.getCentreX(), ic.getY() - 1.0f);
            pw.lineTo (ic.getCentreX(), ic.getCentreY());
            g.setColour (on ? juce::Colours::black : Pal::textDim);
            g.strokePath (pw, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }

        // interrupteur pilule avec texte
        g.setColour (on ? Pal::orangeDeep : Pal::panelHi);
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (on ? Pal::goldBright : (hl ? Pal::gold : Pal::goldDark));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        const float led = r.getHeight() * 0.42f;
        auto ledR = juce::Rectangle<float> (led, led).withCentre ({ r.getX() + r.getHeight() * 0.55f, r.getCentreY() });
        g.setColour (on ? Pal::goldBright : juce::Colour (0xff3a2c1a));
        g.fillEllipse (ledR);
        g.setColour (on ? juce::Colours::white : Pal::textDim);
        g.setFont (ymFont (juce::jmin (11.0f, r.getHeight() * 0.5f), true));
        g.drawFittedText (b.getButtonText(), r.withTrimmedLeft (r.getHeight() * 0.9f).withTrimmedRight (3).toNearestInt(), juce::Justification::centred, 1, 0.7f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (Pal::bg);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (cb.hasKeyboardFocus (true) ? Pal::gold : Pal::goldDark);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
        juce::Path arrow;
        const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
        arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour (Pal::orange);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return ymFont (13.0f, true); }
    juce::Font getPopupMenuFont() override { return ymFont (14.0f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (on ? Pal::orangeDeep : (down ? Pal::goldDark : Pal::panelHi));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (on || hl ? Pal::goldBright : Pal::goldDark);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return ymFont (juce::jmin (14.0f, h * 0.6f), true); }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (ymFont (11.0f));
        l->setColour (juce::Label::textColourId, Pal::text);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineWhenEditingColourId, Pal::orange);
        return l;
    }
};
