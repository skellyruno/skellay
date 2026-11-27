# Experimental native foundation — 2026-11-24

The owner created skellyruno/skellay (public). Private source and audio must not be committed here. Skelly Licensing is provisional; source-specific provenance and dependency terms must be confirmed before distributing any binary.

Private local package: actual supplied Classic and Modern kernels reused from Sound Match; Bank isolated from its old host wrapper. Shared frozen-state interface exposes all raw state fields. Bank validation rejects malformed records, including version-15 overflow before conversion; v15 is canonicalized to v16 by the source parser. Original bytes remain in the configuration. No claim of general FST or FL Studio compatibility.

Local GCC release and ASan/UBSan/float-cast-overflow runs passed two native test executables. 105 default impulse renders: five sample rates, seven block sizes, three models. Zero sample difference across block sizes for these cases. Eight-enabled-bank patterned smoke at 48 kHz passed for oversampling levels 0, 1, 2; this only verifies finite nonzero output, not reference accuracy. One-sample pattern edge explicitly guarded in the private adapted source; reference behavior is unverified.

Important limitations: native prepare allocates; timing, bank pattern/options and engine switching must not be moved into the callback. Frozen-state tests are not proof of real-time allocation safety, full automation, sound equivalence, latency or recall in a DAW. JUCE 8.0.12 pin identified; local network DNS prevents retrieval. No VST3/editor built. SkellCore Quick4 inspection, Windows, pluginval and FL Studio are open. Prior Sound Match isolated-stage results are historical, not new plugin measurements.

Next gate: live parameter/tempo handoff preserving tails, then JUCE APVTS and state recall, native editor with three engine tabs and bank selector, genuine Quick4 captures, host validation. Never publish private source/audio or merge an unverified build.
