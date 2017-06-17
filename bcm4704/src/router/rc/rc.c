/*
 * Router rc control script
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: rc.c,v 1.1.1.1 2010/03/05 07:31:38 reynolds Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <dirent.h>

#include <epivers.h>
#include <bcmnvram.h>
#include <mtd.h>
#include <shutils.h>
#include <rc.h>
#include <netconf.h>
#include <nvparse.h>
#include <bcmdevs.h>
#include <bcmparams.h>
#include <wlutils.h>
#include <ezc.h>


#ifdef __CONFIG_NAT__
static void auto_bridge(void);
#endif	/* __CONFIG_NAT__ */

#include <sys/sysinfo.h> /* foxconn wklin added */

static void restore_defaults(void);
static void sysinit(void);
static void rc_signal(int sig);

extern struct nvram_tuple router_defaults[];

#define RESTORE_DEFAULTS() (!nvram_match("restore_defaults", "0") || nvram_invmatch("os_name", "linux"))

/* for WCN support, Foxconn added start by EricHuang, 12/13/2006 */
static void convert_wlan_params(void)
{
    nvram_set("wla_ssid", nvram_safe_get("wl0_ssid"));

    if ( strncmp(nvram_safe_get("wl0_akm"), "psk psk2", 7) == 0 )
    {
        nvram_set("wla_secu_type", "WPA-AUTO-PSK");
        nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));

        /* Foxconn added start pling 02/25/2007 */
        /* Disable WDS if it is already enabled */
        if (nvram_match("wla_wds_enable", "1"))
        {
            nvram_set("wla_wds_enable",  "0");
            nvram_set("wl0_wds", "");
            nvram_set("wl0_mode", "ap");
        }
        /* Foxconn added end pling 02/25/2007 */
    }
    else if ( strncmp(nvram_safe_get("wl0_akm"), "psk2", 4) == 0 )
    {
        nvram_set("wla_secu_type", "WPA2-PSK");
        nvram_set("wla_temp_secu_type", "WPA2-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));

        /* Foxconn added start pling 02/25/2007 */
        /* Disable WDS if it is already enabled */
        if (nvram_match("wla_wds_enable", "1"))
        {
            nvram_set("wla_wds_enable",  "0");
            nvram_set("wl0_wds", "");
            nvram_set("wl0_mode", "ap");
        }
        /* Foxconn added end pling 02/25/2007 */
    }
    else if ( strncmp(nvram_safe_get("wl0_akm"), "psk", 3) == 0 )
    {
        nvram_set("wla_secu_type", "WPA-PSK");
        nvram_set("wla_temp_secu_type", "WPA-PSK");
        nvram_set("wla_passphrase", nvram_safe_get("wl0_wpa_psk"));

        /* If router changes from 'unconfigured' to 'configured' state by
         * adding a WPS client, the wsc_randomssid and wsc_randomkey will
         * be set. In this case, router should use mixedmode security.
         */
        if (!nvram_match("wsc_randomssid", "") ||
            !nvram_match("wsc_randomkey", ""))
        {
            nvram_set("wla_secu_type", "WPA-AUTO-PSK");
            nvram_set("wla_temp_secu_type", "WPA-AUTO-PSK");

            nvram_set("wl0_akm", "psk psk2 ");
            nvram_set("wl0_crypto", "tkip+aes");

            nvram_set("wsc_mixedmode", "1");
            nvram_set("wsc_randomssid", "");
            nvram_set("wsc_randomkey", "");

            /* Since we changed to mixed mode, 
             * so we need to disable WDS if it is already enabled
             */
            if (nvram_match("wla_wds_enable", "1"))
            {
                nvram_set("wla_wds_enable",  "0");
                nvram_set("wl0_wds", "");
                nvram_set("wl0_mode", "ap");
            }
        }
        /* Foxconn added end pling 02/25/2007 */
    }
    else if ( strncmp(nvram_safe_get("wl0_wep"), "enabled", 7) == 0 )
    {
        int key_len=0;
        if ( strncmp(nvram_safe_get("wl0_auth"), "1", 1) == 0 ) /*shared mode*/
        {
            nvram_set("wla_auth_type", "sharedkey");
            nvram_set("wla_temp_auth_type", "sharedkey");
        }
        else
        {
            nvram_set("wla_auth_type", "opensystem");
            nvram_set("wla_temp_auth_type", "opensystem");
        }
        
        nvram_set("wla_secu_type", "WEP");
        nvram_set("wla_temp_secu_type", "WEP");
        nvram_set("wla_defaKey", "0");
        nvram_set("wla_temp_defaKey", "0");
        nvram_set("wla_key1", nvram_safe_get("wl_key1"));
        nvram_set("wla_temp_key1", nvram_safe_get("wl_key1"));
        
        printf("wla_wep_length: %d\n", strlen(nvram_safe_get("wl_key1")));
        
        key_len = atoi(nvram_safe_get("wl_key1"));
        if (key_len==5 || key_len==10)
        {
            nvram_set("wla_wep_length", "1");
        }
        else
        {
            nvram_set("wla_wep_length", "2");
        }
    }
    else
    {
        nvram_set("wla_secu_type", "None");
        nvram_set("wla_temp_secu_type", "None");
        nvram_set("wla_passphrase", "");
    }

    nvram_set("allow_registrar_config", "0");  /* Foxconn added pling, 05/16/2007 */
}
/* Foxconn added end by EricHuang, 12/13/2006 */

