/* SPDX-License-Identifier: MIT */

#include "modsynth.hpp"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>
#include <iostream>

#include <SDL2/SDL.h>

/**
 * @mainpage ModSynth - a modular synthesizer framework for C++
 *
 * This is a small C++ library that allows you to write a software synthesizer
 * in the same way you would build a real [modular synthesizer]. Several types
 * are provided that implement the basic components of a modular synthesizer,
 * such as a [VCO], [envelope generator], [sequencer] and so on. Each type
 * derives from struct Module, and has public variables that represent the
 * inputs and outputs of the component it implements, and these inputs and
 * outputs can be connected together in two possible ways:
 *
 * 1. By declaring Wire objects that will automatically copy from a given output to a given input.
 * 2. By defining a new Module type, and adding assignment statements in a user-defined update() function.
 *
 * It is also easy to create new module types yourself. Finally, this library
 * will take care of sending the output sent to Speaker objects  to the sound card.
 *
 * [modular synthesizer]: https://en.wikipedia.org/wiki/Modular_synthesizer
 * [VCO]: https://en.wikipedia.org/wiki/Voltage-controlled_oscillator
 * [envelope generator]: https://en.wikipedia.org/wiki/Envelope_(music)
 * [sequencer]: https://en.wikipedia.org/wiki/Music_sequencer
 */

namespace ModSynth
{

namespace
{

/**
 * The module registry.
 *
 * This is a singleton class that provides access to the vector of modules. We
 * use this instead of a regular static variable since we can't guarantee that
 * the vector modules will be initialized before any objects that are derived
 * from Module.
 */
struct Registry {
	std::vector<Module *> modules; ///< The list of registered modules.

	/**
	 * Get a reference to the global module registry instance.
	 *
	 * @return A reference to the module registry.
	 */
	static Registry &get();
};

Registry &Registry::get()
{
	static Registry the_registry;
	return the_registry;
}

/**
 * Audio output handling.
 *
 * This class opens the audio card using SDL and registers a callback for providing audio data to SDL.
 * The callback in turn calls the update() function of all the registered Module objects.
 */
struct Audio {
	static float left;  ///< The left channel sample accumulator.
	static float right; ///< The right channel sample accumulator.

	/**
	 * The SDL audio callback.
	 *
	 * This function is called by SDL whenever a new chunk of samples needs to be sent to the audio card.
	 *
	 * @param userdata[in]  A pointer to the registry object.
	 * @param stream[out]   A pointer to the buffer where the audio samples must be written.
	 * @param len           The length of the buffer in bytes, for one channel only.
	 */
	static void callback(void *userdata, uint8_t *stream, int len)
	{
		auto registry = static_cast<Registry *>(userdata);
		float *ptr = reinterpret_cast<float *>(stream);

		for (size_t i = 0; i < len / sizeof ptr; i++) {
			left = 0;
			right = 0;

			for (auto mod : registry->modules) {
				mod->update();
			}

			// Make the output a bit softer so we don't immediately clip
			*ptr++ = left * 0.1;
			*ptr++ = right * 0.1;
		}
	}

	/**
	 * The constructor.
	 *
	 * This will intialize the SDL audio subsystem and register the audio callback function.
	 */
	Audio()
	{
		SDL_Init(SDL_INIT_AUDIO);
		SDL_AudioSpec desired{};

		desired.freq = 48000;
		desired.format = AUDIO_F32;
		desired.channels = 2;
		desired.samples = 128;
		desired.callback = callback;
		desired.userdata = &Registry::get();

		if (SDL_OpenAudio(&desired, nullptr) != 0) {
			throw std::runtime_error(SDL_GetError());
		}

	}

