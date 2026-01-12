# Nag

A simple SDL2 demo engine.

## Dependecies policy

Some deps are loaded through the system package manager, and some others through CMake.

### CMake-managed deps

- SDL2
- imgui
- imguifiledialog
- nlohmann/json

The above dependencies are managed using the [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) CMake module.

### System prerequisites

The following system packages are required under APT-based GNU/Linux distros (like Ubuntu and Debian):

- libasound2-dev
- libglew-dev
- libmpg123-dev
- libopenmpt-dev
- pkg-config

## What to do next

### Bugs 🐛

- Audio buzzing when the slider is moved while playing
- crash on mp3 load
- audio track UI/info is cluttered and ugly

### Editor Features 🚀

- popup notifications (toasts) - info/warning/error
- timeline class
- keyframe class
- timeline view
- interactively set and edit keyframes on every effect parameter
- interactively select tween keyframe method
- set the start/end of an effect via UI
- fine movements in audio tracks (mods): step forward/backward, seek

## The project name

Nag Champa Agarbathi (नागचम्पा अगरबत्ती) literally means "the incense of the sacred Champaca tree" or "the incense of the Champaca flower".

It is the name of my favorite incense sticks, with "Nag" meaning "snake" and "Champa" referring to the fragrant Champaka flower (the plumeria or Champaca tree) found in India and Nepal. "Agarbathi" is the Hindi word for the incense sticks themselves, which are made from a blend of Champaka flower, sandalwood, and other natural resins. This popular Indian incense is known for its unique, rich, earthy, and slightly sweet aroma and is used for meditation, relaxation, and to create a calming, spiritually uplifting atmosphere.

I had this in mind when I wrote the name of this project, and I thought it was a good name. Right after writing this, I learned that the word "nag" in English has a completely different meaning, and I loved the idea of using this pun even more.
