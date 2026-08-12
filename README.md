# KeyDetector 🎹

**Free real-time musical key detector (AU / VST3) for Logic Pro, Ableton Live, FL Studio, Cubase, Studio One, Bitwig & Reaper.**

KeyDetector tells you the **musical key** of whatever is playing — `A minor`,
`F# major`, and so on — right inside your music software, in real time. Put it on
a track, press play, and read the key. It never changes your sound; it just
listens.

---

## Download & install (no technical knowledge needed)

### 🍎 macOS

1. Go to the **[Releases page](../../releases)** and download **`KeyDetector-macOS.pkg`**.
2. **Double-click** the downloaded file and click through **Continue → Install**.
3. **Restart** your music software (Logic, Ableton, etc.). KeyDetector is now installed.

> **If macOS says *"Apple could not verify KeyDetector-macOS.pkg is free of malware"***
> — this just means the app is free and not paid-Apple-signed. It's safe. To install it:
> 1. On the warning, click **Done**.
> 2. Open **System Settings → Privacy & Security**, scroll to the **Security** section,
>    and click **Open Anyway** next to the KeyDetector message.
> 3. Confirm with Touch ID / your password, then run the installer again.
>
> *Prefer the terminal? Run `xattr -dr com.apple.quarantine ~/Downloads/KeyDetector-macOS.pkg`
> then double-click the installer.*

### 🪟 Windows

1. Go to the **[Releases page](../../releases)** and download **`KeyDetector-Windows-Setup.exe`**.
2. **Double-click** it and click through the installer.
   - If Windows shows a blue *"Windows protected your PC"* box, click **More info → Run anyway**.
     (Same reason as above — it's a free, unsigned app.)
3. **Restart** your music software.

That's the whole installation. There is nothing to configure.

---

## How to use it

1. Open your music software (DAW).
2. Add **KeyDetector** to a track the same way you add any effect:
   - **Logic Pro:** click an **Audio FX** slot on a track → **Audio Units → KeyDetector → KeyDetector**.
   - **Ableton Live:** in the browser, open **Plug-Ins**, find **KeyDetector**, and drag it onto a track.
   - **FL Studio / Cubase / Studio One / Bitwig / Reaper:** add it from your plugin/effects list like any VST3.
3. **Press play.** Within a few seconds the detected key appears in big letters,
   and the green bar shows how confident the detection is.
4. Moved to a different song or section? Click **Reset** to clear it and detect again.

**Tip:** put KeyDetector on the master/main output to read the key of your whole
mix, or on a single track to read just that instrument.

---

## Credits & inspiration

KeyDetector was **inspired by [ifeelvoid/keyfinder](https://github.com/ifeelvoid/keyfinder)**
(MIT-licensed) — a macOS key/BPM analysis app. KeyDetector is an independent,
from-scratch cross-platform plugin: none of that project's code is included here;
we reimplemented the key-detection approach in C++.

It detects key using the **Krumhansl-Schmuckler** key-finding algorithm
(Krumhansl & Kessler, 1982), a well-established method in music-information research.

Built with:
- **[JUCE](https://juce.com)** — the audio-plugin framework (used under the AGPLv3).
- **Steinberg VST3 SDK** — bundled with JUCE (used under the GPLv3).
- **[Catch2](https://github.com/catchorg/Catch2)** — unit-testing framework (Boost Software License 1.0; test-only, not shipped in the plugin).

*Logic Pro and Audio Units are trademarks of Apple Inc.; VST is a trademark of
Steinberg Media Technologies GmbH; Ableton Live, FL Studio, Cubase, Studio One,
Bitwig, and Reaper are trademarks of their respective owners. KeyDetector is an
independent project and is not affiliated with, sponsored by, or endorsed by any
of them.*

---

## License

KeyDetector is free and open source under the **[GNU Affero General Public License v3.0](LICENSE)** (AGPL-3.0).

**Why AGPL and not a more permissive license?** KeyDetector is built on JUCE and
the Steinberg VST3 SDK, which are copyleft (AGPLv3 / GPLv3) unless you buy a
commercial license. Because the plugin links those libraries, the whole project
must be released under a compatible copyleft license — so KeyDetector is AGPL-3.0,
and its complete source is right here in this repository. You're free to use,
study, modify, and share it; if you distribute a modified version, you must share
your source under the same license.

---

## Build from source (for developers)

Requires CMake ≥ 3.22 and a C++17 toolchain (Xcode on macOS, MSVC on Windows).
JUCE 8 and Catch2 are fetched automatically by CMake — no manual dependency setup.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target KeyDetector_All
ctest --test-dir build -C Release --output-on-failure   # run the tests
```

Build artefacts land under `build/KeyDetector_artefacts/Release/`.

**Packaging:**
- macOS: `./packaging/macos/build_pkg.sh 0.1.0` → `KeyDetector-macOS.pkg`
- Windows: compile `packaging/windows/installer.iss` with Inno Setup 6 (`ISCC.exe`)

## How it works (short version)

1. A 16384-point FFT turns each slice of audio into a spectrum.
2. Energy is folded into a 12-note **chroma** histogram that builds up over time.
3. The histogram is compared (Pearson correlation) against all 24 major/minor key profiles.
4. The best match is the detected key; the confidence bar shows how clear the winner is.

Pro Tools (AAX) is not supported.