/* foxconn added start wklin, 11/02/2006 */
static void save_wlan_time(void)
{
    struct sysinfo info;
    char command[128];
    sysinfo(&info);
    sprintf(command, "echo %lu > /tmp/wlan_time", info.uptime);
    system(command);
    return;
}
/* foxconn added end, wklin, 11/02/2006 */

static int
build_ifnames(char *type, char *names, int *size)
{
	char name[32], *next;
	int len = 0;
	int s;
	
	/* open a raw scoket for ioctl */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
       		return -1;

	/*
	 * go thru all device names (wl<N> il<N> et<N> vlan<N>) and interfaces to 
	 * build an interface name list in which each i/f name coresponds to a device
	 * name in device name list. Interface/device name matching rule is device
	 * type dependant:
	 *
	 *	wl:	by unit # provided by the driver, for example, if eth1 is wireless
	 *		i/f and its unit # is 0, then it will be in the i/f name list if
	 *		wl0 is in the device name list.
	 *	il/et:	by mac address, for example, if et0's mac address is identical to
	 *		that of eth2's, then eth2 will be in the i/f name list if et0 is 
	 *		in the device name list.
	 *	vlan:	by name, for example, vlan0 will be in the i/f name list if vlan0
	 *		is in the device name list.
	 */
	foreach (name, type, next) {
		struct ifreq ifr;
		int i, unit;
		char var[32], *mac, ea[ETHER_ADDR_LEN];
		
		/* vlan: add it to interface name list */
		if (!strncmp(name, "vlan", 4)) {
			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", name);
			continue;
		}

		/* others: proceed only when rules are met */
		for (i = 1; i <= DEV_NUMIFS; i ++) {
			/* ignore i/f that is not ethernet */
			ifr.ifr_ifindex = i;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			if (!strncmp(ifr.ifr_name, "vlan", 4))
				continue;
				
			/* wl: use unit # to identify wl */
			if (!strncmp(name, "wl", 2)) {
				if (wl_probe(ifr.ifr_name) ||
				    wl_ioctl(ifr.ifr_name, WLC_GET_INSTANCE, &unit, sizeof(unit)) ||
				    unit != atoi(&name[2]))
					continue;
			}
			/* et/il: use mac addr to identify et/il */
			else if (!strncmp(name, "et", 2) || !strncmp(name, "il", 2)) {
				snprintf(var, sizeof(var), "%smacaddr", name);
				if (!(mac = nvram_get(var)) || !ether_atoe(mac, ea) ||
				    bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
					continue;
			}
			/* mac address: compare value */
			else if (ether_atoe(name, ea) &&
				!bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
				;
			/* others: ignore */
			else
				continue;

			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", ifr.ifr_name);
		}
	}
	
	close(s);

	*size = len;
	return 0;
}	

#ifdef __CONFIG_WSCCMD__
static void rc_randomKey()
{
	char random_ssid[33] = {0};
	unsigned char random_key[65] = {0};
	int i = 0;
	unsigned short key_length;

	RAND_bytes((unsigned char *)&key_length, sizeof(key_length));
	key_length = ((((long)key_length + 56791 )*13579)%8) + 8;

	/* random a ssid */
	sprintf(random_ssid, "Broadcom_");

	while (i < 6) {
		RAND_bytes(&random_ssid[9 + i], 1);
		if ((islower(random_ssid[9 + i]) || isdigit(random_ssid[9 + i])) && (random_ssid[9 + i] < 0x7f)) {
			i++;
		}
	}

	nvram_set("wl_ssid", random_ssid);

	i = 0;
	/* random a key */
	while (i < key_length) {
		RAND_bytes(&random_key[i], 1);
		if ((islower(random_key[i]) || isdigit(random_key[i])) && (random_key[i] < 0x7f)) {
			i++;
		}
	}
	random_key[key_length] = 0;

	nvram_set("wl_wpa_psk", random_key);

	/* Set default config to wps-psk, tkip */
	nvram_set("wl_akm", "psk ");
	nvram_set("wl_auth", "0");
	nvram_set("wl_wep", "disabled");
	nvram_set("wl_crypto", "tkip");

}

static void
wsccmd_restore_defaults(void)
{
	/* cleanly up nvram for WPS/WSC */
	nvram_unset("wsc_seed");
	nvram_unset("wsc_config_state");
	nvram_unset("wsc_addER");
	nvram_unset("wsc_device_pin");
	nvram_unset("wsc_pbc_force");
	nvram_unset("wsc_config_command");
	nvram_unset("wsc_proc_status");
	nvram_unset("wsc_status");
	nvram_unset("wsc_method");
	nvram_unset("wsc_proc_mac");
	nvram_unset("wsc_sta_pin");
	nvram_unset("wsc_currentband");
	nvram_unset("wsc_restart");
	nvram_unset("wsc_event");

}
#endif /* __CONFIG_WSCCMD__ */

static void
virtual_radio_restore_defaults(void)
{
	char tmp[100], prefix[] = "wlXXXXXXXXXX_mssid_";
	int i, j;
	
	nvram_unset("unbridged_ifnames");
	nvram_unset("ure_disable");
	
	/* Delete dynamically generated variables */
	for (i = 0; i < MAX_NVPARSE; i++) {
		sprintf(prefix, "wl%d_", i);
		nvram_unset(strcat_r(prefix, "vifs", tmp));
		nvram_unset(strcat_r(prefix, "ssid", tmp));
		nvram_unset(strcat_r(prefix, "guest", tmp));
		nvram_unset(strcat_r(prefix, "ure", tmp));
		nvram_unset(strcat_r(prefix, "ipconfig_index", tmp));
		nvram_unset(strcat_r(prefix, "nas_dbg", tmp));
		sprintf(prefix, "lan%d_", i);
		nvram_unset(strcat_r(prefix, "ifname", tmp));
		nvram_unset(strcat_r(prefix, "ifnames", tmp));
		nvram_unset(strcat_r(prefix, "gateway", tmp));
		nvram_unset(strcat_r(prefix, "proto", tmp));
		nvram_unset(strcat_r(prefix, "ipaddr", tmp));
		nvram_unset(strcat_r(prefix, "netmask", tmp));
		nvram_unset(strcat_r(prefix, "lease", tmp));
		nvram_unset(strcat_r(prefix, "stp", tmp));
		nvram_unset(strcat_r(prefix, "hwaddr", tmp));
		sprintf(prefix, "dhcp%d_", i);
		nvram_unset(strcat_r(prefix, "start", tmp));
		nvram_unset(strcat_r(prefix, "end", tmp));
		
		/* clear virtual versions */
		for (j=0; j< 16;j++){
			sprintf(prefix, "wl%d.%d_", i,j);
			nvram_unset(strcat_r(prefix, "ssid", tmp));
			nvram_unset(strcat_r(prefix, "ipconfig_index", tmp));
			nvram_unset(strcat_r(prefix, "guest", tmp));
			nvram_unset(strcat_r(prefix, "closed", tmp));
			nvram_unset(strcat_r(prefix, "wpa_psk", tmp));
			nvram_unset(strcat_r(prefix, "auth", tmp));
			nvram_unset(strcat_r(prefix, "wep", tmp));
			nvram_unset(strcat_r(prefix, "auth_mode", tmp));
			nvram_unset(strcat_r(prefix, "crypto", tmp));
			nvram_unset(strcat_r(prefix, "akm", tmp));
			nvram_unset(strcat_r(prefix, "hwaddr", tmp));
			nvram_unset(strcat_r(prefix, "bss_enabled", tmp));
			nvram_unset(strcat_r(prefix, "bss_maxassoc", tmp));
			nvram_unset(strcat_r(prefix, "wme_bss_disable", tmp));
			nvram_unset(strcat_r(prefix, "ifname", tmp));
			nvram_unset(strcat_r(prefix, "unit", tmp));
			nvram_unset(strcat_r(prefix, "ap_isolate", tmp));
			nvram_unset(strcat_r(prefix, "macmode", tmp));
			nvram_unset(strcat_r(prefix, "maclist", tmp));
			nvram_unset(strcat_r(prefix, "maxassoc", tmp));
			nvram_unset(strcat_r(prefix, "mode", tmp));
			nvram_unset(strcat_r(prefix, "radio", tmp));
			nvram_unset(strcat_r(prefix, "radius_ipaddr", tmp));
			nvram_unset(strcat_r(prefix, "radius_port", tmp));
			nvram_unset(strcat_r(prefix, "radius_key", tmp));
			nvram_unset(strcat_r(prefix, "key", tmp));
			nvram_unset(strcat_r(prefix, "key1", tmp));
			nvram_unset(strcat_r(prefix, "key2", tmp));
			nvram_unset(strcat_r(prefix, "key3", tmp));
			nvram_unset(strcat_r(prefix, "key4", tmp));
			nvram_unset(strcat_r(prefix, "wpa_gtk_rekey", tmp));
			nvram_unset(strcat_r(prefix, "nas_dbg", tmp));
		}
	}
}

#ifdef __CONFIG_NAT__
static void
auto_bridge(void)
{
	
	struct nvram_tuple generic[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "eth1", 0 },
		{ "wan_ifnames", "eth1", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "vlan1", 0 },
		{ "wan_ifnames", "vlan1", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	struct nvram_tuple dyna[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
	struct nvram_tuple generic_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth1 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 vlan1 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	/*struct nvram_tuple dyna_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan1 eth1 eth0 eth2 eth3", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};*/
	
	struct nvram_tuple dyna_auto_bridge[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};

	struct nvram_tuple *linux_overrides;
	struct nvram_tuple *t, *u;
	int auto_bridge=0, i;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif	/* __CONFIG_VLAN_ */
	char *landevs, *wandevs;
	char lan_ifnames[128], wan_ifnames[128];
	char dyna_auto_ifnames[128];
	char wan_ifname[32], *next;
	int len;
	int ap=0;

	printf(" INFO : enter function auto_bridge()\n");

	if (!strcmp(nvram_safe_get("auto_bridge_action"),"1")){
		auto_bridge=1;
		cprintf("INFO: Start auto bridge...\n");
	}
	else{
		nvram_set("router_disable_auto", "0");
		cprintf("INFO: Start non auto_bridge...\n");
	}
		

	/* Delete dynamically generated variables */
	if (auto_bridge) {
		char tmp[100], prefix[] = "wlXXXXXXXXXX_";
		for (i = 0; i < MAX_NVPARSE; i++) {

			del_filter_client(i);
			del_forward_port(i);
			del_autofw_port(i);


			snprintf(prefix, sizeof(prefix), "wan%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wan_", 4))
					nvram_unset(strcat_r(prefix, &t->name[4], tmp));
			}
		}						
	}

	/* 
	 * Build bridged i/f name list and wan i/f name list from lan device name list
	 * and wan device name list. Both lan device list "landevs" and wan device list
	 * "wandevs" must exist in order to preceed.
	 */
	if ((landevs = nvram_get("landevs")) && (wandevs = nvram_get("wandevs"))) {
		/* build bridged i/f list based on nvram variable "landevs" */
		len = sizeof(lan_ifnames);
		if (!build_ifnames(landevs, lan_ifnames, &len) && len)
			dyna[1].value = lan_ifnames;
		else
			goto canned_config;
		/* build wan i/f list based on nvram variable "wandevs" */
		len = sizeof(wan_ifnames);
		if (!build_ifnames(wandevs, wan_ifnames, &len) && len) {
			dyna[3].value = wan_ifnames;
			foreach (wan_ifname, wan_ifnames, next) {
				dyna[2].value = wan_ifname;
				break;
			}
		}
		else
			ap = 1;
			
		if(auto_bridge)
		{
			
			printf("INFO: lan_ifnames=%s\n", lan_ifnames);
			printf("INFO: wan_ifnames=%s\n", wan_ifnames);
			sprintf(dyna_auto_ifnames, "%s %s", lan_ifnames, wan_ifnames);
			printf("INFO: dyna_auto_ifnames=%s\n", dyna_auto_ifnames);
			dyna_auto_bridge[1].value = dyna_auto_ifnames;
			linux_overrides = dyna_auto_bridge;
			printf("INFO: linux_overrides=dyna_auto_bridge \n");
		}
		else
		{
			linux_overrides = dyna;
			printf("INFO: linux_overrides=dyna \n");
		}

	}
	/* override lan i/f name list and wan i/f name list with default values */
	else {
canned_config:
#ifdef __CONFIG_VLAN__
		boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		if (boardflags & BFL_ENETVLAN){
			if(auto_bridge)
			{
				linux_overrides = vlan_auto_bridge;
				printf("INFO: linux_overrides=vlan_auto_bridge \n");
			}
			else
			{
				linux_overrides = vlan;
				printf("INFO: linux_overrides=vlan \n");
			}
		}
		else{
#endif	/* __CONFIG_VLAN__ */
			if(auto_bridge)
			{
				linux_overrides = generic_auto_bridge;
				printf("INFO: linux_overrides=generic_auto_bridge \n");
			}
			else
			{
				linux_overrides = generic;
				printf("INFO: linux_overrides=generic \n");
			}
#ifdef __CONFIG_VLAN__
		}
#endif	/* __CONFIG_VLAN__ */
		
	}

		for (u = linux_overrides; u && u->name; u++) {
			nvram_set(u->name, u->value);
			printf("INFO: action nvram_set %s, %s\n", u->name, u->value);	
			}
			
	/* Force to AP */
	if (ap)
		nvram_set("router_disable", "1");
		
	if (auto_bridge){
		printf("INFO: reset auto_bridge flag.\n");
		nvram_set("auto_bridge_action", "0");
	}

	nvram_commit();
	cprintf("auto_bridge done\n");
}

#endif	/* __CONFIG_NAT__ */


static void
upgrade_defaults(void)
{
	char temp[100];
	int i;
	bool bss_enabled = TRUE;
	char *val;

	/* Check whether upgrade is required or not
	 * If lan1_ifnames is not found in NVRAM , upgrade is required.
	 */
	if (!nvram_get("lan1_ifnames") && !RESTORE_DEFAULTS()) {
		cprintf("NVRAM upgrade required.  Starting.\n");

		if (nvram_match("ure_disable", "1")) {
			nvram_set("lan1_ifname", "br1");
			nvram_set("lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3");
		}
		else {
			nvram_set("lan1_ifname", "");
			nvram_set("lan1_ifnames", "");
			for (i = 0; i < 2; i++) {
				snprintf(temp, sizeof(temp), "wl%d_ure", i);
				if (nvram_match(temp, "1")) {
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "1");
				}
				else {
					bss_enabled = FALSE;
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "0");
				}
			}
		}
		if (nvram_get("lan1_ipaddr")) {
			nvram_set("lan1_gateway", nvram_get("lan1_ipaddr"));
		}

		for (i = 0; i < 2; i++) {
			snprintf(temp, sizeof(temp), "wl%d_bss_enabled", i);
			nvram_set(temp, "1");
			snprintf(temp, sizeof(temp), "wl%d.1_guest", i);
			if (nvram_match(temp, "1")) {
				nvram_unset(temp);
				if (bss_enabled) {
					snprintf(temp, sizeof(temp), "wl%d.1_bss_enabled", i);
					nvram_set(temp, "1");
				}
			}

			snprintf(temp, sizeof(temp), "wl%d.1_net_reauth", i);
			val = nvram_get(temp);
			if(!val || (*val == 0))
				nvram_set(temp, nvram_default_get(temp));
			
			snprintf(temp, sizeof(temp), "wl%d.1_wpa_gtk_rekey", i);
			val = nvram_get(temp);
			if(!val || (*val == 0))
				nvram_set(temp, nvram_default_get(temp));
		}

		nvram_commit();

		cprintf("NVRAM upgrade complete.\n");
	}
}

