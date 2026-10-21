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
- **Killer A (the 5,353 death): one of 40 functions** at guest
  addresses 0x27D3E–0x3A4CE (dispatch-table indices 557–596). AW
  acquitted `func_027B84` of being A (substituting it alone changed
  nothing). The bisect resumes there.

> **Correction (2026-10-19, later the same day).** The first version
> of this note said Killer A had been narrowed to 10 functions at
> 0x3B54A and up. That was wrong. Build AX substituted the *lower* 40
> of the top 80 (0x27D3E–0x3A4CE) and sailed, which places A *inside*
> that substituted set — but the inference was recorded backwards in
> the session log, and builds AY–BC then bisected the wrong half
> (indices 597–636), dying at 5,353 on schedule. BC's elimination
> result appeared to name `func_1FA064`; build BD, substituting
> {`func_024A4C`, `func_1FA064`}, died at 5,353 anyway, acquitting
> 1FA064 and exposing the flip. Replaying every verdict mechanically
> (a substituted set that sails contains A; one that dies does not)
> confines A to indices 557–596, consistent with every build from AL
> onward. (For the record: `func_1FA064` is the game's ROM-checksum
> self-test — sums the ROM, paints a red screen and halts on failure —
> an interesting function, just not this killer.) Killer B's
> conviction is unaffected: its chain ran with the top 80 substituted
> throughout, so A was suppressed in every one of those builds. A
> standing rule was added because of this: every future bisect build's
> premise gets replayed against the full verdict history before the
> build is made.
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

## Update — 2026-10-21

### Done

- **Killer A named: `func_02823A`** (guest 0x2823A). Build BH
  (substituting two of the remaining candidates) died at 5,353; BI
  (four) sailed; BJ (only 02823A substituted) sailed. A is a 719-line,
  direct-call-heavy body in the generated code; note that calls made
  directly between generated functions bypass the dispatch mask
  entirely, which matters when the workaround becomes a fix.
- **The "D deaths" were the tool, not the game.** Builds BL–BP died at
  ticks 7,279 / 7,456 / 7,956 / 14,481 depending on how much of the
  table was substituted, while a full-table substitution (BK) ran
  clean past 37,000. Breadcrumb forensics (the on-device state log,
  with its substitution counters) showed the process is deterministic
  per configuration — BL and BM produced byte-identical counter
  series and died on the same tick — and that the lethal dose is on
  the order of several thousand capsule runs in mixed mode. Reading
  the capsule code afterwards: its stack seam is sound by design
  (peek the return, single pop, full CPU checkpoint/rollback). The
  live suspects are cycle-accounting divergence between the two
  backends, and guest-RAM writes from failed capsule runs never being
  rolled back. Either way it is a bisect artifact, not a game bug.
- **The tick gate.** `RR3_MASK3_GATE_TICK` (see `CMakeLists.txt` and
  `src/rr3_dispatch_mask.c`) keeps a substitution range native until
  a chosen tick, engaging it just before the death under study.
  Exposure drops by more than 95%, and late-death bisections are
  trustworthy again (build BQ onwards).
- **Killer E discovered.** Once C could be suppressed past 18,797, a
  further native death appeared at tick 19,985 (build BQ) — visible
  only because every earlier build died at C first.

### In progress

- **Naming Killer C.** With the gate, the chain ran clean: BQ
  (dispatch idx 273–409 substituted) passed 18,797; BR (273–341)
  died on the tick; BS (342–375) died at 18,811, briefly suggesting
  a sixth killer, until BT (376–409) passed both 18,797 and 18,811
  and settled it — the 18,811 death was C's own detonation delayed
  14 ticks by substitution drift. BU (376–392) then sailed past
  18,797, so **C is one of 17 functions, guest 0x20608–0x21748**.
  Halving continues.
- E's tick also shifts with configuration (19,985 in BQ; 20,027 in
  BT and BU) — the same delay phenomenon. E is native outside the
  substituted region in every build so far, and waits until C is
  named.

### Upcoming

- Name C; bisect E; read the convicted functions' generated code
  against the interpreter to find the actual defect (the recomp's
  split-stack bookkeeping is the standing suspect).
- Convert the substitutions into a permanent fix, then a long soak
  well past 20k attract ticks.
- The shipping list: performance pass, audio output, the cores
  0/1/3 split (sim / presenter / audio), the ambient-glow
  presentation, and a Select-button pause menu with save states.
