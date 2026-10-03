# AmiGPT68K

Native experimental ChatGPT client for classic **AmigaOS 3.x / Motorola 68K** using `bsdsocket.library`, **AmiSSL 5**, and OpenAI's official open-source **Sign in with ChatGPT (SIWC)** flow.

The Amiga itself performs model discovery and inference. No proxy sits in the chat data path and no API key is required.

> **Status:** v0.7 is the first runtime-proven PoC. The `main` branch is now **v0.8 development**, migrating from the legacy Codex diagnostic path to the public SIWC endpoints.

## Proof of concept

The v0.7 `AmiGPT040` build completed an authenticated ChatGPT request and streamed the answer directly in an AmigaOS Shell under WinUAE. That release remains preserved as the historical first working PoC.

## v0.8 architecture

The normal v0.8 path is:

```text
Sign in with ChatGPT (OAuth 2.0 Authorization Code + PKCE)
        -> issued oaiapp_... client ID
        -> OAuth access/refresh tokens for https://api.openai.com/v1
        -> GET  https://api.openai.com/v1/models
        -> POST https://api.openai.com/v1/responses
        -> streamed response in the Amiga Shell
```

`originator: codex_cli_rs`, `ChatGPT-Account-ID`, and ChatGPT private `backend-api` endpoints are **not used** by the v0.8 MODELS/CHAT path.

Because classic AmigaOS currently lacks a browser capable of completing the OpenAI sign-in page, v0.8 includes a temporary PC login helper. It performs only the browser/OAuth step; the resulting protected SIWC profile is then copied to the Amiga, which owns refresh, model discovery, and inference. See [`docs/SIWC_LOGIN.md`](docs/SIWC_LOGIN.md).

## Quick start

On the Amiga:

```text
stack 131072
AmiGPT040 HOSTID
```

Copy `PROGDIR:AmiGPT.profile` to a trusted PC, then from this repository run:

```sh
python tools/amigpt_login.py --profile AmiGPT.profile
```

Copy the updated profile back to the Amiga and test:

```text
AmiGPT040 REFRESH
AmiGPT040 MODELS
AmiGPT040 CHAT gpt-6.1-sol hello who are you?
```

The PC helper requires Python 3 plus the `cryptography` package so it can verify the OpenAI ID-token signature.

## Legacy DEVICELOGIN diagnostic

`DEVICELOGIN` is retained only to reproduce/test the older Codex device-auth networking path. If you use it, ChatGPT's **device code sign-in** setting must be enabled for the account/workspace. A legacy DEVICELOGIN profile is deliberately rejected by v0.8 `MODELS` and `CHAT`.

It is not the intended authentication architecture of AmiGPT68K.

## Builds

- `AmiGPT020` - 68020+ soft-float
- `AmiGPT030FPU` - 68030 + FPU
- `AmiGPT040` - 68040+

The v0.7 68040 build is runtime-tested end to end under WinUAE. The 020 and 030FPU binaries have compiled successfully but still need runtime testing on matching real hardware/emulation configurations.

## Runtime requirements

- AmigaOS / Workbench 3.x
- TCP/IP stack exposing `bsdsocket.library`
- AmiSSL 5 with a valid `AmiSSL:` assign and certificate store
- Correct system date/time for TLS certificate validation
- Shell/task stack of at least ~60 KiB; use `stack 131072` during testing
- Approximately **6.4 MB free Fast RAM** was sufficient in the tested WinUAE 68040 configuration
- **8 MB+ Fast RAM recommended** until lower-memory testing is completed

The executable also defines libnix `__stack = 131072` and explicitly references `__stkinit` so swapstack support is linked. The runtime stack guard remains in place until this is validated across more systems.

## Native GUI direction

The next user-facing milestone is a native **Intuition + GadTools** front end so AmigaOS 3.x remains dependency-light. It will reuse the same profile, OAuth refresh, `/v1/models`, `/v1/responses`, TLS and SSE core as the CLI rather than duplicating protocol code.

Planned GUI targets are `AmiGPTGUI020`, `AmiGPTGUI030FPU`, and `AmiGPTGUI040`, while the CLI remains available for diagnostics and low-memory systems. See [`docs/GUI_PLAN.md`](docs/GUI_PLAN.md).

## Security

`PROGDIR:AmiGPT.profile` contains plaintext OAuth access, refresh and retained ID tokens. Treat it as a password-equivalent secret. Classic AmigaOS 3.x does not provide a modern protected credential store, so do not share the profile, include it in disk images, or commit it to Git.

The repository ignores `*.profile`, `AmiGPT.profile`, and profile temp files. See [`docs/SECURITY.md`](docs/SECURITY.md).

## Build portable tests

```sh
make test
```

## Amiga build

The Amiga targets require a current `m68k-amigaos-gcc` toolchain plus AmiSSL 5 headers/libraries:

```sh
make amiga020
make amiga030fpu
make amiga040
# or
make amiga-all
```

## Current limitations

- v0.8 source has not yet been cross-built and runtime-tested after the public-endpoint migration.
- First-time SIWC currently needs the PC helper; the planned native `AmiWebAuth` browser is not implemented yet.
- OAuth tokens are stored in plaintext on AmigaOS.
- Real-hardware 68020/68030 testing is still pending.

## Release history

- **v0.7** - first end-to-end runtime-proven PoC using the legacy Codex-compatible diagnostic path.
- **v0.8 (development)** - official SIWC public endpoints, imported protected credentials, public model discovery and Responses API inference.

This is an experimental open-source client and is **not an official OpenAI application**.

## Protocol references

AmiGPT68K v0.8 follows OpenAI's published open-source SIWC documentation:

- https://developers.openai.com/siwc/token-sharing-open-source/sign-in
- https://developers.openai.com/siwc/token-sharing-open-source/models-and-inference
- https://developers.openai.com/siwc/token-sharing-open-source/self-hosted-vms
- https://developers.openai.com/siwc/token-sharing-open-source/profiles-and-sessions
