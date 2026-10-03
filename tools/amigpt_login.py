#!/usr/bin/env python3
"""AmiGPT68K SIWC login helper for a PC with a modern browser.

Workflow:
  1. On the Amiga run: AmiGPT040 HOSTID
  2. Copy PROGDIR:AmiGPT.profile to this computer.
  3. Run: python tools/amigpt_login.py --profile AmiGPT.profile
  4. Copy the updated profile back to the Amiga over a trusted channel.

The helper uses the official Sign in with ChatGPT authorization-code + PKCE flow.
It never prints access or refresh tokens.
"""
from __future__ import annotations
import argparse, base64, hashlib, http.server, json, os, secrets, socketserver, tempfile, time
import urllib.parse, urllib.request, urllib.error, webbrowser
from pathlib import Path

AUTH_ENDPOINT="https://auth.openai.com/api/accounts/authorize"
TOKEN_ENDPOINT="https://auth.openai.com/api/accounts/oauth/token"
OIDC_CONFIG="https://auth.openai.com/.well-known/openid-configuration"
RESOURCE="https://api.openai.com/v1"
SCOPES="openid profile email offline_access resource.invoke chatgpt.tokens.use.direct"
DYNAMIC_CLIENT="dynamic_agent_client"
ISSUER="https://auth.openai.com"

def b64url(data:bytes)->str:return base64.urlsafe_b64encode(data).rstrip(b"=").decode("ascii")
def b64url_decode(text:str)->bytes:return base64.urlsafe_b64decode(text+"="*((4-len(text)%4)%4))

def read_profile(path:Path)->dict[str,str]:
    out={}
    if not path.exists(): return out
    for raw in path.read_text(encoding="utf-8",errors="strict").splitlines():
        if not raw or raw[0] in "#;" or "=" not in raw: continue
        k,v=raw.split("=",1);out[k]=v
    return out

def atomic_write_profile(path:Path,p:dict[str,str])->None:
    keys=["host_id","client_id","subject","email","account_id","id_token","access_token","refresh_token","scope","expires_in","earliest_refresh_at","saved_at"]
    path.parent.mkdir(parents=True,exist_ok=True)
    fd,tmp=tempfile.mkstemp(prefix=path.name+".",suffix=".tmp",dir=str(path.parent))
    try:
        os.fchmod(fd,0o600)
        with os.fdopen(fd,"w",encoding="utf-8",newline="\n") as f:
            f.write("# AmiGPT profile v1\n")
            for k in keys:f.write(f"{k}={p.get(k,'')}\n")
            f.flush();os.fsync(f.fileno())
        os.replace(tmp,path)
        try:os.chmod(path,0o600)
        except OSError:pass
    except Exception:
        try:os.unlink(tmp)
        except OSError:pass
        raise

def http_json(url:str)->dict:
    req=urllib.request.Request(url,headers={"User-Agent":"AmiGPT68K-login/0.8","Accept":"application/json"})
    with urllib.request.urlopen(req,timeout=30) as r:return json.loads(r.read().decode("utf-8"))

def post_form(url:str,form:dict[str,str])->dict:
    data=urllib.parse.urlencode(form).encode("ascii")
    req=urllib.request.Request(url,data=data,method="POST",headers={"User-Agent":"AmiGPT68K-login/0.8","Content-Type":"application/x-www-form-urlencoded","Accept":"application/json"})
    try:
        with urllib.request.urlopen(req,timeout=30) as r:return json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        body=e.read().decode("utf-8",errors="replace")[:500]
        raise RuntimeError(f"token endpoint HTTP {e.code}: {body}") from e

def verify_id_token(jwt:str,client_id:str,nonce:str)->dict:
    try:
        h64,p64,s64=jwt.split(".");header=json.loads(b64url_decode(h64));claims=json.loads(b64url_decode(p64))
    except Exception as e:raise RuntimeError("cannot parse ID token") from e
    if header.get("alg")!="RS256" or not header.get("kid"):raise RuntimeError("unsupported ID token signing algorithm/key")
    if claims.get("iss")!=ISSUER:raise RuntimeError("ID token issuer mismatch")
    aud=claims.get("aud")
    if not (aud==client_id or isinstance(aud,list) and client_id in aud):raise RuntimeError("ID token audience mismatch")
    if claims.get("nonce")!=nonce:raise RuntimeError("ID token nonce mismatch")
    now=int(time.time())
    if int(claims.get("exp",0))<now-5 or int(claims.get("iat",now))>now+30:raise RuntimeError("ID token time claims invalid")
    if not claims.get("sub"):raise RuntimeError("ID token has no subject")
    cfg=http_json(OIDC_CONFIG);jwks=http_json(cfg["jwks_uri"])
    key=next((k for k in jwks.get("keys",[]) if k.get("kid")==header["kid"] and k.get("kty")=="RSA"),None)
    if not key:raise RuntimeError("ID token signing key not found")
    try:
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import padding,rsa
        n=int.from_bytes(b64url_decode(key["n"]),"big");e=int.from_bytes(b64url_decode(key["e"]),"big")
        pub=rsa.RSAPublicNumbers(e,n).public_key();pub.verify(b64url_decode(s64),f"{h64}.{p64}".encode("ascii"),padding.PKCS1v15(),hashes.SHA256())
    except ImportError as e:raise RuntimeError("Python package 'cryptography' is required to validate the ID token") from e
    except Exception as e:raise RuntimeError("ID token signature invalid") from e
    return claims

