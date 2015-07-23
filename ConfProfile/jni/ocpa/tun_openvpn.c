/*
 * tun_openvpn.c
 *
 *      Author: Dmitry Vorobiev
 */

#include <stdlib.h>
#include <sys/socket.h>
#include "tun_openvpn.h"
#include "tun_private.h"

openvpn_tun_ctx_t* create_openvpn_tun_ctx(openvpn_tun_ctx_t* ptr, ssize_t len) {
	openvpn_tun_ctx_t* result = create_tun_ctx(ptr, len);
	if(result == NULL) {
		return NULL;
	}

	struct tun_ctx_private_t* ctx = (struct tun_ctx_private_t*) result;

	int fds[2];
	if(socketpair(AF_UNIX, SOCK_DGRAM, PF_UNSPEC, fds) != 0) {
		return ctx->public.ref_put(ctx);
	}

	ctx->local_fd = fds[0];
	ctx->remote_fd = fds[1];

	return result;
}

