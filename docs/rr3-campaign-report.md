# Discovery campaign — prep report (Road Rash 3)

**Verdict: the campaign is packed, dress-rehearsed, and ready to drop
in.** The gate amendment passed audit 8/8, the campaign config was run
end-to-end in the VM against the dual-patched engine, and every proof
bar held: **597 functions (563 + 34, zero lost), 0 gate rejections of
any seed, 600 and 10,800 frames [DONE] in both modes, settled hash
`0xBE99E04DC4DE31A9` intact, and the pre-campaign build's only
true-miss address — `$03A4D2`, the one Lola watched happen — is gone:
unique true-miss addresses 1 → 0, raw floor events 21,122 → 6,465
(−69%).** The drop-in bundle is `files/rr3-campaign/` (game.toml,
pure-code.txt, rr3-floor-leads.toml, README.md with Lola's steps).
Raw evidence: `hidden_files/campaign/`. Rehearsal tree:
`~/workspace/rr3-runner-work/rr3-campaign/`. The pristine toolchain
clone was not touched; no generated C was hand-edited; Lola's PC was
not touched.

## 1. Gate audit + amendment — the 8 dispatcher slots

**Mechanism (new precision over Stage 0).** The sound dispatcher at
ROM 0x3A4A8 does not branch through its table — it *calls the slot
itself*:

```
cmpi.w #$13, d0        ; bcc -> error return for D0 >= 0x13
lea.l  $3A4CE(pc), a6   ; table base
lsl.l  #$2, d0          ; index * 4
adda.l d0, a6
jsr    (a6)             ; land ON the slot; the slot's bra.w forwards
                        ; to the handler, whose rts returns here
```

So the valid command range is **0x00–0x12 — 19 commands**, not
Stage 0's "0–0x13, 20" shorthand (0x13 is the first *invalid* index),
and the table at 0x3A4CE holds exactly **19 consecutive `bra.w` slots**
(0x3A4CE–0x3A51A). Each slot is executed code by construction, which
is why the interpreter floor recorded slot addresses as entry points.
Ghidra's seeded pass decoded only slot 0 (as an orphan start outside
any body) and none of the other 18 — a table-shape blind spot — so
the gate vetoed the 8 slots that appear among the 41 floor leads.

**Audit table** (ROM bytes + capstone decode + floor runtime proof;
full log `hidden_files/campaign/audit-disasm.txt`, amendment log
`hidden_files/campaign/amendment-log.md`):

| slot | cmd | bytes | target | target evidence |
|---|---|---|---|---|
| 0x3A4CE | 0 | 60 00 00 6C | 0x3A53C | Ghidra start, in gate; upload/init — ROM decode shows the $A11100/$A11200 Z80 writes Stage 0 described |
| 0x3A4D2 | 1 | 60 00 01 96 | 0x3A66A | Ghidra start, in gate; per-frame tick (register save, frame counter++). **The dispatch miss Lola observed live at frame 4986** |
| 0x3A4DE | 4 | 60 00 04 FE | 0x3A9DE | Ghidra start, in gate |
| 0x3A4E6 | 6 | 60 00 0D 5E | 0x3B246 | Ghidra start, in gate |
| 0x3A4EA | 7 | 60 00 0D B6 | 0x3B2A2 | Ghidra start, in gate |
| 0x3A4F2 | 9 | 60 00 0F 18 | 0x3B40C | Ghidra start, in gate |
| 0x3A502 | 13 | 60 00 08 EE | 0x3ADF2 | target NOT in gate — finding below |
| 0x3A506 | 14 | 60 00 09 26 | 0x3AE2E | target NOT in gate — finding below |

All 8 pass: every one is a genuine `bra.w`, every target lands inside
the 0x3A4A8–0x3B600 sound span, and every slot is runtime-proven (it
sits in the floor-leads file *because the floor executed it*).
**Amended gate: 79,046 + 8 = 79,054 lines** (verified), shipped as
`files/rr3-campaign/pure-code.txt`.

