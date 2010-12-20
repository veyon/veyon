/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 Ultr@VNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include "httpconnect.h"

PARAMETER_ITEM parameter_table[] = {
    { ENV_SOCKS_SERVER, NULL },
    { ENV_SOCKS5_SERVER, NULL },
    { ENV_SOCKS4_SERVER, NULL },
    { ENV_SOCKS_RESOLVE, NULL },
    { ENV_SOCKS5_RESOLVE, NULL },
    { ENV_SOCKS4_RESOLVE, NULL },
    { ENV_SOCKS5_USER, NULL },
    { ENV_SOCKS5_PASSWD, NULL },
    { ENV_SOCKS5_PASSWORD, NULL },
    { ENV_HTTP_PROXY, NULL },
    { ENV_HTTP_PROXY_USER, NULL },
    { ENV_HTTP_PROXY_PASSWORD, NULL },
    { ENV_CONNECT_USER, NULL },
    { ENV_CONNECT_PASSWORD, NULL },
    { ENV_SSH_ASKPASS, NULL },
    { ENV_SOCKS5_DIRECT, NULL },
    { ENV_SOCKS4_DIRECT, NULL },
    { ENV_SOCKS_DIRECT, NULL },
    { ENV_HTTP_DIRECT, NULL },
    { ENV_CONNECT_DIRECT, NULL },
    { ENV_SOCKS5_AUTH, NULL },
    { NULL, NULL }
};

LOOKUP_ITEM socks4_rep_names[] = {
    { SOCKS4_REP_SUCCEEDED,  "request granted (succeeded)"},
    { SOCKS4_REP_REJECTED,   "request rejected or failed"},
    { SOCKS4_REP_IDENT_FAIL, "cannot connect identd"},
    { SOCKS4_REP_USERID,     "user id not matched"},
    { -1, NULL }
};

LOOKUP_ITEM socks5_rep_names[] = {
    { SOCKS5_REP_SUCCEEDED, "succeeded"},
    { SOCKS5_REP_FAIL,      "general SOCKS server failure"},
    { SOCKS5_REP_NALLOWED,  "connection not allowed by ruleset"},
    { SOCKS5_REP_NUNREACH,  "Network unreachable"},
    { SOCKS5_REP_HUNREACH,  "Host unreachable"},
    { SOCKS5_REP_REFUSED,   "connection refused"},
    { SOCKS5_REP_EXPIRED,   "TTL expired"},
    { SOCKS5_REP_CNOTSUP,   "Command not supported"},
    { SOCKS5_REP_ANOTSUP,   "Address not supported"},
    { SOCKS5_REP_INVADDR,   "Invalid address"},
    { -1, NULL }
};

AUTH_METHOD_ITEM socks5_auth_table[] = {
    { "none", SOCKS5_AUTH_NOAUTH },
    { "gssapi", SOCKS5_AUTH_GSSAPI },
    { "userpass", SOCKS5_AUTH_USERPASS },
    { "chap", SOCKS5_AUTH_CHAP },
    { NULL, -1 },
};

const char *digits    = "0123456789";
const char *dotdigits = "0123456789.";
char *method_names[] = { "UNDECIDED", "DIRECT", "SOCKS", "HTTP", "TELNET" };

httpconnect::~httpconnect()
{
}
httpconnect::httpconnect()
{
	relay_method = METHOD_UNDECIDED;          
	relay_host = NULL;                        
	relay_port = 0;                        
	relay_user = NULL;  
	dest_port = 0;
	f_debug = 0;
	proxy_auth_type = PROXY_AUTH_NONE;
	local_port = 0; 
	local_type = LOCAL_STDIO;
	socks_version = 5; 
	socks_resolve = RESOLVE_UNKNOWN;
	f_report = 1;
	socks5_auth = NULL;
	n_direct_addr_list = 0;
}

const char *base64_table =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* packet operation macro */

//int askpassbox(char *input, char *inuser, char *output, char *outuser);

void *
httpconnect::xmalloc (size_t size)
{
    void *ret = malloc(size);
    return ret;
}

void
httpconnect::downcase( char *buf )
{
    while ( *buf ) {
        if ( isupper(*buf) )
            *buf += 'a'-'A';
        buf++;
    }
}

char *
httpconnect::expand_host_and_port (const char *fmt, const char *host, int port)
{
    const char *src;
    char *buf, *dst;//, *ptr;
    size_t len = strlen(fmt) + strlen(host) + 20;
    buf = (char *)xmalloc (len);
    dst = buf;
    src = fmt;
    
    while (*src) {
	if (*src == '%') {
	    switch (src[1]) {
	    case 'h':
		strcpy (dst, host);
		src += 2;
		break;
	    case 'p':
		sprintf (dst, "%d", port);
		src += 2;
		break;
	    default:
		src ++;
		break;
	    }
	    dst = buf + strlen (buf);
	} else if (*src == '\\') {
	    switch (src[1]) {
	    case 'r':				/* CR */
		*dst++ = '\r';
		src += 2;
		break;
	    case 'n':				/* LF */
		*dst++ = '\n';
		src += 2;
		break;
	    case 't':				/* TAB */
		*dst++ = '\t';
		src += 2;
		break;
	    default:
		src ++;
		break;
	    }
	} else {
	    /* usual */
	    *dst++ = *src++;
	}
	*dst = '\0';
    }
    assert (strlen(buf) < len);
    return buf;
}


