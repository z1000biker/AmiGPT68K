CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iinclude
COMMON = src/common/base64url.c src/common/urlcodec.c src/common/jsonlite.c src/common/sse.c src/common/oauth.c src/common/pkce.c src/common/responses.c src/common/httpdec.c src/common/chat.c src/common/scope.c src/common/jwt.c src/common/jwks.c src/common/profile.c src/common/models.c

.PHONY: test clean amiga020 amiga030fpu amiga040 amiga-all m68k-check

test: build/test_common
	./build/test_common

build/test_common: $(COMMON) tests/test_common.c
	mkdir -p build
	$(CC) $(CFLAGS) $(COMMON) tests/test_common.c -o $@

clean:
	rm -rf build

# AmigaOS 3.x cross targets. Requires m68k-amigaos-gcc, bsdsocket netincludes,
# AmiSSL 5 headers and libraries in the compiler SDK search path.
M68K_CC ?= m68k-amigaos-gcc
M68K_CRT ?= -mcrt=nix20
M68K_WARN ?= -Wall -Wextra
M68K_BASE_CFLAGS ?= -O2 $(M68K_WARN) $(M68K_CRT) -Iinclude
M68K_LDLIBS ?= -lamisslauto -lamisslstubs
AMIGA_SRC = src/amiga/amiga_tls.c src/amiga/amiga_crypto.c src/amiga/amiga_http.c \
	 src/amiga/oauth_loopback.c src/amiga/oauth_exchange.c src/amiga/openai_http.c \
	 src/amiga/idtoken_verify.c src/amiga/models_http.c src/amiga/login_flow.c \
	 src/amiga/device_auth.c src/amiga/session.c src/amiga/main_cli.c

build/amiga:
	mkdir -p build/amiga

amiga020: build/amiga
	$(M68K_CC) $(M68K_BASE_CFLAGS) -m68020 -msoft-float $(COMMON) $(AMIGA_SRC) -o build/amiga/AmiGPT020 $(M68K_LDLIBS)

amiga030fpu: build/amiga
	$(M68K_CC) $(M68K_BASE_CFLAGS) -m68030 -mhard-float $(COMMON) $(AMIGA_SRC) -o build/amiga/AmiGPT030FPU $(M68K_LDLIBS)

amiga040: build/amiga
	$(M68K_CC) $(M68K_BASE_CFLAGS) -m68040 $(COMMON) $(AMIGA_SRC) -o build/amiga/AmiGPT040 $(M68K_LDLIBS)

amiga-all: amiga020 amiga030fpu amiga040

m68k-check: build/amiga
	$(M68K_CC) $(M68K_BASE_CFLAGS) -m68020 -msoft-float -c $(COMMON) $(AMIGA_SRC)
	mv *.o build/amiga/ 2>/dev/null || true
