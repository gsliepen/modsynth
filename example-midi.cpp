/* SPDX-License-Identifier: MIT */

#include <iostream>
#include "modsynth.hpp"
#include "midi.hpp"

using namespace ModSynth;

int main()
{
	// Components
	MIDI midi;
	VCO vco;
	VCF vcf{0, 3};
	VCA vca{2000};
	Envelope envelope{0.1, 1, 0.1};
	Speaker speaker;

	// Routing
	Wire wires[] {
		{midi.channels[0].gate,      envelope.gate_in},
		{midi.channels[0].frequency, vco.frequency},
		{envelope.amplitude_out,     vca.audio_in},
		{vca.audio_out,              vcf.cutoff},
		{vco.sawtooth_out,           vcf.audio_in},
		{vcf.lowpass_out,            speaker.left_in},
		{vcf.lowpass_out,            speaker.right_in},
	};

	start();
	std::cout << "Press enter to exit...\n";
	std::cin.get();
}