int
httpconnect::lookup_resolve( const char *str )
{
    char *buf = _strdup( str );
    int ret;

    downcase( buf );
    if ( strcmp( buf, "both" ) == 0 )
        ret = RESOLVE_BOTH;
    else if ( strcmp( buf, "remote" ) == 0 )
        ret = RESOLVE_REMOTE;
    else if ( strcmp( buf, "local" ) == 0 )
        ret = RESOLVE_LOCAL;
    else if ( strspn(buf, dotdigits) == strlen(buf) ) {
        ret = RESOLVE_LOCAL;                    /* this case is also 'local' */
        socks_ns.sin_addr.s_addr = inet_addr(buf);
        socks_ns.sin_family = AF_INET;
    }
    else
        ret = RESOLVE_UNKNOWN;
    free(buf);
    return ret;
}

char *
httpconnect::getusername(void)
{
    static char buf[1024];
    DWORD size = sizeof(buf);
    buf[0] = '\0';
    GetUserName( buf, &size);
    return buf;
}

/* expect
   check STR is begin with substr with case-ignored comparison.
   Return 1 if matched, otherwise 0.
*/
int
httpconnect::expect( char *str, char *substr)
{
    int len = strlen(substr);
    while ( 0 < len-- ) {
        if ( toupper(*str) != toupper(*substr) )
            return 0;                           /* not matched */
        str++, substr++;
    }
    return 1;                   /* good, matched */
}

PARAMETER_ITEM*
httpconnect::find_parameter_item(const char* name)
{
    int i;
    for( i = 0; parameter_table[i].name != NULL; i++ ){
        if ( strncmp(name, parameter_table[i].name, strlen(parameter_table[i].name)) == 0 )
            return &parameter_table[i];
    }
    return NULL;
}

char*
httpconnect::getparam(const char* name)
{
    char *value = getenv(name);
    if ( value == NULL ){
        PARAMETER_ITEM *item = find_parameter_item(name);
        if ( item != NULL )
            value = item->value;
    }
    return value;
}


/** DIRECT connection **/

void
httpconnect::mask_addr (void *addr, void *mask, int addrlen)
{
    char *a, *m;
    a =(char*) addr;
    m = (char *)mask;
    while ( 0 < addrlen-- )
        *a++ &= *m++;
}

int
httpconnect::add_direct_addr (struct in_addr *addr, struct in_addr *mask, int negative)
{
    struct in_addr iaddr;
    char *s;
    if ( MAX_DIRECT_ADDR_LIST <= n_direct_addr_list ) {
        return -1;
    }
    iaddr = *addr;
    mask_addr(&iaddr, mask, sizeof(iaddr));
    s = _strdup(inet_ntoa(iaddr));
    free(s);
    memcpy( &direct_addr_list[n_direct_addr_list].addr,
            &iaddr, sizeof(iaddr));
    memcpy( &direct_addr_list[n_direct_addr_list].mask,
            mask, sizeof(*mask));
    direct_addr_list[n_direct_addr_list].negative = negative;
    n_direct_addr_list++;
    return 0;
}

int
httpconnect::parse_addr_pair (const char *str, struct in_addr *addr, struct in_addr *mask)
{
    const char *ptr;
    u_char *dsta, *dstm;
    int i, n;

    assert( str != NULL );
    addr->s_addr = 0;
    mask->s_addr = 0;
    ptr = str;
    dsta = (u_char*)&addr->s_addr;
    dstm = (u_char*)&mask->s_addr;
    for (i=0; i<4; i++ ) {
        if ( *ptr == '\0' )
            break;              /* case of format #3 */
        if ( !isdigit(*ptr) )
            return -1;          /* format error: */
        *dsta++ = atoi( ptr );
        *dstm++ = 255;          /* automatic mask for format #3 */
        while ( isdigit(*ptr) ) /* skip digits */
            ptr++;
        if ( *ptr == '.' )
            ptr++;
        else
            break;
    }
    /* At this point, *ptr points '/' or EOS ('\0') */
    if ( *ptr == '\0' )
        return 0;                       /* complete as format #3 */
    if ( *ptr != '/' )
        return -1;                      /* format error */
    /* Now parse mask for format #1 or #2 */
    ptr++;
    mask->s_addr = 0;                   /* clear automatic mask */

    if ( strchr( ptr, '.') ) {
        /* case of format #1 */
        dstm = (u_char*)&mask->s_addr;
        for (i=0; i<4; i++) {
            if ( !isdigit(*ptr) )
                return -1;              /* format error: */
            *dstm++ = atoi(ptr);
            while ( isdigit(*ptr) )     /* skip digits */
                ptr++;
            if ( *ptr == '.' )
                ptr++;
            else
                break;                  /* from for loop */
        }
        /* complete as format #1 */
    } else {
        /* case of format #2 */
        if ( !isdigit(*ptr) )
            return -1;                  /* format error: */
        n = atoi(ptr);
        if ( n<0 || 32<n)
            return -1;                  /* format error */
        mask->s_addr = (n==0)? 0: htonl(((u_long)0xFFFFFFFF)<<(32-n));
        /* complete as format #1 */
    }
    return 0;
}

