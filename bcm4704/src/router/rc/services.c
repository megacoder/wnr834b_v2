/*
 * Miscellaneous services
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: services.c,v 1.1.1.1 2010/03/05 07:31:38 reynolds Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>

#define assert(a)

#ifdef __CONFIG_NAT__
static char
*make_var(char *prefix, int index, char *name)
{
	static char buf[100];

	assert(prefix);
	assert(name);

	if (index)
		snprintf(buf, sizeof(buf), "%s%d%s", prefix, index, name);
	else
		snprintf(buf, sizeof(buf), "%s%s", prefix, name);
	return buf;
}
int
start_dhcpd(void)
{
	FILE *fp;
	char name[100];
	char word[32], *next;
	char dhcp_conf_file[128];
	char dhcp_lease_file[128];
	char dhcp_ifnames[128] = "";
	int index;
	char tmp[20];
	int i = 0;
	char *lan_ifname = NULL;

	if (nvram_match("router_disable", "1"))
		return 0;

	/* Setup individual dhcp severs for the bridge and the every unbridged interface */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		snprintf(dhcp_ifnames, sizeof(dhcp_ifnames), "%s %s", dhcp_ifnames, lan_ifname);
	}

	index = 0;
	foreach(word, dhcp_ifnames, next) {

		if (strstr(word, "br0"))
			index = 0;
		else
			index = 1;

		/* Skip interface if no valid config block is found */
		if (index < 0) continue;

		if (nvram_invmatch(make_var("lan", index, "_proto"), "dhcp"))
			continue;

		dprintf("%s %s %s %s\n",
			nvram_safe_get(make_var("lan", index, "_ifname")),
			nvram_safe_get(make_var("dhcp", index, "_start")),
			nvram_safe_get(make_var("dhcp", index, "_end")),
			nvram_safe_get(make_var("lan", index, "_lease")));

		/* Touch leases file */
		sprintf(dhcp_lease_file, "/tmp/udhcpd%d.leases", index);
		if (!(fp = fopen(dhcp_lease_file, "a"))) {
			perror(dhcp_lease_file);
			return errno;
		}
		fclose(fp);

		/* Write configuration file based on current information */
		sprintf(dhcp_conf_file, "/tmp/udhcpd%d.conf", index);
		if (!(fp = fopen(dhcp_conf_file, "w"))) {
			perror(dhcp_conf_file);
			return errno;
		}
		fprintf(fp, "pidfile /var/run/udhcpd%d.pid\n", index);
		fprintf(fp, "start %s\n", nvram_safe_get(make_var("dhcp", index, "_start")));
		fprintf(fp, "end %s\n", nvram_safe_get(make_var("dhcp", index, "_end")));
		fprintf(fp, "interface %s\n", word);
		fprintf(fp, "remaining yes\n");
		fprintf(fp, "lease_file /tmp/udhcpd%d.leases\n", index);
		fprintf(fp, "option subnet %s\n",
			nvram_safe_get(make_var("lan", index, "_netmask")));
		fprintf(fp, "option router %s\n",
			nvram_safe_get(make_var("lan", index, "_ipaddr")));
		fprintf(fp, "option dns %s\n", nvram_safe_get(make_var("lan", index, "_ipaddr")));
		fprintf(fp, "option lease %s\n", nvram_safe_get(make_var("lan", index, "_lease")));
		snprintf(name, sizeof(name), "%s_wins",
			nvram_safe_get(make_var("dhcp", index, "_wins")));
		if (nvram_invmatch(name, ""))
			fprintf(fp, "option wins %s\n", nvram_get(name));
		snprintf(name, sizeof(name), "%s_domain",
			nvram_safe_get(make_var("dhcp", index, "_domain")));
		if (nvram_invmatch(name, ""))
			fprintf(fp, "option domain %s\n", nvram_get(name));
		fclose(fp);

		eval("udhcpd", dhcp_conf_file);
		index++;
	}
	dprintf("done\n");
	return 0;
}

int
stop_dhcpd(void)
{
	char sigusr1[] = "-XX";
	int ret;

/*
* Process udhcpd handles two signals - SIGTERM and SIGUSR1
*
*  - SIGUSR1 saves all leases in /tmp/udhcpd.leases
*  - SIGTERM causes the process to be killed
*
* The SIGUSR1+SIGTERM behavior is what we like so that all current client
* leases will be honorred when the dhcpd restarts and all clients can extend
* their leases and continue their current IP addresses. Otherwise clients
* would get NAK'd when they try to extend/rebind their leases and they
* would have to release current IP and to request a new one which causes
* a no-IP gap in between.
*/
	sprintf(sigusr1, "-%d", SIGUSR1);
	eval("killall", sigusr1, "udhcpd");
	ret = eval("killall", "udhcpd");

	dprintf("done\n");
	return ret;
}