static void
restore_defaults(void)
{
	struct nvram_tuple generic[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "eth1", 0 },
		{ "wan_ifnames", "eth1", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "vlan1", 0 },
		{ "wan_ifnames", "vlan1", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	struct nvram_tuple dyna[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ "lan1_ifname", "br1", 0 },
		{ "lan1_ifnames", "wl0.1 wl0.2 wl0.3 wl1.1 wl1.2 wl1.3", 0 },
		{ 0, 0, 0 }
	};

	struct nvram_tuple *linux_overrides;
	struct nvram_tuple *t, *u;
	int restore_defaults, i;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif	/* __CONFIG_VLAN_ */
	char *landevs, *wandevs;
	char lan_ifnames[128], wan_ifnames[128];
	char wan_ifname[32], *next;
	int len;
	int ap = 0;

	/* Restore defaults if told to or OS has changed */
	restore_defaults = RESTORE_DEFAULTS();

	if (restore_defaults)
		cprintf("Restoring defaults...");

	/* Delete dynamically generated variables */
	if (restore_defaults) {
		char tmp[100], prefix[] = "wlXXXXXXXXXX_";
		for (i = 0; i < MAX_NVPARSE; i++) {
#ifdef __CONFIG_NAT__
			del_filter_client(i);
			del_forward_port(i);
			del_autofw_port(i);
#endif	/* __CONFIG_NAT__ */
			snprintf(prefix, sizeof(prefix), "wl%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wl_", 3))
					nvram_unset(strcat_r(prefix, &t->name[3], tmp));
			}
#ifdef __CONFIG_NAT__
			snprintf(prefix, sizeof(prefix), "wan%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wan_", 4))
					nvram_unset(strcat_r(prefix, &t->name[4], tmp));
			}
