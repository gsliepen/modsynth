/* SPDX-License-Identifier: MIT */

#pragma once

#include <deque>
#include <initializer_list>
#include <string>
#include <vector>

namespace ModSynth
{

/**
 * @brief The base class for all modules.
 *
 * This is the base class that all ModSynth modules should derive from. It can
 * also be used by the user to create new modules. Deriving from this class will
 * ensure that the derived objects will be registered in the module registry,
 * and when audio output is started using the start() function, the update()
 * member function of each module will be called once for each time step.
 */
struct Module {
	/**
	 * @brief The constructor.
	 *
	 * This will register the module, so its update() function will be called
	 * for every time step. Since this is the constructor of the base class, any
	 * member variables in the derived class will not have been initialized yet.
	 * You must therefore ensure that the whole object is completely initialized
	 * before calling the global start() function.
	 */
	Module();

	Module(const Module &other) = delete;
	Module(Module &&other) = delete;

	/**
	 * @brief The destructor.
	 *
	 * This will deregister the module, so its update() function will no longer
	 * called for every time step. The object should not be destroyed while the
	 * audio output is still active.
	 */
	virtual ~Module();

	/**
	 * @brief The update function.
	 *
	 * This function is called every time step. The derived class must implement
	 * this, and it should update all the output values according to the input
	 * values and any other state it might have.
	 */
	virtual void update() = 0;

	/**
	 * @brief The time step used for the update function in seconds.
	 */
	static constexpr float dt = 1.0f / 48000.0f;
};

/**
 * @brief A Value Controlled Oscillator.
 *
 * This implements a [numerically controlled oscillator], whose frequency is
 * determined by the frequency input value. Several waveforms are derived that
 * are made available as outputs.
 *
 * [numerically controlled oscillator]: https://en.wikipedia.org/wiki/Numerically-controlled_oscillator
 */
struct VCO: Module {
	/// @name Inputs
	///@{
	float frequency{}; ///< The frequency of the oscillator, in Hz.
	///@}

	/// @name Outputs
	///@{
	float sawtooth_out{-1}; ///< Sawtooth (ramp) output.
	float sine_out{};       ///< Sine wave output.
	float square_out{1};    ///< Square wave output.
	float triangle_out{};   ///< Triangular wave output.
	///@}

	VCO() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param frequency  The initial frequency to oscillate at, in Hz.
	 */
	VCO(float frequency): frequency(frequency) {}

	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief An envelope generator.
 *
 * This implements an Attack, Decay, Release (ADR) [envelope generator]. The
 * #gate_in going high (> 0) triggers the attack phase, which will linearly
 * increase #amplitude_out until it reaches unity, after #attack seconds. After
 * the attack, the decay phase will decrease #amplitude_out exponentially,
 * halving it once every #decay seconds. The #gate_in going low (<= 0) triggers
 * the release phase, which will decrease #amplitude_out exponentially, halving
 * it once every #release seconds.
 *
 * [envelope generator]: https://en.wikipedia.org/wiki/Envelope_(music)
 */
struct Envelope: Module {
	/// @name Inputs
	///@{
	float gate_in{}; ///< Gate input, > 0 trigger attack, <= 0 triggers release.
	float attack{};  ///< The attack rise time in seconds.
	float decay{};   ///< The decay time in seconds.
	float release{}; ///< The release time in seconds.
	///@}

	/// @name Outputs
	///@{
	float amplitude_out{}; ///< The generated amplitude.
	///@}

	Envelope() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param attack   The initial attack rise time in seconds.
	 * @param decay    The initial decay time in seconds.
	 * @param release  The initial release time in seconds.
	 */
	Envelope(float attack, float decay, float release): attack(attack), decay(decay), release(release) {}

	void update(); ///< The function that will update the state of this module.

private:
	// Internal state
	enum {
		ATTACK,
		DECAY,
		RELEASE,
	} state; ///< The phase in which the envelope generator currectly is.
};

// Modifiers

/**
 * @brief A Value Controlled Amplifier.
 *
 * This implements a [variable amplifier], which multiplies the #audio_in value
 * with the given #amplitude.
 *
 * [amplifier]: https://en.wikipedia.org/wiki/Variable-gain_amplifier
 */
struct VCA: Module {
	/// @name Inputs
	///@{
	float audio_in{};  ///< The audio input signal.
	float amplitude{}; ///< The amplitude to multiply the input with.
	///@}

	/// @name Outputs
	///@{
	float audio_out{}; ///< The amplified audio output signal.
	///@}

	VCA() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param amplitude  The initial amplitude used for amplifying the #audio_in signal.
	 */
	VCA(float amplitude): amplitude(amplitude) {}

	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief A Value Controlled Filter.
 *
 * This is a 12 dB/octave [state variable filter]. It will filter #audio_in
 * according to the given #cutoff frequency and #resonance level, and will
 * provide lowpass, bandpass and highpass filtered versions of the input.
 *
 * [state variable filter]: https://en.wikipedia.org/wiki/State_variable_filter
 */
struct VCF: Module {
	/// @name Inputs
	///@{
	float audio_in{}; ///< The audio input signal.
	float cutoff{}; ///< The cutoff frequency in Hz.
	float resonance{}; ///< The resonance, 0 for no resonance, higher values produce more resonance.
	///@}