void
httpconnect::initialize_direct_addr (void)
{
    int negative;
    int n_entries;
    char *env = NULL, *beg, *next, *envkey = NULL;
    struct in_addr addr, mask;

    if ( relay_method == METHOD_SOCKS ){
        if ( socks_version == 5 )
            envkey = ENV_SOCKS5_DIRECT;
        else
            envkey = ENV_SOCKS4_DIRECT;
        env = getparam(envkey);
        if ( env == NULL )
            env = getparam(ENV_SOCKS_DIRECT);
    } else if ( relay_method == METHOD_HTTP ){
        env = getparam(ENV_HTTP_DIRECT);
    }

    if ( env == NULL )
        env = getparam(ENV_CONNECT_DIRECT);

    if ( env == NULL )
        return;                 /* no entry */
    env = _strdup( env );        /* reallocate to modify */
    beg = next = env;
    n_entries = 0;
    do {
        if ( MAX_DIRECT_ADDR_LIST <= n_entries ) {
            break;              /* from do loop */
        }
        next = strchr( beg, ',');
        if ( next != NULL )
            *next++ = '\0';
        addr.s_addr = 0;
        mask.s_addr = 0;
        if (*beg == '!') {
            negative = 1;
            beg++;
        } else
            negative = 0;
        if ( !parse_addr_pair( beg, &addr, &mask ) ) {
            add_direct_addr( &addr, &mask, negative );
        } else {
            return;
        }
        if ( next != NULL )
            beg = next;
    } while ( next != NULL );

    free( env );
    return;
}

int
httpconnect::cmp_addr (void *addr1, void *addr2, int addrlen)
{
    return memcmp( addr1, addr2, addrlen );
}

int
httpconnect::is_direct_address (const struct sockaddr_in *addr)
{
    int i;
    struct in_addr saddr, iaddr;

    saddr = addr->sin_addr;

    /* Note: assume IPV4 address !! */
    for (i=0; i<n_direct_addr_list; i++ ) {
        iaddr = saddr;
        mask_addr( &iaddr, &direct_addr_list[i].mask,
                   sizeof(struct in_addr));
        if (cmp_addr(&iaddr, &direct_addr_list[i].addr,
                     sizeof(struct in_addr)) == 0) {
            if (direct_addr_list[i].negative) {
                return 0;       /* not direct */
            }
            if (!direct_addr_list[i].negative) {               
                return 1;       /* direct*/
            }
        }
    }
    return 0;                   /* not direct */
}

char *
httpconnect::determine_relay_user ()
{
    char *user = NULL;
    /* get username from environment variable, or system. */
    if (relay_method == METHOD_SOCKS) {
        if (user == NULL && socks_version == 5)
            user = getparam (ENV_SOCKS5_USER);
        if (user == NULL && socks_version == 4)
            user = getparam (ENV_SOCKS4_USER);
        if (user == NULL)
            user = getparam (ENV_SOCKS_USER);
    } else if (relay_method == METHOD_HTTP) {
        if (user == NULL)
            user = getparam (ENV_HTTP_PROXY_USER);
    }
    if (user == NULL)
        user = getparam (ENV_CONNECT_USER);
    /* determine relay user by system call if not yet. */
    if (user == NULL)
        user = getusername();
    return user;
}

char *
httpconnect::determine_relay_password ()
{
    char *pass = NULL;
    if (pass == NULL && relay_method == METHOD_HTTP)
        pass = getparam(ENV_HTTP_PROXY_PASSWORD);
    if (pass == NULL && relay_method == METHOD_SOCKS)
        pass = getparam(ENV_SOCKS5_PASSWD);
    if (pass == NULL && relay_method == METHOD_SOCKS)
        pass = getparam(ENV_SOCKS5_PASSWORD);
    if (pass == NULL)
        pass = getparam(ENV_CONNECT_PASSWORD);
    return pass;
}


int
httpconnect::set_relay( int method, char *spec )
{
    char *buf, *sep, *resolve;

    relay_method = method;
    initialize_direct_addr();
    if (n_direct_addr_list == 0) {
       
    } else {
        int i;
        for ( i=0; i<n_direct_addr_list; i++ ) {
            char *s1, *s2;
            s1 = _strdup(inet_ntoa(direct_addr_list[i].addr));
            s2 = _strdup(inet_ntoa(direct_addr_list[i].mask));
            free(s1);
            free(s2);
        }
    }

    switch ( method ) {
    case METHOD_DIRECT:
        return -1;                              /* nothing to do */

    case METHOD_SOCKS:
        if ( spec == NULL ) {
            switch ( socks_version ) {
            case 5:
                spec = getparam(ENV_SOCKS5_SERVER);
                break;
            case 4:
                spec = getparam(ENV_SOCKS4_SERVER);
                break;
            }
        }
        if ( spec == NULL )
            spec = getparam(ENV_SOCKS_SERVER);

        if ( spec == NULL )
            return 0;
        relay_port = 1080;                      /* set default first */

        /* determine resolve method */
        if ( socks_resolve == RESOLVE_UNKNOWN ) {
            if ( ((socks_version == 5) &&
                  ((resolve = getparam(ENV_SOCKS5_RESOLVE)) != NULL)) ||
                 ((socks_version == 4) &&
                  ((resolve = getparam(ENV_SOCKS4_RESOLVE)) != NULL)) ||
                 ((resolve = getparam(ENV_SOCKS_RESOLVE)) != NULL) ) {
                socks_resolve = lookup_resolve( resolve );
                if ( socks_resolve == RESOLVE_UNKNOWN )
                     return 0;
            } else {
                /* default */
                if ( socks_version == 5 )
                    socks_resolve = RESOLVE_REMOTE;
                else
                    socks_resolve = RESOLVE_LOCAL;
            }
        }
        break;

    case METHOD_HTTP:
        if ( spec == NULL )
            spec = getparam(ENV_HTTP_PROXY);
        if ( spec == NULL )
             return 0;
        relay_port = 80;                        /* set default first */
        break;
    }

    if (expect( spec, HTTP_PROXY_PREFIX)) {
        /* URL format like: "http://server:port/" */
        /* extract server:port part */
        buf = _strdup( spec + strlen(HTTP_PROXY_PREFIX));
        buf[strcspn(buf, "/")] = '\0';
    } else {
        /* assume spec is aready "server:port" format */
        buf = _strdup( spec );
    }
    spec = buf;

    /* check username in spec */
    sep = strchr( spec, '@' );
    if ( sep != NULL ) {
        *sep = '\0';
        relay_user = _strdup( spec );
        spec = sep +1;
    }
    if (relay_user == NULL)
        relay_user = determine_relay_user();

    /* split out hostname and port number from spec */
    sep = strchr(spec,':');
    if ( sep == NULL ) {
        /* hostname only, port is already set as default */
        relay_host = _strdup( spec );
    } else {
        /* hostname and port */
        relay_port = atoi(sep+1);
        *sep = '\0';
        relay_host = _strdup( spec );
    }
    free(buf);
    return 0;
}


