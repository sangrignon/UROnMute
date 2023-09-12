/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2022 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             UROnMute
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Tests the audio latency of a device.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        UROnMute

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include "DemoUtilities.h"
#include "AudioLiveScrollingDisplay.h"

//==============================================================================
class UnMuter  : public AudioIODeviceCallback
{
public:
    UnMuter (TextEditor& editorBox)
        : resultsBox (editorBox)
    {}

    //==============================================================================
    bool beginTest()
    {
        resultsBox.moveCaretToEnd();
        resultsBox.insertTextAtCaret (newLine + newLine + "Starting test..." + newLine);
        resultsBox.moveCaretToEnd();

        if( testIsRunning )
            testIsRunning = false;
        else
            testIsRunning = true;
        
        return testIsRunning;
    }

    //==============================================================================
    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        testIsRunning = false;
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels,
                                           int numSamples, const AudioIODeviceCallbackContext& context) override
    {
        ignoreUnused (context);

        const ScopedLock sl (lock);

        if (testIsRunning)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                for( auto ich = numInputChannels; --ich >= 0;)
                {
                    for( auto och = numOutputChannels; --och >= 0;)
                    {
                        if ((inputChannelData[ich] != nullptr) && (outputChannelData[och] != nullptr))
                            outputChannelData[och][i] = inputChannelData[ich][i];
                        
                    }
                }
            }
        }
        else
        {
            // We need to clear the output buffers, in case they're full of junk..
            for (int i = 0; i < numOutputChannels; ++i)
                if (outputChannelData[i] != nullptr)
                    zeromem (outputChannelData[i], (size_t) numSamples * sizeof (float));
        }
    }

private:
    TextEditor& resultsBox;
    CriticalSection lock;

    bool testIsRunning    = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UnMuter)
};

//==============================================================================
class UROnMute  : public Component
{
public:
    UROnMute()
    {
        setOpaque (true);

        liveAudioScroller.reset (new LiveScrollingAudioDisplay());
        addAndMakeVisible (liveAudioScroller.get());

        addAndMakeVisible (resultsBox);
        resultsBox.setMultiLine (true);
        resultsBox.setReturnKeyStartsNewLine (true);
        resultsBox.setReadOnly (true);
        resultsBox.setScrollbarsShown (true);
        resultsBox.setCaretVisible (false);
        resultsBox.setPopupMenuEnabled (true);

        resultsBox.setColour (TextEditor::outlineColourId, Colour (0x1c000000));
        resultsBox.setColour (TextEditor::shadowColourId,  Colour (0x16000000));

        resultsBox.setText ("This app is used to make your voice louder. Click on the start button and speak into the microphone.");

        addAndMakeVisible (startTestButton);
        startTestButton.onClick = [this] { startTest(); };

        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [this] (bool granted)
                                     {
                                         int numInputChannels = granted ? 2 : 0;
                                         audioDeviceManager.initialise (numInputChannels, 2, nullptr, true, {}, nullptr);
                                     });

        audioDeviceManager.addAudioCallback (liveAudioScroller.get());

        setSize (500, 500);
    }

    ~UROnMute() override
    {
        audioDeviceManager.removeAudioCallback (liveAudioScroller.get());
        audioDeviceManager.removeAudioCallback (unMuter    .get());
        unMuter    .reset();
        liveAudioScroller.reset();
    }

    void startTest()
    {
        if (unMuter.get() == nullptr)
        {
            unMuter.reset (new UnMuter (resultsBox));
            audioDeviceManager.addAudioCallback (unMuter.get());
        }

        if( unMuter->beginTest() )
            startTestButton.setButtonText("STOP");
        else
            startTestButton.setButtonText("START");
    }

    void paint (Graphics& g) override
    {
        g.fillAll (findColour (ResizableWindow::backgroundColourId));
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (5);

        if (liveAudioScroller.get() != nullptr)
        {
            liveAudioScroller->setBounds (b.removeFromTop (b.getHeight() / 5));
            b.removeFromTop (10);
        }

        startTestButton.setBounds (b.removeFromBottom (b.getHeight() / 10));
        b.removeFromBottom (10);

        resultsBox.setBounds (b);
    }

private:
    AudioDeviceManager audioDeviceManager;

    std::unique_ptr<UnMuter> unMuter;
    std::unique_ptr<LiveScrollingAudioDisplay> liveAudioScroller;

    TextButton startTestButton  { "START" };
    TextEditor resultsBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UROnMute)
};
