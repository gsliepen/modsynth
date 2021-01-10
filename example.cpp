/* SPDX-License-Identifier: MIT */

#include <iostream>
#include "modsynth.hpp"

using namespace ModSynth;

struct Example: Module {
	// Components
	VCO clock{4};
	Sequencer sequencer{
		"C4", "E4", "G4", "C5",
		"D4", "F4", "A4", "D5",
		"Bb3", "D4", "F4", "Bb4",
		"F5", "C5", "A4", "F4",
	};
	VCO vco;
	Envelope envelope{0.01, 1, 0.1};
	VCA vca;
	Speaker speaker;

	// Routing
	void update()
	{
		sequencer.clock_in = clock.square_out;
		envelope.gate_in   = sequencer.gate_out;
		vco.frequency      = sequencer.frequency_out;
		vca.amplitude      = envelope.amplitude_out;
		vca.audio_in       = vco.triangle_out;
		speaker.left_in    = vca.audio_out;
		speaker.right_in   = vca.audio_out;
	}
} example;

int main()
{
	// Components
	VCO clock{1};
	Sequencer sequencer{"C2", "D2", "Bb1", "F1"};
	VCO vco;
	VCF vcf{0, 3};
	VCA vca{2000};
	Envelope envelope{0.1, 1, 0.1};
	Speaker speaker;

	// Routing
	Wire wires[] {
		{clock.square_out,        sequencer.clock_in},
		{sequencer.gate_out,      envelope.gate_in},
		{sequencer.frequency_out, vco.frequency},
		{envelope.amplitude_out,  vca.audio_in},
		{vca.audio_out,           vcf.cutoff},
		{vco.sawtooth_out,        vcf.audio_in},
		{vcf.lowpass_out,         speaker.left_in},
		{vcf.lowpass_out,         speaker.right_in},
	};

	start();
	std::cout << "Press enter to exit...\n";
	std::cin.get();
}
