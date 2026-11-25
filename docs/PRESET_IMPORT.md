# Experimental preset import — v0.2

The private native package adds an offline C++ importer and command-line selector, with automatic model identification and explicit instance selection for mixer presets. Classic = Fruity Delay 2; Modern = Fruity Delay 3; Bank = Fruity Delay Bank. Original Fruity Delay / Delay 1 is unsupported; it is not silently remapped. Raw records are retained byte-identically. The importer does not apply changes to an active audio callback.

Strict initial recognition of one FLhd/FLdt named-wrapper layout; unknown formats are rejected. This is not a full FL event parser. Slot mix, project routing, bypass, host automation and project tempo are not imported. No edited version-15 re-export or export of complete FST files.

Local native release and sanitizer suites: 3/3 each. 37/37 recovered raw records (28 Delay 2/3 and 9 Bank; 24 distinct) validated and rendered using reconstructed wrappers. Nine records originated in FST and 28 in FLP according to prior extraction metadata. Original full archives could not be materialized during this session, so genuine end-to-end container validation remains blocked. Synthetic multi-engine wrapper, 1267 truncations, 2000 deterministic mutations, wrong length/version/model and legacy detection passed. CLI byte-exact export and existing-file refusal passed. Private source, settings and audio remain external. Package SHA256: 907580cb09ac38d7be65e05da5638d42e341205e8eba2006a832147f833e5854.

JUCE 8.0.12 retrieval failed again (DNS). Live parameter/tempo changes, VST3 browser, Quick4 screenshots, Windows and FL Studio validation are not implemented or verified. Skell Delay untouched.
