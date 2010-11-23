#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/stat.h>
#include <io.h>
#include <conio.h>
#include <tchar.h>

#define LOCAL_STDIO     0
#define LOCAL_SOCKET    1
#define HTTP_PORT 80
#define HTTPS_PORT 443
#define MAX_HOST_NAME_LEN 256
/* relay method, server and port */
#define METHOD_UNDECIDED 0
#define METHOD_DIRECT    1
#define METHOD_SOCKS     2
#define METHOD_HTTP      3
#define METHOD_TELNET    4
/* informations for SOCKS */
#define SOCKS5_REP_SUCCEEDED    0x00    /* succeeded */
#define SOCKS5_REP_FAIL         0x01    /* general SOCKS serer failure */
#define SOCKS5_REP_NALLOWED     0x02    /* connection not allowed by ruleset */
#define SOCKS5_REP_NUNREACH     0x03    /* Network unreachable */
#define SOCKS5_REP_HUNREACH     0x04    /* Host unreachable */
#define SOCKS5_REP_REFUSED      0x05    /* connection refused */
#define SOCKS5_REP_EXPIRED      0x06    /* TTL expired */
#define SOCKS5_REP_CNOTSUP      0x07    /* Command not supported */
#define SOCKS5_REP_ANOTSUP      0x08    /* Address not supported */
#define SOCKS5_REP_INVADDR      0x09    /* Inalid address */
/* SOCKS5 authentication methods */
#define SOCKS5_AUTH_REJECT      0xFF    /* No acceptable auth method */
#define SOCKS5_AUTH_NOAUTH      0x00    /* without authentication */
#define SOCKS5_AUTH_GSSAPI      0x01    /* GSSAPI */
#define SOCKS5_AUTH_USERPASS    0x02    /* User/Password */
#define SOCKS5_AUTH_CHAP        0x03    /* Challenge-Handshake Auth Proto. */
#define SOCKS5_AUTH_EAP         0x05    /* Extensible Authentication Proto. */
#define SOCKS5_AUTH_MAF         0x08    /* Multi-Authentication Framework */
#define SOCKS4_REP_SUCCEEDED    90      /* rquest granted (succeeded) */
#define SOCKS4_REP_REJECTED     91      /* request rejected or failed */
#define SOCKS4_REP_IDENT_FAIL   92      /* cannot connect identd */
#define SOCKS4_REP_USERID       93      /* user id not matched */
#define RESOLVE_UNKNOWN 0
#define RESOLVE_LOCAL   1
#define RESOLVE_REMOTE  2
#define RESOLVE_BOTH    3
/* Environment variable names */
#define ENV_SOCKS_SERVER  "SOCKS_SERVER"        /* SOCKS server */
#define ENV_SOCKS5_SERVER "SOCKS5_SERVER"
#define ENV_SOCKS4_SERVER "SOCKS4_SERVER"
#define ENV_SOCKS_RESOLVE  "SOCKS_RESOLVE"      /* resolve method */
#define ENV_SOCKS5_RESOLVE "SOCKS5_RESOLVE"
#define ENV_SOCKS4_RESOLVE "SOCKS4_RESOLVE"
#define ENV_SOCKS5_USER     "SOCKS5_USER"       /* auth user for SOCKS5 */
#define ENV_SOCKS4_USER     "SOCKS4_USER"       /* auth user for SOCKS4 */
#define ENV_SOCKS_USER      "SOCKS_USER"        /* auth user for SOCKS */
#define ENV_SOCKS5_PASSWD   "SOCKS5_PASSWD"     /* auth password for SOCKS5 */
#define ENV_SOCKS5_PASSWORD "SOCKS5_PASSWORD"   /* old style */
#define ENV_HTTP_PROXY          "HTTP_PROXY"    /* common env var */
#define ENV_HTTP_PROXY_USER     "HTTP_PROXY_USER" /* auth user */
#define ENV_HTTP_PROXY_PASSWORD "HTTP_PROXY_PASSWORD" /* auth password */
#define ENV_TELNET_PROXY          "TELNET_PROXY"    /* common env var */
#define ENV_CONNECT_USER     "CONNECT_USER"     /* default auth user name */
#define ENV_CONNECT_PASSWORD "CONNECT_PASSWORD" /* default auth password */
#define ENV_SOCKS_DIRECT   "SOCKS_DIRECT"       /* addr-list for non-proxy */
#define ENV_SOCKS5_DIRECT  "SOCKS5_DIRECT"
#define ENV_SOCKS4_DIRECT  "SOCKS4_DIRECT"
#define ENV_HTTP_DIRECT    "HTTP_DIRECT"
#define ENV_CONNECT_DIRECT "CONNECT_DIRECT"
#define ENV_SOCKS5_AUTH "SOCKS5_AUTH"
#define ENV_SSH_ASKPASS "SSH_ASKPASS"           /* askpass program */
/* Prefix string of HTTP_PROXY */
#define HTTP_PROXY_PREFIX "http://"
#define PROXY_AUTH_NONE 0
#define PROXY_AUTH_BASIC 1
#define PROXY_AUTH_DIGEST 2
#define REASON_UNK              -2
#define REASON_ERROR            -1
#define REASON_CLOSED_BY_LOCAL  0
#define REASON_CLOSED_BY_REMOTE 1
#define START_ERROR -1
#define START_OK     0
#define START_RETRY  1
#define socket_errno() WSAGetLastError()
#define popen _popen