**Finding — the +2 that was NOT shipped.** The handler targets of
slots 13/14 (0x3ADF2, 0x3AE2E) are absent from the gate (Ghidra never
emitted them as instruction starts), though capstone decodes both as
unambiguous sound code (alive-magic word handling, Z80 bus-status
poll). The rehearsal proved the consequence precisely: codegen
reported *"Data-gate: skipped 2 boundary-split entries"* — identified
by elimination as these two, then **proven by experiment**: a scratch
gate with +2 (79,056) yields 0 skips and exactly 599 functions, the
delta being exactly {0x3ADF2, 0x3AE2E}, nothing removed (experiment
preserved in `hidden_files/campaign/plus2-test/`). The shipped bundle
stays at the prescribed +8; the +2 amendment is pre-audited and
pre-proven whenever Lola wants it — with it, the last two sound
handlers compile and the floor's sound work shrinks further (the
rehearsal's floor coverage shows commands 13/14 firing as early as
frame 663).

## 2. Campaign config

`files/rr3-campaign/game.toml` = Lola's current config, every field
preserved (output_prefix, jump_table_autodiscovery, full ram_layout,
the 3 [functions] extras), plus two lines in `[game]`:

```toml
code_addrs_file = "pure-code.txt"              # the amended gate
discovery_files = ["rr3-floor-leads.toml"]     # the 41 floor leads
```

Codegen confirms the merge: `[GameConfig] code_addrs_file
'pure-code.txt': 79054 code addresses (boundary-split data-gate ON)`;
`merged 'rr3-floor-leads.toml': +41 extra_funcs`; config total 44
extra funcs. All 3 original extras verified inside the gate (at this
HEAD the gate vetoes even explicit roots — a vetoed seed would have
silently vanished; none did).

## 3. Dress rehearsal (VM work tree, dual-patched engine)

**Codegen.** FunctionFinder 586 found (baseline 559), **0 rejected by
blacklist/code gate**, 44/44 TOML entries emitted; boundary splits
+9/+2 → **final 597 functions**. Set arithmetic vs the known-good
563: **+34, −0**. The 34 = 27 floor leads not previously discovered
+ 6 handler promotions reached by walking the seeded slots
(0x3A53C cmd0, 0x3A66A cmd1, 0x3A9DE cmd4, 0x3B246 cmd6, 0x3B2A2
cmd7, 0x3B40C cmd9) + 1 walk discovery (0x000B6E). All **41/41 leads
are now compiled functions** (14 were already discovered; they emit
as no-ops). The generated slot functions are one-line tail-calls —
`func_03A4D2` → `recomp_tail_call(0x03A66A)` — so the per-frame tick
now runs dispatcher → slot → handler entirely in compiled code.
**Jump tables: 195 targets / 9 unresolved — unchanged** (the leads
exercise call edges, not table edges; see §4 for the 9).

**Proofs.**
- 600 frames: native [DONE], oracle [DONE]; settled hashes
  `0xBE99E04DC4DE31A9` at 480/600 in **both**; 0 misses either mode.
- 10,800 frames: native [DONE], oracle [DONE] — no stall regression
  (both engine fixes fully exercised on the way through).

**Miss accounting (native 10,800, vs pre-campaign dual-patched
build):**

| metric | before | after |
|---|---|---|
| unique true-miss addresses | 1 ($03A4D2) | **0** |
| dispatch_misses.toml entries | 1 | **0** |
| raw floor miss events | 21,122 (≈2/frame) | **6,465** (−69%) |

What still floors (this run's `floor_coverage.toml`, snapshotted to
evidence): 57 subtree entries under **12 independent entries** — the
2 vetoed handlers (frame 663), 2 more dispatcher slots the attract
demo exercises (cmd 5 @ 0x3A4E2, cmd 15 @ 0x3A50A — not in the leads
because spike 1's shorter run never fired them), and a demo-region
family (subtrees rooted at 0x0258AE, 0x020592, 0x02823A, 0x002398,
0x0022A0, 0x0258CC, 0x0258DC, 0x0249AA, 0x024AF8) reached only past
frame ~4,700. That file is the next batch's raw leads, in embryo.

**Hash streams (90 samples each, `--hash-frames 120`).**
- Oracle, campaign vs pre-campaign: **90/90 identical** — the
  interpreter ground truth is untouched by discovery, as it must be.
- Native, campaign vs pre-campaign: **41/41 identical through frame
  4920; 0/49 identical after** — compiling the sound chain shifted
  native's phase through the demo region (a constant phase offset,
  not divergence: the set of frames where native disagrees with the
  oracle is *exactly* the same 57-frame set before and after the
  campaign, and 33/90 samples still agree across all four streams).
  The stall-2 report's demo-region flag is therefore **unchanged in
  character — neither cured nor worsened**; the cosim check remains
  its referee, and this phase move joins the residual risks below.

