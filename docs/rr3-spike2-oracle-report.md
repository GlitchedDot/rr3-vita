# Spike 2 — Ghidra code-oracle export for Road Rash 3

**Verdict: DONE, and proven end-to-end.** RR3 now has a Ghidra-derived
code-address oracle in segagenesisrecomp's native format. Wired into
`game.toml` as the data-gate, it reproduces the current build *exactly*
(563 functions, set-identical), and a build of the gated codegen passes
the standard check in both modes: **600 frames [DONE], settled hashes
`0xBE99E04DC4DE31A9` at frames 480/600, native and oracle
(`GENESIS_FORCE_INTERP=1`)**. The gate now stands guard over every
future discovery step — the protection whose absence caused RKA's
"0 misses but garbled title screen" over-discovery regression.

Export bundle: `files/rr3-oracle-export/` —
`code_addrs.txt`, `function_starts.txt`, `addr_names.txt`, `pure-code.txt`.
Raw logs/scripts: `hidden_files/spike2/`. Trial tree (VM only):
`~/workspace/rr3-runner-work/rr3-spike2/`.

## Method — what the toolchain actually consumes at f875aa8

The brief's named scripts (`ghidra_code_oracle.py`, `run_ghidra.sh`,
`ghidra_export_oracle.java`, `import_disasm_ghidra.py`) **do not exist
in the toolchain clone at this HEAD** (verified by tree-wide search).
The engine's real oracle mechanism at f875aa8 is:

- `[game] code_addrs_file` in game.toml — a flat list of hex
  instruction-start addresses, loaded by `game_config.c` and consulted
  by `game_config_is_known_code()` in
  `external/m68k-recomp-core/profiles/genesis/function_finder.c`
  (`add_function_impl`) and `code_generator.c` (boundary-split
  promotion). It is a **veto**: any finder target, boundary split — and
  even any explicit root — not present in the set is silently dropped.
  With no file loaded, everything is admitted (RR3's state until now;
  also RKA's, which is why RKA's aggressive-discovery experiment could
  corrupt its title screen).
- For the Sonic games the set is generated from the community
  disassembly's `.lst` by `tests/tools/gen_code_addrs.py` ("every
  offset whose first assembled emit is a mnemonic"). RR3 has no
  disassembly — **the Ghidra analysis is our `.lst`**, and this spike
  is the substitution of one oracle source for another, in the
  identical format (one `%06X` instruction-start per line).
- Function candidates reach the finder as audited
  `[functions].extra`/`late_extra` entries (game.toml or
  `discovery_files`) — the RKA discipline — never as a bulk dump.

The brief's gate construction — *code_addrs ∩ function interiors →
pure-code* — was implemented exactly, computed **inside Ghidra** (a
headless post-script, `RR3OracleExport.java`, run read-only with
`-noanalysis` against the Stage 0 projects) so that "interior" means
true body membership, not a range approximation. (The range
approximation undercounts by 108 addresses — 78,938 vs the exact
79,046 — because some Ghidra bodies hold addresses below their entry
point. The shipped `pure-code.txt` is the exact set.)

## Floor vs ceiling — decision

**The seeded pass (ghidra-proj2 / RR3S) is the oracle source; the
unseeded pass (ghidra-proj / RR3) is corroboration only.** The numbers
leave no room for the alternative:

| check | unseeded (floor) | seeded (ceiling) |
|---|---|---|
| functions | 256 | 6,765 |
| instruction starts | 12,372 | 79,254 |
| of the **current 563** known-good function starts, how many are in the pass's pure-code set | **181 / 563** | **563 / 563** |
| ROM vector targets (14) in pure-code | — | 14 / 14 |
| game.toml extras (0x200, 0x21B6, 0x24FA) in pure-code | — | 3 / 3 |

An unseeded gate would veto 382 of the 563 functions we have *already
proven good on hardware-visible runs* — disqualifying. The seeded
pure-code set covers every known-good address, and the unseeded pass
independently corroborates it: all 256 unseeded function starts are
also seeded function starts, and 10,072 of its 10,250 pure-code
addresses (98.3%) are inside seeded pure-code. The gate's error
asymmetry also favors the ceiling: the gate can only *veto* — a
false-positive address sitting in pure-code harms nothing unless the
finder independently speculates it, while a false-negative veto
silently deletes real code (and even explicit seeds: at this HEAD the
veto applies to roots too).

## Counts

| product | count |
|---|---|
| `code_addrs.txt` — seeded instruction starts | 79,254 |
| `pure-code.txt` — of those, inside a Ghidra function body | 79,046 |
| orphan instruction starts (disassembled, no function) | 208 |
| `function_starts.txt` — seeded functions | 6,765 |
| …already discovered by the current build | 543 |
| …NEW candidates | 6,222 |
| current functions Ghidra sees as interior, not starts | 20 (incl. 0x24FA, our own HBlank seed — Ghidra folds it into a larger body) |
| `addr_names.txt` — primary symbols (nearly all `FUN_*` defaults; RR3 has no human labels yet) | 12,909 |

