# RR3 band fix — deferred VDP-write visibility (spike report, 2026-10-16)

**Verdict: PROVED OUT on the VM. The band is gone in rendered frames.** Final gate is Lola's eyes in motion on her own machines.

## The defect (Lola's verdict, 2026-10-16)

The horizon band in real gameplay: "still showing up like a very sore thumb." Not demo-only, not cosmetic.

## Mechanism (as established by the band chase + this spike's code read)

- RR3's HBlank handler (0x24FA) is a six-band raster-split engine: it waits on the VDP HV counter for its target line, *then* does its timed VRAM upload + register writes. On hardware the writes land at the wait-target line and affect rendering from that line on.
- The runner delivers interrupts atomically at the *end* of a scanline iteration (machine_run_frame: begin_scanline → game chunk → Z80 step → glue_own_interrupt → render). The handler runs to completion at the delivery line, and its VDP writes took effect immediately — so every split point sat displaced upward by its wait length (~4–5 lines), wobbling as the waits varied. Fix #2's fake raster let the waits *terminate*; it could not move *when the writes became visible*.

## The fix (files/rr3-defer-fix.patch, applies on top of fix #1 + fix #2)

- glue.c: while the level-4 (H-int) atomic branch services a handler (`s_hint_service`, set only there), writes to the VDP ports ($C00000–$C00007, data + control; 8/16/32-bit) are queued FIFO with a stamp = `vdp.scanline` at write time — fix #2's phase machine has advanced it to exactly the line the handler waited for. Queue depth cap 8192 with immediate-write fallback on overflow (never observed; a frame's six services queue far less). Reads are never deferred. PSG ($C00011) and all other addresses are untouched; V-int service is untouched.
- genesis_machine.c: `glue_deferred_vdp_flush(line)` runs just before each line renders (applies every queued write stamped ≤ that line, replayed through gbus_write16/8 so control-port command assembly, address auto-increment and DMA triggers execute at apply time, in order), and `glue_deferred_vdp_flush_all()` drains the queue at frame end — end-of-frame VDP state is unchanged by construction; only intra-frame visibility moves.

## Evidence (VM, scratch tree engine-defer = engine-final + this patch; micro-campaign game, 637 functions)

- **600 frames, both modes: [DONE]**, settled hash 0xBE99E04DC4DE31A9 at 480/600, 0 misses. Oracle 240/360 = the documented pre-existing title variance, unchanged.
- **10,800 frames, both modes: [DONE]**, 0 unique true-miss addresses; native raw floor events 2,900 — identical to the micro baseline. Boot/title hash region identical to baseline.
- **The decisive visual test** (deterministic oracle runs, input-script screenshots at frames 6000 and 7800, micro build vs defer build): the two frames are **pixel-identical everywhere except the split region** (frame 6000: 16 rows differ; frame 7800: 12 rows; full width). In the micro frames the artifact is unmistakable — a displaced water strip + dark seam at 6000, and at 7800 a solid green BAR across the upper screen with the landscape sliced into wrong strata. In the defer frames the same scenes render as one continuous landscape: sky, tree line/mountains, grass and road unbroken. Stills in `hidden_files/deferfix/`.
- **About the old metric**: native-vs-oracle disagreement stayed 57/90 — expected in hindsight. Both backends share the same atomic delivery and fake-raster machinery, so both displaced the writes *identically*; the metric only ever measured backend-vs-backend, and the deferral moved both to the hardware placement in lockstep (defer-vs-micro streams differ at 30/90 native and 31/90 oracle samples, all in the armed demo region 4800+). The hardware-relative evidence is the rendered frames themselves.
- **Cosim caveat, stated plainly**: the pairing-#1 cosim gate could not be re-established in today's VM session — the *unmodified baseline engine* itself diverges at cp212 (z80ram, a sound-path checkpoint race) under the exact comparable-subs protocol that read zero divergence in the 2026-10-16 morning session, so the harness could not arbitrate either way. The October cosim verdict (codegen bit-faithful) is untouched — this change is presentation timing only, drains fully every frame, and both backends' deterministic 10,800-frame trajectories stayed locked to their baselines (same disagreement set, same floor signature, pixel-identical scenes outside the seam rows).

## Applying it (her trees already carry fix #1 + fix #2; the patch stacks on top)

Gaming PC, **Ubuntu terminal**:
```bash
cd ~/segagenesisrecomp
git apply rr3-defer-fix.patch   # place the file here first
cd ~/rr3
cmake --build build -j
cd build
./RoadRash3Recomp ../rr3.bin --no-launcher
```

Mac, **Mac Terminal**: same two directories (~/segagenesisrecomp, ~/rr3), then her working recipe:
```bash
SDL_RENDER_DRIVER=software ./RoadRash3Recomp ../rr3.bin --no-launcher
```

Rollback (either machine, engine dir): `git apply -R rr3-defer-fix.patch` and rebuild — restores the exact fix #1 + fix #2 state.

Patch stack-verified: applied to a pristine copy of the dual-patched tree, result is byte-identical to the tree that produced all evidence above.
