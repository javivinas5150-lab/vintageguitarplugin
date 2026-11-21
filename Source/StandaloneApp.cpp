//==============================================================================
// Jemini Guitar Rig - standalone application
//
// Same as JUCE's default standalone app, but with our own Audio/MIDI settings
// dialog: the guitar input is chosen channel by channel (Input 1, Input 2, ...)
// instead of the stereo pairs JUCE shows, and the input is not muted by
// default (a guitar DI cannot feed back like a microphone).
//
// Only compiled into the Standalone target (JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP).
//==============================================================================
#include <JuceHeader.h>

#if JucePlugin_Build_Standalone && JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "PluginProcessor.h"

namespace
{
    using namespace juce;

    //--------------------------------------------------------------------------
    // Helpers to read / write which hardware input feeds the guitar chain
    int getGuitarInput (AudioDeviceManager& dm)
    {
        const auto setup = dm.getAudioDeviceSetup();
        const int bit = setup.inputChannels.findNextSetBit (0);
        return bit < 0 ? 0 : bit;
    }

    void setGuitarInput (AudioDeviceManager& dm, int channel)
    {
        auto* device = dm.getCurrentAudioDevice();
        if (device == nullptr || channel < 0 || channel >= device->getInputChannelNames().size())
            return;

        auto setup = dm.getAudioDeviceSetup();
        setup.useDefaultInputChannels = false;
        setup.inputChannels.clear();
        setup.inputChannels.setBit (channel);
        dm.setAudioDeviceSetup (setup, true);
    }

    //--------------------------------------------------------------------------
    class SettingsPanel : public Component, private ChangeListener
    {
    public:
        SettingsPanel (StandalonePluginHolder& h)
            : holder (h),
              selector (h.deviceManager, 0, 1, 0, 2, true, false, false, false)
        {
            guitarLabel.setText ("Guitar input:", dontSendNotification);
            guitarLabel.setJustificationType (Justification::centredRight);
            guitarLabel.setFont (FontOptions (15.0f, Font::bold));
            addAndMakeVisible (guitarLabel);

            guitarInput.onChange = [this]
            {
                if (guitarInput.getSelectedItemIndex() >= 0)
                    setGuitarInput (holder.deviceManager, guitarInput.getSelectedItemIndex());
            };
            addAndMakeVisible (guitarInput);

            muteButton.setButtonText ("Mute input (only if you hear feedback through speakers)");
            muteButton.getToggleStateValue().referTo (holder.getMuteInputValue());
            addAndMakeVisible (muteButton);

            addAndMakeVisible (selector);

            holder.deviceManager.addChangeListener (this);
            refreshInputs();
            setSize (520, 560);
        }

        ~SettingsPanel() override { holder.deviceManager.removeChangeListener (this); }

        void paint (Graphics& g) override
        {
            g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
            g.setColour (Colours::white.withAlpha (0.12f));
            g.drawHorizontalLine (88, 12.0f, (float) getWidth() - 12.0f);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (12, 10);
            auto row = r.removeFromTop (30);
            guitarLabel.setBounds (row.removeFromLeft (170));
            row.removeFromLeft (8);
            guitarInput.setBounds (row.removeFromLeft (260));
            r.removeFromTop (8);
            muteButton.setBounds (r.removeFromTop (26).withTrimmedLeft (178));
            r.removeFromTop (16);
            selector.setBounds (r);
        }

    private:
        void changeListenerCallback (ChangeBroadcaster*) override { refreshInputs(); }

        void refreshInputs()
        {
            guitarInput.clear (dontSendNotification);

            if (auto* device = holder.deviceManager.getCurrentAudioDevice())
            {
                const auto names = device->getInputChannelNames();
                for (int i = 0; i < names.size(); ++i)
                    guitarInput.addItem (String (i + 1) + ":  " + names[i], i + 1);

                guitarInput.setSelectedItemIndex (getGuitarInput (holder.deviceManager), dontSendNotification);
                guitarInput.setEnabled (names.size() > 0);
            }
            else
            {
                guitarInput.setTextWhenNothingSelected ("(no audio device)");
                guitarInput.setEnabled (false);
            }
        }

        StandalonePluginHolder& holder;
        Label guitarLabel;
        ComboBox guitarInput;
        ToggleButton muteButton;
        AudioDeviceSelectorComponent selector;
    };

    //--------------------------------------------------------------------------
    // JUCE's window, with our own Options button/menu on top of the original
    class JeminiWindow : public StandaloneFilterWindow
    {
    public:
        JeminiWindow (const String& title, Colour bg, std::unique_ptr<StandalonePluginHolder> h)
            : StandaloneFilterWindow (title, bg, std::move (h))
        {
            // Hide JUCE's Options button; ours sits in the same place
            for (auto* c : getChildren())
                if (auto* b = dynamic_cast<TextButton*> (c); b != nullptr && b != &options && b->getButtonText() == "Options")
                    b->setVisible (false);

            options.setTriggeredOnMouseDown (true);
            options.onClick = [this] { showMenu(); };
            Component::addAndMakeVisible (options);

            installHooks();
            resized();
        }

