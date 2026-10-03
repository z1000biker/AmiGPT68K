# AmiGPT68K

Native experimental ChatGPT client for classic **AmigaOS 3.x / Motorola 68K**, using `bsdsocket.library` and **AmiSSL 5** for direct HTTPS communication.

The project is designed to run on the Amiga itself: no PC/Raspberry Pi proxy and no API key in the current authenticated path.

## Current status

**v0.7 runtime-fix is the first end-to-end runtime-tested baseline.**

Tested successfully under WinUAE / AmigaOS 3.x with a 68040 configuration:

- AmiSSL 5 initialization
- DNS / TCP / TLS to OpenAI
- device-code authentication
- token/profile persistence
- authenticated ChatGPT request
- streamed response returned to the Amiga Shell

Example:

```text
AmiGPT040 DEVICELOGIN
AmiGPT040 CHAT gpt-6.1-sol "hello who are you?"
```

### Known limitations

- `MODELS` currently receives HTTP 400 and still needs its request format diagnosed/fixed.
- `CHAT` prompts containing spaces must currently be quoted. The present CLI treats a fourth argument as an optional profile path.
- `DEVICELOGIN` is currently the working Codex device-auth path used to validate the native networking/authentication stack. The intended final login architecture is native Sign in with ChatGPT / OAuth PKCE.
- This is an experimental client, not an official OpenAI application.

## Builds

Three 68K targets are maintained from the same source tree:

- `AmiGPT020` — 68020+ soft-float
- `AmiGPT030FPU` — 68030 + FPU
- `AmiGPT040` — 68040+

The runtime-tested configuration so far is `AmiGPT040` under WinUAE.

## Runtime requirements

- AmigaOS / Workbench 3.x
- TCP/IP stack exposing `bsdsocket.library` (WinUAE built-in bsdsocket works for testing)
- AmiSSL 5 with a valid `AmiSSL:` assign and certificate store
- Correct system date/time for TLS certificate validation
- Sufficient stack; v0.7 defines the real libnix `__stack = 131072UL` and moves large buffers to static storage

## Important v0.7 runtime fixes

v0.7 fixes two major 68K/AmiSSL stability problems found during testing:

1. `bsdsocket.library` and AmiSSL are opened once per process instead of being repeatedly opened inside individual HTTPS operations.
2. Large auth/profile/JWT/HTTP buffers are moved out of the small Amiga Shell stack, while the executable defines a real libnix `__stack` value.

A shared `SSL_CTX` is reused during the process lifetime and TLS error reporting now includes certificate verification, `errno`, system clock year and OpenSSL error details.

## Source layout

```text
include/amigpt/     portable core headers
src/common/         portable protocol/auth/JSON/SSE code
src/amiga/          AmigaOS networking, AmiSSL, auth and CLI
src/amiwebauth/     browser/OAuth porting notes
docs/               architecture and porting notes
tests/              portable host tests
tools/              Workbench icon generation tools
```

## Tests

Portable common-core tests:

```sh
make test
```

The actual Amiga executables require an `m68k-amigaos-gcc` toolchain plus AmiSSL headers/libraries.

## Security

- Never collect or submit the user's Google/OpenAI password inside AmiGPT.
- Keep OAuth/token material in the local native client profile.
- Never disable TLS hostname or certificate verification in release builds.
- Treat the current device-auth path as experimental while the final native OAuth flow is completed.

## Project state

This repository starts from the first baseline that completed a real authenticated ChatGPT request from AmigaOS. Further work should preserve that known-good TLS/auth path and change one subsystem at a time.