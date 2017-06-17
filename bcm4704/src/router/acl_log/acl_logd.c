/*******************************************************************************
 * Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
 * All Rights Reserved.
 * No portions of this material shall be reproduced in any form without the
 * written permission of Hon Hai Precision Ind. Co. Ltd.
 *
 * All information contained in this document is Hon Hai Precision Ind.
 * Co. Ltd. company private, proprietary, and trade secret property and
 * are protected by international intellectual property laws and treaties.
 *
 ******************************************************************************/

/*******************************************************************************
 ***  Filename:
 ***      acl_logd.c
 ***
 ***  Description:
 ***      
 ***      A task to monitor wireless association list if ACL is enabled.
 ***
 ***  History:
 ***
 ***   Modify Reason               Author          Date      Search Flag(Option)
 ***----------------------------------------------------------------------------
 ***   Created                     Eric Huang      2007/01/04
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <bcmnvram.h>
#include <arpa/inet.h>
#include <shutils.h>
#include <bcmutils.h>
#include <typedefs.h>
#include <wlioctl.h>
#include <wlutils.h>

#define MAX_STA_COUNT   64 /* maximum ACL list defined in NGR's router spec rev1.0 */
#define LOG_DURATION    30

static int aclWriteLog(char *pcLog, int iLen)
{
    FILE *fp;
    
    if ((fp = fopen("/dev/aglog", "r+")) != NULL)
    {
        *(pcLog + iLen) = '\0';
        fwrite(pcLog, sizeof(char), iLen+1, fp);
        fclose(fp);
        return 0;
    }
    return -1;
}    

int main(int argc, char **argv)
{

    while (1)
    {
        
        sleep(LOG_DURATION);
        
        if ( nvram_match("wlan_acl_enable", "1") )
        {
            struct maclist *mac_list;
            int mac_list_size;
            int i;
            char *wlif = nvram_safe_get("wl0_ifname");

            
            mac_list_size = sizeof(mac_list->count) + 
                            MAX_STA_COUNT * sizeof(struct ether_addr);
            mac_list = malloc(mac_list_size);

            if (!mac_list)
            {
                //printf("[acl_logd] no enough free memory\n");
                continue;
            }

            /* query wl for authenticated sta list */
            strcpy((char*)mac_list, "authe_sta_list");
            if (wl_ioctl(wlif, WLC_GET_VAR, mac_list, mac_list_size)) 
            {
                free(mac_list);
                //printf("[acl_logd] cannot get mac_list\n");
                continue;
            }

            for (i = 0; i < mac_list->count; i++) 
            {
                char buf[300];

                strcpy(buf, "sta_info");
                memcpy( buf+strlen(buf)+1, 
                        (unsigned char *)&(mac_list->ea[i]), 
                        ETHER_ADDR_LEN);

                /* query sta info */
                if (!wl_ioctl(wlif, WLC_GET_VAR, buf, sizeof(buf))) 
                {
                    char logBuffer[96];
                    char ea_str[ETHER_ADDR_STR_LEN];
                    sta_info_t *sta = (sta_info_t *)buf;

                    /*
            		printf("associated client (%d): %s, associated time elapsed: %d secs\n", 
                            i, 
                            ether_etoa((unsigned char *)&(mac_list->ea[i]), ea_str), 
                            sta->in);
                    */

                    /* if the associated time elapse greate than log_duration, 
                       it must be logged last time. */
                    if (sta->in < LOG_DURATION)
                    {
                        sprintf(logBuffer, 
                                "[WLAN access allowed] from MAC: %s,", 
                                ether_etoa((unsigned char *)&(mac_list->ea[i]), 
                                ea_str));
                        aclWriteLog(logBuffer, strlen (logBuffer));
                    }
            	}
            }

            free(mac_list);
        }
    }

    return 0;
}
