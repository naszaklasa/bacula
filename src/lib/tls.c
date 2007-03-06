/*
 * tls.c TLS support functions
 *
 * Author: Landon Fuller <landonf@threerings.net>
 *
 * Version $Id: tls.c,v 1.6.2.1 2005/12/10 13:18:05 kerns Exp $
 *
 * Copyright (C) 2005 Kern Sibbald
 *
 * This file was contributed to the Bacula project by Landon Fuller
 * and Three Rings Design, Inc.
 *
 * Three Rings Design, Inc. has been granted a perpetual, worldwide,
 * non-exclusive, no-charge, royalty-free, irrevocable copyright
 * license to reproduce, prepare derivative works of, publicly
 * display, publicly perform, sublicense, and distribute the original
 * work contributed by Three Rings Design, Inc. and its employees to
 * the Bacula project in source or object form.
 *
 * If you wish to license contributions from Three Rings Design, Inc,
 * under an alternate open source license please contact
 * Landon Fuller <landonf@threerings.net>.
 */
/*
   Copyright (C) 2005 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include <assert.h>

extern time_t watchdog_time;

#ifdef HAVE_TLS /* Is TLS enabled? */

#ifdef HAVE_OPENSSL /* How about OpenSSL? */

/* No anonymous ciphers, no <128 bit ciphers, no export ciphers, no MD5 ciphers */
#define TLS_DEFAULT_CIPHERS "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"

/* Array of mutexes for use with OpenSSL static locking */
static pthread_mutex_t *mutexes;

/* OpenSSL dynamic locking structure */
struct CRYPTO_dynlock_value {
   pthread_mutex_t mutex;
};

/* Are we initialized? */
static int tls_initialized = false;

/* TLS Context Structure */
struct TLS_Context {
   SSL_CTX *openssl;
   TLS_PEM_PASSWD_CB *pem_callback;
   const void *pem_userdata;
};

struct TLS_Connection {
   SSL *openssl;
};

/* post all per-thread openssl errors */
static void openssl_post_errors(int code, const char *errstring)
{
   char buf[512];
   unsigned long sslerr;

   /* Pop errors off of the per-thread queue */
   while((sslerr = ERR_get_error()) != 0) {
      /* Acquire the human readable string */
      ERR_error_string_n(sslerr, (char *) &buf, sizeof(buf));
      Emsg2(M_ERROR, 0, "%s: ERR=%s\n", errstring, buf);
   }
}

/*
 * OpenSSL certificate verification callback.
 * OpenSSL has already performed internal certificate verification.
 * We just report any errors that occured.
 */
static int openssl_verify_peer(int ok, X509_STORE_CTX *store)
{

   if (!ok) {
      X509 *cert = X509_STORE_CTX_get_current_cert(store);
      int depth = X509_STORE_CTX_get_error_depth(store);
      int err = X509_STORE_CTX_get_error(store);
      char issuer[256];
      char subject[256];

      X509_NAME_oneline(X509_get_issuer_name(cert), issuer, 256);
      X509_NAME_oneline(X509_get_subject_name(cert), subject, 256);

      Emsg5(M_ERROR, 0, _("Error with certificate at depth: %d, issuer = %s,"
                          " subject = %s, ERR=%d:%s\n"), depth, issuer,
                          subject, err, X509_verify_cert_error_string(err));

   }

   return ok;
}

/*
 * Default PEM encryption passphrase callback.
 * Returns an empty password.
 */
static int tls_default_pem_callback(char *buf, int size, const void *userdata)
{
   bstrncpy(buf, "", size);
   return (strlen(buf));
}

/* Dispatch user PEM encryption callbacks */
static int openssl_pem_callback_dispatch (char *buf, int size, int rwflag, void *userdata)
{
   TLS_CONTEXT *ctx = (TLS_CONTEXT *) userdata;
   return (ctx->pem_callback(buf, size, ctx->pem_userdata));
}

/*
 * Create a new TLS_CONTEXT instance.
 *  Returns: Pointer to TLS_CONTEXT instance on success
 *           NULL on failure;
 */
