# v0.8 response completeness fix

AmiGPT68K now treats `response.output_text.done` as the authoritative final text for a completed Responses API stream. Incremental `response.output_text.delta` events are still collected as a fallback, but the complete `text` from the done event is preferred before rendering the answer.

The JSON string decoder also accepts `\uXXXX` escapes (including surrogate pairs) instead of dropping text chunks containing escaped Unicode.

These changes address truncated answers observed in the Amiga GUI even when there was still unused display space.
