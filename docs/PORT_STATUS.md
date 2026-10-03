# Port status

## Confirmed in v0.7

The `AmiGPT040` binary completed device login, stored a profile, sent a real authenticated ChatGPT request and streamed the response in AmigaOS under WinUAE.

## Implemented in v0.8 source

- public `/v1/models` model discovery
- public `/v1/responses` streaming inference
- no normal-path Codex impersonation headers
- stable Amiga host ID command
- SIWC PC browser helper with PKCE and ID-token signature validation
- SIWC token refresh on Amiga
- `__stkinit` + 128 KiB libnix stack request + runtime guard
- multi-word CHAT prompts without mandatory quotes
- profile/token ignore rules and security documentation

## Not yet claimed

- v0.8 68K binaries have not yet been cross-built in this environment
- v0.8 has not yet been run end-to-end with a real SIWC profile on AmigaOS
- real 68020/68030 hardware runtime tests are still pending
- `AmiWebAuth` is a design/porting subproject, not a working browser yet