        // The amp's input jacks / IN buttons drive the device input here
        void installHooks()
        {
            if (auto* p = dynamic_cast<VintageGuitarSuiteAudioProcessor*> (getAudioProcessor()))
            {
                auto& dm = getDeviceManager();
                p->hostGetInput = [&dm] { return getGuitarInput (dm); };
                p->hostSetInput = [&dm] (int ch) { setGuitarInput (dm, ch); };
            }
        }

        ~JeminiWindow() override
        {
            if (auto* p = dynamic_cast<VintageGuitarSuiteAudioProcessor*> (getAudioProcessor()))
            {
                p->hostGetInput = nullptr;
                p->hostSetInput = nullptr;
            }
        }

        void resized() override
        {
            StandaloneFilterWindow::resized();

            for (auto* c : getChildren())
                if (auto* b = dynamic_cast<TextButton*> (c); b != nullptr && b != &options && b->getButtonText() == "Options")
                    b->setVisible (false);

            options.setBounds (8, 6, 70, getTitleBarHeight() - 8);
            options.toFront (false);
        }

    private:
        void showMenu()
        {
            PopupMenu m;
            m.addItem (1, "Audio/MIDI Settings...");
            m.addSeparator();
            m.addItem (2, "Save current state...");
            m.addItem (3, "Load a saved state...");
            m.addSeparator();
            m.addItem (4, "Reset to default state");

            m.showMenuAsync (PopupMenu::Options().withTargetComponent (options),
                             [safe = Component::SafePointer<JeminiWindow> (this)] (int result)
            {
                if (safe == nullptr) return;
                auto* holder = safe->getPluginHolder();

                switch (result)
                {
                    case 1:  safe->showSettings(); break;
                    case 2:  holder->askUserToSaveState(); break;
                    case 3:  holder->askUserToLoadState(); break;
                    case 4:  safe->resetToDefaultState(); safe->installHooks(); break;
                    default: break;
                }
            });
        }

        void showSettings()
        {
            DialogWindow::LaunchOptions o;
            o.content.setOwned (new SettingsPanel (*getPluginHolder()));
            o.dialogTitle                  = "Audio/MIDI Settings";
            o.dialogBackgroundColour       = o.content->getLookAndFeel().findColour (ResizableWindow::backgroundColourId);
            o.escapeKeyTriggersCloseButton = true;
            o.useNativeTitleBar            = true;
            o.resizable                    = true;
            o.launchAsync();
        }

        TextButton options { "Options" };
    };

    //--------------------------------------------------------------------------
    class JeminiApp : public JUCEApplication
    {
    public:
        JeminiApp()
        {
            PropertiesFile::Options options;
            options.applicationName     = CharPointer_UTF8 (JucePlugin_Name);
            options.filenameSuffix      = ".settings";
            options.osxLibrarySubFolder = "Application Support";
           #if JUCE_LINUX || JUCE_BSD
            options.folderName          = "~/.config";
           #else
            options.folderName          = "";
           #endif
            appProperties.setStorageParameters (options);
        }

        const String getApplicationName() override              { return CharPointer_UTF8 (JucePlugin_Name); }
        const String getApplicationVersion() override           { return JucePlugin_VersionString; }
        bool moreThanOneInstanceAllowed() override              { return true; }
        void anotherInstanceStarted (const String&) override    {}

        void initialise (const String&) override
        {
            auto* settings = appProperties.getUserSettings();
            const bool firstRun = ! settings->containsKey ("shouldMuteInput");

            auto holder = std::make_unique<StandalonePluginHolder> (settings, false, String{}, nullptr,
                                                                    Array<StandalonePluginHolder::PluginInOuts>{}, false);

            // Guitar DI: start unmuted (users can still enable the mute)
            if (firstRun)
                holder->getMuteInputValue() = false;

            mainWindow = std::make_unique<JeminiWindow> (getApplicationName(),
                                                         LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                                                         std::move (holder));
            mainWindow->setVisible (true);
        }

        void shutdown() override
        {
            mainWindow = nullptr;
            appProperties.saveIfNeeded();
        }

        void systemRequestedQuit() override
        {
            if (mainWindow != nullptr)
                mainWindow->pluginHolder->savePluginState();

            if (ModalComponentManager::getInstance()->cancelAllModalComponents())
            {
                Timer::callAfterDelay (100, []
                {
                    if (auto app = JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
            }
            else
            {
                quit();
            }
        }

    private:
        ApplicationProperties appProperties;
        std::unique_ptr<JeminiWindow> mainWindow;
    };
}

juce::JUCEApplicationBase* juce_CreateApplication();
juce::JUCEApplicationBase* juce_CreateApplication() { return new JeminiApp(); }

#endif
