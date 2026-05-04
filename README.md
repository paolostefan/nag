# Nag

A simple SDL2 demo engine.

## What to do next

### Bugs 🐛

- Audio buzzing when the slider is moved while playing
- audio track UI/info is cluttered and ugly
- the timeline allows deleting all tracks, including the effects: it should not allow deletion of effects and link audio_tracks editor member to the corresponding timelines.

### New engine features 🚀

- Play!
- Update Mandelbrot coloring (which now sucks)
- Add some effects:
  - Julia quaternions
  - plasma
  - fire
  - noise

### Editor Features

#### WIP 🚧

Here should go stuff which is currently developed, but not 100% ready.

#### To be 🚀

- Add titles to timeline tracks
- keyframe handling
- add keyframes to the timeline view
- interactively set and edit keyframes on every effect parameter
- interactively select tween keyframe method
- set the start/end of an effect via UI
- fine movements in mod-audio tracks: step forward/backward in the pattern

## Building the project

So far (January 2026), the project is Linux-only.

Some dependencies are loaded through the system package manager, some others through CMake.
The latter don't need any intervention, as they will be downloaded from GitHub at the Cmake configure step.

### System dependencies

The following packages are required under APT-based GNU/Linux distros (like Ubuntu and Debian):

- libasound2-dev
- libglew-dev
- libfreetype-dev
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

Nag Champa Agarbathi (नागचम्पा अगरबत्ती) literally means "the incense of the sacred Champaca tree" or "the incense of the
Champaca flower."

They're my favorite incense sticks, with "Nag" meaning "snake" and "Champa" referring to the fragrant Champaka flower
(the plumeria or Champaca tree) found in India and Nepal.
"Agarbathi" is the Hindi word for the incense sticks themselves, which are made from a blend of Champaka flower,
sandalwood, and other natural resins.

I had this in mind when I chose the name of this project, and I thought it was a good name. But, right after writing 
the above, I learned that the English word "nag" has a completely different meaning, and I loved this pun even more.

## Credits

The FontStruction “Amiga Topaz” (https://fontstruct.com/fontstructions/show/675155) by Patrick H. Lauke is licensed under a Creative Commons Attribution license (http://creativecommons.org/licenses/by/3.0/).
[ancestry]