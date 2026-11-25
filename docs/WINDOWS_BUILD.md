# Windows experimental build 0.3

The owner-controlled native package now includes a JUCE VST3 target pinned to 8.0.12, a basic native editor, three separate engine states, raw preset controls, a multi-instance FL preset selector and raw export. Engine preparation and disposal are moved to a control worker with a 20 ms crossfade when swapping prepared engines. This resets old tails after the crossfade; live parameter automation, pitch-continuous tempo changes, Bank oversampling latency compensation, host recall and FL Studio are not verified. No screenshots requested for this test build.

Local Linux native regression suites: 4/4 release and 4/4 ASan/UBSan/float-cast-overflow. New isolated exchange test covers crossfade, concurrent producer/consumer (10,000 iterations each), reclamation. This does not certify the native kernels' full callback safety. Local JUCE configure failed on DNS. No plugin compilation claimed.

GitHub Actions is configured for Windows 2022 x64, native regression tests and VST3 packaging. Privacy gate blocks public repositories before fetching private data. After confirming the repository is private, attach the exact private ZIP to a private GitHub release with tag `skellay-native-0.3` and rerun the PR workflow. Never attach it to a public release.

Package: `SKELLAY_PRIVATE_NATIVE_FOUNDATION_0.3.zip`
SHA256: `a371652f647a57eac6387e96fa263eb92d46dbc4f8792b3524dbe47169cea873`

No public DSP or binary distribution authorized. No pluginval or host test performed. Skell Delay untouched.