TLS_CONTEXT *new_tls_context(const char *ca_certfile, const char *ca_certdir,
                             const char *certfile, const char *keyfile,
                             TLS_PEM_PASSWD_CB *pem_callback,
                             const void *pem_userdata, const char *dhfile,
                             bool verify_peer)
{
   TLS_CONTEXT *ctx;
   BIO *bio;
   DH *dh;

   ctx = (TLS_CONTEXT *) malloc(sizeof(TLS_CONTEXT));

   /* Allocate our OpenSSL TLSv1 Context */
   ctx->openssl = SSL_CTX_new(TLSv1_method());

   if (!ctx->openssl) {
      openssl_post_errors(M_ERROR, _("Error initializing SSL context"));
      goto err;
   }

   /* Set up pem encryption callback */
   if (pem_callback) {
      ctx->pem_callback = pem_callback;
      ctx->pem_userdata = pem_userdata;
   } else {
      ctx->pem_callback = tls_default_pem_callback;
      ctx->pem_userdata = NULL;
   }
   SSL_CTX_set_default_passwd_cb(ctx->openssl, openssl_pem_callback_dispatch);
   SSL_CTX_set_default_passwd_cb_userdata(ctx->openssl, (void *) ctx);

   /*
    * Set certificate verification paths. This requires that at least one
    * value be non-NULL
    */
   if (ca_certfile || ca_certdir) {
      if (!SSL_CTX_load_verify_locations(ctx->openssl, ca_certfile, ca_certdir)) {
         openssl_post_errors(M_ERROR, _("Error loading certificate verification stores"));
         goto err;
      }
   } else if (verify_peer) {
      /* At least one CA is required for peer verification */
      Emsg0(M_ERROR, 0, _("Either a certificate file or a directory must be"
                         " specified as a verification store\n"));
      goto err;
   }

   /*
    * Load our certificate file, if available. This file may also contain a
    * private key, though this usage is somewhat unusual.
    */
   if (certfile) {
      if (!SSL_CTX_use_certificate_chain_file(ctx->openssl, certfile)) {
         openssl_post_errors(M_ERROR, _("Error loading certificate file"));
         goto err;
      }
   }

   /* Load our private key. */
   if (keyfile) {
      if (!SSL_CTX_use_PrivateKey_file(ctx->openssl, keyfile, SSL_FILETYPE_PEM)) {
         openssl_post_errors(M_ERROR, _("Error loading private key"));
         goto err;
      }
   }

   /* Load Diffie-Hellman Parameters. */
   if (dhfile) {
      if (!(bio = BIO_new_file(dhfile, "r"))) {
         openssl_post_errors(M_ERROR, _("Unable to open DH parameters file"));
         goto err;
      }
      dh = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
      BIO_free(bio);
      if (!dh) {
         openssl_post_errors(M_ERROR, _("Unable to load DH parameters from specified file"));
         goto err;
      }
      if (!SSL_CTX_set_tmp_dh(ctx->openssl, dh)) {
         openssl_post_errors(M_ERROR, _("Failed to set TLS Diffie-Hellman parameters"));
         DH_free(dh);
         goto err;
      }
      /* Enable Single-Use DH for Ephemeral Keying */
      SSL_CTX_set_options(ctx->openssl, SSL_OP_SINGLE_DH_USE);
   }

   if (SSL_CTX_set_cipher_list(ctx->openssl, TLS_DEFAULT_CIPHERS) != 1) {
      Emsg0(M_ERROR, 0, _("Error setting cipher list, no valid ciphers available\n"));
      goto err;
   }

   /* Verify Peer Certificate */
   if (verify_peer) {
           /* SSL_VERIFY_FAIL_IF_NO_PEER_CERT has no effect in client mode */
           SSL_CTX_set_verify(ctx->openssl,
                              SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                              openssl_verify_peer);
   }

   return ctx;

err:
   /* Clean up after ourselves */
   if(ctx->openssl) {
      SSL_CTX_free(ctx->openssl);
   }
   free(ctx);
   return NULL;
}

/*
 * Free TLS_CONTEXT instance
 */
