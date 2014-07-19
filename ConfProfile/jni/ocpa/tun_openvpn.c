/*
 * tun_openvpn.c
 *
 *      Author: Dmitry Vorobiev
 */

#include <stdlib.h>
#include <sys/socket.h>
#include "tun_openvpn.h"

openvpn_tun_ctx_t* openvpn_tun_init(jobject jtun_instance) {
	openvpn_tun_ctx_t* ctx = malloc(sizeof(openvpn_tun_ctx_t));
	if(ctx == NULL) {
		return NULL;
	}

	int fds[2];
	if(socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds) != 0) {
		free(ctx);
		return NULL;
	}

	common_tun_set((common_tun_ctx_t*) ctx, jtun_instance);

	ctx->common.local_fd = fds[0];
	ctx->common.remote_fd = fds[1];

	return ctx;
}

void openvpn_tun_deinit(openvpn_tun_ctx_t* ctx) {
	if(ctx == NULL) {
		return;
	}

	common_tun_free((common_tun_ctx_t*) ctx);

	free(ctx);
}
