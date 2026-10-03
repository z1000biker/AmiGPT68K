# AmiGPT68K architecture

## v0.8 normal path

1. The Amiga creates and persists a stable `ext_agent_host_id` (`HOSTID`).
2. Until `AmiWebAuth` exists, `tools/amigpt_login.py` performs the first browser OAuth 2.0 Authorization Code + PKCE flow on a trusted PC, using the Amiga host ID.
3. The resulting `oaiapp_...` client ID and OAuth tokens are copied back in `AmiGPT.profile`.
4. The Amiga performs refresh itself against the OpenAI token endpoint.
5. Model discovery uses `GET https://api.openai.com/v1/models`.
6. Inference uses streaming `POST https://api.openai.com/v1/responses` and is successful only after `response.completed`.

The normal v0.8 path does not send `originator: codex_cli_rs`, does not require `ChatGPT-Account-ID`, and does not call private ChatGPT `backend-api` endpoints.

## Legacy diagnostic path

`DEVICELOGIN` is retained only to reproduce the v0.7 Codex-device-auth PoC. Its credentials are explicitly rejected by v0.8 MODELS/CHAT.

## Networking

AmiSSL and `bsdsocket.library` are opened once per process. A shared `SSL_CTX` is used for HTTPS connections. Hostname and certificate verification remain enabled.

## Stack

The Amiga executable defines `__stack = 131072UL` and references `__stkinit`, while large protocol/auth buffers use static storage. A runtime guard refuses to run with less than roughly 60 KiB of task stack.
