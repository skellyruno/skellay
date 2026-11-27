# Research delay interpolation: preserve feedback history

Parent 4175b662b6545ce66295dd96a32eb909da692abf. Original optional native ResearchDelay, not connected to plugin model or state. Add prepare-time cubicOutput=false option. Default linear read and feedback math preserved; when enabled use a four-point cubic wet read and crossfade both wet heads using existing transition age. Feedback uses original linear/convex interpolation and original history. Fall back to linear for integer taps, too-short delay or insufficient history boundary; do not read current unwritten sample. Cubic can overshoot; not a limiter.

Local optimized and ASan/UBSan test at 8/44.1/48/96/192/384 kHz passed integer parity, exact dry, half-sample impulse ([-.0625,.5625,.5625,-.0625]), 1.5-sample fallback, and parity after a fractional-history run settles at integer delay. Existing research_delay optimized test passed. Added CTest target with assertions enabled. No claim of exhaustive parity against parent for all input configurations.

18 paired tone cases at 44.1/48/96 kHz, delay 32.5 samples, feedback zero, fully wet. At 44.1k:

| Hz | Linear dB | Cubic dB |
|---:|---:|---:|
| 1000 | -0.022 | -0.000 |
| 4000 | -0.358 | -0.021 |
| 8000 | -1.494 | -0.314 |
| 12000 | -3.658 | -1.482 |
| 16000 | -7.581 | -4.580 |
| 20000 | -16.742 | -13.281 |

36 fingerprinted private real vocal excerpts (159.25s, 44.1k), 108 pairs, 216 renders: 120 ms plus .5 sample, fully wet, feedback 0/.45 and a .45 case with additional 100 ms jump halfway. One second tail appended. All finite. Sample peaks fixed-feedback +0.605/+0.653 dBFS (linear/cubic); each mode exceeds full scale on one excerpt. Jump case +.261/+.348 dBFS, one excerpt each. No-feedback case below full scale. Relative median waveform difference about -30 dB, not proof of listening quality. Cubic reduces high-band droop but does not remove it and feedback repeats still experience linear filtering. Keep option experimental, headroom and CPU gate open; no default, production models, state or GUI changes. Local source hashes and full private report digest in JSON. Reproduce: python tests/bench_delay_interpolation.py --manifest /private/manifest.json --output /private/metrics.json. No private inputs in Git.
