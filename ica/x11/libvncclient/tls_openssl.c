/*
 *  Copyright (C) 2012 Philip Van Hoof <philip@codeminded.be>
 *  Copyright (C) 2009 Vic Lee.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfbclient.h>
#include <errno.h>

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include <pthread.h>

#include "tls.h"

static rfbBool rfbTLSInitialized = FALSE;
static pthread_mutex_t *mutex_buf = NULL;

struct CRYPTO_dynlock_value {
	pthread_mutex_t mutex;
};

static void locking_function(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&mutex_buf[n]);
	else
		pthread_mutex_unlock(&mutex_buf[n]);
}

static unsigned long id_function(void)
{
	return ((unsigned long) pthread_self());
}

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line)
{
	struct CRYPTO_dynlock_value *value;

	value = (struct CRYPTO_dynlock_value *)
		malloc(sizeof(struct CRYPTO_dynlock_value));
	if (!value)
		goto err;
	pthread_mutex_init(&value->mutex, NULL);

	return value;

err:
	return (NULL);
}

static void dyn_lock_function (int mode, struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&l->mutex);
	else
		pthread_mutex_unlock(&l->mutex);
}


static void
dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
	pthread_mutex_destroy(&l->mutex);
	free(l);
}


static int
ssl_errno (SSL *ssl, int ret)
{
	switch (SSL_get_error (ssl, ret)) {
	case SSL_ERROR_NONE:
		return 0;
	case SSL_ERROR_ZERO_RETURN:
		/* this one does not map well at all */
		//d(printf ("ssl_errno: SSL_ERROR_ZERO_RETURN\n"));
		return EINVAL;
	case SSL_ERROR_WANT_READ:   /* non-fatal; retry */
	case SSL_ERROR_WANT_WRITE:  /* non-fatal; retry */
		//d(printf ("ssl_errno: SSL_ERROR_WANT_[READ,WRITE]\n"));
		return EAGAIN;
	case SSL_ERROR_SYSCALL:
		//d(printf ("ssl_errno: SSL_ERROR_SYSCALL\n"));
		return EINTR;
	case SSL_ERROR_SSL:
		//d(printf ("ssl_errno: SSL_ERROR_SSL  <-- very useful error...riiiiight\n"));
		return EINTR;
	default:
		//d(printf ("ssl_errno: default error\n"));
		return EINTR;
	}
}

static rfbBool
InitializeTLS(void)
{
  int i;

  if (rfbTLSInitialized) return TRUE;

  mutex_buf = malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
  if (mutex_buf == NULL) {
    rfbClientLog("Failed to initialized OpenSSL: memory.\n");
    return (-1);
  }

  for (i = 0; i < CRYPTO_num_locks(); i++)
    pthread_mutex_init(&mutex_buf[i], NULL);

  CRYPTO_set_locking_callback(locking_function);
  CRYPTO_set_id_callback(id_function);
  CRYPTO_set_dynlock_create_callback(dyn_create_function);
  CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
  CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
  SSL_load_error_strings();
  SSLeay_add_ssl_algorithms();
  RAND_load_file("/dev/urandom", 1024);

  rfbClientLog("OpenSSL initialized.\n");
  rfbTLSInitialized = TRUE;
  return TRUE;
}

static int
ssl_verify (int ok, X509_STORE_CTX *ctx)
{
  unsigned char md5sum[16], fingerprint[40], *f;
  rfbClient *client;
  char *prompt, *cert_str;
  int err, i;
  unsigned int md5len;
  //char buf[257];
  X509 *cert;
  SSL *ssl;

  if (ok)
    return TRUE;

  ssl = X509_STORE_CTX_get_ex_data (ctx, SSL_get_ex_data_X509_STORE_CTX_idx ());

  client = SSL_CTX_get_app_data (ssl->ctx);

  cert = X509_STORE_CTX_get_current_cert (ctx);
  err = X509_STORE_CTX_get_error (ctx);

  /* calculate the MD5 hash of the raw certificate */
  md5len = sizeof (md5sum);
  X509_digest (cert, EVP_md5 (), md5sum, &md5len);
  for (i = 0, f = fingerprint; i < 16; i++, f += 3)
    sprintf ((char *) f, "%.2x%c", md5sum[i], i != 15 ? ':' : '\0');

#define GET_STRING(name) X509_NAME_oneline (name, buf, 256)

  /* TODO: Don't just ignore certificate checks

   fingerprint = key to check in db

   GET_STRING (X509_get_issuer_name (cert));
   GET_STRING (X509_get_subject_name (cert));
   cert->valid (bool: GOOD or BAD) */

  ok = TRUE;

  return ok;
}

static int sock_read_ready(SSL *ssl, uint32_t ms)
{
	int r = 0;
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);

	FD_SET(SSL_get_fd(ssl), &fds);

	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	
	r = select (SSL_get_fd(ssl) + 1, &fds, NULL, NULL, &tv); 

	return r;
}