#define ECONNRESET WSAECONNRESET
#define MAX_DIRECT_ADDR_LIST 256

#define PUT_BYTE(ptr,data) (*(unsigned char*)ptr = data)

struct ADDRPAIR {
    struct in_addr addr;
    struct in_addr mask;
    int negative;
};

typedef struct {
    BOOL    fAutoDetect;
    LPWSTR  lpszAutoConfigUrl;
    LPWSTR  lpszProxy;
    LPWSTR  lpszProxyBypass;
} WINHTTP_CURRENT_USER_IE_PROXY_CONFIG;

typedef BOOL (STDAPICALLTYPE * pfnWinHttpGetIEProxyConfig)
(
    IN OUT WINHTTP_CURRENT_USER_IE_PROXY_CONFIG * pProxyConfig
);

typedef struct {
    int num;
    const char *str;
} LOOKUP_ITEM;

typedef struct {
    char* name;
    char* value;
} PARAMETER_ITEM;

typedef struct {
    char* name;
    unsigned char auth;
} AUTH_METHOD_ITEM;


class httpconnect
{
	
public:
	httpconnect();
	~httpconnect();

	void *xmalloc (size_t size);
	void downcase( char *buf );
	char * expand_host_and_port (const char *fmt, const char *host, int port);
	int lookup_resolve( const char *str );
	char *getusername(void);
	int expect( char *str, char *substr);
	PARAMETER_ITEM* find_parameter_item(const char* name);
	char* getparam(const char* name);
	void mask_addr (void *addr, void *mask, int addrlen);
	int add_direct_addr (struct in_addr *addr, struct in_addr *mask, int negative);
	int parse_addr_pair (const char *str, struct in_addr *addr, struct in_addr *mask);
	void initialize_direct_addr (void);
	int cmp_addr (void *addr1, void *addr2, int addrlen);
	int is_direct_address (const struct sockaddr_in *addr);
	char * determine_relay_user ();
	char *determine_relay_password ();
	int set_relay( int method, char *spec );
	u_short resolve_port( const char *service );
	int local_resolve (const char *host, struct sockaddr_in *addr);
	SOCKET open_connection( const char *host, u_short port );
	void report_text( char *prefix, char *buf );
	int atomic_out( SOCKET s, char *buf, int size );
	int atomic_in( SOCKET s, char *buf, int size );
	int line_input( SOCKET s, char *buf, int size );
	char *cut_token( char *str, char *delim);
	const char * lookup(int num, LOOKUP_ITEM *items);
	const char * socks5_getauthname( int auth );
	int socks5_do_auth_userpass( SOCKET s );
	char *readpass( const char* prompt, ...);
	int socks5_auth_parse_1(char *start, char *end);
	int socks5_auth_parse(char *start, unsigned char *auth_list, int max_auth);
	int begin_socks5_relay( SOCKET s );
	int begin_socks4_relay( SOCKET s );
	int sendf(SOCKET s, const char *fmt,...);
	char * make_base64_string(const char *str);
	int basic_auth (SOCKET s);
	int begin_http_relay( SOCKET s );
	BOOL ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport) ;
	BOOL ParseDisplay2(LPTSTR display, LPTSTR phost, int hostlen, int *pport) ;
	SOCKET mainconnect( int meth,char *server,char *remoteserver, int remoteport);
	SOCKET Get_https_socket(char *port, char *host);

	int   relay_method;          
	char *relay_host;                        
	u_short relay_port;                        
	char *relay_user;  
	char dest_host[256];
	u_short dest_port;
	struct sockaddr_in dest_addr;
	int f_debug;
	int proxy_auth_type;
	u_short local_port; 
	int   local_type;
	int socks_version; 
	int socks_resolve;
	int f_report;
	struct sockaddr_in socks_ns;
	char *socks5_auth;
	struct ADDRPAIR direct_addr_list[MAX_DIRECT_ADDR_LIST];
	int n_direct_addr_list;
};