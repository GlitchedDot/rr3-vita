# Road Rash 3 — Interactive Play Test Checklist

*Prepared 2026-10-16, ahead of the micro-campaign apply. Recap: campaign
build (597) is live on Lola's PC, cosim referee proved recomp ≡ interpreter,
band chase closed with fix #2 v1 kept (band = known cosmetic artifact), and
the micro-campaign (637 functions) is packaged. After the micro build's
600-frame pair verifies, this play test is the only item left before
Lola's PC-100% call. Nothing here is declared playable from logs — every
verdict line is Lola's own report of what she sees and hears.*

## Preconditions

- Micro-campaign applied on the PC: build signs in at **637 functions**,
  79,066 code addresses, 0 gate rejections.
- The 600-frame pair passes in both modes: `[DONE] 600 frames completed`,
  settled hash `0xBE99E04DC4DE31A9` at frames 480 and 600.

## Controls (engine runner defaults — verified in source)

From `runner/input_map.c` (`input_map_init_defaults`) in the
segagenesisrecomp engine: with `--no-launcher`, Player 1 defaults to
**keyboard AND gamepad, additive** — a connected pad works plug-and-play,
no settings change needed. (A launcher/settings.ini choice would override
the defaults; the `--no-launcher` runs don't use one.)

| Genesis | Keyboard | Gamepad (SDL) |
|---|---|---|
| Up / Down / Left / Right | Arrow keys | D-pad |
| A | Z | Face-right button (B on Xbox layout) |
| B | X | Face-bottom button (A on Xbox / Cross) |
| C | C | Face-top button (X on Xbox / Square) |
| Start | Return (Enter) | Start |
| X / Y / Z / Mode (6-button ext.) | A / S / D / Backspace | Y / LB / RB / Back |

Which Genesis button is throttle / brake / attack in RR3 itself is part of
what the test discovers — try A/B/C (keys Z/X/C) and report which does what.

## Launch (Ubuntu terminal)

Real-time, windowed — the scaffold README's rule: drop `--turbo` and
`--max-frames` and it runs in real time and opens a real window (WSLg):

```bash
cd ~/rr3/build
./RoadRash3Recomp ../rr3.bin --no-launcher
```

## The test — one step at a time, report what you see

1. **Window & title:** Does a window open? Do you reach the title /
   attract screen? (Screenshot welcome.)
2. **Start:** Press Start (Enter, or pad Start). Do the menus respond?
   Can you navigate with Up/Down?
3. **Into a race:** Start a race. Are you on the road with other riders,
   dashboard live — like the attract demo you confirmed, but under your
   control this time?
4. **Steering:** Left/Right arrows (or d-pad) — does the bike steer?
5. **Throttle / brake / attack:** Try Z / X / C (Genesis A / B / C) one at
   a time. Which one accelerates, which brakes, which punches/kicks?
6. **Sound:** What do you hear — title music, engine, nothing? (All prior
   verification runs were silent turbo runs; this is the first ears-on
   check, so whatever you report is new information.)
7. **The band, in a real race:** The band chase concluded the wobble is
   cosmetic in the attract demo and that *the play test arbitrates
   severity*. In an actual race: is the band there? Does it move? Does it
   bother you while playing, or fade into the background?
8. **Endurance:** Play a few minutes. Any freeze, stall, or slowdown?
   (Both known stalls were fixed and soak-proven in turbo runs; a hang
   here would be a new, third stall — note roughly when/how far in.)

## After quitting (Ubuntu terminal)

The run leaves evidence next to the executable — send / report these so
the log side can be paired with your verdict (logs alone settle nothing
about what was on screen):

```bash
cd ~/rr3/build
cat dispatch_misses.toml
cat floor_coverage.toml
```

New floor roots in `floor_coverage.toml` feed the next batch (the known
frontier starts at 0x0284B8); new entries in `dispatch_misses.toml` would
be the first true misses since the campaign.

## What the outcome decides

- Play test passes in Lola's judgment → the PC-100% call is hers → the
  Vita spike un-parks (pixel-perfect view + ambient-glow presentation
  idea is already logged for that stage).
- Anything fails → it becomes the next bug on the PC list, ahead of any
  Vita work, per her standing order: PC build to 100% first.
