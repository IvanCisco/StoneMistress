/*
  ==============================================================================

    Oscillator.h
    Created: 30 Sep 2024 11:16:27am
    Author:  Ivan

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#define CHORUS_DELAY_TIME 20.0

class LFO
{
public:
	LFO(double defaultRate = 7.0, double defaultPhaseDelta = 0.8)
	{
		rate.setTargetValue(defaultRate);
		phaseDelta = defaultPhaseDelta;
	}

	~LFO() {}

	void prepareToPlay(double sampleRate)
	{
		rate.reset(sampleRate, 0.02);
		samplePeriod = 1.0 / sampleRate;
	}

	void setRate(double newValue)
	{
		// No zero-frequency allowed
		jassert(newValue > 0);
		rate.setTargetValue(newValue);
	}

	void getNextAudioBlock(AudioBuffer<double>& buffer, const int numSamples)
	{
		auto chorusChannelData = buffer.getWritePointer(0);
		auto phaserChannelData = buffer.getWritePointer(1);

		for (int smp = 0; smp < numSamples; ++smp)
		{
			float leftSample = 0.0f;
			float rightSample = 0.0f;
			
			getNextAudioSample(leftSample, rightSample);

			phaserChannelData[smp] = leftSample;
			chorusChannelData[smp] = rightSample;
		}
	}

	void getNextAudioSample(float& leftSample, float& rightSample)
	{
		leftSample = 4.0f * abs(currentPhase - 0.5f) - 1.0f;
		rightSample = 4.0 * abs((currentPhase + phaseDelta) - 0.5) - 1.0;

		phaseIncrement = rate.getNextValue() * samplePeriod;
		currentPhase += phaseIncrement;
		currentPhase -= static_cast<int>(currentPhase);
	}

private:
	SmoothedValue<double, ValueSmoothingTypes::Multiplicative> rate;

	double currentPhase = 0;
	double phaseIncrement = 0;
	double samplePeriod = 1.0;
	double phaseDelta = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFO)
};

// Il canale 0 o sinistro � destinato a modulare il CHORUS.
// Il canale 1 o destro modula invece l'unit� PHASER.
class TimeModulation {
public:

	TimeModulation(const double defaultDelayTime = 0.008, const double defaultPhaserDepth = 0.000, const double defaultChorusDepth = 0.0)
	{
		phaserDelayTime.setCurrentAndTargetValue(defaultDelayTime);
		phaserDepth.setCurrentAndTargetValue(defaultPhaserDepth);
		chorusDepth.setCurrentAndTargetValue(defaultChorusDepth);
	}

	~TimeModulation() {}

	void prepareToPlay(double sampleRate)
	{
		phaserDelayTime.reset(sampleRate, 0.02);
		phaserDepth.reset(sampleRate, 0.02);
		chorusDepth.reset(sampleRate, 0.02);
	}

	void setPhaserDepth(const double newValue)
	{
		phaserDepth.setTargetValue(newValue);
	}

	void setChorusDepth(const double newValue)
	{
		chorusDepth.setTargetValue(newValue);
	}

	void setPhaserDelayTime(const double newValue)
	{
		phaserDelayTime.setTargetValue(newValue);
	}

	void processBlock(AudioBuffer<double>& buffer, const int numSamples)
	{
		auto data = buffer.getArrayOfWritePointers();
		const auto numCh = buffer.getNumChannels();

		for (int ch = 0; ch < numCh; ++ch)
		{
			FloatVectorOperations::add(data[ch], 1.0, numSamples);
			FloatVectorOperations::multiply(data[ch], 0.5, numSamples);
		}

		chorusDepth.applyGain(data[0], numSamples);
		phaserDepth.applyGain(data[1], numSamples);

		if (phaserDelayTime.isSmoothing())
		{
			for (int smp = 0; smp < numSamples; ++smp)
			{
				data[1][smp] += phaserDelayTime.getNextValue();
			}
		}
		else
		{
			FloatVectorOperations::add(data[1], phaserDelayTime.getCurrentValue(), numSamples);
		}

		FloatVectorOperations::add(data[0], CHORUS_DELAY_TIME, numSamples);
	}

private:

	SmoothedValue<double, ValueSmoothingTypes::Linear> phaserDelayTime;
	SmoothedValue<double, ValueSmoothingTypes::Linear> phaserDepth;
	SmoothedValue<double, ValueSmoothingTypes::Linear> chorusDepth;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeModulation)

};

// This class allows the LFO and TimeModulation classes to communicate with each other. If the RATE value is below a certain
// threshold, the pedal will enter "Filter Matrix" mode, so that the PHASER unit's delay time will be manually set via the RATE
// knob. This reults in the ability for the user to manually move the notches across the frequency spectrum. If the RATE knob points
// to a value above the threshold, Filter Matrix mode will be disengaged, that is, the Phaser's delay time will be dynamically 
// modulated by the LFO as usual.

class FilterMatrix
{
public:
	FilterMatrix(LFO& lfo, TimeModulation& modulator)
	{
	}

	~FilterMatrix() {}

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterMatrix)

};