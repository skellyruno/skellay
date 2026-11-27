# ResearchDelay complex feedback-response oracle

Parent 6271e2fc7136c4144d2254e83af4d51e270f51b1. Builds on RESEARCH_DELAY_FEEDBACK.md rather than replacing its real-vocal and wet-trim evidence. This regression adds a broader complex-valued (magnitude and phase) impulse oracle across fixed settings. Initially tested 0c60368; branch advanced to 6271e2f during the run, so verified the latest header blob and repeated optimized GCC and ASan/UBSan tests successfully, with identical measured results. Default zero wet trim. No DSP source change.

For fixed fractional delay d=N+f, the linear read kernel is L(z)=(1-f)+f*z^-1, apart from integer delay. At f=.5 its magnitude is abs(cos(omega/2)). The crossfeed matrix has mid eigenvalue 1 and side eigenvalue 1-2c. For isolated echo e starting at e*N, the mid spectrum is W(z)*(feedback*L(z))^(e-1); side is W(z)*(feedback*(1-2c)*L(z))^(e-1). W is either L or the four-tap cubic output kernel; cubic remains outside feedback.

96 fixed-control configurations (3 rates, 2 read modes, 2 fractions, 2 feedback values, 4 crossfeed settings); 8 isolated echoes, six frequencies, two eigenmodes = 9,216 complex checks. Maximum absolute complex error 1.21061807468e-7 vs 5e-6 tolerance. At 44.1 kHz, half-sample, 12 kHz, feedback .9, cross .35:

| Echo | Linear mid dB | Cubic mid dB | Linear side dB | Cubic side dB |
|---:|---:|---:|---:|---:|
| 1 | -3.658 | -1.482 | -3.658 | -1.482 |
| 4 | -17.376 | -15.200 | -48.749 | -46.573 |
| 8 | -35.667 | -33.491 | -108.877 | -106.704 |

Cubic gives a one-time ~2.18 dB gain here but cannot prevent subsequent linear feedback loss. At cross=.35, relative side/mid decreases by ~10.46 dB per recurrence; at .5 side cancels after the first echo; at 1 it alternates sign with preserved relative magnitude. This is stronger phase-sensitive verification of the previously identified feedback mechanism, not a new vendor finding. Very small side magnitudes should not be interpreted as precise dB accuracy under the absolute tolerance. A separate width parameter and alternative stable feedback interpolation are research ideas only. No change to ResearchDelay, Classic/Modern/Bank, controls, UI or state.

Reproduce: g++ -std=c++17 -O3 -Wall -Wextra -Werror tests/research_delay_decay.cpp -o decay && ./decay. Added CTest registration. New hosted build and UI unverified.
