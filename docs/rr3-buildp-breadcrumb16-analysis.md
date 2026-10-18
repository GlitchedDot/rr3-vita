# Build P / breadcrumb-16 — session map (background forensics, 2026-10-17 evening)

Source: `workspace/user/files/breadcrumb-16.txt` (667 lines, sent by Lola 2026-10-18T02:18:58Z with a psp2core dump, "welp, you did something"). The breadcrumb file is CUMULATIVE across builds — do not read it as one run.

## Session map (split on `boot-start`)
- Sessions 9, 10, 13 (headless), 14: all end at exactly tick=5340, insn=15602171, yld=208505, fbh=9F2A8CA6 — the known deterministic demo-race death already documented for builds M/N/O (crashes #13–#17). Session 14 is the most recent of these in file order.
- Sessions 11, 12: shorter/different runs (last ticks 1680 and 5220, different counters).
- **Sessions 15 and 16 — the LAST two boots in the file — contain ONLY:**
  ```
  boot-start code=0
  display-ok code=0
  rom-loaded code=2097152
  machine-boots code=0
  ```
  No `phase1-done` (tick=600), no play lines at all.

## What this does and doesn't establish
- IF sessions 15/16 are the Build P runs (they are the tail of the file Lola sent right after testing P), then P did NOT reach the tick ~5,340 wall and did NOT clear it either — it died or hung after `machine-boots` but before phase 1 completed (tick 600). That is a different, much earlier failure than the interpreter-bisect question P was built to answer (past 5,400 = native convicted; dies ~5,340 = shared machinery convicted). P's bisect verdict would be: neither — the run never got to the test point.
- UNCERTAIN: the session→build mapping is inferred from file order, not labelled in the file. Sessions 15/16 could in principle be aborted launches. Lola's own report of what she saw on screen is the verdict on what P did — logs establish engine state only.
- The accompanying psp2core dump (psp2core-1792289830-0x0004d93677-eboot.bin.psp2dmp) presumably belongs to the newest crash and can confirm which session actually faulted vs. hung.