void free_tls_context(TLS_CONTEXT *ctx)
{
   SSL_CTX_free(ctx->openssl);
   free(ctx);
}

/*
 * Verifies a list of common names against the certificate
 * commonName attribute.
 *  Returns: true on success
 *           false on failure
 */
bool tls_postconnect_verify_cn(TLS_CONNECTION *tls, alist *verify_list)
{
   SSL *ssl = tls->openssl;
   X509 *cert;
   X509_NAME *subject;
   int auth_success = false;
   char data[256];

   /* Check if peer provided a certificate */
   if (!(cert = SSL_get_peer_certificate(ssl))) {
      Emsg0(M_ERROR, 0, _("Peer failed to present a TLS certificate\n"));
      return false;
   }

   if ((subject = X509_get_subject_name(cert)) != NULL) {
      if (X509_NAME_get_text_by_NID(subject, NID_commonName, data, sizeof(data)) > 0) {
         char *cn;
         /* NULL terminate data */
         data[255] = 0;

         /* Try all the CNs in the list */
         foreach_alist(cn, verify_list) {
            if (strcasecmp(data, cn) == 0) {
               auth_success = true;
            }
         }
      }
   }

   X509_free(cert);
   return auth_success;
}

/*
 * Verifies a peer's hostname against the subjectAltName and commonName
 * attributes.
 *  Returns: true on success
 *           false on failure
 */
bool tls_postconnect_verify_host(TLS_CONNECTION *tls, const char *host)
{
   SSL *ssl = tls->openssl;
   X509 *cert;
   X509_NAME *subject;
   int auth_success = false;
   int extensions;
   char data[256];
   int i, j;


   /* Check if peer provided a certificate */
   if (!(cert = SSL_get_peer_certificate(ssl))) {
      Emsg1(M_ERROR, 0, _("Peer %s failed to present a TLS certificate\n"), host);
      return false;
   }

   /* Check subjectAltName extensions first */
   if ((extensions = X509_get_ext_count(cert)) > 0) {
      for (i = 0; i < extensions; i++) {
         X509_EXTENSION *ext;
         const char *extname;

         ext = X509_get_ext(cert, i);
         extname = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

         if (strcmp(extname, "subjectAltName") == 0) {
            X509V3_EXT_METHOD *method;
            STACK_OF(CONF_VALUE) *val;
            CONF_VALUE *nval;
            void *extstr = NULL;
#if (OPENSSL_VERSION_NUMBER >= 0x0090800FL)
            const unsigned char *ext_value_data;
#else
            unsigned char *ext_value_data;
#endif

            /* Get x509 extension method structure */
            if (!(method = X509V3_EXT_get(ext))) {
               break;
            }

            ext_value_data = ext->value->data;

#if (OPENSSL_VERSION_NUMBER > 0x00907000L)
            if (method->it) {
               /* New style ASN1 */

               /* Decode ASN1 item in data */
               extstr = ASN1_item_d2i(NULL, &ext_value_data, ext->value->length,
                                      ASN1_ITEM_ptr(method->it));
            } else {
               /* Old style ASN1 */

               /* Decode ASN1 item in data */
               extstr = method->d2i(NULL, &ext_value_data, ext->value->length);
            }

#else
            extstr = method->d2i(NULL, &ext_value_data, ext->value->length);
#endif

            /* Iterate through to find the dNSName field(s) */
            val = method->i2v(method, extstr, NULL);

            /* dNSName shortname is "DNS" */
            for (j = 0; j < sk_CONF_VALUE_num(val); j++) {
               nval = sk_CONF_VALUE_value(val, j);
               if (strcmp(nval->name, "DNS") == 0) {
                  if (strcasecmp(nval->name, host) == 0) {
                     auth_success = true;
                     goto success;
                  }
               }
            }
         }
      }
   }

   /* Try verifying against the subject name */
   if (!auth_success) {
      if ((subject = X509_get_subject_name(cert)) != NULL) {
         if (X509_NAME_get_text_by_NID(subject, NID_commonName, data, sizeof(data)) > 0) {
            /* NULL terminate data */
            data[255] = 0;
            if (strcasecmp(data, host) == 0) {
               auth_success = true;
            }
         }
      }
   }


success:
   X509_free(cert);

   return auth_success;
}

