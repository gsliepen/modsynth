# ModSynth - a modular synthesizer framework for C++

This is a small C++ library that allows you to write a software synthesizer
in the same way you would build a real [modular synthesizer]. Several types
are provided that implement the basic components of a modular synthesizer,
such as a [VCO], [envelope generator], [sequencer] and so on. Each type
derives from struct Module, and has public variables that represent the
inputs and outputs of the component it implements, and these inputs and
outputs can be connected together in two possible ways:

1. By declaring Wire objects that will automatically copy from a given output to a given input.
2. By defining a new Module type, and adding assignment statements in a user-defined update() function.

It is also easy to create new module types yourself. Finally, this library
will take care of sending the output sent to Speaker objects  to the sound card.

[modular synthesizer]: https://en.wikipedia.org/wiki/Modular_synthesizer
[VCO]: https://en.wikipedia.org/wiki/Voltage-controlled_oscillator
[envelope generator]: https://en.wikipedia.org/wiki/Envelope_(music)
[sequencer]: https://en.wikipedia.org/wiki/Music_sequencer

## Example

The following shows an example of how you can use this library:

```
#include <iostream>
#include "modsynth.hpp"

int main()
{
    // Declare all the components
    VCO clock{1};
    Sequencer sequencer{"C2", "D2", "Bb1", "F1"};
    VCO vco;
    VCF vcf{0, 3};
    VCA vca{2000};
    Envelope envelope{0.1, 1, 0.1};
    Speaker speaker;

    // Declare the wires connecting them
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

    // Start synthesizing
    start();
    std::cout << "Press enter to exit...\n";
    std::cin.get();
}
```

## Building

To build your own software synthesizers using this library, you must ensure you
compile and link with `modsynth.cpp`, and link with the [`SDL2`] library. The
following command shows how to do this on most UNIX-like operating systems:

```
c++ -o my_synth my_synth.cpp modsynth.cpp -lSDL2
```

The example code that comes with this library can be built using the [Meson]
build system. It might be necessary to ensure you have the SDL2 library and
header files, Meson as well as a C++11 compliant compiler installed. On
Debian-derived operating systems, you can install the dependencies using:

```
sudo apt install build-essential libsdl2-dev meson
```

Once the dependencies are installed, configure and build the examples as
follows:

```
meson build
ninja -C build
```

The generated example binaries can then be found in the directory `build/`.

[SDL2]: https://www.libsdl.org/
[Meson]: https://mesonbuild.com/

## Documentation

[Doxygen] can be used to generate documentation for this library. Ensure you
have Doxygen installed, for example by doing the following on Debian-derived
operating systems:

```
sudo apt install doxygen
```

Then build the documentation as follows:

```
doxygen Doxyfile
```

The generated documentation in HTML format can be found in the directory
`html/`.

[Doxygen]: https://www.doxygen.nl/index.html
