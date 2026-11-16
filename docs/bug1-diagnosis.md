# Road Rash 3 recomp — Bug #1 diagnosis (Z80 handshake stall)

*Prepared 2026-10-15 evening (PDT) from a VM reproduction run + disassembly
of both CPUs. All addresses are ROM/Z80-RAM addresses. This supersedes the
README's tentative "title-screen music-start handshake / driver never sets
the byte" framing — the driver was actively clearing the byte when it froze.*

## Reproduction (Candy's VM, native mode, SDL software renderer)

```
cd ~/workspace/rr3-runner-work/rr3/build
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  ./RoadRash3Recomp ../rr3.bin --no-launcher --turbo --max-frames 2000 --hash-frames 120
```

Result: crash report at **frame 1043** (README session saw ~1052 — run-to-run
variance in when the fatal command is sent), reason
`instruction watchdog: 20000000 native insns without yielding`.

Crash-time `[OWN-DIAG]`:

```
cpuPC=$03AB6E  z80pc=$099F  z80_run=1  busreq=0  reset_off=1
z80_ram[0..0x1F]: C3 33 05 1B 25 1E FC 00 00 00 00 FF 01 E8 65 00 00 E8 71 00 00 ...
around z80pc[$099F]: CB 27 CB 27 4F DD 7E 00 77 DD 23 23 0D 20 F6 3E 00
```

## The 68K side — send-command function at $03AB40

Floor-executed only so far (it is one of the 41 floor_coverage leads,
first seen frame 672 via $03A4D2). Disassembly (Capstone, ROM offsets):

- `$03B54A` helper: write `$0100` to `$A11100` (Z80 bus request), poll busack.
- `$03B55E` helper: write `$0000` to `$A11100` (bus release).
- `$03AB40`: save/clear word at `$FFFF0000`; acquire bus; `lea $A0000B, a0`;
  `tst.b (a0)` — **proceed only if Z80 RAM byte $000B == 0**. If nonzero:
  release bus, `move.w #$64, d1` and spin a 100-count delay at
  `$03AB6E` (exactly where OWN-DIAG caught the 68K), then retry.
- Once `$000B == 0`: slot count at Z80 RAM `$000C` (must be < $10), write a
  4-byte command at `$000D + count*4`, increment `$000C`, release bus, RTS.

So `$000B` is the driver's queue-busy flag and `$000C`/`$000D+` are a
4-byte-entry command queue in Z80 RAM. The game code is correct — this same
loop works on hardware, where the Z80 runs concurrently during the delay.

## The Z80 side — queue-pop routine at $0982

Driver image: ROM `$3B568`, 6,951 bytes, loaded at Z80 $0000
(`JP $0533` at $0000, init routine `$096F` called once from `$0548`).
Disassembly (z80dis) of the only writers of `$000B`:

- `$096F` (init): sets `$000B=$FF`, zeroes `$000C..$004B`, then writes
  `$000B=$00` — flag clear at startup.
- `$0982` (pop/ack, called from 7 sites: $066A, $06F9, $07B7, $081E, $0830,
  $084B, $089D):
  - `$0982`: `LD A,$FF` / `LD ($000B),A` — set busy.
  - `$098E`: load `$000C`, `DEC A`, store back — decrement queue count.
  - If count didn't reach 0: shift remaining entries down — copy
    `(count-1)*4` bytes from `$0011` (IX) to `$000D` (HL), loop at
    `$099C..$09A4`.
  - `$09A6`: `LD A,0` / `LD ($000B),A` — clear busy. `RET`.

Crash state matches a Z80 frozen **mid-pop**: `z80pc=$099F` is inside the
shift copy loop; `$000B` still `$FF` (clear at $09A8 not yet reached);
`$000C` already decremented (2→1, shows `$01`); one 4-byte entry
(`E8 65 00 00`) still queued at `$000D`.

## Root cause: scheduler starvation, not a driver bug

Engine source (segagenesisrecomp `runner/video/genesis_machine.c`,
`machine_run_frame`): for each scanline the runner first calls
`glue_run_game_chunk(M68K_PER_LINE)` and only afterwards `step_z80(m,
Z80_PER_LINE)`. `step_z80` also returns immediately while busreq/reset is
asserted. **The Z80 only ever advances when the 68K chunk returns or yields.**

At ~frame 1043 the 68K enters the $03AB40 poll: acquire bus → read $FF →
release → 100-count delay → retry, forever, with no yield point in the loop
(it runs on the Tier-3 floor; the native budget-yield path in
`runner/glue.c` `check_cycle_budget` is driven by bus accesses in
recompiled code). The scanline loop therefore never reaches `step_z80`
again, the Z80 stays frozen mid-pop at $099F, `$000B` stays $FF, and the
instruction watchdog eventually kills the run. A two-CPU concurrency the
hardware provides for free is exactly what the cooperative scheduler
withholds here.

## Suggested next experiments (in order)

1. **Regen with the floor-coverage leads** (already open issue #2 in the
   scaffold README): add `floor_coverage.toml`'s 41 functions — including
   `$03AB40` — to `game.toml` discovery, rebuild, rerun the 2000-frame
   test. Hypothesis: as native code, the poll loop's bus accesses go
   through `check_cycle_budget`, the chunk yields per scanline budget, the
   Z80 gets its slices, the pop completes, `$000B` clears. Cheap to test;
   falsifies/confirms the starvation theory end-to-end.
2. If (1) still stalls: engine-side fix direction — step the Z80 (or yield
   to the scanline scheduler) from within the bus-request/bus-read path
   when the 68K polls `$A11100`/`$A000xx` repeatedly, instead of only at
   scanline boundaries.
3. Not recommended: patching game code or forcing `$000B=0` — the guest
   behavior is hardware-correct; the defect is in run scheduling.

## Evidence / how this was checked

- VM run log (this session): `/tmp/rr3-2000.log` on Candy's VM (ephemeral —
  the crash-time values are transcribed above).
- Scaffold README open issue #1:
  `~/workspace/goals/road-rash-3-native-vita-port/files/rr3-runner-scaffold/README.md`
- 68K disassembly via Capstone (M68K, big-endian) over `rr3.bin` at
  $03AB40–$03AC00 and $03B54A–$03B568; Z80 disassembly via z80dis over the
  driver image at ROM $3B568, offsets $0960–$09C8.
- Engine scheduling: `runner/video/genesis_machine.c` (`machine_run_frame`,
  `step_z80`) and `runner/glue.c` (`glue_run_game_chunk`,
  `check_cycle_budget`) in `~/workspace/rr3-vetting/segagenesisrecomp/`.
