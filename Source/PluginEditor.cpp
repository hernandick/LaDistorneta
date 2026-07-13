#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

static std::vector<std::byte> toByteVec (const char* data, int size)
{
    return std::vector<std::byte> (reinterpret_cast<const std::byte*> (data),
                                   reinterpret_cast<const std::byte*> (data) + size);
}

LaDistornetaAudioProcessorEditor::LaDistornetaAudioProcessorEditor (LaDistornetaAudioProcessor& proc)
    : AudioProcessorEditor (&proc), processor (proc),
      webBrowser (juce::WebBrowserComponent::Options{}
          // En Windows hay que pedir WebView2 explicitamente: el backend por
          // defecto es IE11 y ahi la UI no carga ("Se canceló la navegación...").
          // En macOS esta opcion se ignora (usa WKWebView igual).
          .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
          .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
              // WebView2 necesita una carpeta escribible para sus datos; sin esto
              // falla cuando el DAW corre desde Program Files.
              .withUserDataFolder (juce::File::getSpecialLocation (
                  juce::File::userApplicationDataDirectory).getChildFile ("LaDistorneta")))
          .withNativeIntegrationEnabled()
          .withNativeFunction ("setParam",
              [&proc] (const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
              {
                  if (args.size() >= 2)
                  {
                      const auto paramId = args[0].toString();
                      const auto norm    = static_cast<float> (static_cast<double> (args[1]));
                      if (auto* param = proc.apvts.getParameter (paramId))
                          param->setValueNotifyingHost (norm);
                  }
                  complete ({});
              })
          .withNativeFunction ("getState",
              [&proc] (const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
              {
                  auto getNorm = [&proc] (const char* id)
                  {
                      auto* p = proc.apvts.getParameter (id);
                      return p != nullptr ? p->getValue() : 0.0f;
                  };

                  auto obj = std::make_unique<juce::DynamicObject>();
                  obj->setProperty ("type",   juce::roundToInt (getNorm ("type") * 4.0f));
                  obj->setProperty ("drive",   getNorm ("drive"));
                  obj->setProperty ("lowcut",  getNorm ("lowcut"));
                  obj->setProperty ("tone",    getNorm ("tone"));
                  obj->setProperty ("highcut", getNorm ("highcut"));
                  obj->setProperty ("mix",    getNorm ("mix"));
                  obj->setProperty ("output", getNorm ("output"));
                  obj->setProperty ("garra",  getNorm ("garra") > 0.5f);
                  obj->setProperty ("levelL", proc.getMeterLevel (0));
                  obj->setProperty ("levelR", proc.getMeterLevel (1));

                  complete (juce::var (obj.release()));
              })
          .withResourceProvider ([this] (const juce::String& url)
          {
              return getResource (url);
          }))
{
    addAndMakeVisible (webBrowser);
    webBrowser.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    setSize (760, 560);
}

LaDistornetaAudioProcessorEditor::~LaDistornetaAudioProcessorEditor() {}

void LaDistornetaAudioProcessorEditor::resized()
{
    webBrowser.setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
LaDistornetaAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource {
            toByteVec (BinaryData::ui_html, BinaryData::ui_htmlSize), "text/html" };

    return std::nullopt;
}