u_short
httpconnect::resolve_port( const char *service )
{
    int port;
    if ( service[strspn (service, digits)] == '\0'  ) {
        /* all digits, port number */
        port = atoi(service);
    } else {
        /* treat as service name */
        struct servent *ent;
        ent = getservbyname( service, NULL );
        if ( ent == NULL ) {
            port = 0;
        } else {
            port = ntohs(ent->s_port);
        }
    }
    return (u_short)port;
}

int
httpconnect::local_resolve (const char *host, struct sockaddr_in *addr)
{
    struct hostent *ent;
    if ( strspn(host, dotdigits) == strlen(host) ) {
        /* given by IPv4 address */
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = inet_addr(host);
    } else {
        ent = gethostbyname (host);
        if ( ent ) {
            memcpy (&addr->sin_addr, ent->h_addr, ent->h_length);
            addr->sin_family = ent->h_addrtype;
        } else {
            return -1;                          /* failed */
        }
    }
    return 0;                                   /* good */
}

SOCKET
httpconnect::open_connection( const char *host, u_short port )
{
    SOCKET s;
    struct sockaddr_in saddr;

    if ( relay_method == METHOD_DIRECT ) {
        host = dest_host;
        port = dest_port;
    } else if ((local_resolve (dest_host, &saddr) >= 0)&&
               (is_direct_address(&saddr))) {
        relay_method = METHOD_DIRECT;
        host = dest_host;
        port = dest_port;
    } else {
        host = relay_host;
        port = relay_port;
    }

    if (local_resolve (host, &saddr) < 0) {
        return SOCKET_ERROR;
    }
    saddr.sin_port = htons(port);
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( connect( s, (struct sockaddr *)&saddr, sizeof(saddr))
         == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }
    return s;
}

void
httpconnect::report_text( char *prefix, char *buf )
{
    static char work[1024];
    char *tmp;

    if ( !f_debug )
        return;
    if ( !f_report )
        return;                                 /* don't report */
    while ( *buf ) {
        memset( work, 0, sizeof(work));
        tmp = work;
        while ( *buf && ((tmp-work) < (int)sizeof(work)-5) ) {
            switch ( *buf ) {
            case '\t': *tmp++ = '\\'; *tmp++ = 't'; break;
            case '\r': *tmp++ = '\\'; *tmp++ = 'r'; break;
            case '\n': *tmp++ = '\\'; *tmp++ = 'n'; break;
            case '\\': *tmp++ = '\\'; *tmp++ = '\\'; break;
            default:
                if ( isprint(*buf) ) {
                    *tmp++ = *buf;
                } else {
                    sprintf( tmp, "\\x%02X", (unsigned char)*buf);
                    tmp += strlen(tmp);
                }
            }
            buf++;
            *tmp = '\0';
        }
    }
}

int
httpconnect::atomic_out( SOCKET s, char *buf, int size )
{
    int ret, len;

    assert( buf != NULL );
    assert( 0<=size );
    /* do atomic out */
    ret = 0;
    while ( 0 < size ) {
        len = send( s, buf+ret, size, 0 );
        if ( len == -1 )
           return -1;
        ret += len;
        size -= len;
    }
    return ret;
}

int
httpconnect::atomic_in( SOCKET s, char *buf, int size )
{
    int ret, len;

    assert( buf != NULL );
    assert( 0<=size );

    /* do atomic in */
    ret = 0;
    while ( 0 < size ) {
        len = recv( s, buf+ret, size, 0 );
        if ( len == -1 ) {
            return -1;
        } else if ( len == 0 ) {
            return -1;
        }
        ret += len;
        size -= len;
    }
    return ret;
}

int
httpconnect::line_input( SOCKET s, char *buf, int size )
{
    char *dst = buf;
    if ( size == 0 )
        return 0;                               /* no error */
    size--;
    while ( 0 < size ) {
        switch ( recv( s, dst, 1, 0) ) {        /* recv one-by-one */
        case SOCKET_ERROR:
            return -1;                          /* error */
        case 0:
            size = 0;                           /* end of stream */
            break;
        default:
            /* continue reading until last 1 char is EOL? */
            if ( *dst == '\n' ) {
                /* finished */
                size = 0;
            } else {
                /* more... */
                size--;
            }
            dst++;
        }
    }
    *dst = '\0';
//    report_text( "<<<", buf);
    return 0;
}


char *
httpconnect::cut_token( char *str, char *delim)
{
    char *ptr = str + strcspn(str, delim);
    char *end = ptr + strspn(ptr, delim);
    if ( ptr == str )
        return NULL;
    while ( ptr < end )
        *ptr++ = '\0';
    return ptr;
}