static int wait_for_data(SSL *ssl, int ret, int timeout)
{
  struct timeval tv;
  fd_set fds;
  int err;
  int retval = 1;

  err = SSL_get_error(ssl, ret);
	
  switch(err)
  {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
      ret = sock_read_ready(ssl, timeout*1000);
			
      if (ret == -1) {
        retval = 2;
      }
				
      break;
      default:
      retval = 3;
      break;
   }
	
  ERR_clear_error();
				
  return retval;
}

static SSL *
open_ssl_connection (rfbClient *client, int sockfd, rfbBool anonTLS)
{
  SSL_CTX *ssl_ctx = NULL;
  SSL *ssl = NULL;
  int n, finished = 0;
  BIO *sbio;

  ssl_ctx = SSL_CTX_new (SSLv23_client_method ());
  SSL_CTX_set_default_verify_paths (ssl_ctx);
  SSL_CTX_set_verify (ssl_ctx, SSL_VERIFY_NONE, &ssl_verify);
  ssl = SSL_new (ssl_ctx);

  /* TODO: finetune this list, take into account anonTLS bool */
  SSL_set_cipher_list(ssl, "ALL");

  SSL_set_fd (ssl, sockfd);
  SSL_CTX_set_app_data (ssl_ctx, client);

  do
  {
    n = SSL_connect(ssl);
		
    if (n != 1) 
    {
      if (wait_for_data(ssl, n, 1) != 1) 
      {
        finished = 1; 
        if (ssl->ctx)
          SSL_CTX_free (ssl->ctx);
        SSL_free(ssl);
        SSL_shutdown (ssl);

        return NULL;
      }
    }
  } while( n != 1 && finished != 1 );

  return ssl;
}


static rfbBool
InitializeTLSSession(rfbClient* client, rfbBool anonTLS)
{
  int ret;

  if (client->tlsSession) return TRUE;

  client->tlsSession = open_ssl_connection (client, client->sock, anonTLS);

  if (!client->tlsSession)
    return FALSE;

  rfbClientLog("TLS session initialized.\n");

  return TRUE;
}

static rfbBool
SetTLSAnonCredential(rfbClient* client)
{
  rfbClientLog("TLS anonymous credential created.\n");
  return TRUE;
}

static rfbBool
HandshakeTLS(rfbClient* client)
{
  int timeout = 15;
  int ret;

return TRUE;

  while (timeout > 0 && (ret = SSL_do_handshake(client->tlsSession)) < 0)
  {
    if (ret != -1)
    {
      rfbClientLog("TLS handshake blocking.\n");
      sleep(1);
      timeout--;
      continue;
    }
    rfbClientLog("TLS handshake failed: -.\n");
    FreeTLS(client);
    return FALSE;
  }

  if (timeout <= 0)
  {
    rfbClientLog("TLS handshake timeout.\n");
    FreeTLS(client);
    return FALSE;
  }

  rfbClientLog("TLS handshake done.\n");
  return TRUE;
}

/* VeNCrypt sub auth. 1 byte auth count, followed by count * 4 byte integers */
static rfbBool
ReadVeNCryptSecurityType(rfbClient* client, uint32_t *result)
{
    uint8_t count=0;
    uint8_t loop=0;
    uint8_t flag=0;
    uint32_t tAuth[256], t;
    char buf1[500],buf2[10];
    uint32_t authScheme;

    if (!ReadFromRFBServer(client, (char *)&count, 1)) return FALSE;

    if (count==0)
    {
        rfbClientLog("List of security types is ZERO. Giving up.\n");
        return FALSE;
    }

    if (count>sizeof(tAuth))
    {
        rfbClientLog("%d security types are too many; maximum is %d\n", count, sizeof(tAuth));
        return FALSE;
    }

    rfbClientLog("We have %d security types to read\n", count);
    authScheme=0;
    /* now, we have a list of available security types to read ( uint8_t[] ) */
    for (loop=0;loop<count;loop++)
    {
        if (!ReadFromRFBServer(client, (char *)&tAuth[loop], 4)) return FALSE;
        t=rfbClientSwap32IfLE(tAuth[loop]);
        rfbClientLog("%d) Received security type %d\n", loop, t);
        if (flag) continue;
        if (t==rfbVeNCryptTLSNone ||
            t==rfbVeNCryptTLSVNC ||
            t==rfbVeNCryptTLSPlain ||
            t==rfbVeNCryptX509None ||
            t==rfbVeNCryptX509VNC ||
            t==rfbVeNCryptX509Plain)
        {
            flag++;
            authScheme=t;
            rfbClientLog("Selecting security type %d (%d/%d in the list)\n", authScheme, loop, count);
            /* send back 4 bytes (in original byte order!) indicating which security type to use */
            if (!WriteToRFBServer(client, (char *)&tAuth[loop], 4)) return FALSE;
        }
        tAuth[loop]=t;
    }
    if (authScheme==0)
    {
        memset(buf1, 0, sizeof(buf1));
        for (loop=0;loop<count;loop++)
        {
            if (strlen(buf1)>=sizeof(buf1)-1) break;
            snprintf(buf2, sizeof(buf2), (loop>0 ? ", %d" : "%d"), (int)tAuth[loop]);
            strncat(buf1, buf2, sizeof(buf1)-strlen(buf1)-1);
        }
        rfbClientLog("Unknown VeNCrypt authentication scheme from VNC server: %s\n",
               buf1);
        return FALSE;
    }
    *result = authScheme;
    return TRUE;
}

