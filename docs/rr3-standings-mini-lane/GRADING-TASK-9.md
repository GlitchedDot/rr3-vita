# Task 9 grading — Candy Pebbles, 2026-11-13 (against the real rr3-vita source)

## The file (standings-elapsed-store.md, 11,691 B, DeepSeek V4.1 Flash Free, 2m42s)
Verdict: PASS. Candy verified: the quoted operand loads at
rr3_part19.c:7692, 7771-7772 (D[2] = long@0xFF0994), 7780-7781
(D[3] low word = word@0xFF1BDC) and the store at 7848 are exact;
the +104 OR-set at 7547-7552 is exact (bit 7, idempotent — the gate
at 7539 only falls through when bit 7 was already set); func_01E0AC
contains zero recomp_call_func in lines 7678-7873 and func_01E03A
contains zero A[1] assignments in 7430-7677, both as claimed; both
word-pair bases load from the pointer slot 0xFF276A (part14:4672-73,
part15:2984-85), exact.

## Established by this task (now verified ground truth)
- The game's ONLY write32 to record offset +98 (func_01E0AC, guest
  $01E0E6) stores a computed correction: long@0xFF0994 minus the high
  16 bits of word@0xFF1BDC × the quotient ((long@rec+4 −
  (word@0xFF024C<<16)) << 8) / ((long@rec+38) >> 8) — the same
  formula family as func_01A718/func_01ABB2's results math, not a
  race-clock stamp. Nothing in its operands forces a multiple of 60
  or 6.
- On the func_01E03A path the store lands on a record that is
  ALREADY DNF-gated (bit 7 of +104 set before the call; the OR-set
  at 7552 is idempotent). So this store adjusts a DNF record's time
  field; it is not the finisher time stamp.
- func_01E0AC writes nothing else to the record and makes no calls;
  the store is unconditional once called.
- The +98/+100 word pairs (func_0183FC / func_018DC0) are keyed off
  a pointer loaded from guest 0xFF276A — a different structure,
  classified out of the rider-record hunt.

## Candy's follow-up (same day)
The finisher stamp is still unfound in literal-offset forms, but a
new anchor appeared: ALL the +102 and +116 writes via A[1] in the
codebase sit in ONE function — func_015B40 (rr3_part12.c, from line
4596): write16 +102 at 5472, write8 +116 at 4906 and 6051. A
function that stamps a record's rider index and its +102 word is a
record setup/init candidate. Task 10 = full read of func_015B40:
every field it writes, in order, with origins — and specifically
whether +98 is among them in any form.