#endif	/* __CONFIG_NAT__ */
		}
#ifdef __CONFIG_WSCCMD__
		wsccmd_restore_defaults();
#endif /* __CONFIG_WSCCMD__ */

		virtual_radio_restore_defaults();
	}

#ifdef __CONFIG_WSCCMD__
	/* unset ap lock down */
	nvram_unset("wsc_apLockDown");
#endif	
	/* 
	 * Build bridged i/f name list and wan i/f name list from lan device name list
	 * and wan device name list. Both lan device list "landevs" and wan device list
	 * "wandevs" must exist in order to preceed.
	 */
	if ((landevs = nvram_get("landevs")) && (wandevs = nvram_get("wandevs"))) {
		/* build bridged i/f list based on nvram variable "landevs" */
		len = sizeof(lan_ifnames);
		if (!build_ifnames(landevs, lan_ifnames, &len) && len)
			dyna[1].value = lan_ifnames;
		else
			goto canned_config;
		/* build wan i/f list based on nvram variable "wandevs" */
		len = sizeof(wan_ifnames);
		if (!build_ifnames(wandevs, wan_ifnames, &len) && len) {
			dyna[3].value = wan_ifnames;
			foreach (wan_ifname, wan_ifnames, next) {
				dyna[2].value = wan_ifname;
				break;
			}
		}
		else
			ap = 1;
		linux_overrides = dyna;
	}
	/* override lan i/f name list and wan i/f name list with default values */
	else {
canned_config:
#ifdef __CONFIG_VLAN__
		boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		if (boardflags & BFL_ENETVLAN)
			linux_overrides = vlan;
		else
#endif	/* __CONFIG_VLAN__ */
			linux_overrides = generic;
	}

	/* Check if nvram version is set, but old */
	if (nvram_get("nvram_version")) {
		int old_ver, new_ver;

		old_ver = atoi(nvram_get("nvram_version"));
		new_ver = atoi(NVRAM_SOFTWARE_VERSION);
		if (old_ver < new_ver) {
			cprintf("NVRAM: Updating from %d to %d\n", old_ver, new_ver);
			nvram_set("nvram_version", NVRAM_SOFTWARE_VERSION);
		}
	}

	/* Restore defaults */
	for (t = router_defaults; t->name; t++) {
		if (restore_defaults || !nvram_get(t->name)) {
			for (u = linux_overrides; u && u->name; u++) {
				if (!strcmp(t->name, u->name)) {
					nvram_set(u->name, u->value);
					break;
				}
			}
			if (!u || !u->name)
				nvram_set(t->name, t->value);
		}
	}

