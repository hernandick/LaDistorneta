#include "PluginProcessor.h"
#include "PluginEditor.h"

LaDistornetaAudioProcessor::LaDistornetaAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

LaDistornetaAudioProcessor::~LaDistornetaAudioProcessor() {}

bool LaDistornetaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

juce::AudioProcessorValueTreeState::ParameterLayout LaDistornetaAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Type: elige el algoritmo de distorsion (uno por jugador).
    // El nombre incluye el tipo de distorsion para que se entienda desde el host tambien.
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "type", "Type",
        juce::StringArray {
            "Messi - Valvula",
            "Dibu Martinez - Fuzz",
            "Julian Alvarez - Overdrive",
            "Lautaro Martinez - Hard Clip",
            "De Paul - Bit Crush"
        },
        0));

    // Drive: cantidad de distorsion, normalizado 0..1 (cada tipo lo mapea distinto)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "drive", "Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

    // Low Cut: high-pass PRE-distorsion, 20Hz (off) a 2kHz. Limpia graves antes de saturar.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "lowcut", "Low Cut",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

    // Tone: tilt EQ post-distorsion centrado en 1kHz. 0.5 = plano,
    // 0 = oscuro (-6dB agudos / +6dB graves), 1 = brillante (al reves).
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "tone", "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    // High Cut: low-pass POST-distorsion, 1kHz a 20kHz (off). Doma los armonicos generados.
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "highcut", "High Cut",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    // Mix: dry/wet
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));

    // Output: ganancia de salida en dB
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "output", "Output",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f));

    // Garra: empuje extra de gain antes de saturar (equivalente al "Punish" de Decapitator)
    layout.add (std::make_unique<juce::AudioParameterBool> ("garra", "Garra", false));

    return layout;
}

//==============================================================================
const juce::String LaDistornetaAudioProcessor::getName() const { return JucePlugin_Name; }
bool LaDistornetaAudioProcessor::acceptsMidi() const { return false; }
bool LaDistornetaAudioProcessor::producesMidi() const { return false; }
bool LaDistornetaAudioProcessor::isMidiEffect() const { return false; }
double LaDistornetaAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int LaDistornetaAudioProcessor::getNumPrograms() { return 1; }
int LaDistornetaAudioProcessor::getCurrentProgram() { return 0; }
void LaDistornetaAudioProcessor::setCurrentProgram (int) {}
const juce::String LaDistornetaAudioProcessor::getProgramName (int) { return {}; }
void LaDistornetaAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void LaDistornetaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // 2 etapas de 2x = 4x oversampling total. Se usa para todos los tipos
    // menos De Paul (bit crush), que necesita correr a la sample rate base.
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (getTotalNumInputChannels()), 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    oversampling->initProcessing (static_cast<size_t> (samplesPerBlock));

    for (int ch = 0; ch < 2; ++ch)
    {
        lowCutFilter[ch].reset();
        tiltLowShelf[ch].reset();
        tiltHighShelf[ch].reset();
        highCutFilter[ch].reset();
    }

    crushHeld[0] = crushHeld[1] = 0.0f;
    crushCounter[0] = crushCounter[1] = 0;

    levelL = levelR = 0.0f;
    meterLevelL.store (0.0f);
    meterLevelR.store (0.0f);
}

void LaDistornetaAudioProcessor::releaseResources()
{
    if (oversampling != nullptr)
        oversampling->reset();
}