int
start_dns(void)
{
	int ret;
	FILE *fp;
	char dns_ifnames[64] = "";
	char cmdline[128];
	char tmp[20];
	int i = 0;
	char *lan_ifname = NULL;

	if (nvram_match("router_disable", "1"))
		return 0;

	/* Create resolv.conf with empty nameserver list */
	if (!(fp = fopen("/tmp/resolv.conf", "w"))) {
		perror("/tmp/resolv.conf");
		return errno;
	}
	fclose(fp);

	/* Setup interface list for the dns relay */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		snprintf(dns_ifnames, sizeof(dns_ifnames), "%s -i %s", dns_ifnames, lan_ifname);
	}

	/* Start the dns relay */
	snprintf(cmdline, sizeof(cmdline),
		"/usr/sbin/dnsmasq -h -n %s -r /tmp/resolv.conf", dns_ifnames);
	ret = system(cmdline);


	dprintf("done\n");
	return ret;
}

int
stop_dns(void)
{
	int ret = eval("killall", "dnsmasq");

	/* Remove resolv.conf */
	unlink("/tmp/resolv.conf");

	dprintf("done\n");
	return ret;
}
#endif	/* __CONFIG_NAT__ */

int
start_httpd(void)
{
	int ret;

	chdir("/www");
	ret = eval("httpd");
	chdir("/");

	dprintf("done\n");
	return ret;
}

int
stop_httpd(void)
{
	int ret = eval("killall", "httpd");

	dprintf("done\n");
	return ret;
}

#ifdef __CONFIG_NAT__
int
start_upnp(void)
{
	char *wan_ifname;
	int ret;
	char var[100], prefix[] = "wanXXXXXXXXXX_";
	char tmp[20];
	int i = 0;
	char *lan_ifname = NULL;

	if (!nvram_invmatch("upnp_enable", "0"))
		return 0;

	ret = eval("killall", "-SIGUSR1", "upnp");
	if (ret != 0) {
	    snprintf(prefix, sizeof(prefix), "wan%d_", wan_primary_ifunit());
	    wan_ifname = nvram_match(strcat_r(prefix, "proto", var), "pppoe") ?\
				nvram_safe_get(strcat_r(prefix, "pppoe_ifname", var)) :\
				nvram_safe_get(strcat_r(prefix, "ifname", var));

		for (i = 0; i < MAX_NO_BRIDGE; i++) {
			if (!i)
				snprintf(tmp, sizeof(tmp), "lan_ifname");
			else
				snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

			lan_ifname = nvram_safe_get(tmp);

			if (!strcmp(lan_ifname, ""))
				continue;

			ret = eval("upnp", "-D", "-L", lan_ifname, "-W", wan_ifname);
		}

	}
	dprintf("done\n");
	return ret;
}

int
stop_upnp(void)
{
	int ret = 0;

	if (nvram_match("upnp_enable", "0"))
	    ret = eval("killall", "upnp");

	dprintf("done\n");
	return ret;
}
#endif	/* __CONFIG_NAT__ */



int
start_nas(char *type)
{
	char cfgfile[64];
	char pidfile[64];

	if (!type || !*type)
		type = "lan";
	/* Configure NAS on the bridged interface */
	snprintf(cfgfile, sizeof(cfgfile), "/tmp/nas.%s.conf", type);
	snprintf(pidfile, sizeof(pidfile), "/tmp/nas.%s.pid", type);
	{
		char *argv[] = {"nas", cfgfile, pidfile, type, NULL};
		pid_t pid;

		_eval(argv, NULL, 0, &pid);
		dprintf("done\n");
	}

	return 0;
}

int
stop_nas(void)
{
	int ret = eval("killall", "nas");

	dprintf("done\n");
	return ret;
}

int
start_ntpc(void)
{
	char *servers = nvram_safe_get("ntp_server");

	if (strlen(servers)) {
		char *nas_argv[] = {"ntpclient", "-h", servers, "-i", "3", "-l", "-s", NULL};
		pid_t pid;

		_eval(nas_argv, NULL, 0, &pid);
	}

	dprintf("done\n");
	return 0;
}

int
stop_ntpc(void)
{
	int ret = eval("killall", "ntpclient");

	dprintf("done\n");
	return ret;
}

