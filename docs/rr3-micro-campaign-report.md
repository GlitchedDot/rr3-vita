# Road Rash 3 — Micro-Campaign Report (2026-10-16)

**Result: 597 → 637 functions (+40), gate 79,054 → 79,066 (+12 audited
amendments), 0 rejected seeds, all proofs green. Bundle ready to apply:
`files/rr3-micro-campaign/`.**

## What went in

**1. The +2 gate amendment (proven in advance).** The cmd13/14 sound
handler targets **0x3ADF2 / 0x3AE2E** — the campaign codegen's only two
gate-skipped boundary splits, floor-executed at frame 663 of the
campaign run. The scratch plus2 proof (gate-only change) produced
exactly 599 functions, 0 skips. In this bundle they arrive the same
way: amended into the gate, *not* seeded — they carve via boundary
splitting from the seeded dispatcher slots.

**2. The campaign run's floor batch.** The campaign build's own
10,800-frame floor coverage held 13 roots (true dispatch misses):
- 9 already inside the gate → seeded directly (0x0258AE, 0x0022A0,
  0x020592, 0x0258CC, 0x02823A, 0x0249AA, 0x0258DC, 0x024AF8, 0x002398).
- 2 are sound-dispatcher `bra.w` slots floor-executed for the first
  time (0x3A50A slot 15 → 0x3AE6A; 0x3A4E2 slot 5 → 0x3AADE, both
  targets in-gate) → same audited slot class as the campaign's 8;
  gate-amended and seeded.
- The remaining 2 roots are the +2 pair above.

**3. The 0x012E5A state-table conversion.** The 6-slot inline `bra.w`
table (index at $FF3236 proven closed at 0–5 by writer census).
Capstone re-verified all 6 slots and their targets for this batch.
5 slot instructions + 3 targets (0x012E88, 0x012EF8, 0x012F3E) were
missing from the gate → amended; all 6 slots seeded; the targets
carved via boundary splitting exactly as the audit predicted.

## Rehearsal (VM, dual-patched engine-final, scratch tree rr3-micro)

- Gate loaded: **79,066**; config coverage **61/61 TOML entries
  emitted, 0 rejected** by the gate.
- FunctionFinder 611 → boundary splits +21/+4/+1 → **final 637**.
- Roster-verified in the generated dispatch table: all 25 audited
  addresses (the pair, both slots, 6 state slots, 6 state targets,
  9 floor roots) are compiled functions.
- **600 frames [DONE] both modes**; settled hash 0xBE99E04DC4DE31A9
  at 480/600 in both — unchanged.
- **10,800 frames [DONE] both modes**; **0 unique true-miss
  addresses** in both; raw floor events 6,465 → **2,900** (native).
- Hash streams vs the campaign build: oracle **identical at all 90
  samples**; native identical outside the demo region (29 demo-region
  samples shift phase — the expected signature of floor→native
  promotion in the timing-sensitive demo, the same artifact class the
  cosim referee arbitrated; state invariants all hold).
- Native-vs-oracle sample disagreement: 57 — unchanged in count and
  character (the known presentation artifact; see the cosim and
  band-chase reports).

## Left standing (deliberately)

- **A new floor frontier**: the micro build's 10,800 run exposed the
  next layer of roots (0x0284B8, 0x028468, 0x02841E, 0x020588,
  0x028498, 0x024AD8, 0x00EB84, …) — the batch after this one, same
  audited rhythm, when Lola wants it.
- **One new dynamic site, 0x00245C**: a register-indirect JMP inside a
  newly discovered function (reason: non-PC-indexed variant — no
  static table exists to enumerate). It did not fire as a true miss in
  10,800 frames; the floor covers it if it ever does.
- The 0x012E5A and 0x185D56 sites remain listed as dynamic dispatches
  (their targets are all compiled now; the sites themselves have no
  static-table form in this codegen).

## Road to PC 100% after this

Micro-campaign applied (pending Lola's run) → interactive play test →
the PC-100% call is Lola's → Vita spike un-parks.
