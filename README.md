# Nag

A simple SDL2 demo engine.

## @FIXME && @TODO

- [ ] Nodes: add mandelbrot and julia set nodes
- [ ] Graph editor: for texture-output nodes, add the texture size in pixels in the node body
- [ ] Add an MCP server to allow remote control of the engine (e.g. for live coding)
- [ ] Scene editor: time nodes should get their values from the current timeline position
- [ ] Scene editor: add a "Recent scenes" submenu
- [ ] Scene editor: add audio tracks
- [ ] Scene editor: associate graph <-> timeline and set initial time 
- [ ] Timeline: set/save to disk initial particle system state  

## Building the project

Linux-only (as tested so far).

Dependencies split: system packages (APT) + CMake FetchContent (auto-downloaded at configure step).

```bash
sudo apt install libasound2-dev libglew-dev libfreetype-dev libmpg123-dev libopenmpt-dev pkg-config
```

### System dependencies (APT)

| Package           | Purpose                                   |
|-------------------|-------------------------------------------|
| `libasound2-dev`  | ALSA audio (SDL2 audio backend)           |
| `libglew-dev`     | OpenGL extension wrangler (ShaderManager) |
| `libfreetype-dev` | Font rendering (ImGui)                    |
| `libmpg123-dev`   | MP3 decoding                              |
| `libopenmpt-dev`  | Module tracking audio (OpenMPT)           |
| `pkg-config`      | Build tool (locates system libs)          |

### CMake-managed deps (FetchContent)

Auto-downloaded from GitHub at configure — no manual steps:

- **SDL2** — windowing, input, audio
- **imgui** (docking branch) — immediate-mode GUI
- **ImGuiFileDialog** — file browser dialogs
- **ImPlot** — plotting widgets for ImGui
- **nlohmann/json** — JSON serialization
- **spdlog** — logging
- **IconFontCppHeaders** — icon font header maps
- **stb** — single-header image/utility libs

---

## The project name

Nag Champa Agarbathi (नागचम्पा अगरबत्ती) literally means "the incense of the sacred Champaca tree" or "the incense of
the
Champaca flower."

They're my favorite incense sticks, with "Nag" meaning "snake" and "Champa" referring to the fragrant Champaka flower
(the plumeria or Champaca tree) found in India and Nepal.
"Agarbathi" is the Hindi word for the incense sticks themselves, which are made from a blend of Champaka flower,
sandalwood, and other natural resins.

I had this in mind when I chose the name of this project, and I thought it was a good name. But, right after writing
the above, I learned that the English word "nag" has a completely different meaning, and I loved this pun even more.

## Credits

The FontStruction “Amiga Topaz” (https://fontstruct.com/fontstructions/show/675155) by Patrick H. Lauke is licensed
under a Creative Commons Attribution license (http://creativecommons.org/licenses/by/3.0/).
[ancestry]