rfbBool
HandleAnonTLSAuth(rfbClient* client)
{
  if (!InitializeTLS() || !InitializeTLSSession(client, TRUE)) return FALSE;

  if (!SetTLSAnonCredential(client)) return FALSE;

  if (!HandshakeTLS(client)) return FALSE;

  return TRUE;
}

rfbBool
HandleVeNCryptAuth(rfbClient* client)
{
  uint8_t major, minor, status;
  uint32_t authScheme;
  rfbBool anonTLS;
//  gnutls_certificate_credentials_t x509_cred = NULL;
  int ret;

  if (!InitializeTLS()) return FALSE;

  /* Read VeNCrypt version */
  if (!ReadFromRFBServer(client, (char *)&major, 1) ||
      !ReadFromRFBServer(client, (char *)&minor, 1))
  {
    return FALSE;
  }
  rfbClientLog("Got VeNCrypt version %d.%d from server.\n", (int)major, (int)minor);

  if (major != 0 && minor != 2)
  {
    rfbClientLog("Unsupported VeNCrypt version.\n");
    return FALSE;
  }

  if (!WriteToRFBServer(client, (char *)&major, 1) ||
      !WriteToRFBServer(client, (char *)&minor, 1) ||
      !ReadFromRFBServer(client, (char *)&status, 1))
  {
    return FALSE;
  }

  if (status != 0)
  {
    rfbClientLog("Server refused VeNCrypt version %d.%d.\n", (int)major, (int)minor);
    return FALSE;
  }

  if (!ReadVeNCryptSecurityType(client, &authScheme)) return FALSE;
  if (!ReadFromRFBServer(client, (char *)&status, 1) || status != 1)
  {
    rfbClientLog("Server refused VeNCrypt authentication %d (%d).\n", authScheme, (int)status);
    return FALSE;
  }
  client->subAuthScheme = authScheme;

  /* Some VeNCrypt security types are anonymous TLS, others are X509 */
  switch (authScheme)
  {
    case rfbVeNCryptTLSNone:
    case rfbVeNCryptTLSVNC:
    case rfbVeNCryptTLSPlain:
      anonTLS = TRUE;
      break;
    default:
      anonTLS = FALSE;
      break;
  }

  /* Get X509 Credentials if it's not anonymous */
  if (!anonTLS)
  {
    rfbCredential *cred;

    if (!client->GetCredential)
    {
      rfbClientLog("GetCredential callback is not set.\n");
      return FALSE;
    }
    cred = client->GetCredential(client, rfbCredentialTypeX509);
    if (!cred)
    {
      rfbClientLog("Reading credential failed\n");
      return FALSE;
    }

    /* TODO: don't just ignore this
    x509_cred = CreateX509CertCredential(cred);
    FreeX509Credential(cred);
    if (!x509_cred) return FALSE; */
  }

  /* Start up the TLS session */
  if (!InitializeTLSSession(client, anonTLS)) return FALSE;

  if (anonTLS)
  {
    if (!SetTLSAnonCredential(client)) return FALSE;
  }
  else
  {
/* TODO: don't just ignore this
     if ((ret = gnutls_credentials_set(client->tlsSession, GNUTLS_CRD_CERTIFICATE, x509_cred)) < 0)
     {
        rfbClientLog("Cannot set x509 credential: %s.\n", gnutls_strerror(ret));
        FreeTLS(client); */
      return FALSE;
      //  }  
  }

  if (!HandshakeTLS(client)) return FALSE;

  /* TODO: validate certificate */

  /* We are done here. The caller should continue with client->subAuthScheme
   * to do actual sub authentication.
   */
  return TRUE;
}

int
ReadFromTLS(rfbClient* client, char *out, unsigned int n)
{
  ssize_t ret;

  ret = SSL_read (client->tlsSession, out, n);

  if (ret >= 0)
    return ret;
  else {
    errno = ssl_errno (client->tlsSession, ret);

    if (errno != EAGAIN) {
      rfbClientLog("Error reading from TLS: -.\n");
    }
  }

  return -1;
}

int
WriteToTLS(rfbClient* client, char *buf, unsigned int n)
{
  unsigned int offset = 0;
  ssize_t ret;

  while (offset < n)
  {

    ret = SSL_write (client->tlsSession, buf + offset, (size_t)(n-offset));

    if (ret < 0)
      errno = ssl_errno (client->tlsSession, ret);

    if (ret == 0) continue;
    if (ret < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      rfbClientLog("Error writing to TLS: -\n");
      return -1;
    }
    offset += (unsigned int)ret;
  }
  return offset;
}

void FreeTLS(rfbClient* client)
{
  int i;

  if (mutex_buf != NULL) {
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);

    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++)
      pthread_mutex_destroy(&mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
  }

  SSL_free(client->tlsSession);
}

