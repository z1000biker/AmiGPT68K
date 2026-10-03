/* AmigaOS 3.x TCP/TLS transport. */
#include "amiga_tls.h"
#include "amiga_crypto.h"
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/socket.h>
#include <proto/amissl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
struct amigpt_tls {int fd;SSL *ssl;int handshake_ok;};
static SSL_CTX *g_ctx=0;
static void seterr(char *out,size_t cap,const char *s){if(out&&cap)snprintf(out,cap,"%s",s?s:"error");}
static int current_year(void){time_t now=time(0);struct tm *tmv;if(now==(time_t)-1)return 0;tmv=localtime(&now);if(!tmv)return 0;return tmv->tm_year+1900;}
static int tls_global_init(char *err,size_t errcap){if(g_ctx)return 0;if(!amigpt_crypto_is_open()){seterr(err,errcap,"AmiSSL runtime is not initialized");return -1;}ERR_clear_error();g_ctx=SSL_CTX_new(TLS_client_method());if(!g_ctx){seterr(err,errcap,"SSL_CTX_new failed");return -1;}SSL_CTX_set_verify(g_ctx,SSL_VERIFY_PEER,0);if(!SSL_CTX_set_default_verify_paths(g_ctx)){SSL_CTX_free(g_ctx);g_ctx=0;seterr(err,errcap,"AmiSSL CA paths unavailable");return -1;}return 0;}
void amigpt_tls_global_cleanup(void){if(g_ctx){SSL_CTX_free(g_ctx);g_ctx=0;}}
void amigpt_tls_close(amigpt_tls *c){if(!c)return;if(c->ssl){if(c->handshake_ok)SSL_shutdown(c->ssl);SSL_free(c->ssl);c->ssl=0;}if(c->fd>=0 && SocketBase){CloseSocket(c->fd);c->fd=-1;}free(c);}
static void handshake_error(SSL *ssl,int ret,char *err,size_t errcap){int se=SSL_get_error(ssl,ret);long vr=SSL_get_verify_result(ssl);unsigned long oe=ERR_get_error();const char *vmsg=X509_verify_cert_error_string(vr);char obuf[192];int year=current_year();obuf[0]=0;if(oe)ERR_error_string_n(oe,obuf,sizeof obuf);if(err&&errcap)snprintf(err,errcap,"TLS handshake failed: ssl=%d verify=%ld (%s) errno=%d year=%d%s%s",se,vr,vmsg?vmsg:"unknown",errno,year,obuf[0]?" openssl=":"",obuf[0]?obuf:"");}
amigpt_tls *amigpt_tls_connect(const char *host,unsigned short port,char *err,size_t errcap){amigpt_tls *c;struct sockaddr_in a;struct hostent *he;int sr;if(!amigpt_crypto_is_open()){seterr(err,errcap,"network/AmiSSL runtime not initialized");return 0;}if(tls_global_init(err,errcap)<0)return 0;c=(amigpt_tls*)calloc(1,sizeof(*c));if(!c){seterr(err,errcap,"out of memory");return 0;}c->fd=-1;memset(&a,0,sizeof a);a.sin_family=AF_INET;a.sin_port=htons(port);he=gethostbyname((STRPTR)host);if(!he||!he->h_addr_list||!he->h_addr_list[0]){seterr(err,errcap,"DNS lookup failed");goto fail;}memcpy(&a.sin_addr,he->h_addr_list[0],(size_t)he->h_length);c->fd=socket(AF_INET,SOCK_STREAM,0);if(c->fd<0){if(err&&errcap)snprintf(err,errcap,"socket failed errno=%d",errno);goto fail;}if(connect(c->fd,(struct sockaddr*)&a,sizeof a)<0){if(err&&errcap)snprintf(err,errcap,"TCP connect failed errno=%d",errno);goto fail;}c->ssl=SSL_new(g_ctx);if(!c->ssl){seterr(err,errcap,"SSL_new failed");goto fail;}if(!SSL_set_tlsext_host_name(c->ssl,host)||!SSL_set1_host(c->ssl,host)){seterr(err,errcap,"TLS hostname setup failed");goto fail;}if(!SSL_set_fd(c->ssl,c->fd)){seterr(err,errcap,"SSL_set_fd failed");goto fail;}ERR_clear_error();sr=SSL_connect(c->ssl);if(sr!=1){handshake_error(c->ssl,sr,err,errcap);goto fail;}c->handshake_ok=1;return c;fail:amigpt_tls_close(c);return 0;}
int amigpt_tls_write_all(amigpt_tls *c,const void *buf,size_t len,char *err,size_t errcap){const unsigned char *p=(const unsigned char*)buf;size_t done=0;if(!c||!c->ssl)return -1;while(done<len){int n=SSL_write(c->ssl,p+done,(int)(len-done));if(n<=0){if(err&&errcap)snprintf(err,errcap,"SSL_write failed ssl=%d",SSL_get_error(c->ssl,n));return -1;}done+=(size_t)n;}return 0;}
int amigpt_tls_read(amigpt_tls *c,void *buf,size_t cap,char *err,size_t errcap){int n;if(!c||!c->ssl)return -1;n=SSL_read(c->ssl,buf,(int)cap);if(n<0){if(err&&errcap)snprintf(err,errcap,"SSL_read failed ssl=%d",SSL_get_error(c->ssl,n));return -1;}return n;}