#ifdef __CONFIG_WSCCMD__
	//rc_randomKey();
#endif
	/* Force to AP */
	if (ap)
		nvram_set("router_disable", "1");

	/* Always set OS defaults */
	nvram_set("os_name", "linux");
	nvram_set("os_version", EPI_ROUTER_VERSION_STR);
	nvram_set("os_date", __DATE__);

	nvram_set("is_modified", "0");
	nvram_set("ezc_version", EZC_VERSION_STR);

	/* Commit values */
	if (restore_defaults) {
		nvram_commit();
        sync();         /* Foxconn added start pling 12/25/2006 */
		cprintf("done\n");
	}
}

#ifdef __CONFIG_NAT__
static void
set_wan0_vars(void)
{
	int unit;
	char tmp[100], prefix[] = "wanXXXXXXXXXX_";
	
	/* check if there are any connections configured */
	for (unit = 0; unit < MAX_NVPARSE; unit ++) {
		snprintf(prefix, sizeof(prefix), "wan%d_", unit);
		if (nvram_get(strcat_r(prefix, "unit", tmp)))
			break;
	}
	/* automatically configure wan0_ if no connections found */
	if (unit >= MAX_NVPARSE) {
		struct nvram_tuple *t;
		char *v;

		/* Write through to wan0_ variable set */
		snprintf(prefix, sizeof(prefix), "wan%d_", 0);
		for (t = router_defaults; t->name; t ++) {
			if (!strncmp(t->name, "wan_", 4)) {
				if (nvram_get(strcat_r(prefix, &t->name[4], tmp)))
					continue;
				v = nvram_get(t->name);
				nvram_set(tmp, v ? v : t->value);
			}
		}
		nvram_set(strcat_r(prefix, "unit", tmp), "0");
		nvram_set(strcat_r(prefix, "desc", tmp), "Default Connection");
		nvram_set(strcat_r(prefix, "primary", tmp), "1");
	}
}
#endif	/* __CONFIG_NAT__ */