class CallbackHandler(http.server.BaseHTTPRequestHandler):
    result:dict[str,str]|None=None
    expected_path="/auth/callback"
    def do_GET(self)->None:
        parsed=urllib.parse.urlparse(self.path)
        if parsed.path!=self.expected_path:self.send_response(404);self.end_headers();return
        q=urllib.parse.parse_qs(parsed.query,keep_blank_values=True);CallbackHandler.result={k:v[0] for k,v in q.items() if v}
        body=b"AmiGPT68K sign-in received. You may close this tab."
        self.send_response(200);self.send_header("Content-Type","text/plain; charset=utf-8");self.send_header("Content-Length",str(len(body)));self.end_headers();self.wfile.write(body)
    def log_message(self,fmt:str,*args)->None:return
class OneShotServer(socketserver.TCPServer):allow_reuse_address=True

def main()->int:
    ap=argparse.ArgumentParser(description="Create/update an AmiGPT68K SIWC profile using a PC browser")
    ap.add_argument("--profile",default="AmiGPT.profile",help="profile copied from the Amiga (default: AmiGPT.profile)")
    ap.add_argument("--host-id",help="Amiga ext_agent_host_id; normally already stored in the profile")
    ap.add_argument("--agent-name",default="AmiGPT68K",help="initial registration display name")
    ap.add_argument("--timeout",type=int,default=600,help="browser callback timeout in seconds")
    args=ap.parse_args();path=Path(args.profile).expanduser().resolve();profile=read_profile(path);host_id=args.host_id or profile.get("host_id","")
    if not host_id:raise SystemExit("No host_id. On the Amiga run 'AmiGPT040 HOSTID', then copy AmiGPT.profile here (or pass --host-id).")
    saved_client=profile.get("client_id","");returning=saved_client.startswith("oaiapp_");request_client=saved_client if returning else DYNAMIC_CLIENT
    verifier=b64url(secrets.token_bytes(48));challenge=b64url(hashlib.sha256(verifier.encode("ascii")).digest());state=b64url(secrets.token_bytes(24));nonce=b64url(secrets.token_bytes(24))
    CallbackHandler.result=None
    with OneShotServer(("127.0.0.1",0),CallbackHandler) as srv:
        srv.timeout=args.timeout;port=srv.server_address[1];redirect=f"http://127.0.0.1:{port}/auth/callback"
        params={"response_type":"code","client_id":request_client,"redirect_uri":redirect,"scope":SCOPES,"resource":RESOURCE,"code_challenge":challenge,"code_challenge_method":"S256","state":state,"nonce":nonce,"ext_agent_host_id":host_id}
        if returning:
            if profile.get("id_token"):params["id_token_hint"]=profile["id_token"]
            if profile.get("email"):params["login_hint"]=profile["email"]
        else:params["agent_name_hint"]=args.agent_name
        url=AUTH_ENDPOINT+"?"+urllib.parse.urlencode(params);print("Opening the official ChatGPT sign-in page in your browser...")
        if not webbrowser.open(url):
            if returning and params.get("id_token_hint"):raise SystemExit("Could not launch the browser; refusing to print an authorization URL containing id_token_hint")
            print("Open this URL manually:\n"+url)
        deadline=time.time()+args.timeout
        while CallbackHandler.result is None and time.time()<deadline:srv.handle_request()
        cb=CallbackHandler.result
    if not cb:raise SystemExit("Timed out waiting for the OAuth callback")
    if cb.get("error"):raise SystemExit(f"OAuth error: {cb['error']}: {cb.get('error_description','')}")
    if cb.get("state")!=state:raise SystemExit("OAuth state mismatch")
    code=cb.get("code","")
    if not code:raise SystemExit("OAuth callback contained no authorization code")
    if returning:
        issued_client=saved_client
        if cb.get("client_id") and cb["client_id"]!=issued_client:raise SystemExit("OAuth callback client_id changed unexpectedly")
    else:
        issued_client=cb.get("client_id","")
        if not issued_client.startswith("oaiapp_"):raise SystemExit("Initial registration did not return an issued oaiapp_ client_id")
    tok=post_form(TOKEN_ENDPOINT,{"grant_type":"authorization_code","client_id":issued_client,"code":code,"code_verifier":verifier,"redirect_uri":redirect,"resource":RESOURCE})
    for k in ("access_token","refresh_token","id_token"):
        if not tok.get(k):raise SystemExit(f"Token response missing {k}")
    scopes=tok.get("scope","")
    if "chatgpt.tokens.use.direct" not in scopes.split():raise SystemExit("ChatGPT plan permission chatgpt.tokens.use.direct was not granted")
    claims=verify_id_token(tok["id_token"],issued_client,nonce)
    profile.update({"host_id":host_id,"client_id":issued_client,"subject":str(claims.get("sub","")),"email":str(claims.get("email","")),"account_id":"","id_token":tok["id_token"],"access_token":tok["access_token"],"refresh_token":tok["refresh_token"],"scope":scopes,"expires_in":str(tok.get("expires_in",0)),"earliest_refresh_at":str(tok.get("earliest_refresh_at",0)),"saved_at":str(int(time.time()))})
    atomic_write_profile(path,profile);print(f"SIWC profile written securely: {path}");print(f"client_id: {issued_client}");print(f"host_id:   {host_id}")
    if profile.get("email"):print(f"account:   {profile['email']}")
    print("Copy this profile back to PROGDIR:AmiGPT.profile on the Amiga over a trusted channel.");return 0
if __name__=="__main__":raise SystemExit(main())
