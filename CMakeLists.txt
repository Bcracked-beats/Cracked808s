cmake_minimum_required(VERSION 3.22)
project(Cracked808s VERSION 2.5.4)

# Ship a self-contained Windows plugin; do not require a separate VC++ runtime install.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# JUCE is vendored by the build step into third_party/JUCE for reproducible builds.
add_subdirectory(third_party/JUCE)

juce_add_binary_data(Cracked808sAssets NAMESPACE CrackedAssets HEADER_NAME CrackedAssets.h
    SOURCES assets/CrackedVaultLogo.png assets/SpaceGrotesk.ttf)

juce_add_plugin(Cracked808s
    COMPANY_NAME "Cracked Vault"
    BUNDLE_ID "com.crackedaudio.cracked808s"
    # Published IDs from the original working FL Studio instance. Keep these stable.
    PLUGIN_MANUFACTURER_CODE Bcrk
    PLUGIN_CODE Ck8s
    FORMATS VST3 Standalone
    PRODUCT_NAME "Cracked808s"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE)

# JUCE's generated Windows resource needs the version-info dependency too;
# otherwise an incremental version bump can retain an old Explorer version.
if(WIN32)
    get_target_property(cracked_generated Cracked808s JUCE_GENERATED_SOURCES_DIRECTORY)
    get_target_property(cracked_info Cracked808s JUCE_INFO_FILE)
    add_custom_command(OUTPUT "${cracked_generated}/Cracked808s_resources.rc"
        APPEND DEPENDS "${cracked_info}")
endif()

target_sources(Cracked808s PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h)

juce_generate_juce_header(Cracked808s)

target_compile_definitions(Cracked808s PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    "JUCE_VST3_COMPATIBLE_CLASSES=juce::VST3Interface::jucePluginId(0x43724163,0x436b3830)")

target_link_libraries(Cracked808s PRIVATE
    Cracked808sAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

enable_testing()
add_executable(Cracked808sDSPTests tests/DSPTests.cpp)
target_compile_features(Cracked808sDSPTests PRIVATE cxx_std_17)
target_include_directories(Cracked808sDSPTests PRIVATE src)
add_test(NAME DSPRegression COMMAND Cracked808sDSPTests)
add_executable(Cracked808sSoundTests tests/SoundQualityTests.cpp)
target_compile_features(Cracked808sSoundTests PRIVATE cxx_std_17)
target_include_directories(Cracked808sSoundTests PRIVATE src)
add_test(NAME SoundQuality COMMAND Cracked808sSoundTests)
add_executable(Cracked808sAudioFingerprint tests/AudioFingerprint.cpp)
target_compile_features(Cracked808sAudioFingerprint PRIVATE cxx_std_17)
target_include_directories(Cracked808sAudioFingerprint PRIVATE src)

juce_add_console_app(Cracked808sValidation PRODUCT_NAME "Cracked808s Validation")
target_sources(Cracked808sValidation PRIVATE tests/IntegrationTests.cpp)
target_include_directories(Cracked808sValidation PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/Cracked808s_artefacts/JuceLibraryCode")
target_compile_definitions(Cracked808sValidation PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(Cracked808sValidation PRIVATE Cracked808s Cracked808sAssets juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra juce::juce_recommended_config_flags)
add_test(NAME PluginIntegration COMMAND Cracked808sValidation)

if(WIN32)
    add_executable(Cracked808sUpdateFixture tests/UpdateFixture.cpp)
    target_compile_features(Cracked808sUpdateFixture PRIVATE cxx_std_17)
    add_dependencies(Cracked808sValidation Cracked808sUpdateFixture)
    add_custom_command(TARGET Cracked808sValidation POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Cracked808sUpdateFixture>" "$<TARGET_FILE_DIR:Cracked808sValidation>/Cracked808sUpdateFixture.exe")
    juce_add_console_app(Cracked808sUpdate PRODUCT_NAME "Cracked808s Update Check")
    target_sources(Cracked808sUpdate PRIVATE src/Updates/UpdateHelper.cpp)
    target_compile_definitions(Cracked808sUpdate PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(Cracked808sUpdate PRIVATE juce::juce_core juce::juce_recommended_config_flags winhttp)
    foreach(update_target Cracked808s_VST3 Cracked808s_Standalone Cracked808sValidation)
        add_custom_target(${update_target}_UpdateBundle
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${update_target}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:Cracked808sUpdate>" "$<TARGET_FILE_DIR:${update_target}>/Cracked808sUpdate.exe"
            DEPENDS Cracked808sUpdate)
        add_dependencies(${update_target} ${update_target}_UpdateBundle)
    endforeach()
    add_executable(Cracked808sBundleProbe tests/BundleProbe.cpp)
    target_compile_features(Cracked808sBundleProbe PRIVATE cxx_std_17)
    target_include_directories(Cracked808sBundleProbe PRIVATE third_party/JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK)
    target_link_libraries(Cracked808sBundleProbe PRIVATE ole32)
    add_test(NAME VST3IdentityAndLoad COMMAND Cracked808sBundleProbe "$<TARGET_FILE:Cracked808s_VST3>" "{ABCDEF01-9182-FAEB-4263-726B436B3873}")
    add_test(NAME VST3EditorShutdown COMMAND Cracked808sBundleProbe "$<TARGET_FILE:Cracked808s_VST3>" "{ABCDEF01-9182-FAEB-4263-726B436B3873}" --editor)
    set_tests_properties(VST3EditorShutdown PROPERTIES TIMEOUT 20)
    add_test(NAME VST3WorkerUnload COMMAND Cracked808sBundleProbe "$<TARGET_FILE:Cracked808s_VST3>" "{ABCDEF01-9182-FAEB-4263-726B436B3873}" --editor --worker-unload)
    set_tests_properties(VST3WorkerUnload PROPERTIES TIMEOUT 15)
    add_test(NAME VST3ProcessExit COMMAND Cracked808sBundleProbe "$<TARGET_FILE:Cracked808s_VST3>" "{ABCDEF01-9182-FAEB-4263-726B436B3873}" --editor --process-exit)
    set_tests_properties(VST3ProcessExit PROPERTIES TIMEOUT 15)
endif()