## 4. The 9 unresolved jump-table sites — audit

The log's 9 lines are **2 unique sites** (0x012E5A ×6, 0x185D56 ×3 —
repeat lines from finder re-scans), both reason 2 (non-PC-indexed JMP
variant). Full worksheet: `hidden_files/campaign/jumptable-audit.txt`.

**Site A — 0x012E5A `jmp (a0, d0.w)`, inside func_012D7C.**
A0 is loaded with 0x012E5E — *the next instruction*: another inline
`bra.w` table, 6 slots (0x012E5E–0x012E76), index = RAM word
$FF3236 × 4. **The bound is proven closed by a writer census:** every
write to $FF3236 in the ROM is an immediate 0–5 (reset at 0x011DF2,
and each state handler writes the next state: targets write 1, 2, 3,
4, 5 in sequence) — this is a sequential state machine, and its full
target set is {0x012E76, 0x012E88, 0x012ED8, 0x012EF8, 0x012F3E,
0x012FD2}, all interiors of the containing function (the recomp emits
the JMP as a runtime `recomp_tail_call` to computed interior
labels).
*Classification: **needs manual table extraction — now extracted and
bound-proven.** Conversion is a `[[jump_table]]` directive (the log's
own prescription) or seeding the targets; note 3 of 6 targets
(0x012E88, 0x012EF8, 0x012F3E) are not in the gate set and would need
audited amendments of the same evidence class as the sound slots.*

**Site B — 0x185D56 `jmp (a1)`, inside func_185BCE.**
A1 is loaded from a longword pointer table at **0x186340**, indexed
by (RAM word $FF7536 >> 1). The table is closed: exactly **5 entries**
(0x185E3E, 0x186192, 0x185DEA, 0x185D58, 0x000AA8), the 6th longword
(0xDF080800) breaks the pointer pattern — and **all 5 targets are
already discovered, gate-clean functions**. The index variable,
however, is a busy global (74 ROM references, several register
writes), so its range is *not* statically provable from here.
*Classification: **genuinely dynamic index over a closed,
fully-covered table** — runtime dispatch always lands on compiled
code; a `[[jump_table]]` directive listing the 5 targets would make
it static, but nothing is at risk while it stays dynamic.*

Neither site can floor-fallback to undiscovered code in the attract
demo's exercised paths (Site B by coverage; Site A because its
targets are interior labels of its own compiled function) — which
matches the rehearsal's 0 true-miss result.

## 5. Residual risks & next steps

- **Demo-region native phase** moved when the sound chain compiled
  (§3). Settled screens are bit-identical and oracle agreement is
  structurally unchanged, but until the cosim check referees the
  demo region, treat native demo timing as *plausible, unproven*.
  This is the same open flag stall 2 left, not a new one.
- **The +2 amendment** (0x3ADF2, 0x3AE2E) is audited and pre-proven
  (599 functions, 0 skips) but deliberately not in the shipped
  bundle. One-line change to the gate file whenever approved.
- **Next floor batch:** the rehearsal's floor_coverage (57 entries /
  12 independent) wants the same audit treatment — it includes two
  more dispatcher slots (cmd 5, cmd 15; identical evidence class,
  same amendment path) and the demo-region family past frame 4,700.
- **Site A's `[[jump_table]]`** (and optionally Site B's) converts
  the last two dynamic dispatches to static enumeration; Site A
  needs 3 small gate amendments first (same audit class as §1).
- None of this touches the band-wobble question; that still slots
  into the cosim check once discovery stabilizes — which, after this
  campaign lands on Lola's PC, it has.
