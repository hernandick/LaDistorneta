#pragma once
#include <atomic>
#include <JuceHeader.h>

// Los 5 "tipos" de distorsion, cada uno con el nombre de un jugador
// y un algoritmo DSP distinto.
enum class DistortionType
{
    Messi = 0,        // saturacion de valvula (tanh), calida y musical
    Dibu = 1,         // fuzz asimetrico, ruidoso y con caracter
    JulianAlvarez = 2, // overdrive (soft clip cubico), motor incansable
    Lautaro = 3,      // distorsion normal / hard clip, directo y contundente
    DePaul = 4        // bit crush / sample-hold, textura cruda
};

class LaDistornetaAudioProcessor : public juce::AudioProcessor
{
public:
    LaDistornetaAudioProcessor();
    ~LaDistornetaAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Solo mono o estereo, con la misma cantidad de canales en entrada y salida.
    // El oversampling se dimensiona una vez en prepareToPlay para el layout activo,
    // asi que aceptar layouts arbitrarios (ej. 1 in / 8 out) lo deja desincronizado
    // con el buffer real que llega a processBlock y provoca un crash.
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // El APVTS guarda: type, drive, lowcut, tone, highcut, mix, output, garra
    juce::AudioProcessorValueTreeState apvts;

    // Nivel para el VU meter de la UI, estilo "Attitude" (Decapitator): peak pre-shaper
    // multiplicado por el gain del Drive, o sea que tan fuerte golpea la señal al circuito
    // de saturacion (>1.0 = 0dB = zona no lineal). Se publica con escala comprimida
    // (lvl^(1/3): 1 dB mostrado = 3 dB reales) para que la aguja tenga mas recorrido.
    // channel: 0 = izq, 1 = der. Lo actualiza el audio thread; la UI solo lee (atomic).
    float getMeterLevel (int channel) const noexcept
    {
        return channel == 0 ? meterLevelL.load() : meterLevelR.load();
    }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Waveshaper por-muestra segun el tipo elegido. `amount` viene normalizado 0..1
    float shape (DistortionType type, float x, float amount) const noexcept;

    // Oversampling 4x para los tipos que generan armonicos altos (todos menos DePaul,
    // que usa reduccion de sample-rate y no necesita anti-aliasing).
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Filtros de la seccion de EQ, uno por canal:
    // lowCut  = high-pass pre-distorsion
    // tilt    = par de shelves opuestos (Tone) post-distorsion
    // highCut = low-pass post-distorsion
    juce::dsp::IIR::Filter<float> lowCutFilter[2];
    juce::dsp::IIR::Filter<float> tiltLowShelf[2];
    juce::dsp::IIR::Filter<float> tiltHighShelf[2];
    juce::dsp::IIR::Filter<float> highCutFilter[2];

    // Estado del bit-crusher (De Paul): valor retenido y contador de sample-hold, por canal
    float crushHeld[2] { 0.0f, 0.0f };
    int   crushCounter[2] { 0, 0 };

    double currentSampleRate = 44100.0;

    // Estado del VU meter (estilo Attitude): valores "crudos" que solo toca el audio thread
    // (ballistics de attack instantaneo / release ~300ms), publicados a los atomics para la UI.
    float levelL = 0.0f, levelR = 0.0f;
    std::atomic<float> meterLevelL { 0.0f }, meterLevelR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaDistornetaAudioProcessor)
};