const char *
httpconnect::lookup(int num, LOOKUP_ITEM *items)
{
    int i = 0;
    while (0 <= items[i].num) {
        if (items[i].num == num)
            return items[i].str;
        i++;
    }
    return "(unknown)";
}


char *
httpconnect::readpass( const char* prompt, ...)
{
    static char buf[1000];                      /* XXX, don't be fix length */
    va_list args;
    va_start(args, prompt);
    vsprintf(buf, prompt, args);
    va_end(args);
	//askpassbox(buf,relay_user, buf,relay_user);
    buf[strcspn(buf, "\r\n")] = '\0';
    return buf;
}

int
httpconnect::socks5_do_auth_userpass( SOCKET s )
{
    unsigned char buf[1024], *ptr;
    char *pass = NULL;
    int len;

    /* do User/Password authentication. */
    /* This feature requires username and password from
       command line argument or environment variable,
       or terminal. */
    if (relay_user == NULL)
        return -1;

    /* get password from environment variable if exists. */
    if ((pass=determine_relay_password()) == NULL &&
        (pass=readpass("Enter SOCKS5 password for %s@%s: ",
                       relay_user, relay_host)) == NULL)
        return -1;

    /* make authentication packet */
    ptr = buf;
    PUT_BYTE( ptr++, 1 );                       /* subnegotiation ver.: 1 */
    len = strlen( relay_user );                 /* ULEN and UNAME */
    PUT_BYTE( ptr++, len );
    strcpy( (char*)ptr, relay_user );
    ptr += len;
    len = strlen( pass );                       /* PLEN and PASSWD */
    PUT_BYTE( ptr++, strlen(pass));
    strcpy( (char *)ptr, pass );
    ptr += len;
    memset (pass, 0, strlen(pass));             /* erase password */

    /* send it and get answer */
    f_report = 0;
    atomic_out( s, (char*)buf, ptr-buf );
    f_report = 1;
    atomic_in( s, (char*)buf, 2 );

    /* check status */
    if ( buf[1] == 0 )
        return 0;                               /* success */
    else
        return -1;                              /* fail */
}

const char *
httpconnect::socks5_getauthname( int auth )
{
    switch ( auth ) {
    case SOCKS5_AUTH_REJECT: return "REJECTED";
    case SOCKS5_AUTH_NOAUTH: return "NO-AUTH";
    case SOCKS5_AUTH_GSSAPI: return "GSSAPI";
    case SOCKS5_AUTH_USERPASS: return "USERPASS";
    case SOCKS5_AUTH_CHAP: return "CHAP";
    case SOCKS5_AUTH_EAP: return "EAP";
    case SOCKS5_AUTH_MAF: return "MAF";
    default: return "(unknown)";
    }
}

int
httpconnect::socks5_auth_parse_1(char *start, char *end){
    int i, len;
    for ( ; *start; start++ )
        if ( *start != ' ' && *start != '\t') break;
    for ( end--; end >= start; end-- ) {
        if ( *end != ' ' && *end != '\t'){
            end++;
            break;
        }
    }
    len = end - start;
    for ( i = 0; socks5_auth_table[i].name != NULL; i++ ){
        if ( strncmp(start, socks5_auth_table[i].name, len) == 0) {
            return socks5_auth_table[i].auth;
        }
    }
    return -1;
}

int
httpconnect::socks5_auth_parse(char *start, unsigned char *auth_list, int max_auth){
    char *end;
    int i = 0;
    while ( i < max_auth ) {
        end = strchr(start, ',');
        if (*start && end) {
            auth_list[i++] = socks5_auth_parse_1(start, end);
            start = ++end;
        } else {
            break;
        }
    }
    if ( *start && ( i < max_auth ) ){
        for( end = start; *end; end++ );
        auth_list[i++] = socks5_auth_parse_1(start, end);
    } else {
       return -1;
    }
    return i;
}

/* begin SOCKS5 relaying
   And no authentication is supported.
 */
