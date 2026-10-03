#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "amigpt/base64url.h"
#include "amigpt/urlcodec.h"
#include "amigpt/oauth.h"
#include "amigpt/sse.h"
#include "amigpt/jsonlite.h"
#include "amigpt/responses.h"
#include "amigpt/httpdec.h"
#include "amigpt/chat.h"
#include "amigpt/scope.h"
#include "amigpt/jwt.h"
#include "amigpt/jwks.h"
#include "amigpt/profile.h"
#include "amigpt/models.h"
static int fails;
#define T(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);fails++;}}while(0)
static char http_body[256];static size_t http_used;
static void hcb(const char*d,size_t n,void*u){(void)u;if(http_used+n<sizeof http_body){memcpy(http_body+http_used,d,n);http_used+=n;http_body[http_used]=0;}}
static int nevents;static char last_event[64],last_data[256];
static void cb(const char*e,const char*d,void*u){(void)u;nevents++;strncpy(last_event,e,sizeof last_event-1);strncpy(last_data,d,sizeof last_data-1);}
int main(void){char b[4096];amigpt_sse s;char j[128];int v;
 T(amigpt_base64url_encode((const unsigned char*)"foo",3,b,sizeof b)==4&&!strcmp(b,"Zm9v"));
 T(amigpt_base64url_encode((const unsigned char*)"f",1,b,sizeof b)==2&&!strcmp(b,"Zg"));
 {unsigned char db[16];size_t dn=0;T(amigpt_base64url_decode("Zm9v",db,sizeof db,&dn)==0&&dn==3&&!memcmp(db,"foo",3));}
 T(amigpt_urlencode("a b/+",b,sizeof b)>0&&!strcmp(b,"a%20b%2F%2B"));
 {amigpt_oauth_params p={"dynamic_agent_client","urn:uuid:123","AmiGPT68K","http://127.0.0.1:1455/auth/callback","STATE","NONCE","CHALLENGE",0,0};T(amigpt_oauth_authorize_url(&p,b,sizeof b)>0);T(strstr(b,"client_id=dynamic_agent_client")!=0);T(strstr(b,"chatgpt.tokens.use.direct")!=0);T(strstr(b,"code_challenge_method=S256")!=0);T(strstr(b,"resource=https%3A%2F%2Fapi.openai.com%2Fv1")!=0);}
 {amigpt_oauth_params p={"oaiapp_abc","urn:uuid:123",0,"http://127.0.0.1:1455/auth/callback","STATE","NONCE","CHALLENGE","IDTOKEN","a@b.c"};T(amigpt_oauth_authorize_url(&p,b,sizeof b)>0);T(strstr(b,"client_id=oaiapp_abc")!=0);T(strstr(b,"id_token_hint=IDTOKEN")!=0);T(strstr(b,"login_hint=a%40b.c")!=0);T(strstr(b,"agent_name_hint")==0);}
 amigpt_sse_init(&s);T(amigpt_sse_feed(&s,"event: response.output_text.delta\ndata: {\"delta\":\"Hi\"}\n\n",68,cb,0)==0);T(nevents==1);T(!strcmp(last_event,"response.output_text.delta"));
 T(amigpt_json_string("{\"access_token\":\"abc\",\"ok\":true}","access_token",j,sizeof j)==1&&!strcmp(j,"abc"));T(amigpt_json_bool("{\"ok\":true}","ok",&v)==1&&v==1);
 T(amigpt_responses_body("gpt-test","Hello \"Amiga\"",b,sizeof b)>0);T(strstr(b,"\"store\":false")!=0);T(strstr(b,"\"stream\":true")!=0);T(strstr(b,"\"input\":[{\"role\":\"user\"")!=0);
 T(amigpt_response_delta("{\"type\":\"response.output_text.delta\",\"delta\":\"Hello\"}",j,sizeof j)==1&&!strcmp(j,"Hello"));
 {amigpt_httpdec h;const char *a="HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n";amigpt_httpdec_init(&h);http_used=0;T(amigpt_httpdec_feed(&h,a,17,hcb,0)==0);T(amigpt_httpdec_feed(&h,a+17,strlen(a)-17,hcb,0)==1);T(h.status==200);T(!strcmp(http_body,"Wikipedia"));}
 {amigpt_chat c;amigpt_chat_init(&c);T(amigpt_chat_add(&c,"user","Hello")==0);T(amigpt_chat_add(&c,"assistant","Hi")==0);T(amigpt_chat_add(&c,"user","Again")==0);T(amigpt_chat_request(&c,"gpt-test",b,sizeof b)>0);T(strstr(b,"\"role\":\"assistant\"")!=0);T(strstr(b,"\"store\":false")!=0);amigpt_chat_free(&c);}
 T(amigpt_scope_has("openid profile chatgpt.tokens.use.direct email","chatgpt.tokens.use.direct")==1);T(amigpt_scope_has("openid chatgpt.tokens.use.directX","chatgpt.tokens.use.direct")==0);
 {char nn[128],ee[32];const char *jw="{\"keys\":[{\"kty\":\"RSA\",\"kid\":\"A\",\"alg\":\"RS256\",\"n\":\"abc\",\"e\":\"AQAB\"},{\"kty\":\"RSA\",\"kid\":\"B\",\"alg\":\"RS256\",\"n\":\"def\",\"e\":\"AQAB\"}]}";T(amigpt_jwks_find_rsa(jw,"B",nn,sizeof nn,ee,sizeof ee)==1);T(!strcmp(nn,"def"));T(!strcmp(ee,"AQAB"));}
 {amigpt_profile p1,p2;amigpt_profile_init(&p1);strcpy(p1.host_id,"urn:uuid:test");strcpy(p1.client_id,"oaiapp_test");strcpy(p1.email,"a@b.c");strcpy(p1.refresh_token,"r=a.b-c_d");p1.expires_in=3600;p1.saved_at=12345;T(amigpt_profile_save("build/test.profile",&p1)==0);T(amigpt_profile_load("build/test.profile",&p2)==1);T(!strcmp(p2.client_id,"oaiapp_test"));T(!strcmp(p2.refresh_token,"r=a.b-c_d"));T(p2.expires_in==3600);remove("build/test.profile");}
 {amigpt_model_list ml;const char *mj="{\"models\":[{\"slug\":\"gpt-a\",\"display_name\":\"GPT A\",\"visibility\":\"list\"},{\"slug\":\"hidden\",\"display_name\":\"Hidden\",\"visibility\":\"hide\"},{\"slug\":\"gpt-b\",\"display_name\":\"GPT B\",\"visibility\":\"list\"}]}";T(amigpt_models_parse(mj,&ml)==0);T(ml.count==2);T(!strcmp(ml.item[0].slug,"gpt-a"));T(!strcmp(ml.item[0].display_name,"GPT A"));T(!strcmp(ml.item[1].slug,"gpt-b"));}
 if(fails){fprintf(stderr,"%d test(s) failed\n",fails);return 1;}puts("all common-core tests passed");return 0;}
