# Skellay

Independent three-engine native JUCE VST3: Classic, Modern and Bank. Separate from Skell Delay. Experimental owner-authorized testing only. No public release or general distribution license is granted; Skelly Licensing terms pending. See docs/WINDOWS_BUILD.md for the current direct Windows Actions build and existing evidence.

Native-only regression build: cmake -S . -B build -DSKELLAY_BUILD_VST3=OFF -DCMAKE_BUILD_TYPE=Release, then cmake --build build and ctest --test-dir build --output-on-failure. Windows VST3: enable SKELLAY_BUILD_VST3. JUCE pinned to 8.0.12. Private audio and recovered raw preset fixtures excluded.

Development rules: [SkellWorld/AGENTS.md](https://github.com/skellyruno/SkellWorld/blob/main/AGENTS.md).
