# Sign in with ChatGPT for AmiGPT68K

AmiGPT68K v0.8 uses the official open-source **Sign in with ChatGPT** authorization-code + PKCE flow and the public OpenAI endpoints.

Until a modern native Amiga browser is available, first-time authorization can be completed on a PC while keeping the **Amiga's own `ext_agent_host_id`**.

## 1. Create the Amiga host profile

On AmigaOS:

```text
stack 131072
AmiGPT040 HOSTID
```

This creates `PROGDIR:AmiGPT.profile` containing only the stable host ID. Copy that profile to a trusted PC.

## 2. Complete OAuth on the PC

Install Python 3 and the `cryptography` package, then run from the source tree:

```sh
python tools/amigpt_login.py --profile AmiGPT.profile
```

The helper opens the official `auth.openai.com` page, uses a `127.0.0.1` loopback callback, validates the returned ID token and required `chatgpt.tokens.use.direct` scope, and writes the issued `oaiapp_...` client ID plus tokens into the profile.

It does not print tokens.

## 3. Transfer the protected profile back

Copy the updated profile back to:

```text
PROGDIR:AmiGPT.profile
```

Treat this file as a password-equivalent secret. AmigaOS 3.x has no modern per-user credential vault or Unix-style `0600` file permissions, so anyone who can copy the profile can potentially use the stored renewable session.

## 4. Test on the Amiga

```text
AmiGPT040 REFRESH
AmiGPT040 MODELS
AmiGPT040 CHAT gpt-6.1-sol hello from my Amiga
```

`CHAT` now joins all remaining words into the prompt; quotes are optional unless the Amiga shell itself needs them for special characters. Use `--profile <path>` at the end to select another profile.

## Legacy DEVICELOGIN

`DEVICELOGIN` remains only as a diagnostic for the older Codex device-auth path. Its profile is deliberately rejected by v0.8 `MODELS` and `CHAT`; the public SIWC endpoints require a SIWC access token issued for `https://api.openai.com/v1`.
