# Qwen-on-mini prompt #1 — RR3 breadcrumb summarizer (single-shot)

Prepared 2026-11-11 (PT) for the mini OpenCode lane, after Lola asked what Qwen
could do in tandem with the RR3 work now that WrightDisc is done. This is the
prompt the assistant offered to write in the "Qwen on Mini with RR3" chat —
Lola's go-ahead to run it is still hers; nothing here has been run yet.

## Lane rules baked in (from standing memory)
- Mini lane is SINGLE-SHOT only; Qwen3 8B, CPU-only, context 40960.
- Prompts must name the working directory up front (the 2026-10-25 wrong-cwd
  stumble: Qwen ran from the wrong directory and got "not a git repository").
- New files only. Qwen never edits RR3 source, bisect configs, capsule sets,
  or the ledger. Candy diffs / re-validates everything it produces.
- Run it BETWEEN test sessions, not during a Vita log grab — Qwen pegs the
  mini's CPU and the mini is also the NAS / FileZilla host for the pipeline.

## Where things live (fill in before pasting — verify on the mini first)
- Working directory to create/use on the mini: `~/rr3-qwen-tools`
- Breadcrumb logs on the NAS (mounted on the mini): the RR3 Logs folder —
  era-named files such as `breadcrumb-en.txt` / `breadcrumb-eq.txt`.
  IMPORTANT: never write to, rename, or overwrite `breadcrumb.txt` in that
  folder — it is the sacred sessions 1–141 archive (58,389,132 B).

## The prompt (paste into OpenCode on the mini, Build mode)

```
cd ~/rr3-qwen-tools first and stay there for the whole task. If that folder
does not exist, create it. Do not work in any other directory.

Write ONE new Python 3 file named breadcrumb_summary.py in ~/rr3-qwen-tools.
Do not modify, move, or delete any existing file anywhere. The script uses
only the Python standard library.

What the script does:
- Takes one argument: the path to an RR3 breadcrumb log file (plain text,
  possibly tens of MB — read it line by line, never load it whole).
- Prints a plain-text summary with exactly these fields, in this order:
  1. file: the file name given
  2. build: the build identifier from the log's own header/boot lines if
     present (search case-insensitively for lines containing "build");
     if none is found, print "build: unknown" — never guess one.
  3. sessions: count of session-start markers you actually find; also
     print the marker text you counted, so a human can check your work.
  4. tried_hit_last: the LAST tried= and hit= values appearing in the file
     (they occur on maskstat-style lines, e.g. "tried=hit 25,832" or
     "tried=hit=200,192"); print both numbers exactly as found.
  5. counts: how many lines contain each of these literal strings:
     maskfail, maskskip, arm, enrel, FAIL. Print one line per string with
     its count (0 is a valid count).
  6. last_pc: the pc= value on the last tail-style line that has one;
     print it exactly as found.
  7. ms_avg: if the file contains per-frame ms (frame time) samples,
     print their average to one decimal; if you cannot identify them
     unambiguously, print "ms_avg: unknown".
- If a field cannot be determined from the file, print "unknown" for it.
  Never invent a value, a marker name, or a build ID.
- After writing the file, run: python3 -m py_compile breadcrumb_summary.py
  and report whether it compiled. Do NOT run the script on any log yet.

When you are done, reply with: the file path, its size in bytes, and the
py_compile result. Nothing else.
```

## Grading fixtures (Candy runs these after Qwen delivers — ground truth)

Run the script against the era-named NAS logs and compare with the verdicts
already established by hand in the RR3 chat:

| Log | Known truth the summary must match |
|---|---|
| EM (#143) | Sailed. Race 1 finished; tried=hit=92,323 at the end; zero maskfail |
| EN (#144) | Full sail. enrel FIRED (tick 25269); tried=hit=200,192; zero maskfail; race 1 ms avg 97.1 |
| EO (#145) | Died mid race 1, final tail pc=0146AE; tried=hit 14,002; zero maskfail; ms avg 91.5 |
| EP (#146) | Died mid race 1, final tail pc=0146AE; tried=hit 3,893; zero maskfail; ms avg 90.6 |
| EQ (#147) | Full-flow sail. Race 1 finished (maskskip at tick 13374, eqarm fired); zero maskfail; race 1 ms ~106 |

If the summary matches all five, the tool is trustworthy for first-pass
digests on future grabs. If any field mismatches, the tool — not the
verdicts — is wrong, and it gets fixed or dropped. Ticks are NOT a reliable
ruler (Lola pauses during runs), so the script deliberately reports counts
and last values, never tick-gap reasoning.

## Not in this prompt (deliberately)
- No desk-review of recompiled functions yet (that was candidate #2).
- No perf bucketing yet (candidate #3) — ER's profile build splits frame
  time into game / machine / rest / present first; bucketing waits until
  those new lines exist so the tool parses the real format.