static int noconsole = 0;

static void
sysinit(void)
{
	char buf[PATH_MAX];
	struct utsname name;
	struct stat tmp_stat;
	time_t tm = 0;
	char *loglevel;

	/* /proc */
	mount("proc", "/proc", "proc", MS_MGC_VAL, NULL);

	/* /tmp */
	mount("ramfs", "/tmp", "ramfs", MS_MGC_VAL, NULL);

	/* /var */
	mkdir("/tmp/var", 0777);
	mkdir("/var/lock", 0777);
	mkdir("/var/log", 0777);
	mkdir("/var/run", 0777);
	mkdir("/var/tmp", 0777);

	/* Setup console */
	if (console_init())
		noconsole = 1;
	if ((loglevel = nvram_get("console_loglevel")))
		klogctl(8, NULL, atoi(loglevel));
	else
		klogctl(8, NULL, 1);

	/* Modules */
	uname(&name);
	snprintf(buf, sizeof(buf), "/lib/modules/%s", name.release);
	if (stat("/proc/modules", &tmp_stat) == 0 &&
	    stat(buf, &tmp_stat) == 0) {
		char module[80], *modules, *next;
		modules = nvram_get("kernel_mods") ? : "et bcm57xx wl";
		foreach(module, modules, next)
			eval("insmod", module);

#ifdef __CONFIG_WCN__
		modules = "scsi_mod sd_mod usbcore usb-ohci usb-storage fat vfat msdos";
		foreach(module, modules, next)
			eval("insmod", module);
#endif
	}
	
	/* Set a sane date */
	stime(&tm);

	dprintf("done\n");
}

/* States */
enum {
	RESTART,
	STOP,
	START,
	TIMER,
	IDLE,
	WSC_RESTART,
	WLANRESTART, /* Foxconn added by EricHuang, 11/24/2006 */
};
static int state = START;
static int signalled = -1;

/* Signal handling */
static void
rc_signal(int sig)
{
	if (state == IDLE) {	
		if (sig == SIGHUP) {
			dprintf("signalling RESTART\n");
			signalled = RESTART;
		}
		else if (sig == SIGUSR2) {
			dprintf("signalling START\n");
			signalled = START;
		}
		else if (sig == SIGINT) {
			dprintf("signalling STOP\n");
			signalled = STOP;
		}
		else if (sig == SIGALRM) {
			dprintf("signalling TIMER\n");
			signalled = TIMER;
		}
		else if (sig == SIGUSR1) {
			dprintf("signalling WSC RESTART\n");
			signalled = WSC_RESTART;
		}
		/* Foxconn added start by EricHuang, 11/24/2006 */
		else if (sig == SIGQUIT) {
		    dprintf("signalling WLANRESTART\n");
		    signalled = WLANRESTART;
		}
		/* Foxconn added end by EricHuang, 11/24/2006 */
	}
}

/* Get the timezone from NVRAM and set the timezone in the kernel
 * and export the TZ variable 
 */
static void 
set_timezone(void)
{
	time_t now;
	struct tm gm, local;
	struct timezone tz;
	
	/* Export TZ variable for the time libraries to 
	 * use.
	 */
	setenv("TZ",nvram_get("time_zone"),1);
	
	/* Update kernel timezone */
	time(&now);
	gmtime_r(&now, &gm);
	localtime_r(&now, &local);
	tz.tz_minuteswest = (mktime(&gm) - mktime(&local)) / 60;
	settimeofday(NULL, &tz);
}

/* Timer procedure.Gets time from the NTP servers once every timer interval 
 * Interval specified by the NVRAM variable timer_interval
 */