int
httpconnect::begin_socks5_relay( SOCKET s )
{
    unsigned char buf[256], *ptr, *env = (unsigned char*)socks5_auth;
    unsigned char n_auth = 0; unsigned char auth_list[10], auth_method;
    int len, auth_result, i;

    /* request authentication */
    ptr = buf;
    PUT_BYTE( ptr++, 5);                        /* SOCKS version (5) */

    if ( env == NULL )
        env = (unsigned char*)getparam(ENV_SOCKS5_AUTH);
    if ( env == NULL ) {
        /* add no-auth authentication */
        auth_list[n_auth++] = SOCKS5_AUTH_NOAUTH;
        /* add user/pass authentication */
        auth_list[n_auth++] = SOCKS5_AUTH_USERPASS;
    } else {
        n_auth = socks5_auth_parse((char*)env, auth_list, 10);
    }
    PUT_BYTE( ptr++, n_auth);                   /* num auth */
    for (i=0; i<n_auth; i++) {
        PUT_BYTE( ptr++, auth_list[i]);         /* authentications */
    }
    atomic_out( s, (char*)buf, ptr-buf );              /* send requst */
    atomic_in( s, (char*)buf, 2 );                     /* recv response */
    if ( (buf[0] != 5) ||                       /* ver5 response */
         (buf[1] == 0xFF) ) {                   /* check auth method */
        return -1;
    }
    auth_method = buf[1];


    switch ( auth_method ) {
    case SOCKS5_AUTH_REJECT:
        return -1;                              /* fail */

    case SOCKS5_AUTH_NOAUTH:
        /* nothing to do */
        auth_result = 0;
        break;

    case SOCKS5_AUTH_USERPASS:
        auth_result = socks5_do_auth_userpass(s);
        break;

    default:
        return -1;                              /* fail */
    }
    if ( auth_result != 0 ) {
        return -1;
    }
    /* request to connect */
    ptr = buf;
    PUT_BYTE( ptr++, 5);                        /* SOCKS version (5) */
    PUT_BYTE( ptr++, 1);                        /* CMD: CONNECT */
    PUT_BYTE( ptr++, 0);                        /* FLG: 0 */
    if ( dest_addr.sin_addr.s_addr == 0 ) {
        /* resolved by SOCKS server */
        PUT_BYTE( ptr++, 3);                    /* ATYP: DOMAINNAME */
        len = strlen(dest_host);
        PUT_BYTE( ptr++, len);                  /* DST.ADDR (len) */
        memcpy( ptr, dest_host, len );          /* (hostname) */
        ptr += len;
    } else {
        /* resolved localy */
        PUT_BYTE( ptr++, 1 );                   /* ATYP: IPv4 */
        memcpy( ptr, &dest_addr.sin_addr.s_addr, sizeof(dest_addr.sin_addr));
        ptr += sizeof(dest_addr.sin_addr);
    }
    PUT_BYTE( ptr++, dest_port>>8);     /* DST.PORT */
    PUT_BYTE( ptr++, dest_port&0xFF);
    atomic_out( s, (char*)buf, ptr-buf);               /* send request */
    atomic_in( s, (char*)buf, 4 );                     /* recv response */
    if ( (buf[1] != SOCKS5_REP_SUCCEEDED) ) {   /* check reply code */
        return -1;
    }
    ptr = buf + 4;
    switch ( buf[3] ) {                         /* case by ATYP */
    case 1:                                     /* IP v4 ADDR*/
        atomic_in( s, (char*)ptr, 4+2 );               /* recv IPv4 addr and port */
        break;
    case 3:                                     /* DOMAINNAME */
        atomic_in( s, (char*)ptr, 1 );                 /* recv name and port */
        atomic_in( s, (char*)ptr+1, *(unsigned char*)ptr + 2);
        break;
    case 4:                                     /* IP v6 ADDR */
        atomic_in( s, (char*)ptr, 16+2 );              /* recv IPv6 addr and port */
        break;
    }

    /* Conguraturation, connected via SOCKS5 server! */
    return 0;
}

int
httpconnect::begin_socks4_relay( SOCKET s )
{
    unsigned char buf[256], *ptr;
    ptr = buf;
    PUT_BYTE( ptr++, 4);                        /* protocol version (4) */
    PUT_BYTE( ptr++, 1);                        /* CONNECT command */
    PUT_BYTE( ptr++, dest_port>>8);     /* destination Port */
    PUT_BYTE( ptr++, dest_port&0xFF);
    /* destination IP */
    memcpy(ptr, &dest_addr.sin_addr, sizeof(dest_addr.sin_addr));
    ptr += sizeof(dest_addr.sin_addr);
    if ( dest_addr.sin_addr.s_addr == 0 )
        *(ptr-1) = 1;                           /* fake, protocol 4a */
    /* username */
    if (relay_user == NULL)
        return -1;
    strcpy( (char*)ptr, relay_user );
    ptr += strlen( relay_user ) +1;
    /* destination host name (for protocol 4a) */
    if ( (socks_version == 4) && (dest_addr.sin_addr.s_addr == 0)) {
        strcpy( (char*)ptr, dest_host );
        ptr += strlen( dest_host ) +1;
    }
    /* send command and get response
       response is: VN:1, CD:1, PORT:2, ADDR:4 */
    atomic_out( s, (char*)buf, ptr-buf);               /* send request */
    atomic_in( s, (char*)buf, 8 );                     /* recv response */
    if ( (buf[1] != SOCKS4_REP_SUCCEEDED) ) {   /* check reply code */       
        return -1;                              /* failed */
    }

    /* Conguraturation, connected via SOCKS4 server! */
    return 0;
}

int
httpconnect::sendf(SOCKET s, const char *fmt,...)
{
    static char buf[10240];                     /* xxx, enough? */

    va_list args;
    va_start( args, fmt );
    vsprintf( buf, fmt, args );
    va_end( args );

//    report_text(">>>", buf);
    if ( send(s, buf, strlen(buf), 0) == SOCKET_ERROR ) {       
        return -1;
    }
    return 0;
}

char *
httpconnect::make_base64_string(const char *str)
{
    static char *buf;
    unsigned char *src;
    char *dst;
    int bits, data, src_len, dst_len;
    /* make base64 string */
    src_len = strlen(str);
    dst_len = (src_len+2)/3*4;
    buf = (char*)xmalloc(dst_len+1);
    bits = data = 0;
    src = (unsigned char *)str;
    dst = (char *)buf;
    while ( dst_len-- ) {
        if ( bits < 6 ) {
            data = (data << 8) | *src;
            bits += 8;
            if ( *src != 0 )
                src++;
        }
        *dst++ = base64_table[0x3F & (data >> (bits-6))];
        bits -= 6;
    }
    *dst = '\0';
    /* fix-up tail padding */
    switch ( src_len%3 ) {
    case 1:
        *--dst = '=';
    case 2:
        *--dst = '=';
    }
    return buf;
}


