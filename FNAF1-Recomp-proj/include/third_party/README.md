# Third-party audio decoder attribution

The Xbox 360 game uses two **unmodified** single-header libraries from
**David Reid (mackron)**'s **dr_libs** project:

- [`dr_wav.h`](https://github.com/mackron/dr_libs/blob/master/dr_wav.h) —
  WAV loading/decoding, **David Reid (mackron)**.
- [`dr_mp3.h`](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) —
  MP3 decoding, **David Reid (mackron)**. Its decoder is based on
  [**minimp3 by Lieff**](https://github.com/lieff/minimp3), as documented in
  the header itself.

Upstream project: https://github.com/mackron/dr_libs

Both dr_libs headers offer a choice of **public domain or MIT-0** licensing.
Their complete original attribution and license statements remain embedded in
`dr_wav.h` and `dr_mp3.h`, including the upstream minimp3 notice.

Downloaded from the linked upstream `master` files on 2026-11-18:

- `dr_wav.h` SHA-256:
  `a0843ec781559ec2bc733e352356a6df8739a6237ad55540773cd8b8d17cac33`
- `dr_mp3.h` SHA-256:
  `95498b23b1fce49824140c28c4be6c8721ea31722925f6639d6d08281e23f4d1`

Implementations are compiled exactly once by `src/AudioDecoders.cpp` with
stdio and wchar file APIs disabled. No decoder is used by ctfak-cpp.

---

## SMC.h — Xbox 360 Southbridge Micro Controller helpers

The game also vendors **one unmodified header** from the **Lightshow**
Ring-of-Light tester project by **Derf** (ConsoleMods.org):

- [`SMC.h`](../../Lightshow/Lightshow/SMC.h) — SMC quadrant LEDs
  (message `0x99`), center power LED (`0x8c`), fan speed, temperature /
  tilt / tray / AV-pack queries. LED messages credit
  [free60.org/SMC](http://www.free60.org/wiki/SMC) and tmbinc's smc.c;
  the header credits landaire's LaunchCode.

License: **CC0 1.0 Universal** (the Lightshow repository's LICENSE).
The four functions Lightshow implements (`PrepareBuffers`, `SetPowerLED`,
`SetSolidRing`, `SetLEDs`) are copied verbatim into `src/RoL.cpp`; the
header is byte-identical to the upstream copy and is included by exactly
one translation unit (`src/RoL.cpp`), because it defines the SMC buffers.