int
do_timer(void)
{
	int interval = atoi(nvram_safe_get("timer_interval"));
	
	dprintf("%d\n", interval);

	if (interval == 0)
		return 0;

	/* Report stats */
	if (nvram_invmatch("stats_server", "")) {
		char *stats_argv[] = { "stats", nvram_get("stats_server"), NULL };
		_eval(stats_argv, NULL, 5, NULL);
	}

	/* Sync time */
	start_ntpc();

	alarm(interval);
	
	return 0;
}

/* Main loop */
static void
main_loop(void)
{
	sigset_t sigset;
	pid_t shell_pid = 0;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif
	
    /* Foxconn added start pling 07/03/2006 */
    /* Read ethernet MAC, etc */
    eval("read_bd");

    /* Reset some wps-related parameters */
    nvram_set("wps_start",   "none");
    /* Foxconn added end pling 07/03/2006 */
    
	/* Basic initialization */
	sysinit();

	/* Setup signal handlers */
	signal_init();
	signal(SIGHUP, rc_signal);
	signal(SIGUSR2, rc_signal);
	signal(SIGINT, rc_signal);
	signal(SIGALRM, rc_signal);
	signal(SIGUSR1, rc_signal);	
	signal(SIGQUIT, rc_signal); /* Foxconn added by EricHuang, 11/24/2006 */
	sigemptyset(&sigset);

	/* Give user a chance to run a shell before bringing up the rest of the system */
	if (!noconsole)
		run_shell(1, 0);
	
	/* Get boardflags to see if VLAN is supported */
#ifdef __CONFIG_VLAN__
	boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
#endif	/* __CONFIG_VLAN__ */

	/* Add loopback */
	config_loopback();

	/* Convert deprecated variables */
	convert_deprecated();



	/* Upgrade NVRAM variables to MBSS mode */
	upgrade_defaults();

	/* Restore defaults if necessary */
	restore_defaults();

    /* Foxconn added start pling 06/20/2007 */
    /* Read board data again, since the "restore_defaults" action
     * above will overwrite some of our settings */
    eval("read_bd");
    /* Foxconn added end pling 06/20/2006 */
    
#ifdef __CONFIG_NAT__
	/* Auto Bridge if neccessary */
	if (!strcmp(nvram_safe_get("auto_bridge"),"1") )
	{
		auto_bridge();
	}
	/* Setup wan0 variables if necessary */
	set_wan0_vars();
#endif	/* __CONFIG_NAT__ */

	/* Loop forever */
	for (;;) {
		switch (state) {
		case RESTART:
			dprintf("RESTART\n");

			/* Foxconn added start pling 06/14/2007 */
            /* When vista finished configuring this router (wsc_config_state: 0->1),
             * then we come here to restart WLAN 
             */
			stop_nas();
            stop_wsc();
			stop_wlan();

    	    convert_wlan_params();  /* For WCN , added by EricHuang, 12/21/2006 */
            sleep(2);               /* Wait some time for wsc, etc to terminate */

            /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
            /* added start by EricHuang, 09/05/2007 */
            if ( nvram_match("wsc_status", "0") ) //restart wlan for wsc
            {
                nvram_set("wl_wsc_reg", "enabled");
                nvram_set("wl0_wsc_reg", "enabled");
                printf("restart -- wsc_config_state=%s\n", nvram_get("wsc_config_state"));
            }
            /* added end by EricHuang, 09/05/2007 */

			start_wlan();
            start_wsc();
            save_wlan_time();
            sleep(2);               /* Wait for WSC to start */
            start_nas("lan");

            nvram_commit();         /* Save WCN obtained parameters */

			state = IDLE;
			break;
			/* Foxconn added end pling 06/14/2007 */

		case STOP:
			dprintf("STOP\n");
            /* Foxconn modified start pling 06/27/2006 */
#if 0
			stop_services();
#ifdef __CONFIG_NAT__
			stop_wan();
			stop_wsc();
#endif	/* __CONFIG_NAT__ */
#endif
            stop_nas();
			stop_wsc();
            /* Foxconn modfiied end pling 06/27/2006 */

			stop_lan();
#ifdef __CONFIG_VLAN__
			if (boardflags & BFL_ENETVLAN)
				stop_vlan();
#endif	/* __CONFIG_VLAN__ */
			if (state == STOP) {
				state = IDLE;
				break;
			}
			/* Fall through */
		case START:
			dprintf("START\n");
			{ /* Set log level on restart */
				char *loglevel;
				int loglev = 1;

				if ((loglevel = nvram_get("console_loglevel"))) {
					loglev = atoi(loglevel);
				}
				klogctl(8, NULL, loglev);
				if (loglev < 7) {
					printf("WARNING: console log level set to %d\n", loglev);
				}
			}
			set_timezone();
#ifdef __CONFIG_VLAN__
			if (boardflags & BFL_ENETVLAN)
				start_vlan();
#endif	/* __CONFIG_VLAN__ */
			start_lan();
			start_wsc();

            /* Foxconn modified start pling 06/27/2006 */
#if 0
			start_services();
#ifdef __CONFIG_NAT__
			start_wan();
			start_nas("wan");
#endif	/* __CONFIG_NAT__ */
#endif
            /* Foxconn modified end pling 06/27/2006 */

            /* foxconn added start by pling, 05/31/2007 */
            /* Now start ACOS services */
            save_wlan_time();
            start_nas("lan");
            eval("acos_init");
            eval("acos_service", "start");

            /* Start wsc if it is in 'unconfiged' state, and if PIN is not disabled */
            if (nvram_match("wsc_config_state", "0") && !nvram_match("wsc_pin_disable", "1"))
            {
                /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
                /* added start by EricHuang, 09/05/2007 */
                nvram_set("wl_wsc_reg", "enabled");
                nvram_set("wl0_wsc_reg", "enabled");
                /* added end by EricHuang, 09/05/2007 */
                nvram_set("wsc_config_command", "1");
            }
            /* foxconn added end pling, 05/31/2007 */

			/* Fall through */
		case TIMER:
            /* Foxconn removed start pling 07/12/2006 */
#if 0
			dprintf("TIMER\n");
			do_timer();
#endif
            /* Foxconn removed end pling 07/12/2006 */
			/* Fall through */
		case IDLE:
			dprintf("IDLE\n");
			state = IDLE;
			/* Wait for user input or state change */
			while (signalled == -1) {
				if (!noconsole && (!shell_pid || kill(shell_pid, 0) != 0))
					shell_pid = run_shell(0, 1);
				else
					sigsuspend(&sigset);
			}
			state = signalled;
			signalled = -1;
			break;

		case WSC_RESTART:
			dprintf("WSC_RESTART\n");
			state = IDLE;
			stop_wsc();
			start_wsc();
			break;

            /* Foxconn added start pling 06/14/2007 */
            /* We come here only if user press "apply" in Wireless GUI */
		case WLANRESTART:
            stop_wsc();
			stop_nas();
			stop_wlan();
            
			eval("read_bd");    /* sync foxconn and brcm nvram params */
            
			start_wlan();
            start_wsc();
            save_wlan_time();
            sleep(2);           /* Wait for WSC to start */
            start_nas("lan");

            /* Start wsc if it is in 'unconfiged' state */
            if (nvram_match("wsc_config_state", "0") && !nvram_match("wsc_pin_disable", "1"))
            {
                /* if "unconfig" to "config" mode, force it to built-in registrar and proxy mode */
                /* added start by EricHuang, 09/05/2007 */
                nvram_set("wl_wsc_reg", "enabled");
                nvram_set("wl0_wsc_reg", "enabled");
                /* added end by EricHuang, 09/05/2007 */
                nvram_set("wsc_config_command", "1");
            }
            
			state = IDLE;
		    break;
            /* Foxconn added end pling 06/14/2007 */
		    
		default:
			dprintf("UNKNOWN\n");
			return;
		}
	}
}

