#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. Azimuth Slider (Main)
    azimuthSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    azimuthSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(azimuthSlider);
    azimuthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.parameters, "sourceAzimuth", azimuthSlider);

    // 2. Test Configuration (Όπως το HTML)
    addAndMakeVisible(testGroup);
    
    addAndMakeVisible(numPointsLabel);
    
    numPointsSlider.setRange(2, 36, 1); // Από 2 έως 36 σημεία δοκιμής
    numPointsSlider.setValue(4);
    numPointsSlider.addListener(this);
    addAndMakeVisible(numPointsSlider);

    nextBtn.addListener(this);
    addAndMakeVisible(nextBtn);
    
    prevBtn.addListener(this);
    addAndMakeVisible(prevBtn);

    currentInfoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(currentInfoLabel);

    updateTestPosition(); // Αρχικοποίηση κειμένου
    setSize (400, 550);
}

void AudioPluginAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &numPointsSlider)
    {
        totalTestPoints = (int)numPointsSlider.getValue();
        currentTestIndex = 0; // Reset όταν αλλάζει το πλήθος
        updateTestPosition();
    }
}

void AudioPluginAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &nextBtn)
    {
        currentTestIndex = (currentTestIndex + 1) % totalTestPoints;
        updateTestPosition();
    }
    else if (button == &prevBtn)
    {
        currentTestIndex = (currentTestIndex - 1 + totalTestPoints) % totalTestPoints;
        updateTestPosition();
    }
}

void AudioPluginAudioProcessorEditor::updateTestPosition()
{
    // Υπολογισμός γωνίας: 0 είναι πάνω, 90 δεξιά
    float angle = (float)currentTestIndex * (360.0f / (float)totalTestPoints);
    
    // Ενημέρωση του επίσημου parameter του plugin
    auto* param = audioProcessor.parameters.getParameter("sourceAzimuth");
    float normalizedValue = audioProcessor.parameters.getParameterRange("sourceAzimuth").convertTo0to1(angle);
    param->setValueNotifyingHost(normalizedValue);

    // Ενημέρωση Label
    currentInfoLabel.setText("Point " + juce::String(currentTestIndex + 1) + " of " + juce::String(totalTestPoints) + 
                             " (Angle: " + juce::String(angle, 1) + "°)", juce::dontSendNotification);
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    auto area = getLocalBounds().reduced(20);
    auto visualizerArea = area.removeFromTop(200).toFloat();

    // Σχεδίαση "Κεφαλιού" (Listener)
    g.setColour(juce::Colours::grey);
    g.drawEllipse(visualizerArea.getCentreX() - 15, visualizerArea.getCentreY() - 15, 30, 30, 2.0f);
    
    // Σχεδίαση "Μύτης" για να δείχνει το "Πάνω" (0 μοίρες)
    g.drawLine(visualizerArea.getCentreX(), visualizerArea.getCentreY() - 15, 
               visualizerArea.getCentreX(), visualizerArea.getCentreY() - 25, 2.0f);

    // Σχεδίαση Πηγής (0=Πάνω, 90=Δεξιά)
    // Στα μαθηματικά της JUCE το 0 rad είναι δεξιά, οπότε αφαιρούμε 90 μοίρες για το visual
    float visualAngle = juce::DegreesToRadians(azimuthSlider.getValue() - 90.0f);
    auto sourcePos = visualizerArea.getCentre().getPointOnCircumference(80.0f, visualAngle);
    
    g.setColour(juce::Colours::orange);
    g.fillEllipse(sourcePos.getX() - 8, sourcePos.getY() - 8, 16, 16);
}

void AudioPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(210); // Visualizer space
    
    azimuthSlider.setBounds(area.removeFromTop(100));
    
    area.removeFromTop(20);
    testGroup.setBounds(area);
    
    auto testArea = area.reduced(15, 25);
    numPointsLabel.setBounds(testArea.removeFromTop(20));
    numPointsSlider.setBounds(testArea.removeFromTop(30));
    
    testArea.removeFromTop(10);
    auto btnRow = testArea.removeFromTop(40);
    prevBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth()/2).reduced(5));
    nextBtn.setBounds(btnRow.reduced(5));
    
    currentInfoLabel.setBounds(testArea.removeFromTop(30));
}