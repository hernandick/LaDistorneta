#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// UI custom (HTML/CSS/JS, mockup de Claude Design "La Distorneta") corriendo
// dentro de un WebBrowserComponent nativo de JUCE. Se comunica con el processor
// via el puente __JUCE__: "setParam" (JS -> C++) y "getState" (C++ -> JS, polling
// para el VU meter real y el estado inicial de knobs/tipo/garra).
class LaDistornetaAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit LaDistornetaAudioProcessorEditor (LaDistornetaAudioProcessor&);
    ~LaDistornetaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url);

    LaDistornetaAudioProcessor& processor;
    juce::WebBrowserComponent webBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaDistornetaAudioProcessorEditor)
};
