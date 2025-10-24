# Nag

A simple SDL2 demo engine.

## Dependecies policy

Some deps are loaded through the system package manager, and some others through CMake.

### CMake-managed deps

- SDL2
- imgui
- imguifiledialog

The above dependencies are managed using the [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) CMake module.

### System prerequisites

The following system packages are required:

- libopenmpt-dev
- libmpg123-dev
- libasound2-dev
- pkg-config

In a Debian-based system, you can install them with:

```bash
sudo apt update
sudo apt install libopenmpt-dev libmpg123-dev libasound2-dev pkg-config
```

## What to do next

### Bugs 🐛

- Audio buzzing when the slider is moved while playing
- crash on mp3 load

### Editor Features 🚀

- store audio track(s) in current project
- manage project files (json)
  - Save current project
  - Load project
- fine movements in audio tracks (mods): step forward/backward, seek

## The name

Nag Champa Agarbathi (नागचम्पा अगरबत्ती) literally means "the incense of the sacred Champaca tree" or "the incense of the Champaca flower".

"Nāga" refers to the sacred Nagara tree or spirit/breath, and "Champa" refers to the flower or the tree itself (the plumeria or Champaca tree), and "Agarbatti" is the Sanskrit word for a scented incense stick.

"Nagchampa agarbathi" refers to Nag Champa incense sticks, with "Nag" meaning "snake" and "Champa" referring to the fragrant Champaka flower found in India and Nepal. "Agarbathi" is the Hindi word for the incense sticks themselves, which are made from a blend of Champaka flower, sandalwood, and other natural resins. This popular Indian incense is known for its unique, rich, earthy, and slightly sweet aroma and is used for meditation, relaxation, and to create a calming, spiritually uplifting atmosphere.

I had this in mind when I wrote the name of this project, and I thought it was a good name.

Just after writing this, I learned that the word "nag" in English has a completely different meaning, and I loved the idea of using this pun even more.
