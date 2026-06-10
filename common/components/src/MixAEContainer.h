/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "EclipsaColours.h"
#include "components/icons/svg/SvgIconComponent.h"
#include "components/icons/svg/SvgIconLookup.h"

class MixAEContainer : public juce::Component, public juce::Timer {
 public:
  static constexpr int kCollapsedHeight = 36;
  static constexpr int kExpandedHeight = 220;
  static constexpr int kBinauralLockedExpandedHeight = 120;

  MixAEContainer(const juce::String& title, const juce::String& desc)
      : name_(title),
        desc_(desc),
        binauralRadio_("Binaural (3D Spatial)"),
        standardStereoRadio_("Standard Stereo") {
    nameLabel_.setText(name_, juce::dontSendNotification);
    nameLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::headingGrey);
    nameLabel_.setJustificationType(juce::Justification::bottomLeft);
    addAndMakeVisible(nameLabel_);

    descLabel_.setText(desc_, juce::dontSendNotification);
    descLabel_.setColour(juce::Label::textColourId,
                         EclipsaColours::tabTextGrey);
    descLabel_.setFont(juce::Font(12.0f));
    descLabel_.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(descLabel_);

    addAndMakeVisible(removeAEButton_);
    addAndMakeVisible(headphonesIcon_);

    selectedOptionLabel_.setText("Standard Stereo", juce::dontSendNotification);
    selectedOptionLabel_.setColour(juce::Label::textColourId,
                                   EclipsaColours::headingGrey);
    selectedOptionLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(selectedOptionLabel_);

    expandToggleButton_.onClick = [this]() { toggleExpand(); };
    addAndMakeVisible(expandToggleButton_);

    nameLabel_.setInterceptsMouseClicks(false, false);
    descLabel_.setInterceptsMouseClicks(false, false);
    selectedOptionLabel_.setInterceptsMouseClicks(false, false);
    headphonesIcon_.setInterceptsMouseClicks(false, false);

    addChildComponent(headingRow_);

    sectionSubtitleLabel_.setText(
        "Defines how playback devices render this Audio Element on headphones.",
        juce::dontSendNotification);
    sectionSubtitleLabel_.setColour(juce::Label::textColourId,
                                    EclipsaColours::tabTextGrey);
    sectionSubtitleLabel_.setFont(juce::Font(13.0f));
    sectionSubtitleLabel_.setJustificationType(
        juce::Justification::centredLeft);
    addChildComponent(sectionSubtitleLabel_);

    binauralRadio_.setRadioGroupId(1);
    binauralRadio_.setLookAndFeel(&radioLookAndFeel_);
    addChildComponent(binauralRadio_);

    addChildComponent(binauralDesc_);

    standardStereoRadio_.setRadioGroupId(1);
    standardStereoRadio_.setToggleState(true, juce::dontSendNotification);
    standardStereoRadio_.setLookAndFeel(&radioLookAndFeel_);
    addChildComponent(standardStereoRadio_);

    addChildComponent(standardStereoDesc_);

