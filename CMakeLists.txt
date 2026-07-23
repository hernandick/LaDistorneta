cmake_minimum_required(VERSION 3.22)

# Binario universal: corre nativo en Apple Silicon y en Macs Intel.
# (Tiene que definirse ANTES de project() para que tome efecto.)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)

# Version minima de macOS. SIN esto, el build toma la del SDK de la maquina
# que compila (ej. macOS 15) y el plugin NO carga en Macs mas viejas
# (error "OpenAComponent: -1"). 10.13 es el piso que necesita el WebView.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)

project(LaDistorneta VERSION 0.0.2)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LaDistorneta
    COMPANY_NAME           "HappyDickAudio"
    PLUGIN_MANUFACTURER_CODE Hern
    PLUGIN_CODE            Dstr
    FORMATS                VST3 AU
    PRODUCT_NAME           "LaDistorneta"
    # En Windows la UI web corre sobre WebView2; esto linkea el loader necesario.
    NEEDS_WEBVIEW2         TRUE
    # Copia el plugin a ~/Library/Audio/Plug-Ins despues de cada build,
    # asi el DAW siempre carga la ultima version compilada.
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_add_binary_data(LaDistornetaAssets
    SOURCES
        Source/ui.html
)

juce_generate_juce_header(LaDistorneta)

target_sources(LaDistorneta
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(LaDistorneta
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        # Activa el backend WebView2 en Windows; sin esto la API moderna del
        # WebBrowserComponent (withResourceProvider, etc.) no se compila alla.
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

target_link_libraries(LaDistorneta
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        LaDistornetaAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
