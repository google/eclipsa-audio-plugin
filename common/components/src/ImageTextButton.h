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

#include <string>

#include "Icons.h"

class ImageTextButtonLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  virtual ~ImageTextButtonLookAndFeel() = default;  // Add a virtual destructor
  ImageTextButtonLookAndFeel(const juce::Image& image) : image_(image) {}

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton,
                            bool isButtonDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    auto cornerSize = bounds.getHeight() * 0.5f;
    g.setColour(button.findColour(juce::TextButton::buttonColourId));
    g.fillRoundedRectangle(bounds, cornerSize);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }

  void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                      bool isMouseOverButton, bool isButtonDown) override {
    juce::Font font = juce::Font("Roboto", 12.f, juce::Font::plain);
    g.setFont(font);
    g.setColour(button.findColour(juce::TextButton::textColourOnId));

    auto cornerSize = juce::jmin(button.getHeight() / 2, button.getWidth()) / 2;

    auto fontHeight = juce::roundToInt(font.getHeight());
    auto yIndent = (button.getHeight() - fontHeight) / 2.f;
    auto leftIndent = cornerSize;
    auto rightIndent = leftIndent;
    auto textHeight = font.getHeight();
    auto maxTextWidth =
        button.getWidth() -
        (leftIndent + rightIndent +
         (3 * textHeight * 1.25f));  // Adjust for image width and padding
    int textWidth = font.getStringWidth(button.getButtonText());

    auto rect = juce::Rectangle<int>(
        (button.getWidth() - textWidth) / 2 + (fontHeight * 0.5f), yIndent + 1,
        textWidth * 1.25f,
        textHeight);  // Adjusted for image position

    g.drawFittedText(button.getButtonText(), rect,
                     juce::Justification::centredLeft, 2);

    // Draw the image next to the text
    if (!image_.isNull()) {
      auto imageRect =
          juce::Rectangle<float>(rect.getX() - fontHeight * 1.5f,
                                 rect.getY() - 2, fontHeight, fontHeight);
      g.drawImage(image_, imageRect);
    }
  }

  virtual void dimButton() {
    for (const auto& colourId : colourIds_) {
      setColour(colourId, findColour(colourId).withAlpha(alpha_));
    }
  };

  virtual void resetButton() {
    for (const auto& colourId : colourIds_) {
      setColour(colourId, findColour(colourId).withAlpha(1.f));
    }
  };

 protected:
  juce::Image image_;
  const std::array<juce::TextButton::ColourIds, 4> colourIds_ = {
      juce::TextButton::textColourOffId, juce::TextButton::textColourOnId,
      juce::TextButton::buttonColourId, juce::TextButton::buttonOnColourId};
  const float alpha_ = 0.4f;  // Alpha value for dimming
};

class GreyImageTextButtonLookAndFeel : public ImageTextButtonLookAndFeel {
 public:
  GreyImageTextButtonLookAndFeel(const juce::Image& image)
      : ImageTextButtonLookAndFeel(image) {
    setColour(juce::TextButton::textColourOffId,
              EclipsaColours::drawButtonGrey);
    setColour(juce::TextButton::textColourOnId, EclipsaColours::drawButtonGrey);
    setColour(juce::TextButton::buttonColourId, EclipsaColours::onButtonGrey);
    setColour(juce::TextButton::buttonOnColourId, EclipsaColours::onButtonGrey);
  }
};

class CyanImageTextButtonLookAndFeel : public ImageTextButtonLookAndFeel {
 public:
  CyanImageTextButtonLookAndFeel(const juce::Image& image)
      : ImageTextButtonLookAndFeel(image) {
    setColour(juce::TextButton::textColourOffId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextButton::textColourOnId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextButton::buttonColourId, EclipsaColours::selectCyan);
    setColour(juce::TextButton::buttonOnColourId, EclipsaColours::selectCyan);
  }
};

