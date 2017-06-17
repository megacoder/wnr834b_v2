#!/bin/sh


TARGETDIR=../../router/mipsel/target
LINUXDIR=../../linux/linux
APDIR=../../../ap

# copy the "stripped" version

cp $TARGETDIR/lib/modules/2.4.20/kernel/net/ipv4/acos_nat/acos_nat.o .
cp $TARGETDIR/usr/sbin/cli .
cp $TARGETDIR/usr/lib/libnat.so .
cp $TARGETDIR/sbin/bd .
cp $TARGETDIR/usr/sbin/bpa_monitor .
cp $TARGETDIR/usr/sbin/ddnsd .
cp $TARGETDIR/usr/sbin/dnsRedirectReplyd .
cp $TARGETDIR/usr/sbin/email .
cp $TARGETDIR/usr/sbin/ftpc .
cp $TARGETDIR/usr/sbin/heartbeat .
cp $TARGETDIR/usr/sbin/lld2d .
cp $TARGETDIR/etc/wrt54g.small.ico .
cp $TARGETDIR/etc/wrt54g.large.ico .
cp $TARGETDIR/etc/icon.ico .
cp $TARGETDIR/etc/lld2d.conf .
cp $TARGETDIR/usr/sbin/httpd .
cp $TARGETDIR/usr/sbin/pot .
cp $TARGETDIR/sbin/acos_service .
cp $TARGETDIR/usr/lib/libacos_shared.so .
cp $TARGETDIR/usr/sbin/timesync .
cp $TARGETDIR/usr/sbin/upnpd .
cp $TARGETDIR/usr/sbin/wandetect .
cp $TARGETDIR/usr/sbin/wan_debug .
cp $TARGETDIR/usr/sbin/wlanconfigd .
cp $TARGETDIR/usr/sbin/wpsd .
cp $TARGETDIR/usr/lib/libnvram.so .
cp $TARGETDIR/usr/sbin/nvram .
cp $TARGETDIR/usr/lib/libnvram.so .
cp $TARGETDIR/usr/lib/libshared.so .
cp $TARGETDIR/usr/lib/libbcmcrypto.so .
cp $TARGETDIR/usr/lib/libbcm.so .
cp $TARGETDIR/usr/sbin/telnetenabled .
cp $TARGETDIR/usr/sbin/outputimage .
cp -R $TARGETDIR/www/ .
cp $TARGETDIR/lib/libc.so.6 .