int
httpconnect::basic_auth (SOCKET s)
{
    char *userpass;
    char *cred;
    const char *user = relay_user;
    char *pass = NULL;
    int len, ret;

    /* Get username/password for authentication */
    if (user == NULL)
	{       
		return -1;
	}
    if ((pass = determine_relay_password ()) == NULL &&
        (pass = readpass("Enter proxy authentication password for %s@%s: ",
                         relay_user, relay_host)) == NULL)
	{
		return -1;
	}

    len = strlen(user)+strlen(pass)+1;
    userpass = (char*)xmalloc(len+1);
    sprintf(userpass,"%s:%s", user, pass);
    memset (pass, 0, strlen(pass));
    cred = make_base64_string(userpass);
    memset (userpass, 0, len);

    f_report = 0;                               /* don't report for security */
    ret = sendf(s, "Proxy-Authorization: Basic %s\r\n", cred);
    f_report = 1;
    report_text(">>>", "Proxy-Authorization: Basic xxxxx\r\n");

    memset(cred, 0, strlen(cred));
    free(cred);

    return ret;
}

/* begin relaying via HTTP proxy
   Directs CONNECT method to proxy server to connect to
   destination host (and port). It may not be allowed on your
   proxy server.
 */
int
httpconnect::begin_http_relay( SOCKET s )
{
	char auth_www[] = "WWW-Authenticate:";
	char auth_proxy[] = "Proxy-Authenticate:";
    char buf[1024];
    int result;
    char *auth_what;
    if (sendf(s,"CONNECT %s:%d HTTP/1.0\r\n", dest_host, dest_port) < 0)
        return START_ERROR;
    if (proxy_auth_type == PROXY_AUTH_BASIC && basic_auth (s) < 0)
        return START_ERROR;
    if (sendf(s,"\r\n") < 0)
        return START_ERROR;

    /* get response */
    if ( line_input(s, buf, sizeof(buf)) < 0 ) {
        return START_ERROR;
    }

    /* check status */
    if (!strchr(buf, ' ')) {
	return START_ERROR;
    }
    result = atoi(strchr(buf,' '));

    switch ( result ) {
    case 200:
        break;
    case 302:                                   /* redirect */
        do {
            if (line_input(s, buf, sizeof(buf)))
                break;
            downcase(buf);
            if (expect(buf, "Location: ")) {
                relay_host = cut_token(buf, "//");
                cut_token(buf, "/");
                relay_port = atoi(cut_token(buf, ":"));
            }
        } while (strcmp(buf,"\r\n") != 0);
        return START_RETRY;

    /* We handle both 401 and 407 codes here: 401 is WWW-Authenticate, which
     * not strictly the correct response, but some proxies do send this (e.g.
     * Symantec's Raptor firewall) */
    case 401:                                   /* WWW-Auth required */
    case 407:                                   /* Proxy-Auth required */
        /** NOTE: As easy implementation, we support only BASIC scheme
            and ignore realm. */
        /* If proxy_auth_type is PROXY_AUTH_BASIC and get
         this result code, authentication was failed. */
        if (proxy_auth_type != PROXY_AUTH_NONE) {
            return START_ERROR;
        }
        auth_what = (result == 401) ? auth_www : auth_proxy;
        do {
            if ( line_input(s, buf, sizeof(buf)) ) {
                break;
            }
            downcase(buf);
            if (expect(buf, auth_what)) {
                /* parse type and realm */
                char *scheme, *realm;
                scheme = cut_token(buf, " ");
                realm = cut_token(scheme, " ");
                if ( scheme == NULL || realm == NULL ) {
                    return START_ERROR;         /* fail */
                }
                /* check supported auth type */
                if (expect(scheme, "basic")) {
                    proxy_auth_type = PROXY_AUTH_BASIC;
                } 
            }
        } while (strcmp(buf,"\r\n") != 0);
        if ( proxy_auth_type == PROXY_AUTH_NONE ) {
            return START_ERROR;
        } else {
            return START_RETRY;
        }

    default:
        /* Not allowed */
        return START_ERROR;
    }
    /* skip to end of response header */
    do {
        if ( line_input(s, buf, sizeof(buf) ) ) {
            return START_ERROR;
        }
    } while ( strcmp(buf,"\r\n") != 0 );

    return START_OK;
}


