/* SPDX-License-Identifier: MIT */

#pragma once

#include <alsa/asoundlib.h>
#include <bitset>
#include <string>

#include "modsynth.hpp"

namespace ModSynth
{

/**
 * @brief A MIDI input module.
 *
 * This implements a virtual MIDI port, events sent to it will be converted to values.
 * The supported events are:
 *
 * - note on/off: will be converted to frequency and gate outputs, one pair per channel.
 * - control changes: will be converted to values
 * - pitch bend, aftertouch: will be converted to values
 */
struct MIDI: Module {
	/// @name Outputs
	///@{
	struct Channel {
		float frequency;        ///< The frequency of the last pressed note
		float velocity;         ///< The velocity of the last pressed note
		float release_velocity; ///< The release velocity of the last pressed note
		float gate;             ///< Gate output, > 0 if the note is pressed, <= 0 if the note is released
		float aftertouch;       ///< The amount of aftertouch, between 0 and 1
		float pitch_bend;       ///< The amount of pitch bend, between -1 and 1
		float parameter[128];   ///< Holds the value for each MIDI parameter, between 0 and 1
	private:
		friend struct MIDI;
		std::bitset<128> notes;
	} channels[16]; ///< The state of all the channels of this MIDI port
	///@}

	MIDI(const std::string &name = "modsynth");
	~MIDI();

	void update(); ///< The function that will update the state of this module.

private:
	void process_event(const snd_seq_event_t *event); ///< Process a single MIDI event

	snd_seq_t *seq{}; ///< The ALSA sequencer handle
};

}
