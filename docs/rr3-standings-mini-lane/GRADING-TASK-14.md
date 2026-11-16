# GRADING — Task 14 (standings-loose-ends.md): FAIL

Graded by Candy Pebbles against the real source
(~/workspace/rr3-vita/generated/rr3_part20.c and rr3_part12.c),
2026-11-13. Run conditions: LOCAL Qwen3 8B (the free DeepSeek
bucket was exhausted), 2h 9m at 0.9 tok/s, in a session that
carried prior context (titled "func_014BDA swap byte
investigation", ~9.5K tokens preloaded). Deliverable:
standings-loose-ends.md, 1,045 bytes, zero quoted lines.

## Why it fails

1. **Part A item 1 — inverted.** The file says func_01E548 has
   "No direct record field writes detected" and "lacks +104
   flag writing". The prompt itself named the three +104 write8s
   (rr3_part20.c lines 2896, 2982, 3108, immediately before the
   calls at 2904, 2990, 3116) — all three are present in the
   source, verified during task scoping. The function also
   writes A[1]+26/+28 (write16, lines 2506/2515) and A[1]+30/+4
   (write32, lines 2541/2560). None of this is in the file; the
   stored bytes' bit meanings were never traced.
2. **Part A items 2–3 — absent.** No A[1] assignment census, no
   per-call-site branches. Not attempted.
3. **Part B item 4 — wrong for both helpers.** Ground truth:
   func_015AAE makes exactly 2 guest writes — write16 through
   A[0] (advancing), values 0x004B (line 4250) and D[0]
   (line 4286). func_015AC8 writes through A[1] as an advancing
   emitter (write16 line 4519, write16 line 4539, write32
   line 4555) plus one write32 of 0 to (A[0] + 90) at line
   4580. The file reports "no record field writes" for both
   without listing a single write.
4. **Cross-session contamination.** The file claims func_015AC8
   "Manipulates 0xFF253C → 0xFF022C mapping" — that is Task 13
   material (func_0108EC/func_010A1E in rr3_part08.c), not
   anything in func_015AC8. Prior session context leaked into
   the analysis.
5. **Closing summary non-compliant.** Instead of answering the
   two closing questions, it says the helpers "require separate
   analysis" — the task WAS that analysis. The on-screen reply
   likewise ended by proposing next steps rather than
   delivering the verdict.

## What grading itself established (Candy's own verification)

- The Part A pattern question is answered at the existence
  level by the scoping check: func_01E548 DOES follow the
  func_01E03A pattern — a write8 to (A[1]+104) immediately
  precedes each of its three func_01E0AC calls. Still open:
  what bits those three stored bytes set or clear (whether the
  records are already DNF-gated on those paths), and A[1]'s
  provenance per call site.
- Part B: neither helper writes at fixed record offsets.
  func_015AAE emits two words through a pointer in A[0];
  func_015AC8 emits through a pointer in A[1] and zeroes
  (A[0]+90). Whether those pointers hold record bases at
  func_015B40's call sites — which would make the (A[0]+90)
  store a record-field write the Task 10 census missed —
  remains open.

## Verdict and disposition

FAIL — no claim in the file is usable. Task 14 stays OPEN,
to be re-run on DeepSeek V4.1 Flash Free after the free
bucket's reset (quoted 4:59 PM), in a FRESH session. A/B
tally now: DeepSeek 12 passes in 13 tasks; local Qwen3 8B
0 for 2 (Tasks 1 and 14), at ~150x the wall-clock time.
