/*
 * tun_openvpn.c
 *
 *      Author: Dmitry Vorobiev
 */

#include <sys/socket.h>
#include "tun_openvpn.h"

openvpn_tun_ctx_t* openvpn_tun_init() {
	openvpn_tun_ctx_t* ctx = malloc(sizeof(openvpn_tun_ctx_t));
	if(ctx == NULL) {
		return NULL;
	}

	int fds[2];
	if(socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds) != 0) {
		free(ctx);
		return NULL;
	}

	ctx->common.local_fd = fds[0];
	ctx->common.remote_fd = fds[1];
	ctx->common.masquerade4 = 0;
	ctx->common.use_masquerade4 = false;
	memset(ctx->common.masquerade6, 0, sizeof(ctx->common.masquerade6));
	ctx->common.use_masquerade6 = false;
	ctx->common.send_func = common_tun_send;
	ctx->common.recv_func = common_tun_recv;
	ctx->common.router_ctx = NULL;

	return ctx;
}

void openvpn_tun_deinit(openvpn_tun_ctx_t* ctx) {
	if(ctx == NULL) {
		return;
	}

	if(ctx->common.local_fd != -1) {
		shutdown(ctx->common.local_fd, SHUT_RDWR);
	}

	if(ctx->common.remote_fd != -1) {
		shutdown(ctx->common.remote_fd, SHUT_RDWR);
	}

	ctx->common.masquerade4 = 0;
	ctx->common.use_masquerade4 = false;
	memset(ctx->common.masquerade6, 0, sizeof(ctx->common.masquerade6));
	ctx->common.use_masquerade6 = false;
	ctx->common.send_func = NULL;
	ctx->common.recv_func = NULL;
	ctx->common.router_ctx = NULL;

	free(ctx);
}
