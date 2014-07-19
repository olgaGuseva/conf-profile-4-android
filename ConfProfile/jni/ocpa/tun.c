/*
 * tun.c
 *
 *      Author: Dmitry Vorobiev
 */

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "android_log_utils.h"
#include "android_jni.h"
#include "protoheaders.h"
#include "tun.h"
#include "router.h"
#include "iputils.h"

#define LOG_TAG "tun.c"

void common_tun_set(common_tun_ctx_t* ctx, jobject jtun_instance) {
	if(ctx == NULL) {
		return;
	}

	ctx->local_fd = UNDEFINED_FD;
	ctx->remote_fd = UNDEFINED_FD;
	ctx->masquerade4 = 0;
	memset(ctx->masquerade6, 0, sizeof(ctx->masquerade6));
	ctx->use_masquerade4 = false;
	ctx->use_masquerade6 = false;
	ctx->send_func = common_tun_send;
	ctx->recv_func = common_tun_recv;
	ctx->router_ctx = NULL;

	ctx->bytes_in = 0;
	ctx->bytes_out = 0;

	if(jtun_instance != NULL) {
		JNIEnv* jnienv;
		bool need_detach = androidjni_attach_thread(&jnienv);
		ctx->jni._tun_instance = (*jnienv)->NewGlobalRef(jnienv, jtun_instance);

		jobject clazz = (*jnienv)->FindClass(jnienv, JNI_PACKAGE_STRING "/RouterLoop");

		if(clazz != NULL) {
			ctx->jni._method_protect = (*jnienv)->GetMethodID(jnienv, clazz, "protect", "(I)Z");
		}

		if(need_detach) {
			androidjni_detach_thread();
		}
	}

	ctx->pcap_output = NULL;
}

void common_tun_free(common_tun_ctx_t* ctx) {
	if(ctx == NULL) {
		return;
	}

	if(ctx->local_fd != UNDEFINED_FD) {
		shutdown(ctx->local_fd, SHUT_RDWR);
	}

	if(ctx->remote_fd != UNDEFINED_FD) {
		shutdown(ctx->remote_fd, SHUT_RDWR);
	}

	ctx->masquerade4 = 0;
	ctx->use_masquerade4 = false;
	memset(ctx->masquerade6, 0, sizeof(ctx->masquerade6));
	ctx->use_masquerade6 = false;
	ctx->send_func = NULL;
	ctx->recv_func = NULL;
	ctx->router_ctx = NULL;

	ctx->bytes_in = 0;
	ctx->bytes_out = 0;

	if(ctx->jni._tun_instance != NULL) {
		JNIEnv* jnienv;
		bool need_detach = androidjni_attach_thread(&jnienv);

		(*jnienv)->DeleteGlobalRef(jnienv, ctx->jni._tun_instance);

		if(need_detach) {
			androidjni_detach_thread();
		}

		ctx->jni._tun_instance = NULL;
	}
	ctx->jni._method_protect = NULL;

	pcap_output_destroy(ctx->pcap_output);
	ctx->pcap_output = NULL;
}

ssize_t common_tun_send(intptr_t tun_ctx, uint8_t* buff, int len) {
	if(tun_ctx == (intptr_t) NULL) {
		errno = EBADF;
		return -1;
	}

	common_tun_ctx_t* ctx = (common_tun_ctx_t*) tun_ctx;

	//start capture
	pcap_output_write(ctx->pcap_output, buff, 0, len);
	//end capture

	ctx->bytes_out += len;

	return write(ctx->local_fd, buff, len);
}

ssize_t common_tun_recv(intptr_t tun_ctx, uint8_t* buff, int len) {
	if(tun_ctx == (intptr_t) NULL) {
		errno = EBADF;
		return -1;
	}

	common_tun_ctx_t* ctx = (common_tun_ctx_t*) tun_ctx;

	int res = read_ip_packet(ctx->local_fd, buff, len);
	if(res < 0) {
		return res;
	}

	ctx->bytes_in += res;

	//start capture
	pcap_output_write(ctx->pcap_output, buff, 0, res);
	//end capture

	pthread_rwlock_rdlock(ctx->router_ctx->rwlock4);
	if((buff[0] & 0xf0) == 0x40 && ctx->router_ctx->dev_tun_ctx.use_masquerade4) {
		LOGE(LOG_TAG, "Before masquerading back:");
		log_dump_packet(LOG_TAG, buff, res);

		ip4_header* hdr = (ip4_header*) buff;
		hdr->daddr = htonl(ctx->router_ctx->dev_tun_ctx.masquerade4);
		ip4_calc_ip_checksum(buff, res);

		switch(hdr->protocol) {
		case IPPROTO_TCP: {
			ip4_calc_tcp_checksum(buff, res);
			break;
		}
		default: {
			LOGE(LOG_TAG, "Can't calculate checksum for protocol %d", hdr->protocol);
			break;
		}
		}
		LOGE(LOG_TAG, "Masquerading back:");
		log_dump_packet(LOG_TAG, buff, res);
	} else if((buff[0] & 0xf0) == 0x60 && ctx->router_ctx->dev_tun_ctx.use_masquerade6) {

	}

	ctx->router_ctx->dev_tun_ctx.bytes_out += res;

	res = write(ctx->router_ctx->dev_tun_ctx.local_fd, buff, res);
	pthread_rwlock_unlock(ctx->router_ctx->rwlock4);
	return res;
}

ssize_t common_tun_pipe(intptr_t tun_ctx, uint8_t* buff, int len) {
	if(tun_ctx == (intptr_t) NULL) {
		errno = EBADF;
		return -1;
	}

	common_tun_ctx_t* ctx = (common_tun_ctx_t*) tun_ctx;

	int res = read_ip_packet(ctx->local_fd, buff, len);
	if(res < 0) {
		return res;
	}

	res = write(ctx->router_ctx->dev_tun_ctx.local_fd, buff, res);
	return res;
}