class BlueTextBlackButtonLookAndFeel : public ImageTextButtonLookAndFeel {
 public:
  BlueTextBlackButtonLookAndFeel(const juce::Image& image)
      : ImageTextButtonLookAndFeel(image) {
    setColour(juce::TextButton::textColourOffId, EclipsaColours::selectCyan);
    setColour(juce::TextButton::textColourOnId, EclipsaColours::selectCyan);
    setColour(juce::TextButton::buttonColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextButton::buttonOnColourId,
              EclipsaColours::backgroundOffBlack);
  }

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton,
                            bool isButtonDown) override {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    auto cornerSize = bounds.getHeight() * 0.5f;

    // Background color
    g.setColour(button.findColour(juce::TextButton::buttonColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    // Border color - brighten or choose a contrasting color
    g.setColour(juce::Colours::white);  // White border
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
  }
};

// New LookAndFeel class for dual-image buttons inheriting from
// CyanImageTextButtonLookAndFeel
class ExportImageTextButtonLookAndFeel : public CyanImageTextButtonLookAndFeel {
 public:
  ExportImageTextButtonLookAndFeel(const juce::Image& image1,
                                   const juce::Image& image2)
      : CyanImageTextButtonLookAndFeel(image1), image2_(image2) {}

  void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                      bool isMouseOverButton, bool isButtonDown) override {
    juce::Font font =
        juce::Font("Roboto", 16.f, juce::Font::plain);  // Font size remains 16
    g.setFont(font);
    g.setColour(button.findColour(juce::TextButton::textColourOnId));

    auto bounds = button.getLocalBounds().toFloat();
    float buttonHeight = bounds.getHeight();
    float buttonWidth = bounds.getWidth();

    float imageHeight =
        font.getHeight() * 1.2f;  // Slightly increase the icon size
    float dividerHeight =
        buttonHeight *
        0.5f;  // Reduce the divider height to 50% of the button height
    float dividerWidth = 2.0f;  // Divider line width remains the same
    float padding = 10.0f;      // Padding between elements

    // Calculate total width for centering
    float totalContentWidth = imageHeight + dividerWidth +
                              font.getStringWidth(button.getButtonText()) +
                              2 * padding;

    // Calculate starting X positions to center all elements
    float startX = (buttonWidth - totalContentWidth) / 2.0f;

    // Positions for first image, divider, and text
    float firstIconX = startX;
    float secondIconX = firstIconX + imageHeight +
                        padding;  // Position divider after the first icon
    float textStartX = secondIconX + dividerWidth +
                       padding;  // Position text after the divider

    // Draw the first icon (settings icon) with high-quality transformation
    if (!image_.isNull()) {
      auto image1Rect =
          juce::Rectangle<float>(firstIconX, (buttonHeight - imageHeight) / 2,
                                 imageHeight, imageHeight);
      juce::AffineTransform transform =
          juce::AffineTransform::scale(imageHeight / image_.getWidth(),
                                       imageHeight / image_.getHeight())
              .translated(image1Rect.getX(), image1Rect.getY());
      g.drawImageTransformed(image_, transform, false);
    }

    // Draw the divider (second icon) as a vertical line
    if (!image2_.isNull()) {
      auto image2Rect = juce::Rectangle<float>(
          secondIconX, (buttonHeight - dividerHeight) / 2, dividerWidth,
          dividerHeight);
      g.drawImage(image2_, image2Rect);
    }

    // Draw the text
    auto textRect = juce::Rectangle<float>(
        textStartX, 0, buttonWidth - textStartX, buttonHeight);
    g.drawFittedText(button.getButtonText(), textRect.toType<int>(),
                     juce::Justification::centredLeft, 1);
  }

 private:
  juce::Image image2_;
};

// Component class for the ImageTextButton
class ImageTextButton : public juce::Component {
 public:
  ImageTextButton(const juce::Image& image)
      : greyLookAndFeel(image), cyanLookAndFeel(image), blueLookAndFeel(image) {
    textButton.setLookAndFeel(&greyLookAndFeel);
    addAndMakeVisible(textButton);
  }

  ~ImageTextButton() override { textButton.setLookAndFeel(nullptr); }

  void paint(juce::Graphics&) override {
    textButton.setBounds(
        getLocalBounds());  // Adjust size to fit image and text
  }

  void setButtonText(const juce::String& text) {
    textButton.setButtonText(text);
  }

  void setButtonOnClick(const std::function<void()>& onClick) {
    textButton.onClick = onClick;
  }

  void setGreyLookAndFeel() { textButton.setLookAndFeel(&greyLookAndFeel); }

  void setCyanLookAndFeel() { textButton.setLookAndFeel(&cyanLookAndFeel); }

  void setBlueLookAndFeel() { textButton.setLookAndFeel(&blueLookAndFeel); }

  // Method to set Export LookAndFeel for dual-image functionality
  void setExportLookAndFeel(const juce::Image& image1,
                            const juce::Image& image2) {
    exportLookAndFeel =
        std::make_unique<ExportImageTextButtonLookAndFeel>(image1, image2);
    textButton.setLookAndFeel(exportLookAndFeel.get());
  }

  void dimButton() {
    ImageTextButtonLookAndFeel& lookAndFeel =
        static_cast<ImageTextButtonLookAndFeel&>(textButton.getLookAndFeel());
    lookAndFeel.dimButton();
  }

  void resetButton() {
    ImageTextButtonLookAndFeel& lookAndFeel =
        static_cast<ImageTextButtonLookAndFeel&>(textButton.getLookAndFeel());
    lookAndFeel.resetButton();
  }

  void setButtonListener(juce::Button::Listener* listener) {
    textButton.addListener(listener);
  }

  juce::Button* getButton() { return &textButton; }

  std::string getButtonText() {
    return textButton.getButtonText().toStdString();
  }

 private:
  juce::TextButton textButton;
  GreyImageTextButtonLookAndFeel greyLookAndFeel;
  CyanImageTextButtonLookAndFeel cyanLookAndFeel;
  BlueTextBlackButtonLookAndFeel blueLookAndFeel;
  std::unique_ptr<ExportImageTextButtonLookAndFeel> exportLookAndFeel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageTextButton)
};