# Crash hunt update — 2026-10-19

Two days of bisection (builds AA–AZ) resolved one of the Vita port's
two deaths outright and cornered the other. This note is the concise
record; the per-build breadcrumbs and core dumps are kept with the
project owner's local archive.

## The interpreter death — SOLVED

Every build that forced the engine's reference interpreter died inside
the first emulated tick on the Vita, while the identical build ran
10,800 frames cleanly on PC. A ladder of single-variable builds
(stage markers inside the frame driver, then inside the first CPU
chunk, then inside the game fiber's entry sequence) collapsed the
window to the fiber's first statements, and then to a stranger fact:
**the fiber dies on its second file write.**

Experiments, in order:

1. The engine's interpreter branch prints
   `[FORCE_INTERP] driving from reset PC …` with `fprintf` *on the game
   fiber* before entering the interpreter loop. That print had been in
   every interpreter build since the backend was first tested.
2. Replacing stdio with raw `sceIoWrite` on a pre-opened descriptor did
   not save it; nor did removing the `snprintf` formatting, nor issuing
   the write as a bare string literal. The fiber survives exactly one
   file-IO operation of any kind; the second terminates the process
   (C2-12828-1). Even `fopen` as the *first* fiber-side call killed it
   (a stdio open is many operations internally).
3. All scheduler-side IO (the main/worker threads) is unaffected —
   thousands of breadcrumb writes per run complete fine.

Conclusion: on this platform, stdio/file IO executed on the minicoro
game fiber is fatal. The PC never showed it because glibc fibers
tolerate it. A candid footnote: five diagnostic builds (AF–AI)
re-introduced the fault through the investigator's own fiber-side
logging, each moving the death exactly one log line later — the ladder
that finally proved the rule.

**Fix:** the game fiber performs zero stdio/file IO. The two fiber-side
`fprintf`s in `engine/runner/glue.c` (interpreter entry, interpreter
halt path) are deleted, with a comment recording the rule. Build AJ
then ran the interpreter on hardware past tick 10,020 (stopped by
hand, still running), with phase-1 counters **identical to the PC
interpreter to the unit** (8,124,470 instructions / 138,274 yields
over 600 frames). The port now has a bit-exact oracle on the device.

## The native death — function-level bisect

The recompiled (native) build still dies in the attract-mode demo race.
Because the fix above doesn't touch the native path, the two deaths
are formally separate. A native re-baseline (AK) reproduced the classic
death: last checkpoint tick 5,340 with frame hash / instruction /
yield counters identical to the PC verification runs, death inside the
next 60 ticks (5,353 on the on-screen counter).

Method: a patched copy of the generated dispatcher
(`src/rr3_dispatch_mask.c`) substitutes the interpreter's framed
capsule for guest functions inside a configurable address window
(checkpoint/rollback guarded: a capsule run whose exit doesn't match
the caller's expected return falls back to the native body). Outcome
per build is read from the on-screen tick counter at death: ~5,353,
~5,579, or "sails past."

Results (637 guest functions; windows are guest addresses):

| Build | Substituted | Outcome |
|---|---|---|
| AK | — (baseline) | dies 5,353 |
| AL | ≥ 0x1C686 (319) | sails |
| AM | ≥ 0x2378C (160) | sails |
| AN | ≥ 0x27D3E (top 80) | dies **5,579** |
| AO | 0x2378C–0x24F86 (band low 40) | dies 5,353 |
| AP | AO window + top 80 | sails |
| AQ–AV | narrowing within the band | B convicted (below) |
| AW | only {0x24A4C, 0x27B84} | dies 5,353 |
| AX | 0x24A4C + top-80 low 40 | sails |
| AY | 0x24A4C + 0x3A4CE–0x3AF74 | dies 5,354 |
| AZ | 0x24A4C + 0x3AF74–0x3B54A | dies 5,353 |

Two distinct killers fell out of the pattern:

- **Killer B (the 5,579 death): guest `func_024A4C`.** AQ–AV halved the
  band 0x2378C–0x27D3E down through 20, 10, 5, 3, and 2 candidates to
  this single function. Its body is small (172 bytes of 68K): it calls
  `func_027B84` — notably via a *direct* C call in the generated code,
  bypassing dispatch — then indexes a table with the result. Masking
  0x24A4C routes the whole subtree through the interpreter, which is
  why it convicts the subtree rather than one instruction.
- **Killer A (the 5,353 death): one of 10 functions** at guest
  addresses 0x3B54A–0x1FA064 (dispatch-table indices 627–636). AW
  acquitted `func_027B84` of being A (substituting it alone changed
  nothing); AX–AZ then halved the top 80 to these ten. The bisect
  resumes there.
- **Killer C (unlocalized):** a long run of AX crashed at tick 18,797,
  in a late attract segment (breadcrumbs show the demo had stopped
  dispatching into any substituted function thousands of ticks
  earlier). No "sails" verdict in this campaign certifies more than
  ~9,000 ticks; C needs a 20k+ tick soak once A and B are both
  substituted in a single build.

Side observations worth keeping:

- Dispatch entries into the band 0x2378C–0x27D3E are **zero for the
  first ~3,600 demo frames** and begin only as the demo pack closes
  up — the "exposure" pattern noted in earlier hunts, now measured.
- Frame-hash comparison between the native and interpreter backends is
  only valid for the locked title segment (~first 1,920 ticks); the
  attract sequences phase-drift after that. The substitution bisect
  doesn't depend on hashes at all.
- Core-dump memory forensics remain void on this firmware (the app's
  segments are recorded erased), and exception-PC records in dumps
  were previously proven unreliable; neither was used here.
