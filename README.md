# AlphaSix
6-voice DCO polysynth (VST3) in the style of the Roland Alpha Juno. JUCE 8 / CMake.

Local build: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release --target AlphaSix_VST3`
(JUCE is fetched automatically if there is no `JUCE/` folder.)

Windows/macOS/Linux builds: push to GitHub; the workflow in `.github/workflows/build.yml` uploads
a `AlphaSix-<OS>-VST3` artifact per platform. Install on Windows: copy `AlphaSix.vst3` to `C:\Program Files\Common Files\VST3\`.