/*
 * Create a new TLS_CONNECTION instance.
 *
 * Returns: Pointer to TLS_CONNECTION instance on success
 *          NULL on failure;
 */
TLS_CONNECTION *new_tls_connection (TLS_CONTEXT *ctx, int fd)
{
   BIO *bio;

   /*
    * Create a new BIO and assign the fd.
    * The caller will remain responsible for closing the associated fd
    */
   bio = BIO_new(BIO_s_socket());
   if (!bio) {
      /* Not likely, but never say never */
      openssl_post_errors(M_ERROR, _("Error creating file descriptor-based BIO"));
      return NULL; /* Nothing allocated, nothing to clean up */
   }
   BIO_set_fd(bio, fd, BIO_NOCLOSE);

   /* Allocate our new tls connection */
   TLS_CONNECTION *tls = (TLS_CONNECTION *) malloc(sizeof(TLS_CONNECTION));

   /* Create the SSL object and attach the socket BIO */
   if ((tls->openssl = SSL_new(ctx->openssl)) == NULL) {
      /* Not likely, but never say never */
      openssl_post_errors(M_ERROR, _("Error creating new SSL object"));
      goto err;
   }

   SSL_set_bio(tls->openssl, bio, bio);

   /* Non-blocking partial writes */
   SSL_set_mode(tls->openssl, SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

   return (tls);

err:
   /* Clean up */
   BIO_free(bio);
   SSL_free(tls->openssl);
   free(tls);

   return NULL;
}

/*
 * Free TLS_CONNECTION instance
 */
void free_tls_connection (TLS_CONNECTION *tls)
{
   SSL_free(tls->openssl);
   free(tls);
}

/* Does all the manual labor for tls_bsock_accept() and tls_bsock_connect() */
static inline bool openssl_bsock_session_start(BSOCK *bsock, bool server)
{
   TLS_CONNECTION *tls = bsock->tls;
   int err;
   int fdmax, flags;
   int stat = true;
   fd_set fdset;
   struct timeval tv;

   /* Zero the fdset, we'll set our fd prior to each invocation of select() */
   FD_ZERO(&fdset);
   fdmax = bsock->fd + 1;

   /* Ensure that socket is non-blocking */
   flags = bnet_set_nonblocking(bsock);

   /* start timer */
   bsock->timer_start = watchdog_time;
   bsock->timed_out = 0;

   for (;;) { 
      if (server) {
         err = SSL_accept(tls->openssl);
      } else {
         err = SSL_connect(tls->openssl);
      }

      /* Handle errors */
      switch (SSL_get_error(tls->openssl, err)) {
      case SSL_ERROR_NONE:
         stat = true;
         goto cleanup;
      case SSL_ERROR_ZERO_RETURN:
         /* TLS connection was cleanly shut down */
         openssl_post_errors(M_ERROR, _("Connect failure"));
         stat = false;
         goto cleanup;
      case SSL_ERROR_WANT_READ:
         /* If we timeout of a select, this will be unset */
         FD_SET((unsigned) bsock->fd, &fdset);
         /* Set our timeout */
         tv.tv_sec = 10;
         tv.tv_usec = 0;
         /* Block until we can read */
         select(fdmax, &fdset, NULL, &fdset, &tv);
         break;
      case SSL_ERROR_WANT_WRITE:
         /* If we timeout of a select, this will be unset */
         FD_SET((unsigned) bsock->fd, &fdset);
         /* Set our timeout */
         tv.tv_sec = 10;
         tv.tv_usec = 0;
         /* Block until we can write */
         select(fdmax, NULL, &fdset, &fdset, &tv);
         break;
      default:
         /* Socket Error Occured */
         openssl_post_errors(M_ERROR, _("Connect failure"));
         stat = false;
         goto cleanup;
      }

      if (bsock->timed_out) {
         goto cleanup;
      }
   }

cleanup:
   /* Restore saved flags */
   bnet_restore_blocking(bsock, flags);
   /* Clear timer */
   bsock->timer_start = 0;

   return stat;
}

/*
 * Initiates a TLS connection with the server.
 *  Returns: true on success
 *           false on failure
 */
bool tls_bsock_connect(BSOCK *bsock)
{
   /* SSL_connect(bsock->tls) */
   return (openssl_bsock_session_start(bsock, false));
}

/*
 * Listens for a TLS connection from a client.
 *  Returns: true on success
 *           false on failure
 */
bool tls_bsock_accept(BSOCK *bsock)
{
   /* SSL_accept(bsock->tls) */
   return (openssl_bsock_session_start(bsock, true));
}

/*
 * Shutdown TLS_CONNECTION instance
 */
void tls_bsock_shutdown (BSOCK *bsock)
{
   /*
    * SSL_shutdown must be called twice to fully complete the process -
    * The first time to initiate the shutdown handshake, and the second to
    * receive the peer's reply.
    *
    * However, it is valid to close the SSL connection after the initial
    * shutdown notification is sent to the peer, without waiting for the
    * peer's reply, as long as you do not plan to re-use that particular
    * SSL connection object.
    *
    * Because we do not re-use SSL connection objects, I do not bother
    * calling SSL_shutdown a second time.
    *
    * In addition, if the underlying socket is blocking, SSL_shutdown()
    * will not return until the current stage of the shutdown process has
    * completed or an error has occured. By setting the socket blocking
    * we can avoid the ugly for()/switch()/select() loop.
    */
   int err;
   int flags;

   /* Set socket blocking for shutdown */
   flags = bnet_set_blocking(bsock);

   err = SSL_shutdown(bsock->tls->openssl);

   switch (SSL_get_error(bsock->tls->openssl, err)) {
      case SSL_ERROR_NONE:
         break;
      case SSL_ERROR_ZERO_RETURN:
         /* TLS connection was shut down on us via a TLS protocol-level closure */
         openssl_post_errors(M_ERROR, _("TLS shutdown failure."));
         break;
      default:
         /* Socket Error Occured */
         openssl_post_errors(M_ERROR, _("TLS shutdown failure."));
         break;
   }

   /* Restore saved flags */
   bnet_restore_blocking(bsock, flags);
}

/* Does all the manual labor for tls_bsock_readn() and tls_bsock_writen() */
static inline int openssl_bsock_readwrite(BSOCK *bsock, char *ptr, int nbytes, bool write)
{
   TLS_CONNECTION *tls = bsock->tls;
   int fdmax, flags;
   fd_set fdset;
   struct timeval tv;
   int nleft = 0;
   int nwritten = 0;

   /* Zero the fdset, we'll set our fd prior to each invocation of select() */
   FD_ZERO(&fdset);
   fdmax = bsock->fd + 1;

   /* Ensure that socket is non-blocking */
   flags = bnet_set_nonblocking(bsock);

   /* start timer */
   bsock->timer_start = watchdog_time;
   bsock->timed_out = 0;

   nleft = nbytes;

   while (nleft > 0) { 

      if (write) {
         nwritten = SSL_write(tls->openssl, ptr, nleft);
      } else {
         nwritten = SSL_read(tls->openssl, ptr, nleft);
      }

      /* Handle errors */
      switch (SSL_get_error(tls->openssl, nwritten)) {
      case SSL_ERROR_NONE:
         nleft -= nwritten;
         if (nleft) {
            ptr += nwritten;
         }
         break;
      case SSL_ERROR_ZERO_RETURN:
         /* TLS connection was cleanly shut down */
         openssl_post_errors(M_ERROR, _("TLS read/write failure."));
         goto cleanup;
      case SSL_ERROR_WANT_READ:
         /* If we timeout of a select, this will be unset */
         FD_SET((unsigned) bsock->fd, &fdset);
         tv.tv_sec = 10;
         tv.tv_usec = 0;
         /* Block until we can read */
         select(fdmax, &fdset, NULL, &fdset, &tv);
         break;
      case SSL_ERROR_WANT_WRITE:
         /* If we timeout of a select, this will be unset */
         FD_SET((unsigned) bsock->fd, &fdset);
         tv.tv_sec = 10;
         tv.tv_usec = 0;
         /* Block until we can write */
         select(fdmax, NULL, &fdset, &fdset, &tv);
         break;
      default:
         /* Socket Error Occured */
         openssl_post_errors(M_ERROR, _("TLS read/write failure."));
         goto cleanup;
      }

      /* Everything done? */
      if (nleft == 0) {
         goto cleanup;
      }

      /* Timeout/Termination, let's take what we can get */
      if (bsock->timed_out || bsock->terminated) {
         goto cleanup;
      }
   }

cleanup:
   /* Restore saved flags */
   bnet_restore_blocking(bsock, flags);

   /* Clear timer */
   bsock->timer_start = 0;

   return nbytes - nleft;
}


int tls_bsock_writen(BSOCK *bsock, char *ptr, int32_t nbytes) {
   /* SSL_write(bsock->tls->openssl, ptr, nbytes) */
   return (openssl_bsock_readwrite(bsock, ptr, nbytes, true));
}

int tls_bsock_readn(BSOCK *bsock, char *ptr, int32_t nbytes) {
   /* SSL_read(bsock->tls->openssl, ptr, nbytes) */
   return (openssl_bsock_readwrite(bsock, ptr, nbytes, false));
}

/*
 * Return an OpenSSL thread ID
 *  Returns: thread ID
 *
 */
static unsigned long get_openssl_thread_id (void)
{
   /* Comparison without use of pthread_equal() is mandated by the OpenSSL API */
   return ((unsigned long) pthread_self());
}

/*
 * Allocate a dynamic OpenSSL mutex
 */
static struct CRYPTO_dynlock_value *openssl_create_dynamic_mutex (const char *file, int line)
{
   struct CRYPTO_dynlock_value *dynlock;
   int stat;

   dynlock = (struct CRYPTO_dynlock_value *) malloc(sizeof(struct CRYPTO_dynlock_value));

   if ((stat = pthread_mutex_init(&dynlock->mutex, NULL)) != 0) {
      Emsg1(M_ABORT, 0, _("Unable to init mutex: ERR=%s\n"), strerror(stat));
   }

   return dynlock;
}

static void openssl_update_dynamic_mutex (int mode, struct CRYPTO_dynlock_value *dynlock, const char *file, int line)
{
   if (mode & CRYPTO_LOCK) {
      P(dynlock->mutex);
   } else {
      V(dynlock->mutex);
   }
}

static void openssl_destroy_dynamic_mutex (struct CRYPTO_dynlock_value *dynlock, const char *file, int line)
{
   int stat;

   if ((stat = pthread_mutex_destroy(&dynlock->mutex)) != 0) {
      Emsg1(M_ABORT, 0, _("Unable to destroy mutex: ERR=%s\n"), strerror(stat));
   }

   free(dynlock);
}

/*
 * (Un)Lock a static OpenSSL mutex
 */
static void openssl_update_static_mutex (int mode, int i, const char *file, int line)
{
   if (mode & CRYPTO_LOCK) {
      P(mutexes[i]);
   } else {
      V(mutexes[i]);
   }
}

/*
 * Initialize OpenSSL thread support
 *  Returns: 0 on success
 *           errno on failure
 */
static int openssl_init_threads (void)
{
   int i, numlocks;
   int stat;


   /* Set thread ID callback */
   CRYPTO_set_id_callback(get_openssl_thread_id);

   /* Initialize static locking */
   numlocks = CRYPTO_num_locks();
   mutexes = (pthread_mutex_t *) malloc(numlocks * sizeof(pthread_mutex_t));
   for (i = 0; i < numlocks; i++) {
      if ((stat = pthread_mutex_init(&mutexes[i], NULL)) != 0) {
         Emsg1(M_ERROR, 0, _("Unable to init mutex: ERR=%s\n"), strerror(stat));
         return stat;
      }
   }

   /* Set static locking callback */
   CRYPTO_set_locking_callback(openssl_update_static_mutex);

   /* Initialize dyanmic locking */
   CRYPTO_set_dynlock_create_callback(openssl_create_dynamic_mutex);
   CRYPTO_set_dynlock_lock_callback(openssl_update_dynamic_mutex);
   CRYPTO_set_dynlock_destroy_callback(openssl_destroy_dynamic_mutex);

   return 0;
}

/*
 * Clean up OpenSSL threading support
 */
static void openssl_cleanup_threads (void)
{
   int i, numlocks;
   int stat;

   /* Unset thread ID callback */
   CRYPTO_set_id_callback(NULL);
  
   /* Deallocate static lock mutexes */
   numlocks = CRYPTO_num_locks();
   for (i = 0; i < numlocks; i++) {
      if ((stat = pthread_mutex_destroy(&mutexes[i])) != 0) {
         /* We don't halt execution, reporting the error should be sufficient */
         Emsg1(M_ERROR, 0, _("Unable to destroy mutex: ERR=%s\n"), strerror(stat));
      }
   }

   /* Unset static locking callback */
   CRYPTO_set_locking_callback(NULL);

   /* Free static lock array */
   free(mutexes);

   /* Unset dynamic locking callbacks */
   CRYPTO_set_dynlock_create_callback(NULL);
   CRYPTO_set_dynlock_lock_callback(NULL);
   CRYPTO_set_dynlock_destroy_callback(NULL);
}


/*
 * Seed TLS PRNG
 *  Returns: 1 on success
 *           0 on failure
 */
static int seed_tls_prng (void)
{
   const char *names[]  = { "/dev/urandom", "/dev/random", NULL };
   int i;

   // ***FIXME***
   // Win32 Support
   // Read saved entropy?

   for (i = 0; names[i]; i++) {
      if (RAND_load_file(names[i], 1024) != -1) {
         /* Success */
         return 1;
      }
   }

   /* Fail */
   return 0;
}

/*
 * Save TLS Entropy
 *  Returns: 1 on success
 *           0 on failure
 */
static int save_tls_prng (void)
{
   // ***FIXME***
   // Implement PRNG state save
   return 1;
}

/*
 * Perform global initialization of TLS
 * This function is not thread safe.
 *  Returns: 0 on success
 *           errno on failure
 */
int init_tls (void)
{
   int stat;

   if ((stat = openssl_init_threads()) != 0) {
      Emsg1(M_ABORT, 0, _("Unable to init OpenSSL threading: ERR=%s\n"), strerror(stat));
   }

   /* Load libssl and libcrypto human-readable error strings */
   SSL_load_error_strings();

   /* Register OpenSSL ciphers */
   SSL_library_init();

   if (!seed_tls_prng()) {
      Emsg0(M_ERROR_TERM, 0, _("Failed to seed OpenSSL PRNG\n"));
   }

   tls_initialized = true;

   return stat;
}

/*
 * Perform global cleanup of TLS
 * All TLS connections must be closed before calling this function.
 * This function is not thread safe.
 *  Returns: 0 on success
 *           errno on failure
 */
int cleanup_tls (void)
{
   /*
    * Ensure that we've actually been initialized; Doing this here decreases the
    * complexity of client's termination/cleanup code.
    */
   if (!tls_initialized) {
      return 0;
   }

   if (!save_tls_prng()) {
      Emsg0(M_ERROR, 0, _("Failed to save OpenSSL PRNG\n"));
   }

   openssl_cleanup_threads();

   /* Free libssl and libcrypto error strings */
   ERR_free_strings();

   /* Free memory used by PRNG */
   RAND_cleanup();

   tls_initialized = false;

   return 0;
}

#else /* HAVE_OPENSSL */
# error No TLS implementation available.
#endif /* !HAVE_OPENSSL */

#else

/* Dummy routines */
int init_tls(void) { return 0; }
int cleanup_tls (void) { return 0; }
TLS_CONTEXT *new_tls_context(const char *ca_certfile, const char *ca_certdir,
                             const char *certfile, const char *keyfile,
                             TLS_PEM_PASSWD_CB *pem_callback,
                             const void *pem_userdata, const char *dhfile,
                             bool verify_peer)
{
   return NULL;
}
void free_tls_context(TLS_CONTEXT *ctx) { }

#endif /* HAVE_TLS */
