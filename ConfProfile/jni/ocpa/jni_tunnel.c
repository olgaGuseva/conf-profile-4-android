/*
 * tun_ipsec.c
 *
 *      Author: S. Martyanov
 */

#include "strongswan.h"
#include "tun_ipsec.h"

JNI_METHOD(IpSecVpnTunnel, initializeCharon, jboolean, jstring jlogfile, jboolean byod, jlong jtunctx) {
	return initialize_library(env, this, androidjni_convert_jstring(env, jlogfile), byod, jtunctx);
}

JNI_METHOD(IpSecVpnTunnel, deinitializeCharon, void)
{
	deinitialize_library(env);
}

JNI_METHOD(IpSecVpnTunnel, initiate, void,
	jstring jtype, jstring jgateway, jstring jusername, jstring jpassword)
{
	char *type, *gateway, *username, *password;

	type = androidjni_convert_jstring(env, jtype);
	gateway = androidjni_convert_jstring(env, jgateway);
	username = androidjni_convert_jstring(env, jusername);
	password = androidjni_convert_jstring(env, jpassword);

	initialize_tunnel(type, gateway, username, password);
}

JNI_METHOD(IpSecVpnTunnel, networkChanged, void, jboolean jdisconnected)
{
	notify_library(jdisconnected);
}

JNI_METHOD(IpSecVpnTunnel, initIpSecTun, jlong) {
	return (jlong) (intptr_t) ipsec_tun_init();
}

JNI_METHOD(IpSecVpnTunnel, deinitIpSecTun, void, jlong jtunctx) {
	ipsec_tun_deinit((ipsec_tun_ctx_t*) (intptr_t) jtunctx);
}
