# RR3 Vita — Phase 2 audio scoping (2026-11-14)

Lola's call after FQ's full sail: **Phase 2 (audio) starts with this scoping pass.**
Verdict up front: **Phase 2 is an output-stage job, not a chip-emulation job.**
The entire Genesis audio pipeline already exists inside the shipped build and
renders real samples every frame — the port just throws them away.

## What already exists (all verified in source, 2026-11-14)

- **Z80 sound CPU: emulated and running.** The Vita build compiles
  `external/superzazu/z80.c` (CMakeLists line 303); the shipped machine
  (`genesis_machine.c`) steps it in lockstep with the 68K. (The
  `genesis_machine_noz80.c` variant is only the old build-R bisect config.)
  The game's Z80 sound driver executes, receives the 68K's mailbox commands,
  and writes the sound chips through the bus.
- **Chip-write capture: live.** `video/genesis_bus.c` pushes every YM2612 and
  PSG write — 68K-origin (PSG at 0xC00011, FM window) and Z80-origin — into the
  audio event queue with cycle stamps (`audio_event_push`, bus lines
  409/418/468/472/502/508). YM timer side-effects are modelled in the bus.
- **Chip emulation: present and compiled into every shipped build.**
  CMakeLists lines 292–302 link `audio/event_queue.c`, `audio/sn76489.c`,
  `audio/mixer.c`, `audio/observability.c`, `audio/audio_shadow.c`,
  `audio/ym2612_ymfm.cpp`, `audio/fm_shadow.cpp` and ymfm itself
  (`ymfm_opn/ssg/adpcm/pcm`). FM = ymfm YM2612 (incl. the ch-6 DAC); PSG =
  the engine's SN76489. The FM "shadow" re-render is an optional verified
  enhancement, **default OFF** — not needed for V1 audio.
- **Per-frame render: already happening, already paid for.**
  `genesis_sim_step` (engine `sim_step.c`) calls `audio_mixer_drain(895780
  master cycles, …)` every step; the mixer walks the event queue and renders
  via `ym2612_render` / `psg_render`. The port's `src/main.c` (lines 56–60)
  passes **real** buffers — `s_fm[4096×2]`, `s_psg[16384]`, with counts
  `s_fm_count` / `s_psg_count` — at every step call, in all phases. So FQ's
  ~29 ms step time **already includes full FM+PSG rendering**. Per wall frame
  the drain yields ≈888 FM stereo frames @ ~53,267 Hz and ≈3,729 PSG mono
  samples @ ~223,721 Hz.
- **Reference mix: defined.** Host `audio.c`'s `audio_flush` mixes at PSG
  rate with nearest-neighbour FM upsampling, divisors **FM ÷1, PSG ÷8**
  (clownmdemu reference), then a bridge downsamples to the device rate.

## What is missing (the whole of Phase 2)

1. **A Vita output backend.** `grep sceAudio` over the port's `src/` returns
   nothing. Nobody consumes `s_fm`/`s_psg` — the samples are rendered and
   discarded. Needed: mix FM+PSG → stereo 48 kHz (the Vita's sceAudioOut
   rate), a ring buffer, an opened audio port, and a feeder — cleanly done
   on a dedicated audio thread, which is where the long-planned core split
   finally pays (the sim thread never touches the audio device).
2. **A rate strategy — the one real design problem.** Each sim step produces
   exactly 1/60 s of audio, but the Vita loop currently iterates every
   ~50–75 ms (banked perf reality), i.e. ~15–20 steps/s against the 60/s
   realtime rate. Fed naively, output starves: 16.7 ms of sound per ~50 ms
   of wall time. Options:
   - **(a) Follow the game:** resample each frame's audio by the measured
     step cadence so playback is continuous and synced to the (slow-motion)
     game. Pitch drops with game speed. Honest, gapless, and matches what
     the player sees; sounds deep/slow until perf improves.
   - **(b) Realtime or silence:** play samples at true rate and let the ring
     underrun — bursts of correct-pitch audio separated by gaps. Worst
     option; listed for completeness.
   - **(c) Defer output until the frame rate improves.** No perf avenue is
     currently open (renderer closed, 50 ms banked), so this parks Phase 2
     indefinitely. Not recommended.
   Recommendation: build Stage A with (a) behind a compile-time switch so
   Lola can A/B (a) vs (b) on hardware with her own ears as the judge.
3. **Lifecycle polish (later stage):** silence on pause/suspend, no stale
   audio across build menus, volume (master volume exists host-side as a
   settings value; Vita gets a fixed sane level first), and behaviour in
   the eventual logging-stripped final build.

## Cost estimate

- Mix + resample to 48 kHz: ~800 output samples per game frame from
  ~888 FM frames + ~3,729 PSG samples — well under 1 ms, and it can live
  on the audio thread's core, off the sim's critical path entirely.
- No change to sim determinism: the drain already runs; the output stage
  only reads the buffers the step already fills. Capsule/mask/witness
  machinery untouched. Tripwire exposure is limited to the usual
  any-change re-roll risk, mitigated the usual way (layout probe).

## Risks / unknowns

- **Is the generated stream actually correct?** The desk can prove samples
  flow (counts + non-silence), never that they sound right. The cheap
  oracle: the PC build already plays this same stream through SDL — if it
  sounds right there, the Vita work is purely delivery. First Vita build
  still gets an ears test from Lola either way.
- **Event-queue depth at slow cadence:** the queue is drained every step
  regardless of wall time, so no build-up; cycle stamps are in master
  cycles, not wall time — sub-frame write timing stays accurate.
- **sceAudioOut grain/latency on the Vita:** standard solved problem
  (48 kHz, stereo, grain 256–512); the ring absorbs the sim's uneven
  production cadence under strategy (a).

## Staged plan

- **Stage A — first audible build (GR, #174, from `build-fs/`):** FQ config
  + Vita output backend only. Mix (FM ÷1, PSG ÷8) + resample to 48 kHz
  stereo, ring buffer, audio thread feeding sceAudioOut, rate strategy (a)
  with (b) behind a switch. Breadcrumb additions: per-step `s_fm_count`/
  `s_psg_count` sanity line once per boot + ring underrun/overrun counters
  in the maskstat line. Witness, gate v2, standings fix all kept. Layout
  re-roll probe as usual insurance.
- **Stage B — tune by ear:** levels vs the reference divisors, strategy
  A/B verdict from Lola, latency/grain tuning, menu/pause behaviour.
- **Stage C — shipping shape:** volume control surface (with the Phase 3
  pause menu), suspend/resume cleanliness, final-build (logging-stripped)
  audio stability proof alongside the graphics one.

First-build success definition: Lola hears the title music and the engine
from the Vita's speakers, continuous (not bursts), at whatever pitch the
game's current speed implies — and the breadcrumb proves the sample counts
never flatline.