BOOL httpconnect::ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport) 
{
	int tmp_port;
	TCHAR *colonpos = _tcschr(display, L':');
    if (hostlen < (int)_tcslen(display))
        return FALSE;

    if (colonpos == NULL)
	{
		// No colon -- use default port number
        tmp_port = HTTP_PORT;
		_tcsncpy(phost, display, MAX_HOST_NAME_LEN);
	}
	else
	{
		_tcsncpy(phost, display, colonpos - display);
		phost[colonpos - display] = L'\0';
		if (colonpos[1] == L':') {
			// Two colons -- interpret as a port number
			if (_stscanf(colonpos + 2, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;
		}
		else
		{
			// One colon -- interpret as a display number or port number
			if (_stscanf(colonpos + 1, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;

			// RealVNC method - If port < 100 interpret as display number else as Port number
			if (tmp_port < 100)
				tmp_port += HTTP_PORT;
		}
	}
    *pport = tmp_port;
    return TRUE;
}

BOOL httpconnect::ParseDisplay2(LPTSTR display, LPTSTR phost, int hostlen, int *pport) 
{
	int tmp_port;
	TCHAR *colonpos = _tcschr(display, L':');
    if (hostlen < (int)_tcslen(display))
        return FALSE;

    if (colonpos == NULL)
	{
		// No colon -- use default port number
        tmp_port = HTTPS_PORT;
		_tcsncpy(phost, display, MAX_HOST_NAME_LEN);
	}
	else
	{
		_tcsncpy(phost, display, colonpos - display);
		phost[colonpos - display] = L'\0';
		if (colonpos[1] == L':') {
			// Two colons -- interpret as a port number
			if (_stscanf(colonpos + 2, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;
		}
		else
		{
			// One colon -- interpret as a display number or port number
			if (_stscanf(colonpos + 1, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;

			// RealVNC method - If port < 100 interpret as display number else as Port number
			if (tmp_port < 100)
				tmp_port += HTTPS_PORT;
		}
	}
    *pport = tmp_port;
    return TRUE;
}

/** Main of program **/
SOCKET
httpconnect::mainconnect( int meth,char *server,char *remoteserver, int remoteport)
{
	int ret;
    SOCKET remote;
    WSADATA wsadata;
    WSAStartup( 0x101, &wsadata);

	//set_relay( METHOD_SOCKS, "24.125.118.146:26836");
	/*	f_debug++;
	set_relay(METHOD_HTTP,"proxy.skynet.be:8080");
	local_type=1;
	local_port=1111;
	dest_host="uvnc.com";
	dest_port=443;*/


	f_debug++;
	local_type = LOCAL_SOCKET;
    local_port = 1111;
	strcpy(dest_host,remoteserver);
	dest_port=remoteport;
	set_relay( meth, server );


retry:

    /* make connection */
    if ( relay_method == METHOD_DIRECT ) {
        remote = open_connection (dest_host, dest_port);
        if ( remote == SOCKET_ERROR )
		{
			return 0;
		}
    } else {
        remote = open_connection (relay_host, relay_port);
        if ( remote == SOCKET_ERROR )
		{
			return 0;
		}
    }

    if (relay_method == METHOD_SOCKS &&
        socks_resolve == RESOLVE_LOCAL &&
        local_resolve (dest_host, &dest_addr) < 0) {
		return 0;
    }

    /** relay negociation **/
    switch ( relay_method ) {
    case METHOD_SOCKS:
        if ( ((socks_version == 5) && (begin_socks5_relay(remote) < 0)) ||
             ((socks_version == 4) && (begin_socks4_relay(remote) < 0)) )
		{
			return 0;
		}
        break;

    case METHOD_HTTP:
        ret = begin_http_relay(remote);
        switch (ret) {
        case START_ERROR:
            closesocket(remote);
			return 0;
        case START_OK:
            break;
        case START_RETRY:
            /* retry with authentication */
           closesocket(remote);
            goto retry;
        }
        break;
    }
	return remote;
}


SOCKET 
httpconnect::Get_https_socket(char *port, char *host)
{
	TCHAR proxy[512];
	memset (proxy,0,sizeof(proxy));
	TCHAR * pch=NULL;
	long ProxyEnable=0;
	pfnWinHttpGetIEProxyConfig pWHGIEPC = NULL;
	HMODULE hModWH=NULL;
	//New function, ask current user proxy
	if ((hModWH = LoadLibrary("winhttp.dll"))) {
	pWHGIEPC = (pfnWinHttpGetIEProxyConfig) (GetProcAddress(hModWH, "WinHttpGetIEProxyConfigForCurrentUser"));
	}
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG MyProxyConfig;
	if(!pWHGIEPC(&MyProxyConfig))
	{
		ProxyEnable=false;
		return 0;
	}
	else
	{
		ProxyEnable=true;
	}
	if (hModWH) FreeLibrary(hModWH);
	if(NULL != MyProxyConfig.lpszAutoConfigUrl)
		{
			GlobalFree(MyProxyConfig.lpszAutoConfigUrl);
		}
	if(NULL != MyProxyConfig.lpszProxyBypass)
		{
			GlobalFree(MyProxyConfig.lpszProxyBypass);
		}
	if(NULL != MyProxyConfig.lpszProxy)
		{
			WideCharToMultiByte( CP_ACP, 0, MyProxyConfig.lpszProxy,-1,proxy,512,NULL,NULL);
			GlobalFree(MyProxyConfig.lpszProxy);
		}

	TCHAR remotehost[256];
	int remoteport=0;
	TCHAR remotehost2[256];
	int remoteport2=0;
	TCHAR tempchar1[256];
	TCHAR tempchar2[256];
	_tcscpy(tempchar1,"");
	_tcscpy(tempchar2,"");

	if (strlen(proxy)!=0)
	{
		pch = _tcstok (proxy,";");
	
		if (_tcsstr(pch,"http=")!=NULL)
		{
			_tcscpy(tempchar1,pch);
		}
		if (_tcsstr(pch,"https=")!=NULL)
		{
			_tcscpy(tempchar2,pch);
		}
		if (_tcsstr(pch,"=")==NULL)
		{
			_tcscpy(tempchar2,pch);
		}
	}

	while (pch != NULL)
		{
			pch = _tcstok (NULL, ";");
			if (pch != NULL)
			{
				if (_tcsstr(pch,"http=")!=NULL)
					{
						_tcscpy(tempchar1,pch+5);
					}
				if (_tcsstr(pch,"https=")!=NULL)
					{
						_tcscpy(tempchar2,pch+6);
					}

			}
	}
	ParseDisplay(tempchar1, remotehost, 255, &remoteport);
	ParseDisplay2(tempchar2, remotehost2, 255, &remoteport2);
	SOCKET mysock=0;

	if (remoteport2!=1080)
		{
			mysock=mainconnect(METHOD_HTTP,tempchar2,host,atoi(port));
		}
	else
		{
			mysock=mainconnect(METHOD_SOCKS,tempchar2,host,atoi(port));
		}

	return mysock;
}
