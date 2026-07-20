# macos/ — Operator laptop control app

Full project context (hardware, BLE frame format, milestones, security model) lives in
[`../AGENTS.md`](../AGENTS.md) — read that first. This file only holds what's specific to
working in this folder.

## What lives here

The macOS-side half of KeyBug's Milestone 0 and 1: a `uv`-managed Python project that will
become the BLE central + global input listener described in Feature 1 of the root spec.
Later milestones (4-5) may move this to Swift — see "Open Questions" in the root doc.

## Current state

- `main.py` is still the unmodified `uv init` placeholder (just prints "Hello from macos!").
- `pyproject.toml` declares `bleak` (BLE central) and `pynput` (input capture) as deps, but
  neither is used yet.
- Milestone 0 target: a script that scans for the KeyBug board by its NUS service UUID,
  connects, and sends a few hardcoded test frames — no real input capture yet, just a BLE
  send/receive harness. See root `AGENTS.md` → Build Order → Milestone 0.

## Collaboration reminder

Same as the root doc: hint/snippet/debug on request, don't autonomously implement whole
milestones.