int
main(int argc, char **argv)
{
	char *base = strrchr(argv[0], '/');
	
	base = base ? base + 1 : argv[0];

	/* init */
	if (strstr(base, "init")) {
		main_loop();
		return 0;
	}

	/* Set TZ for all rc programs */
	setenv("TZ", nvram_safe_get("time_zone"), 1);

	/* rc [stop|start|restart ] */
	if (strstr(base, "rc")) {
		if (argv[1]) {
			if (strncmp(argv[1], "start", 5) == 0)
				return kill(1, SIGUSR2);
			else if (strncmp(argv[1], "stop", 4) == 0)
				return kill(1, SIGINT);
			else if (strncmp(argv[1], "restart", 7) == 0)
				return kill(1, SIGHUP);
		    /* Foxconn added start by EricHuang, 11/24/2006 */
		    else if (strcmp(argv[1], "wlanrestart") == 0)
		        return kill(1, SIGQUIT);
		    /* Foxconn added end by EricHuang, 11/24/2006 */
		} else {
			fprintf(stderr, "usage: rc [start|stop|restart|wlanrestart]\n");
			return EINVAL;
		}
	}
	
#ifdef __CONFIG_NAT__
	/* ppp */
	else if (strstr(base, "ip-up"))
		return ipup_main(argc, argv);
	else if (strstr(base, "ip-down"))
		return ipdown_main(argc, argv);

	/* udhcpc [ deconfig bound renew ] */
	else if (strstr(base, "udhcpc"))
		return udhcpc_wan(argc, argv);
#endif	/* __CONFIG_NAT__ */

	/* ldhclnt [ deconfig bound renew ] */
	else if (strstr(base, "ldhclnt"))
		return udhcpc_lan(argc, argv);

	/* stats [ url ] */
	else if (strstr(base, "stats"))
		return http_stats(argv[1] ? : nvram_safe_get("stats_server"));

	/* erase [device] */
	else if (strstr(base, "erase")) {
		if (argv[1])
			return mtd_erase(argv[1]);
		else {
			fprintf(stderr, "usage: erase [device]\n");
			return EINVAL;
		}
	}

	/* write [path] [device] */
	else if (strstr(base, "write")) {
		if (argc >= 3)
			return mtd_write(argv[1], argv[2]);
		else {
			fprintf(stderr, "usage: write [path] [device]\n");
			return EINVAL;
		}
	}

	/* hotplug [event] */
	else if (strstr(base, "hotplug")) {
		if (argc >= 2) {
			if (!strcmp(argv[1], "net"))
				return hotplug_net();
#ifdef __CONFIG_WCN__
			else if (!strcmp(argv[1], "usb"))
				return hotplug_usb();
#endif
		} else {
			fprintf(stderr, "usage: hotplug [event]\n");
			return EINVAL;
		}
	}

	return EINVAL;
}
