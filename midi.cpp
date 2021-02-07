/* SPDX-License-Identifier: MIT */

#include "midi.hpp"

#include <cmath>
#include <iostream>

namespace ModSynth
{

MIDI::MIDI(const std::string &name)
{
	snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0);
	snd_seq_nonblock(seq, true);
	snd_seq_set_client_name(seq, name.c_str());
	snd_seq_create_simple_port(seq, name.c_str(),
	                           SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
	                           SND_SEQ_PORT_TYPE_SOFTWARE | SND_SEQ_PORT_TYPE_SYNTHESIZER);
	struct pollfd pfds[10];
	auto npfd = snd_seq_poll_descriptors_count(seq, POLLIN);
	snd_seq_poll_descriptors(seq, pfds, npfd, POLLIN);
}

MIDI::~MIDI()
{
	if (seq) {
		snd_seq_delete_port(seq, 0);
		snd_seq_close(seq);
	}
}

void MIDI::update()
{
	while (true) {
		snd_seq_event_t *event;
		auto left = snd_seq_event_input(seq, &event);

		if (event) {
			process_event(event);
		}

		if (left <= 0) {
			break;
		}
	}
}

static float note_to_frequency(uint8_t note)
{
	return 440.0f * std::exp2((note - 69.0f) / 12.0f);
}

static float highest_note_frequency(const std::bitset<128> &notes)
{
	for (uint8_t note = 128; note--;) {
		if (notes.test(note)) {
			return note_to_frequency(note);
		}
	}

	return {};
}

void MIDI::process_event(const snd_seq_event_t *event)
{
	std::cerr << ".";

	switch (event->type) {
	case SND_SEQ_EVENT_NOTEON: {
		auto &ch = channels[event->data.note.channel];

		if (event->data.note.velocity) {
			if (ch.notes.none()) {
				ch.velocity = event->data.note.velocity / 127.0f;
			}

			ch.notes.set(event->data.note.note);
			ch.frequency = highest_note_frequency(ch.notes);
			ch.gate = 1;
		} else {
			ch.notes.reset(event->data.note.note);

			if (ch.notes.none()) {
				ch.release_velocity = ch.velocity;
				ch.gate = 0;
			} else {
				ch.frequency = highest_note_frequency(ch.notes);
			}
		}

		break;
	}

	case SND_SEQ_EVENT_NOTEOFF: {
		auto &ch = channels[event->data.note.channel];

		ch.notes.reset(event->data.note.note);

		if (ch.notes.none()) {
			ch.release_velocity = ch.velocity;
			ch.gate = 0;
		} else {
			ch.frequency = highest_note_frequency(ch.notes);
		}

		break;
	}

	case SND_SEQ_EVENT_KEYPRESS: {
		auto &ch = channels[event->data.note.channel];
		auto frequency = event->data.note.note;

		if (ch.frequency == frequency) {
			ch.aftertouch = event->data.note.velocity / 127.0f;
		}

		break;
	}

	case SND_SEQ_EVENT_CHANPRESS: {
		auto &ch = channels[event->data.control.channel];
		ch.aftertouch = event->data.control.value / 127.0f;
		break;
	}

	case SND_SEQ_EVENT_PITCHBEND: {
		auto &ch = channels[event->data.control.channel];
		ch.pitch_bend = event->data.control.value / 4096.0f;
		break;
	}

	case SND_SEQ_EVENT_CONTROLLER: {
		auto &ch = channels[event->data.control.channel];
		ch.parameter[event->data.control.param] = event->data.control.value / 127.0f;
		break;
	}

	default:
		break;
	}
}

}
