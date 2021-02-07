project('ModSynth', 'cpp',
  version: '0.1',
  license: 'MIT'
)

sdl2 = dependency('SDL2')
alsa = dependency('alsa')

executable('example',
  'example.cpp',
  'modsynth.cpp',
  dependencies: [
    sdl2,
  ],
)

executable('example-midi',
  'example-midi.cpp',
  'modsynth.cpp',
  'midi.cpp',
  dependencies: [
    alsa,
    sdl2,
  ],
)
