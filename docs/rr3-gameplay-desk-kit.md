# Gameplay desk-hunt kit — combat family inventory

Prepared 2026-10-23 (evening PDT) while build DB (#106) awaits Lola's race test.
Purpose: if DB dies the same way DA did — every mask capsule-clean, death a few
dozen ticks after some combat-family function is first touched under fight
state — the desk common-defect hunt becomes the main road. This kit puts the
family's raw facts in one place so the desk session starts at speed instead of
with an hour of reassembly.

Trigger for the desk session: DB's result. If DB sails past ~2850 into long
combat, the hunt splits DOWN from the table instead.

## The combat family (current tree: ~/workspace/rr3-vita, DB config)

Table columns:
- **addr** — guest entry address (THE key; stable across builds)
- **extent** — the masked/table extent from the dispatch table
- **idx** — dispatch-table index in THIS tree (637 entries, addr-sorted).
  CAUTION: the bisect ledger quotes idx 392/515/526/587 for 02014A/023FCC/
  0249AA/028AF4 from earlier tables; the current tree (which also gives the
  ledger's own 374/378/379 for 0203E8/0207EE/020974) says 371/494/505/566.
  Trust addresses, not old idx numbers.
- **part / lines** — generated part file and body line count (measured, this tree)
- **entry** — how the engine reaches it: `direct` = static recomp_call_func/
  mask-run call sites found in generated code; `dispatch/dyn` = no static call
  sites (enters via recomp_dispatch_once or dynamic recomp_call_addr)
- **verdict** — capsule history

| addr | extent | idx | part / lines | entry | verdict |
|---|---|---|---|---|---|
| 017C14 | [017C14,017F6E) | 255 | part14 / 1426 | direct | 160 capsules, all clean (DA) |
| 01B414 | [01B414,01B4EC) | 297 | part17 / 468 | direct | table (first seen CZ) |
| 01B4EC | [01B4EC,01B5CA) | 298 | part17 / 458 | direct | table |
| 01C482 | [01C482,01C4E4) | 315 | part18 / 105 | direct | table |
| 01D21C | [01D21C,01D228) | 327 | part19 / 38 | dispatch/dyn | MASK (CW+); trampoline -> 01D796 |
| 01D386 | [01D386,01D406) | 333 | part19 / 344 | direct | table (first seen CW @1906) |
| 01D406 | [01D406,01D532) | 334 | part19 / 598 | dispatch/dyn | table (first seen CW @1906) |
| 01EAF6 | [01EAF6,01EB36) | 352 | part20 / 147 | direct | table |
| 02014A | [02014A,02019C) | 371 | part22 / 203 | direct | EXONERATED — CW+CX capsules all clean |
| 020974 | [020974,020D2C) | 379 | part22 / 35 | direct | MASK11 (DB); last first-sighting @2823 in DA |
| 020D2C | [020D2C,020F04) | 380 | part22 / 896 | direct | 62 capsules, all clean (DA) |
| 0211A6 | [0211A6,0211E6) | 387 | part22 / 196 | direct | table |
| 02172C | [02172C,021748) | 392 | part23 / 54 | direct | table |
| 02266E | [02266E,0226C0) | 408 | part23 / 255 | direct | table (first seen CZ @1869) |
| 022A4C | [022A4C,022B86) | 415 | part24 / 514 | direct | table |
| 0231AE | [0231AE,0231C0) | 453 | part24 / 74 | direct | table |
| 023564 | [023564,0235FE) | 471 | part24 / 340 | direct | table |
| 02370A | [02370A,02378C) | 476 | part25 / 291 | direct | table |
| 0239B2 | [0239B2,023ACA) | 490 | part25 / 586 | direct | table |
| 023ACA | [023ACA,023B28) | 491 | part25 / 255 | direct | table |
| 023FCC | [023FCC,0241B8) | 494 | part25 / 893 | direct | EXONERATED — 1 capsule, clean (CY) |
| 0241B8 | [0241B8,0241F0) | 495 | part25 / 179 | direct | table (first seen CY @1702) |
| 0241F0 | [0241F0,024302) | 496 | part25 / 464 | direct | table |
| 02491E | [02491E,024950) | 502 | part26 / 93 | direct | table |
| 024982 | [024982,0249AA) | 504 | part26 / 71 | direct | table |
| 0249AA | [0249AA,024A4C) | 505 | part26 / 70 | dispatch/dyn | MASK10 (CZ); never individually tested |
| 024C0C | [024C0C,024C48) | 513 | part26 / 154 | direct | table |
| 025892 | [025892,025896) | 535 | part27 / 19 | dispatch/dyn | table |
| 025896 | [025896,02589A) | 536 | part27 / 13 | dispatch/dyn | table |
| 0259A6 | [0259A6,0259D6) | 543 | part27 / 91 | dispatch/dyn | table |
| 02834A | [02834A,0283AA) | 560 | part27 / 222 | direct | table (first seen CW @1906) |
| 028AF4 | [028AF4,028B76) | 566 | part27 / 335 | direct | MASK9 (CZ); 1 capsule, clean |
| 03AF74 | [03AF74,03AFBE) | 617 | part29 / 170 | direct | table (first seen CZ @1875) |

### Convicted killers (demo phase) — the defect-shape reference set

| addr | idx | part / lines | entry | convicted by |
|---|---|---|---|---|
| 02823A (A) | 559 | part27 / 565 | dispatch/dyn (metronome) | CR-era bisect |
| 024A4C (B) | 506 | part26 / 71 | direct | CR-era bisect |
| 0207EE (C) | 378 | part22 / 948 | dispatch/dyn | CR-era bisect |
| 03A50A (E) | 606 | part28 / 7 | dispatch/dyn | CP/CQ |
| 03A4D2 (F) | 598 | part28 / 7 | dispatch/dyn | CP/CQ |

### Neighbor pattern (recorded, not yet explained)

The suspects keep being the convicted killers' literal neighbors:
- 0249AA ends exactly where B (024A4C) begins
- 02834A starts right after A (02823A) ends
- 020974 starts exactly where C (0207EE) ends

## Also in play but NOT family

- **func_0203E8** (idx 374, [0203E8,020592)) — demo-proven innocent, entered via
  recomp_call_func from part11 ($0142C2). Contains the dynamic per-entity
  handler call at guest $020556: `A6 = [A4]; recomp_call_addr(A6)` — 020558 is
  its return point. Entity workhorse hub.
- **func_01D796** (idx ~348, [01D796,01D8B2), part19 / 591 lines) — EXONERATED
  (14 clean capsules in CX + race sailed through CW's death window).

## Desk session starting questions (not answers)

1. What is the shared defect shape in the generated code of A/B/C/E/F?
   (the five span 7 to 948 generated lines, parts 22/26/27/28 — what's common?)
2. Which family members share that shape?
3. If DB dies with all capsules clean: the killer enters outside call-entry —
   fallthrough from an adjacent function in the same part file, or via glue
   (glue_*.c / rte paths), or via the dynamic recomp_call_addr path.
4. Entry paths to recheck: `dispatch/dyn` rows have NO static call sites —
   map how each is actually reached (dispatch vs dynamic call_addr).

## Mechanical notes

- Extents for the 29 table members are verbatim from
  src/rr3_dispatch_mask.c (s_table_mask). Individual-mask extents from the
  bisect ledger's MASK6-MASK11 entries.
- Body line counts measured from `void func_XXXXXX(void) {` to the next
  column-0 `}` in the current tree's generated/rr3_partNN.c.
- Line counts and idx values are for the DB tree; they shift when generated
  code changes (the BV gate patch shifted the table once already).
