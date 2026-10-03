# AmiWebAuth porting target

`AmiWebAuth` is intended to become a standalone native AmigaOS 3.x browser process used only for the Sign in with ChatGPT authorization page. It is not part of the v0.8 working path yet.

Current candidate direction: an auth-only 68K cut of a modern C browser stack with HTML/DOM/forms, enough CSS/layout and JavaScript for authentication, cookies/redirects, fetch/XHR/Promise/timers, WebCrypto subset, AmiSSL TLS and an Intuition/Picasso96 platform layer.

Until that exists, v0.8 uses the documented PC helper flow while preserving the Amiga's own host ID. No PC proxy is used for model discovery or inference.

A native browser will not be considered ready until it can render and submit the OpenAI sign-in page and complete the full redirect to `http://127.0.0.1:<port>/auth/callback` on AmigaOS.
