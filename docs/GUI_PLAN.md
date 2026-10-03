# AmiGPT68K native GUI plan

The v0.8 protocol/authentication core remains shared between the CLI and GUI front ends. The GUI should not duplicate TLS, OAuth, profile, model or Responses API code.

## Toolkit

Use **Intuition + GadTools** as the baseline GUI layer. They are native to classic AmigaOS 3.x and avoid making MUI or ReAction a hard runtime dependency. MUI/ReAction front ends can be considered later as optional variants.

## First GUI milestone

A single resizable Workbench window with:

- profile/authentication status
- model selector populated by the existing MODELS path
- scrollable conversation view
- one-line input gadget
- Send, Clear, Refresh Models and Profile buttons
- status line for Connecting / Streaming / Done / Error
- streamed assistant text appended as SSE deltas arrive

The GUI must use the same `amigpt_profile`, refresh, `/v1/models`, `/v1/responses`, TLS and SSE modules as the CLI.

## Build targets

Keep the existing CLI binaries and add matching GUI targets:

- `AmiGPTGUI020`
- `AmiGPTGUI030FPU`
- `AmiGPTGUI040`

The CLI remains useful for diagnostics and low-memory systems.

## Sequence

1. Cross-build and runtime-test the v0.8 CLI public-endpoint migration.
2. Add the GadTools front end without changing the transport/auth core.
3. Test streaming, window resizing and low-memory behavior under WinUAE.
4. Test at least one GUI build on real 68K hardware.

This keeps the known-good networking path isolated while the user interface evolves.