float LaDistornetaAudioProcessor::shape (DistortionType type, float x, float amount) const noexcept
{
    switch (type)
    {
        case DistortionType::Messi:
        {
            // Saturacion de valvula: tanh clasico, calido y musical
            const float driveGain = 1.0f + amount * 19.0f; // 1..20
            return std::tanh (x * driveGain) / std::tanh (driveGain);
        }

        case DistortionType::Dibu:
        {
            // Fuzz asimetrico: mas agresivo del lado positivo, con caracter ruidoso
            const float driveGain = 1.0f + amount * 39.0f; // 1..40
            if (x >= 0.0f)
                return std::tanh (x * driveGain) / std::tanh (driveGain);

            const float negGain = driveGain * 1.6f;
            return (std::tanh (x * negGain) / std::tanh (negGain)) * 0.85f;
        }

        case DistortionType::JulianAlvarez:
        {
            // Overdrive: soft clip cubico, motor incansable pero prolijo
            const float driveGain = 1.0f + amount * 9.0f; // 1..10
            const float scaled = juce::jlimit (-1.0f, 1.0f, x * driveGain);
            return scaled - (scaled * scaled * scaled) / 3.0f;
        }

        case DistortionType::Lautaro:
        {
            // Distorsion normal: hard clip directo y contundente
            const float driveGain = 1.0f + amount * 9.0f; // 1..10
            return juce::jlimit (-1.0f, 1.0f, x * driveGain);
        }

        case DistortionType::DePaul:
            // El bit crush se aplica aparte en processBlock (no pasa por oversampling
            // ni por shape()); este caso no deberia alcanzarse nunca.
            return x;
    }

    return x;
}

void LaDistornetaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const auto type = static_cast<DistortionType> (
        static_cast<int> (apvts.getRawParameterValue ("type")->load()));
    const float amount     = apvts.getRawParameterValue ("drive")->load();
    const float lowCut     = apvts.getRawParameterValue ("lowcut")->load();
    const float tone       = apvts.getRawParameterValue ("tone")->load();
    const float highCut    = apvts.getRawParameterValue ("highcut")->load();
    const float mix        = apvts.getRawParameterValue ("mix")->load();
    const float outputGain = juce::Decibels::decibelsToGain (
                                 apvts.getRawParameterValue ("output")->load());
    const bool  garra       = apvts.getRawParameterValue ("garra")->load() > 0.5f;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Copia seca para el Mix
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf (buffer);

    // Low Cut: high-pass PRE-distorsion (20Hz a 2kHz, exponencial). Recortar graves antes
    // de saturar evita que el bajo ensucie/intermodule con el resto (estilo Decapitator).
    if (lowCut > 0.001f)
    {
        const float lowCutHz = 20.0f * std::pow (100.0f, lowCut);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            *lowCutFilter[channel].coefficients =
                *juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, lowCutHz, 0.707f);

            auto* data = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i)
                data[i] = lowCutFilter[channel].processSample (data[i]);
        }
    }

    // Garra: empuje extra de gain antes de saturar (bajado de 18dB a 8dB, era muy agresivo)
    if (garra)
        buffer.applyGain (juce::Decibels::decibelsToGain (8.0f));

    // VU meter estilo "Attitude" (Decapitator): mide que tan fuerte golpea la señal
    // al circuito de saturacion (post low-cut y garra, con el gain del Drive aplicado),
    // NO el output del plugin. Como las curvas empiezan a recortar cuando |x * driveGain|
    // pasa de ~1.0 (0 dB), aguja en el rojo = la señal entro en zona no lineal = distorsion.
    // Ballistics tipo VU: attack instantaneo, release ~300ms.
    {
        float meterDriveGain = 1.0f;
        switch (type)
        {
            case DistortionType::Messi:         meterDriveGain = 1.0f + amount * 19.0f; break;
            case DistortionType::Dibu:          meterDriveGain = 1.0f + amount * 39.0f; break;
            case DistortionType::JulianAlvarez:
            case DistortionType::Lautaro:       meterDriveGain = 1.0f + amount * 9.0f;  break;
            case DistortionType::DePaul:        meterDriveGain = 1.0f + amount * 9.0f;  break;
                // De Paul no tiene gain real (el drive controla bits/sample-hold);
                // se usa un equivalente visual para que la aguja acompañe al Drive.
        }

        const float releaseCoeff = std::exp (-static_cast<float> (numSamples)
                                              / (0.3f * static_cast<float> (currentSampleRate)));
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float peak = buffer.getMagnitude (channel, 0, numSamples) * meterDriveGain;
            float& level = (channel == 0 ? levelL : levelR);
            level = juce::jmax (peak, level * releaseCoeff);
        }
        // Escala comprimida: cada dB mostrado equivale a 3 dB reales (lvl^(1/3) en lineal),
        // asi la aguja recorre mas dial en vez de vivir clavada arriba.
        // meterOffsetDb corre todo el dial hacia atras: con -6, el punto donde arranca la
        // saturacion (0 dB reales) aparece en -6 dB del dial, dejando headroom visual para
        // que la aguja siga subiendo hacia el rojo a medida que saturas mas fuerte.
        // Mas negativo = cero mas atras / mas recorrido antes del rojo.
        constexpr float meterScale    = 1.0f / 3.0f;
        constexpr float meterOffsetDb = -6.0f;
        const float offsetLin = juce::Decibels::decibelsToGain (meterOffsetDb);
        meterLevelL.store (std::pow (levelL, meterScale) * offsetLin);
        meterLevelR.store (std::pow (numChannels > 1 ? levelR : levelL, meterScale) * offsetLin);
    }

    if (type == DistortionType::DePaul)
    {
        // Bit crush: opera directo a la sample rate base.
        // El aliasing es parte del caracter del efecto, por eso no lleva oversampling.
        const int bits = juce::jlimit (2, 16, 16 - juce::roundToInt (amount * 13.0f));
        const float step = 2.0f / static_cast<float> (1 << bits);
        const int holdSamples = juce::jlimit (1, 16, 1 + juce::roundToInt (amount * 15.0f));

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i)
            {
                if (crushCounter[channel] <= 0)
                {
                    crushHeld[channel] = step * std::round (data[i] / step);
                    crushCounter[channel] = holdSamples;
                }
                data[i] = crushHeld[channel];
                --crushCounter[channel];
            }
        }
    }
    else
    {
        // Resto de los tipos: shaping con 4x oversampling para evitar aliasing
        juce::dsp::AudioBlock<float> block (buffer);
        auto oversampledBlock = oversampling->processSamplesUp (block);

        for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
        {
            auto* data = oversampledBlock.getChannelPointer (channel);
            for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
                data[i] = shape (type, data[i], amount);
        }

        oversampling->processSamplesDown (block);
    }

    // Tone: tilt EQ post-distorsion centrado en 1kHz. Un low shelf y un high shelf
    // con ganancias opuestas (+-6dB): a la izquierda oscurece, a la derecha abrillanta.
    const float tiltDb = (tone - 0.5f) * 12.0f;
    if (std::abs (tiltDb) > 0.01f)
    {
        const float lowGain  = juce::Decibels::decibelsToGain (-tiltDb);
        const float highGain = juce::Decibels::decibelsToGain ( tiltDb);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            *tiltLowShelf[channel].coefficients =
                *juce::dsp::IIR::Coefficients<float>::makeLowShelf (currentSampleRate, 1000.0f, 0.707f, lowGain);
            *tiltHighShelf[channel].coefficients =
                *juce::dsp::IIR::Coefficients<float>::makeHighShelf (currentSampleRate, 1000.0f, 0.707f, highGain);

            auto* data = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i)
                data[i] = tiltHighShelf[channel].processSample (
                              tiltLowShelf[channel].processSample (data[i]));
        }
    }

    // High Cut: low-pass POST-distorsion (1kHz a 20kHz, exponencial), doma los armonicos
    if (highCut < 0.999f)
    {
        const float highCutHz = 1000.0f * std::pow (20.0f, highCut);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            *highCutFilter[channel].coefficients =
                *juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, highCutHz, 0.707f);

            auto* data = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i)
                data[i] = highCutFilter[channel].processSample (data[i]);
        }
    }

    // Output gain
    buffer.applyGain (outputGain);

    // Mix: dry/wet
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* wet = buffer.getWritePointer (channel);
        auto* dry = dryBuffer.getReadPointer (channel);
        for (int i = 0; i < numSamples; ++i)
            wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
    }
}

//==============================================================================
bool LaDistornetaAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* LaDistornetaAudioProcessor::createEditor()
{
    return new LaDistornetaAudioProcessorEditor (*this);
}

void LaDistornetaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LaDistornetaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaDistornetaAudioProcessor();
}
