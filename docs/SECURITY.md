# Security notes

- Never commit or publish `AmiGPT.profile` or any `*.profile` file.
- The profile contains access, refresh and retained ID tokens in plaintext.
- On a PC, `tools/amigpt_login.py` writes the profile atomically and requests owner-only (`0600`) permissions where supported.
- Classic AmigaOS 3.x does not provide equivalent per-user filesystem permissions. Store the profile on trusted media and do not include it in disk images, support bundles or screenshots.
- If the profile is copied by an untrusted party, disconnect AmiGPT68K in ChatGPT Settings and authorize again.
- Never disable TLS hostname or certificate verification.
- Never log authorization URLs containing `id_token_hint` or any access/refresh token.