    addChildComponent(binauralLockedMessage_);
  }

  ~MixAEContainer() override {
    stopTimer();
    juce::Desktop::getInstance().removeGlobalMouseListener(
        &globalMouseWatcher_);
    for (auto listener : listeners_) removeAEButton_.removeListener(listener);
    binauralRadio_.setLookAndFeel(nullptr);
    standardStereoRadio_.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
  }

  void setBinauralChangeHandler(std::function<void(bool)> callback) {
    binauralRadio_.onClick = [callback, this]() {
      if (binauralRadio_.getToggleState()) {
        selectedOptionLabel_.setText("Binaural", juce::dontSendNotification);
        resized();
        callback(true);
      }
    };
    standardStereoRadio_.onClick = [callback, this]() {
      if (standardStereoRadio_.getToggleState()) {
        selectedOptionLabel_.setText("Standard Stereo",
                                     juce::dontSendNotification);
        resized();
        callback(false);
      }
    };
  }

  void setBinauralState(bool isBinaural) {
    binauralRadio_.setToggleState(isBinaural, juce::dontSendNotification);
    standardStereoRadio_.setToggleState(!isBinaural,
                                        juce::dontSendNotification);
    selectedOptionLabel_.setText(isBinaural ? "Binaural" : "Standard Stereo",
                                 juce::dontSendNotification);
  }

  void setAudioElementConstraints(bool isBinauralElement,
                                  bool isStereoElement) {
    isBinauralLocked_ = isBinauralElement;
    const bool defaultBinaural = !isBinauralElement && !isStereoElement;
    setBinauralState(defaultBinaural);
    binauralRadio_.setEnabled(true);
    standardStereoRadio_.setEnabled(true);
    updateExpandedVisibility();
    resized();
  }

  void setOnExpandStateChanged(std::function<void()> cb) {
    onExpandStateChanged_ = std::move(cb);
  }

  int getPreferredHeight() const { return (int)std::round(animatedHeight_); }

  void resized() override {
    auto bounds = getLocalBounds();

    // Header
    auto headerBounds = bounds.removeFromTop(kCollapsedHeight);
    const auto fullHeaderBounds = headerBounds;

    removeAEButton_.setBounds(headerBounds.removeFromLeft(36).reduced(8, 8));

    auto labelBounds =
        headerBounds.removeFromLeft(headerBounds.proportionOfWidth(0.5f));
    labelBounds.removeFromTop(4);
    nameLabel_.setBounds(
        labelBounds.removeFromTop(labelBounds.getHeight() / 2));
    labelBounds.removeFromTop(-2);
    descLabel_.setBounds(labelBounds);

    headerBounds.reduce(10, 10);
    constexpr int kIconTextGap = 6;
    expandToggleButton_.setBounds(headerBounds.removeFromRight(14));
    headerBounds.removeFromRight(kIconTextGap);
    juce::AttributedString as;
    as.append(selectedOptionLabel_.getText(), selectedOptionLabel_.getFont(),
              EclipsaColours::headingGrey);
    juce::TextLayout tl;
    tl.createLayout(as, 10000.0f);
    const int textWidth = (int)std::ceil(tl.getWidth()) + 8;
    selectedOptionLabel_.setBounds(headerBounds.removeFromRight(textWidth));
    headerBounds.removeFromRight(kIconTextGap);
    headphonesIcon_.setBounds(headerBounds.removeFromRight(20));

    if (!isExpanded_) return;

    // Expanded panel
    auto panelBounds = bounds;
    panelBounds.removeFromLeft(12);
    panelBounds.removeFromRight(12);
    panelBounds.removeFromTop(12);

    const int w = panelBounds.getWidth();
    const int descWidth = w - 32;

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::column;

    fb.items.add(
        juce::FlexItem(headingRow_).withWidth((float)w).withHeight(26.0f));

    if (isBinauralLocked_) {
      fb.items.add(
          juce::FlexItem(binauralLockedMessage_)
              .withWidth((float)w)
              .withHeight((float)binauralLockedMessage_.getPreferredHeight(w))
              .withMargin({0.0f, 0.0f, 0.0f, 0.0f}));
    } else {
      fb.items.add(juce::FlexItem(sectionSubtitleLabel_)
                       .withWidth((float)w)
                       .withHeight(22.0f)
                       .withMargin({-5.0f, 0.0f, 10.0f, -6.0f}));
      fb.items.add(
          juce::FlexItem(binauralRadio_).withWidth((float)w).withHeight(26.0f));
      fb.items.add(
          juce::FlexItem(binauralDesc_)
              .withWidth((float)descWidth)
              .withHeight((float)binauralDesc_.getPreferredHeight(descWidth))
              .withMargin({0.0f, 0.0f, 10.0f, 32.0f}));
      fb.items.add(juce::FlexItem(standardStereoRadio_)
                       .withWidth((float)w)
                       .withHeight(26.0f));
      fb.items.add(
          juce::FlexItem(standardStereoDesc_)
              .withWidth((float)descWidth)
              .withHeight(
                  (float)standardStereoDesc_.getPreferredHeight(descWidth))
              .withMargin({0.0f, 0.0f, 0.0f, 32.0f}));
    }

    fb.performLayout(panelBounds.toFloat());
  }

  void paint(juce::Graphics& g) override {
    g.setColour(EclipsaColours::inactiveGrey);
    g.fillRect(getLocalBounds().toFloat());

    if (isExpanded_) {
      g.setColour(EclipsaColours::gridLine);
      g.fillRect(getLocalBounds()
                     .removeFromTop(kCollapsedHeight + 1)
                     .removeFromBottom(1)
                     .toFloat());
    }
  }

  const juce::Button* const getDeleteButton() const { return &removeAEButton_; }

  void setDeleteButtonListener(juce::Button::Listener* listener) {
    removeAEButton_.addListener(listener);
    listeners_.push_back(listener);
  }

  void updateName(const juce::String& name) {
    name_ = name;
    nameLabel_.setText(name_, juce::dontSendNotification);
    nameLabel_.repaint();
  }

 private:
  class SvgButton : public juce::Button {
   public:
    SvgButton(SvgMap::Icon icon, bool hoverDarken = false)
        : juce::Button(""), iconComponent_(icon), hoverDarken_(hoverDarken) {
      addAndMakeVisible(iconComponent_);
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                     bool /*shouldDrawButtonAsDown*/) override {
      if (hoverDarken_ && shouldDrawButtonAsHighlighted) {
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillEllipse(getLocalBounds().toFloat());
      }
    }

    void resized() override { iconComponent_.setBounds(getLocalBounds()); }

   private:
    SvgIconComponent iconComponent_;
    bool hoverDarken_ = false;
  };

  class SVGToolTip : public juce::Component,
                     public juce::SettableTooltipClient {
   public:
    SVGToolTip(SvgMap::Icon icon, const juce::String& tooltipText)
        : iconComponent_(icon) {
      setTooltip(tooltipText);
      addAndMakeVisible(iconComponent_);
    }

    void resized() override {
      int sz = juce::jmin(getWidth(), getHeight());
      iconComponent_.setBounds(juce::Rectangle<int>(sz, sz).withCentre(
          getLocalBounds().getCentre()));
    }

   private:
    SvgIconComponent iconComponent_;
  };

  class RadioLookAndFeel : public juce::LookAndFeel_V4 {
   public:
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool /*shouldDrawButtonAsHighlighted*/,
                          bool /*shouldDrawButtonAsDown*/) override {
      const float alpha = button.isEnabled() ? 1.0f : 0.35f;
      const float circleSize = 20.0f;
      const float cx = 2.0f;
      const float cy = (button.getHeight() - circleSize) * 0.5f;

      if (button.getToggleState()) {
        g.setColour(EclipsaColours::selectCyan.withAlpha(alpha));
        g.drawEllipse(cx, cy, circleSize, circleSize, 2.0f);
        const float innerSize = circleSize * 0.6f;
        g.fillEllipse(cx + (circleSize - innerSize) * 0.5f,
                      cy + (circleSize - innerSize) * 0.5f, innerSize,
                      innerSize);
      } else {
        g.setColour(EclipsaColours::selectionToggleBorderGrey.withAlpha(alpha));
        g.drawEllipse(cx, cy, circleSize, circleSize, 1.5f);
      }

      const float textX = cx + circleSize + 10.0f;
      g.setColour(EclipsaColours::headingGrey.withAlpha(alpha));
      g.setFont(juce::Font(15.0f, juce::Font::bold));
      g.drawText(button.getButtonText(),
                 juce::Rectangle<float>(textX, 0.0f, button.getWidth() - textX,
                                        static_cast<float>(button.getHeight())),
                 juce::Justification::centredLeft);
    }
  };

  class MultilineText : public juce::Component {
   public:
    MultilineText(const juce::String& text, float fontSize, juce::Colour colour)
        : text_(text), font_(fontSize), colour_(colour) {}

    int getPreferredHeight(int width) const {
      if (width <= 0) return 0;
      juce::AttributedString as;
      as.append(text_, font_, colour_);
      as.setWordWrap(juce::AttributedString::byWord);
      juce::TextLayout tl;
      tl.createLayout(as, (float)width);
      return (int)std::ceil(tl.getHeight());
    }

    void paint(juce::Graphics& g) override {
      juce::AttributedString as;
      as.append(text_, font_, colour_);
      as.setWordWrap(juce::AttributedString::byWord);
      juce::TextLayout tl;
      tl.createLayout(as, (float)getWidth());
      tl.draw(g, getLocalBounds().toFloat());
    }

   private:
    juce::String text_;
    juce::Font font_;
    juce::Colour colour_;
  };

  class HeadingRow : public juce::Component {
   public:
    HeadingRow() {
      titleLabel_.setText("Headphone Spatialization",
                          juce::dontSendNotification);
      titleLabel_.setColour(juce::Label::textColourId,
                            EclipsaColours::headingGrey);
      titleLabel_.setFont(juce::Font(16.0f, juce::Font::bold));
      addAndMakeVisible(icon_);
      addAndMakeVisible(titleLabel_);
      addAndMakeVisible(helpIcon_);
    }

    void resized() override {
      const float iconSize = 14.0f;
      const float iconGap = 4.0f;
      const float tooltipGap = 4.0f;
      const float tooltipSize = 14.0f;

      juce::FlexBox fb;
      fb.flexDirection = juce::FlexBox::Direction::row;
      fb.alignItems = juce::FlexBox::AlignItems::center;

      fb.items.add(juce::FlexItem(icon_)
                       .withWidth(iconSize)
                       .withHeight(iconSize)
                       .withMargin({0.0f, iconGap, 0.0f, 0.0f}));
      const float titleWidth =
          titleLabel_.getFont().getStringWidthFloat(titleLabel_.getText());
      fb.items.add(juce::FlexItem(titleLabel_)
                       .withWidth(titleWidth)
                       .withHeight((float)getHeight()));
      fb.items.add(juce::FlexItem(helpIcon_)
                       .withWidth(tooltipSize)
                       .withHeight(tooltipSize)
                       .withMargin({0.0f, 0.0f, 0.0f, tooltipGap}));

      fb.performLayout(getLocalBounds().toFloat());
    }

   private:
    SvgIconComponent icon_{SvgMap::kHeadphones};
    juce::Label titleLabel_;
    SvgIconComponent helpIcon_{SvgMap::kHelp};
  };

  void mouseDown(const juce::MouseEvent&) override {
    if (getMouseXYRelative().getY() >= kCollapsedHeight) return;
    toggleExpand();
  }

  void toggleExpand() { setExpanded(!isExpanded_); }

  void setExpanded(bool expanded) {
    if (isExpanded_ == expanded) return;
    isExpanded_ = expanded;
    if (isExpanded_) {
      updateExpandedVisibility();
      juce::Desktop::getInstance().addGlobalMouseListener(&globalMouseWatcher_);
    }
    startTimerHz(360);
  }

  void timerCallback() override {
    const int expandedTarget =
        isBinauralLocked_ ? kBinauralLockedExpandedHeight : kExpandedHeight;
    const float target =
        isExpanded_ ? (float)expandedTarget : (float)kCollapsedHeight;
    animatedHeight_ += (target - animatedHeight_) * 0.25f;

    if (std::abs(animatedHeight_ - target) < 0.5f) {
      animatedHeight_ = target;
      stopTimer();
      if (!isExpanded_) {
        updateExpandedVisibility();
        juce::Desktop::getInstance().removeGlobalMouseListener(
            &globalMouseWatcher_);
      }
    }

    if (onExpandStateChanged_) onExpandStateChanged_();
    resized();
    repaint();
  }

  void updateExpandedVisibility() {
    const bool showRadios = isExpanded_ && !isBinauralLocked_;
    const bool showLockedMessage = isExpanded_ && isBinauralLocked_;
    headingRow_.setVisible(isExpanded_);
    sectionSubtitleLabel_.setVisible(showRadios);
    binauralRadio_.setVisible(showRadios);
    binauralDesc_.setVisible(showRadios);
    standardStereoRadio_.setVisible(showRadios);
    standardStereoDesc_.setVisible(showRadios);
    binauralLockedMessage_.setVisible(showLockedMessage);
  }

  juce::String name_;
  juce::String desc_;
  bool isExpanded_ = false;
  float animatedHeight_ = (float)kCollapsedHeight;

  juce::Label nameLabel_;
  juce::Label descLabel_;

  SvgButton removeAEButton_{SvgMap::kRemoveAE, true};
  SvgIconComponent headphonesIcon_{SvgMap::kHeadphones};
  juce::Label selectedOptionLabel_;
  SvgButton expandToggleButton_{SvgMap::kDropdownArrow};

  HeadingRow headingRow_;
  juce::TooltipWindow tooltipWindow_{this};
  juce::Label sectionSubtitleLabel_;

  RadioLookAndFeel radioLookAndFeel_;
  juce::ToggleButton binauralRadio_;
  MultilineText binauralDesc_{
      "Uses virtualization during playback on headphones to create an "
      "externalized 3D soundstage outside the listener's head.",
      12.0f, EclipsaColours::tabTextGrey};
  juce::ToggleButton standardStereoRadio_;
  MultilineText standardStereoDesc_{
      "Uses standard stereo fold-down without virtualization. Stereo and "
      "pre-rendered binaural Audio Elements are directly passed through.",
      12.0f, EclipsaColours::tabTextGrey};

  bool isBinauralLocked_ = false;
  MultilineText binauralLockedMessage_{
      "This Audio Element has already been spatially rendered. The audio will "
      "be passed through without additional spatial processing.",
      13.0f, EclipsaColours::tabTextGrey};

  class GlobalMouseWatcher : public juce::MouseListener {
   public:
    explicit GlobalMouseWatcher(MixAEContainer& owner) : owner_(owner) {}
    void mouseDown(const juce::MouseEvent& e) override {
      auto screenBounds = owner_.localAreaToGlobal(owner_.getLocalBounds());
      if (!screenBounds.contains(e.getScreenPosition())) {
        owner_.setExpanded(false);
      }
    }

   private:
    MixAEContainer& owner_;
  };

  std::function<void()> onExpandStateChanged_;
  std::vector<juce::Button::Listener*> listeners_;
  GlobalMouseWatcher globalMouseWatcher_{*this};
};