	/**
	 * The destructor.
	 *
	 * This will shut down SDL.
	 */
	~Audio()
	{
		SDL_Quit();
	}
} audio;

float Audio::left;
float Audio::right;

const float pi = 4.0f * std::atan(1.0f);

}

Module::Module()
{
	auto &registry = Registry::get();
	registry.modules.push_back(this);
}

Module::~Module()
{
	auto &registry = Registry::get();
	registry.modules.push_back(this);
}

void VCO::update()
{
	float phase = sawtooth_out * 0.5f - 0.5f;
	phase += frequency * dt;
	phase -= std::floor(phase);

	sawtooth_out = phase * 2.0f - 1.0f;
	sine_out = std::sin(phase * 2.0f * pi);
	square_out = std::rint(phase) * -2.0f + 1.0f;
	triangle_out = std::abs(phase - 0.5f) * 4.0f - 1.0f;
}

void Envelope::update()
{
	if (gate_in <= 0.0f) {
		state = RELEASE;
	} else if (state == RELEASE) {
		state = ATTACK;
	}

	switch (state) {
	case ATTACK:
		amplitude_out += dt / attack;

		if (amplitude_out >= 1.0f) {
			amplitude_out = 1.0f;
			state = DECAY;
		}

		break;

	case DECAY:
		amplitude_out *= std::exp2(-dt / decay);
		break;

	case RELEASE:
		amplitude_out *= std::exp2(-dt / release);
		break;
	}
}

void VCA::update()
{
	audio_out = audio_in * amplitude;
}

void VCF::update()
{
	float f = 2.0f * std::sin(std::min(pi * cutoff * dt, std::asin(0.5f)));
	float q = 1.0f / resonance;

	lowpass_out += f * bandpass_out;
	highpass_out = audio_in - q * bandpass_out - lowpass_out;
	bandpass_out += f * highpass_out;
}

void LinearSlew::update()
{
	float delta = in - out;

	if (delta > rate * dt) {
		delta = rate * dt;
	} else if (delta < -rate * dt) {
		delta = -rate * dt;
	}

	out += delta;
}

void ExponentialSlew::update()
{
	float delta = std::log2(in / out);

	if (delta > rate * dt) {
		delta = rate * dt;
	} else if (delta < -rate * dt) {
		delta = -rate * dt;
	}

	out *= std::exp2(delta);
}

Delay::Delay(float max_delay):
	delay(max_delay),
	buffer(size_t(std::ceil(max_delay / dt)) + 1)
{
}

void Delay::update()
{
	// Update the delay line
	buffer.pop_back();
	buffer.push_front(in);

	// Clamp delay to the supported range
	auto nsamples = buffer.size() - 1;

	if (delay < 0) {
		delay = 0;
	}

	if (delay > nsamples * dt) {
		delay = nsamples * dt;
	}

	// Linear interpolation of the value at the delay position
	size_t pos = delay / dt;
	float fraction = delay / dt - pos;
	out = buffer[pos] * (1 - fraction) + buffer[pos + 1] * fraction;
}

Sequencer::Sequencer(std::initializer_list<std::string> notes)
{
	static const std::map<std::string, int> base_notes = {
		{"Cb", -1}, {"C", 0}, {"C#", 1},
		{"Db", 1}, {"D", 2}, {"D#", 3},
		{"Eb", 3}, {"E", 4}, {"E#", 5},
		{"Fb", 4}, {"F", 5}, {"F#", 6},
		{"Gb", 6}, {"G", 7}, {"G#", 8},
		{"Ab", 8}, {"A", 9}, {"A#", 10},
		{"Bb", 10}, {"B", 11}, {"B#", 12},
	};

	for (auto &note : notes) {
		auto octave_pos = note.find_first_of("0123456789");
		auto base_note = base_notes.at(note.substr(0, octave_pos));
		auto octave = std::stoi(note.substr(octave_pos));
		frequencies.push_back(440.0f * std::exp2((base_note - 9) / 12.0f + octave - 4));
	}

	index = frequencies.size() - 1;
}

void Sequencer::update()
{
	if (clock_in > 0 && !gate_out) {
		index++;
		index %= frequencies.size();
	}

	frequency_out = frequencies[index];
	gate_out = clock_in > 0;
}

void Speaker::update()
{
	audio.left += left_in;
	audio.right += right_in;
}

void Wire::update()
{
	to = from;
}

void start()
{
	SDL_PauseAudio(0);
}

void stop()
{
	SDL_PauseAudio(1);
}

}