	/// @name Outputs
	///@{
	float lowpass_out{}; ///< A lowpass filtered version of #audio_in.
	float bandpass_out{}; ///< A bandpass filtered version of #audio_in.
	float highpass_out{}; ///< A highpass filtered version of #audio_in.
	///@}

	VCF() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param cutoff     The initial cutoff frequency to initialize the filter with, in Hz.
	 * @param resonance  The initial resonance level to initialize the filter with.
	 */
	VCF(float cutoff, float resonance): cutoff(cutoff), resonance(resonance) {}
	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief A linear slew.
 *
 * The output will try to follow the input, but the change in the output is limited to a linear slew rate.
 */
struct LinearSlew: Module {
	/// @name Inputs
	///@{
	float in{}; ///< The input signal.
	float rate{1}; ///< The slew rate in units/second.
	///@}

	/// @name Outputs
	///@{
	float out{}; ///< The output signal.
	///@}

	LinearSlew() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param rate     The slew rate in units/second.
	 * @param initial  The initial value of the output.
	 */
	LinearSlew(float rate = 1, float initial = 0): rate(rate), out(initial) {}

	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief An exponential slew.
 *
 * The output will try to follow the input, but the change in the output is limited to an exponential slew rate.
 */
struct ExponentialSlew: Module {
	/// @name Inputs
	///@{
	float in{1};    ///< The input signal.
	float rate{1}; ///< The slew rate in octaves/second.
	///@}

	/// @name Outputs
	///@{
	float out{1}; ///< The output signal.
	///@}

	ExponentialSlew() = default;

	/**
	 * @brief The constructor.
	 *
	 * @param rate     The slew rate in octaves/second.
	 * @param initial  The initial value of the output.
	 */
	ExponentialSlew(float rate = 1, float initial = 1): rate(rate), out(initial) {}

	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief A delay.
 *
 * The output is the input delayed by a given time.
 * The maximum delay must be passed to the constructor.
 */
struct Delay: Module {
	/// @name Inputs
	///@{
	float in{};  ///< The input signal.
	float delay; ///< The delay in seconds.
	///@}

	/// @name Outputs
	///@{
	float out{}; ///< The output signal.
	///@}

	/**
	 * @brief The constructor.
	 *
	 * @param max_delay  The maximum delay in seconds.
	 */
	Delay(float max_delay = 1);

	void update(); ///< The function that will update the state of this module.
private:
	std::deque<float> buffer;
};

// Sequencer

/**
 * @brief A simple sequencer.
 *
 * A [sequencer] has a list of #frequencies, one of which will be sent to
 * #frequency_out. If the #clock_in signal goes high (> 0), the next frequency
 * in the list will be used. After the last frequency is reached, it will
 * restart from the beginning.
 *
 * The #gate_out signal will become 1 when the #clock_in goes high, and will
 * become 0 when #clock_in goes low (<= 0), thus providing a clean version of
 * the #clock_in.
 *
 * After construction, the size of the vector #frequencies must not be changed,
 * but the frequencies can be modified.
 *
 * [sequencer]: https://en.wikipedia.org/wiki/Analog_sequencer
 */
struct Sequencer: Module {
	/// @name Inputs
	///@{
	float clock_in{};               ///< The clock input.
	std::vector<float> frequencies; ///< The list of frequencies the sequencer cycles through.
	///@}

	/// @name Outputs
	///@{
	float frequency_out; ///< The currently selected frequency.
	float gate_out;      ///< A cleaned up copy of #clock_in.
	///@}

	/**
	 * @brief The constructor.
	 *
	 * @param notes  The initial list of notes used to initialize the list of frequencies.
	 */
	Sequencer(std::initializer_list<std::string> notes);

	void update(); ///< The function that will update the state of this module.

private:
	// Internal state
	std::size_t index; ///< The index of the currently selected frequency.
};


/**
 * @brief A speaker that will generate audible output.
 *
 * A speaker will ensure the input signals are sent to the audio card. Multiple
 * speaker objects can be used, and the input of all of them will be mixed
 * together.
 */
struct Speaker: Module {
	/// @name Inputs
	///@{
	float left_in{};  ///< The left channel audio input.
	float right_in{}; ///< The right channel audio input.
	///@}

	void update(); ///< The function that will update the state of this module.
};

/**
 * @brief A wire connecting an output value to an input value.
 *
 * This is a module that will automatically copy the given output value to the input value.
 */
struct Wire: Module {
	/**
	 * @brief The constructor.
	 *
	 * @param from  A reference to the value to copy from.
	 * @param to    A reference to the value to copy to.
	 */
	Wire(const float &from, float &to): from(from), to(to) {}
	void update();     ///< The function that copies from one value to the other.
private:
	const float &from; ///< A reference to the value to copy from.
	float &to;         ///< A reference to the value to copy to.
};


/**
 * @brief Start the audio output.
 *
 * This function should be called after all the Module objects have been
 * instantiated.
 */
void start();

/**
 * @brief Stop the audio output.
 */
void stop();

}
