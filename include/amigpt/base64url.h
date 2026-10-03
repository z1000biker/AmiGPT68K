#ifndef AMIGPT_BASE64URL_H
#define AMIGPT_BASE64URL_H
#include <stddef.h>
int amigpt_base64url_encode(const unsigned char *src, size_t n, char *dst, size_t cap);
int amigpt_base64url_decode(const char *src, unsigned char *dst, size_t cap, size_t *out_len);
#endif
