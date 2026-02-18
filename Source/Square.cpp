#include "Square.h"

void Square::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::white);
	g.setColour(juce::Colours::white);
	g.drawRect(getLocalBounds(), 2); // Draw a black border around the component
}

void Square::resized()
{
	// This method is called when the component is resized.
	// You can use this to reposition or resize any child components if needed.
}