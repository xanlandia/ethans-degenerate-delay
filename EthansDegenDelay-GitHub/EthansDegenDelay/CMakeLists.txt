cmake_minimum_required(VERSION 3.22)
project(EthansDegenDelay VERSION 1.0.0)

add_subdirectory(deps/JUCE)

juce_add_plugin(EthansDegenDelay
    COMPANY_NAME                "Dirtbag Audio"
    PLUGIN_MANUFACTURER_CODE    "DbAu"
    PLUGIN_CODE                 "EgDl"
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Ethan's Degenerate Delay"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Delay"
)

target_sources(EthansDegenDelay PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(EthansDegenDelay PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EthansDegenDelay PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
