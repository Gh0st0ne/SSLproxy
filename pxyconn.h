/*
 * SSLsplit - transparent SSL/TLS interception
 * Copyright (c) 2009-2016, Daniel Roethlisberger <daniel@roe.ch>
 * All rights reserved.
 * http://www.roe.ch/SSLsplit
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PXYCONN_H
#define PXYCONN_H
#include "proxy.h"

#include "opts.h"
#include "attrib.h"
#include "pxythrmgr.h"
#include "log.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <event2/event.h>
#include <event2/util.h>

typedef struct pxy_conn_desc {
	struct bufferevent *bev;
	SSL *ssl;
	unsigned int closed : 1;
} pxy_conn_desc_t;

#ifdef HAVE_LOCAL_PROCINFO
/* local process data - filled in iff pid != -1 */
typedef struct pxy_conn_lproc_desc {
	struct sockaddr_storage srcaddr;
	socklen_t srcaddrlen;

	pid_t pid;
	uid_t uid;
	gid_t gid;

	/* derived log strings */
	char *exec_path;
	char *user;
	char *group;
} pxy_conn_lproc_desc_t;
#endif /* HAVE_LOCAL_PROCINFO */

typedef struct pxy_conn_ctx pxy_conn_ctx_t;

/* actual proxy connection state consisting of two connection descriptors,
 * connection-wide state and the specs and options */
typedef struct pxy_conn_ctx {
	/* per-connection state */
	struct pxy_conn_desc src;
	unsigned int src_eof : 1;
	struct pxy_conn_desc dst;
	unsigned int dst_eof : 1;

	struct pxy_conn_desc e2src;
	unsigned int e2src_eof : 1;

	struct pxy_conn_desc e2dst;
	unsigned int e2dst_eof : 1;
//	unsigned int e2dst_assigned;
	
	struct pxy_conn_ctx *parent_ctx;
	struct pxy_conn_ctx *child_ctx;

	/* status flags */
	unsigned int immutable_cert : 1;  /* 1 if the cert cannot be changed */
	unsigned int generated_cert : 1;     /* 1 if we generated a new cert */
	unsigned int connected : 1;       /* 0 until both ends are connected */
	unsigned int dst_connected : 1;       /* 0 until both ends are connected */
	unsigned int e2src_connected : 1;       /* 0 until both ends are connected */
	unsigned int seen_req_header : 1; /* 0 until request header complete */
	unsigned int seen_resp_header : 1;  /* 0 until response hdr complete */
	unsigned int sent_http_conn_close : 1;   /* 0 until Conn: close sent */
	unsigned int passthrough : 1;      /* 1 if SSL passthrough is active */
	unsigned int ocsp_denied : 1;                /* 1 if OCSP was denied */
	unsigned int enomem : 1;                       /* 1 if out of memory */
	unsigned int sni_peek_retries : 6;       /* max 64 SNI parse retries */
	unsigned int clienthello_search : 1;       /* 1 if waiting for hello */
	unsigned int clienthello_found : 1;      /* 1 if conn upgrade to SSL */

	unsigned int initialized : 1;

	/* server name indicated by client in SNI TLS extension */
	char *sni;

	/* log strings from socket */
	char *srchost_str;
	char *srcport_str;
	char *dsthost_str;
	char *dstport_str;

	/* log strings from HTTP request */
	char *http_method;
	char *http_uri;
	char *http_host;
	char *http_content_type;

	/* log strings from HTTP response */
	char *http_status_code;
	char *http_status_text;
	char *http_content_length;

	/* log strings related to SSL */
	char *ssl_names;
	char *origcrtfpr;
	char *usedcrtfpr;

#ifdef HAVE_LOCAL_PROCINFO
	/* local process information */
	pxy_conn_lproc_desc_t lproc;
#endif /* HAVE_LOCAL_PROCINFO */

	/* content log context */
	log_content_ctx_t *logctx;

	/* store fd and fd event while connected is 0 */
	evutil_socket_t fd;
	struct event *ev;

	/* original destination address, family and certificate */
	struct sockaddr_storage addr;
	socklen_t addrlen;
	int af;
	X509 *origcrt;

	/* references to event base and configuration */
	struct event_base *evbase;
	struct evdns_base *dnsbase;
	int thridx;
	pxy_thrmgr_ctx_t *thrmgr;
	proxyspec_t *spec;
	opts_t *opts;
	
	proxy_conn_meta_ctx_t *mctx;
} pxy_conn_ctx_t;

pxy_conn_ctx_t *
pxy_conn_setup(evutil_socket_t, struct sockaddr *, int,
                    proxy_conn_meta_ctx_t *)
                    NONNULL(2,4);

int
my_pthread_mutex_destroy(pthread_mutex_t *__mutex);

int
my_pthread_mutex_lock(pthread_mutex_t *__mutex);

void
my_pthread_mutex_unlock(pthread_mutex_t *__mutex);

#endif /* !PXYCONN_H */

/* vim: set noet ft=c: */
