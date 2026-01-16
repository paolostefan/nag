# Nag

A simple SDL2 demo engine.

## What to do next

### Bugs 🐛

- Audio buzzing when the slider is moved while playing
- audio track UI/info is cluttered and ugly
- the timeline allows deleting all tracks, included the effects: it should not allow deletion of effects and link audio_tracks editor member to the corresponding timelines.

### New engine features 🚀

- Update Mandelbrot coloring (which now sucks)
- Add some effects:
  - Julia quaternions
  - plasma
  - fire
  - noise

### New editor Features 🚀

- popup notifications (toasts) - info/warning/error
- keyframe class
- timeline view
- interactively set and edit keyframes on every effect parameter
- interactively select tween keyframe method
- set the start/end of an effect via UI
- fine movements in mod-audio tracks: step forward/backward in pattern

## Building the project

So far (January 2026), the project is Linux-only.

Some dependencies are loaded through the system package manager, some others through CMake. The latter don't need any intervention, as they will be downloaded from github at Cmake configure step.

### System dependencies

The following system packages are required under APT-based GNU/Linux distros (like Ubuntu and Debian):

- libasound2-dev
- libglew-dev
- libmpg123-dev
- libopenmpt-dev
- pkg-config

### CMake-managed deps

These dependencies are automatically downloaded using the [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) CMake module.

- SDL2
- imgui
- imguifiledialog
- nlohmann/json

---

## The project name

Nag Champa Agarbathi (नागचम्पा अगरबत्ती) literally means "the incense of the sacred Champaca tree" or "the incense of the Champaca flower".

It is the name of my favorite incense sticks, with "Nag" meaning "snake" and "Champa" referring to the fragrant Champaka flower (the plumeria or Champaca tree) found in India and Nepal. "Agarbathi" is the Hindi word for the incense sticks themselves, which are made from a blend of Champaka flower, sandalwood, and other natural resins. This popular Indian incense is known for its unique, rich, earthy, and slightly sweet aroma and is used for meditation, relaxation, and to create a calming, spiritually uplifting atmosphere.

I had this in mind when I wrote the name of this project, and I thought it was a good name. Right after writing this, I learned that the word "nag" in English has a completely different meaning, and I loved the idea of using this pun even more.