Seeded-function shape stats (why the candidate pool needs auditing):
median body 16 bytes; 2,318 bodies ≤ 8 bytes; 658 single-instruction
bodies; **451 functions start inside another function's range**
(bulk-seeding them would feed the finder overlapping entries it
assumes cannot exist); one 29,988-byte fall-through mega-body.
NEW starts appear in every 64 KB band of the 2 MB ROM (698 in band 0,
still ~50–90 per band at the top) while Stage 0 bounded true code at
≤ ~300 KB — the high-band tail is necessarily mostly data.

## Spot-checks (capstone, independent decoder)

Ten NEW starts sampled across bands; capstone's linear decode agrees
with Ghidra's body span in 9/10. Content splits cleanly by band:

- Low bands: plausible code (`0x009080`: `move.l -(a0),d0` prologue
  shape, 34-byte body, 3 ROM longword references to the start).
- High bands: data wearing a code costume — `0x105340` and `0x144242`
  decode as `ori.b #$0,d0` (the bytes are `00 00 …` — the classic
  zero-data false decode), `0x088550` is a lone 2-byte `sbcd`,
  `0x1F59A6` desyncs capstone entirely (the 1/10 disagreement).

Conclusion: `function_starts.txt` is a **candidate pool for audited,
band-by-band promotion** — strongest at low addresses — and must
never be bulk-seeded. `pure-code.txt` as a veto gate is the safe
consumption mode, and is what the trial proved.

## Trial regen (VM work tree only)

Config: the current game.toml + one line —
`code_addrs_file = "pure-code.txt"` — against the patched engine
(`engine-final`: pristine + `rr3-z80-fix.patch` + `rr3-stall2-fix.patch`).

- CLI: `[GameConfig] code_addrs_file 'pure-code.txt': 79046 code
  addresses (boundary-split data-gate ON)`; FunctionFinder 559 found,
  **0 rejected by the code gate**; boundary splits +2/+2 → **final
  563**; jump tables 195 targets / 9 unresolved — all identical to
  the ungated baseline.
- Function **set** comparison vs the known-good build: **identical,
  address for address** (563 = 563, no deltas either way).
- Build clean; runs: native `[DONE] 600 frames`, oracle
  `[FORCE_INTERP] [DONE] 600 frames`; settled hashes
  `0xBE99E04DC4DE31A9` at 480/600 in both; **zero dispatch misses**
  recorded by either run.

## The precise next step this unlocks (discovery regen, on Lola's PC)

1. **Adopt the gate (zero-risk, do first):** copy `pure-code.txt`
   beside `game.toml` in `~/rr3`, add `code_addrs_file =
   "pure-code.txt"` to `[game]`, rebuild, re-run the 600-frame pair.
   Expected: identical 563 and hashes — the gate changes nothing
   today; it insures everything after.
2. **Amend the oracle with the runtime-proven table slots.** The
   41 floor-coverage leads (`files/rr3-floor-leads.toml`) split
   33/41 gate-clean — but 8 are vetoed, all in the sound dispatcher's
   bra.w table: `0x3A4CE, 0x3A4D2, 0x3A4DE, 0x3A4E6, 0x3A4EA,
   0x3A4F2, 0x3A502, 0x3A506`. Ghidra decoded the table base as an
   orphan instruction and the remaining slots not at all; the
   interpreter floor *executed* them, and one of them — **0x3A4D2 —
   is the very dispatch miss from Lola's 2026-10-15 run**. Runtime
   evidence outranks the static model here: append these 8 (audited,
   commented) to `pure-code.txt` before enabling the leads file,
   or the gate will silently eat them.
3. **Then the campaign proper**, RKA-style, gate standing guard:
   enable the floor-leads discovery file; work the 9 unresolved
   jump-table sites (`generated/rr3.unresolved_jumptables.log`);
   promote from `function_starts.txt` in audited batches
   (low bands first), re-running the 600-frame pair + longer soaks
   after each batch and watching dispatch misses and the settled
   hashes. The band-wobble / demo-region divergence cosim check
   slots in once discovery stabilizes.

## Caveats

- The gate shrinks the data-as-code class; it does not abolish it —
  a data address that Ghidra mis-attributed to a function body is
  inside pure-code and can still be promoted if the finder
  independently speculates it. Audited seeding discipline (step 3)
  remains the real protection; hashes + Lola's eyes remain the
  verdict.
- `pure-code.txt` is a living per-game artifact (Sonic's is
  regenerated from its maintained disassembly; ours is amended by
  audited additions like step 2). Treat edits to it with the same
  care as game.toml seeds.
- No generated C was edited; the pristine clone is untouched; the
  trial lived entirely in the VM work tree.