int
start_telnet(void)
{
	uchar tmp[20];
	int i = 0;
	int ret = 0;
	char *lan_ifname = NULL;

	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan_ifname");
		else
			snprintf(tmp, sizeof(tmp), "lan%x_ifname", i);

		lan_ifname = nvram_safe_get(tmp);

		if (!strcmp(lan_ifname, ""))
			continue;

		ret = eval("utelnetd", "-d", "-i", lan_ifname);
	}

	return ret;
}

int
stop_telnet(void)
{
	int ret;
	ret = eval("killall", "utelnetd");
	return ret;
}


int
start_wsc(void)
{
	if((!nvram_match("wsc_cli","1")) && nvram_match("wsc_mode","enabled"))
	{
#if 0   /* Foxconn removed pling 10/18/2007, always restart no matter what */
		if (nvram_match("wsc_restart", "no")) {

			nvram_set("wsc_restart", "normal");
		}
		else
#endif
		{
			char *wsc_argv[] = {"/bin/wsccmd", NULL};
			pid_t pid;

			dprintf("Starting wsc\n");
			nvram_set("wsc_restart", "yes");
			_eval(wsc_argv, NULL, 0, &pid);
			//sleep(5);         // 2008-03-04, Broadcom remove by Borg, for WDS patch
			if (nvram_match("wsc_addER", "1"))
			{
				nvram_set("wsc_config_command", "1");
				nvram_set("wsc_method", "1");
			}
		}
	}
	return 0;

}

int
stop_wsc(void)
{
   	int ret;

	//if((!nvram_match("wsc_cli","1")) && nvram_match("wl_wsc_mode","enabled"))
	if(!nvram_match("wsc_cli","1"))
	{
#if 0   /* Foxconn removed pling 10/18/2007, always restart no matter what */
		if (nvram_match("wsc_restart", "no")) {
			return 0;
		}
		else
#endif
		{
			nvram_set("wsc_restart", "yes");
		   	ret = eval("killall","wsccmd");
		}
	}
   	return ret;

}

#ifdef __CONFIG_IGMP_PROXY__
void
start_igmp_proxy(void)
{
	/* Start IGMP Proxy in Router mode only */
	if ((!nvram_match("router_disable", "1")) && nvram_match("emf_enable", "1"))
		eval("igmp", nvram_get("wan_ifname"));
	return;
}

void
stop_igmp_proxy(void)
{
	eval("killall", "igmp");
	return;
}
#endif /* __CONFIG_IGMP_PROXY__ */

#ifdef __CONFIG_LLD2D__
int start_lltd(void)
{
	chdir("/usr/sbin");
	eval("lld2d", "br0");
	chdir("/");
	
	return 0;
}

int stop_lltd(void)
{
	int ret;
	
	ret = eval("killall", "lld2d");

	return ret;
}
#endif /* __CONFIG_LLD2D__ */

int
start_services(void)
{
	char tmp[20];
	int i;
	char var[20];

	start_httpd();
#ifdef __CONFIG_NAT__
	start_dns();
	start_dhcpd();
	start_upnp();
#endif	/* __CONFIG_NAT__ */
	for (i = 0; i < MAX_NO_BRIDGE; i++) {
		if (!i)
			snprintf(tmp, sizeof(tmp), "lan");
		else
			snprintf(tmp, sizeof(tmp), "lan%x", i);

		if (!strcmp(nvram_safe_get(strcat_r(tmp, "_ifname", var)), ""))
			continue;

		start_nas(tmp);
	}
#ifdef __CONFIG_UTELNETD__
	start_telnet();
#endif
#ifdef __CONFIG_IGMP_PROXY__
	start_igmp_proxy();
#endif /* __CONFIG_IGMP_PROXY__ */
#ifdef __CONFIG_LLD2D__
	start_lltd();
#endif /* __CONFIG_LLD2D__*/

	dprintf("done\n");
	return 0;
}

int
stop_services(void)
{
	stop_nas();
#ifdef __CONFIG_NAT__
	stop_upnp();
	stop_dhcpd();
	stop_dns();
#endif	/* __CONFIG_NAT__ */
	stop_httpd();
#ifdef __CONFIG_UTELNETD__
	stop_telnet();
#endif
#ifdef __CONFIG_IGMP_PROXY__
	stop_igmp_proxy();
#endif /* __CONFIG_IGMP_PROXY__ */
#ifdef __CONFIG_LLD2D__
	stop_lltd();
#endif 	/* __CONFIG_LLD2D__*/

	dprintf("done\n");
	return 0;
}
