# Feedback-loop decay and optional wet-path trim

Parent 0c603680874ee1d94387fb5731493e086f0ff654; parent Windows workflow completed successfully. All 13 cached Quick4 manifest checks passed. The exact parent ResearchDelay header was checked by Git hash before editing. No new dependency or UI work.

## Repeat coloration

At fixed delay D+0.5 samples, the original linear feedback read has amplitude response abs(cos(pi*f/fs)); the cubic wet read affects the first pass, not the recursive response. Feedback .75 supplies -2.4988 dB per round trip before interpolation loss. Native impulse renders at three rates, six echoes each, five frequencies, three modes: all 45 decay oracles matched within 1e-3 dB (maximum observed error 1.78e-14 dB). At 44.1 kHz, 8/12/16 kHz decay per additional repeat is -3.993/-6.156/-10.080 dB for both half-sample readout modes, versus -2.499 dB for integer delay. This distinguishes first-echo brightness from repeat coloration.

The symmetric crossfeed matrix has mid eigenvalue 1 and side eigenvalue 1-2*cross. At feedback .75, cross 0/.25/.5/1 yields side gains .75/.375/0/-.75. Verified 72 stereo difference impulse observations (three rates, four crossfeed settings, six echoes) within 1e-7. Crossfeed .5 removes side on subsequent repeats, and crossfeed 1 reverses side on every loop. No new feedback algorithm adopted.

36 source-fingerprint-matched real vocal excerpts at 44.1k; select highest-energy 125 ms window on half-window grid, isolate six echoes with 250 ms base delay, feedback .75, zero crossfeed. 108 renders. Median amplitude loss per additional repeat calculated from first to sixth echo band energy:

| Band | Integer | Half linear | Half cubic wet |
|---|---:|---:|---:|
| 250–1000 Hz | -2.499 | -2.507 | -2.507 |
| 4–8 kHz | -2.499 | -3.248 | -3.262 |
| 8–12 kHz | -2.499 | -4.452 | -4.476 |

These are selected real vocal bands, not a universal decay curve. Separate output interpolation and feedback coloration in any future model.

## Optional headroom control

Add prepare-time wetTrimDb (bounded -18 to 0 dB, default zero). At zero trim, the original return expression is retained. For nonzero trim only the wet output is multiplied, after feedback write; dry gain and feedback history remain unchanged. No extra delay, allocation in process or parameter ID change. Existing Classic/Modern/Bank and serialization untouched.

36 full private recordings; 288 paired configurations (576 renders), feedback .45, crossfeed .35, delay 120ms plus half sample, fixed or +100ms jump, linear/cubic output, 50/100% mix. All wet-gain oracles <=3e-7, observed max 5.96e-8. Highest sample peak +.6531 -> -2.3469 dBFS at -3dB trim, four untrimmed over-scale cases, zero trimmed. Fixed trim is not a peak limiter. Peaks are sample peaks, not certified true peaks. Partial mix changes relative wet level by design.

Optimized GCC and ASan/UBSan new test passed, six rates x two interpolation modes x 100k frames, changing delay, crossfeed, feedback and reset. Fully wet exact scaling, dry exact. Registered CTest. New hosted CI unverified; no new plugin model selection or state/UI integration, no DAW CPU, listening or release claim. No private audio or per-input metrics in Git. Reproduce: python tests/bench_delay_echoes.py --manifest /private/manifest.json --output /private/metrics.json. NumPy, SoundFile, GCC required.
