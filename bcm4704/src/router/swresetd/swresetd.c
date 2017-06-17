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
 ***      reset_btn.c
 ***
 ***  Description:
 ***      Check reset button
 ***
 ***  History:
 ***
 ***   Modify Reason                    Author          Date            Search Flag(Option)
 ***--------------------------------------------------------------------------------------
 ***   Check Reset Button               Peter Ling      2006/04/17
 ***   Port to U12H081 platform         Peter Ling     2006/10/22
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <epivers.h>
#include <shutils.h>
#include <bcmdevs.h>
#include <bcmparams.h>
#include <bcmgpio.h>


#define GPIO_RESET_BUTTON               6
#define GPIO_POWER_GREEN                2
#define GPIO_POWER_AMBER                3

#define ACTION_NONE                     0
#define ACTION_REBOOT                   1
#define ACTION_LOAD_DEFAULT             2

#define TMP_UPGRADE_FILE                "/tmp/upgrade.tmp"

static int check_upgrade_status(void)
{
    int fd;

    /* Check tmp file */
    if ((fd = open(TMP_UPGRADE_FILE, O_RDONLY)) < 0)
        return 0;   /* No firmware upgrade in progress */
    
    return 1;       /* upgrade in progress */

}

static int check_reset_button(void)
{
    unsigned long value;
    int is_loaddefault = 1;
    int i;
    
    bcmgpio_in(1 << GPIO_RESET_BUTTON, &value);
    if (value)
        return ACTION_NONE;

    /* Change Power LED to amber, when reset button is first pressed */
    bcmgpio_out(1 << GPIO_POWER_GREEN, 0);
    bcmgpio_out(1 << GPIO_POWER_AMBER, 1 << GPIO_POWER_AMBER);

    /* Wait up to 5 second for user load default */
    for (i=0; i<50; i++)
    {
        bcmgpio_in(1 << GPIO_RESET_BUTTON, &value);
        if (value)
        {
            is_loaddefault = 0;
            break;
        }
        usleep(100000);
    }
    
    /* Reboot */
    if (!is_loaddefault)
    {
        fprintf(stderr, "Reset\n");
        return ACTION_REBOOT;
    }
 
    /*
     * User has not release reset button yet, 
     * Blink Power LED every sec, 0.25 sec on, 0.75 sec off
     */    
    /* 5 sec passed, need to load default */
    while (1)
    {
        bcmgpio_in(1 << GPIO_RESET_BUTTON, &value);
        if (value)
            break;

        bcmgpio_out(1 << GPIO_POWER_AMBER, 0);
        usleep(750000);

        bcmgpio_in(1 << GPIO_RESET_BUTTON, &value);
        if (value)
            break;
        
        bcmgpio_out(1 << GPIO_POWER_AMBER, 1 << GPIO_POWER_AMBER);
        usleep(250000);
    }

    fprintf(stderr, "Load Default\n");

    /* Turn off power LED */
    bcmgpio_out(1 << GPIO_POWER_AMBER, 0);

    return ACTION_LOAD_DEFAULT;
}

static int do_reboot(int action)
{
    if (action == ACTION_LOAD_DEFAULT)
    {
        /* Pull low Ethernet PHY according to Netgear Ken's requirement */
        system("et robowr 0x0 0xF 0x0F"); /*setup the POWRN DOWN MODE register*/

        system("/usr/sbin/nvram loaddefault");
    }
    
    system("/sbin/reboot");

    return 0;
}

int main(int argc, char *argv[])
{
    int status;
    
    /* Daemonize myself */
    if (daemon (1, 1) == -1)
        perror ("daemon");

    bcmgpio_connect(GPIO_POWER_GREEN, BCMGPIO_DIRN_OUT);
    bcmgpio_connect(GPIO_POWER_AMBER, BCMGPIO_DIRN_OUT);
    bcmgpio_connect(GPIO_RESET_BUTTON, BCMGPIO_DIRN_IN);

    while (1)
    {
        status = check_reset_button();

        if (status == ACTION_REBOOT)
        {
            /* Make sure we are not doing firmware upgrade */
            if (check_upgrade_status())
                printf("Upgrading firmware, don't reboot\n");
            else
                do_reboot(ACTION_REBOOT);
        }
        else
        if (status  == ACTION_LOAD_DEFAULT)
        {
            /* 5 sec passed, load default */
            /* Make sure we are not doing firmware upgrade */
            if (check_upgrade_status())
                printf("Upgrading firmware, don't load default\n");
            else
                do_reboot(ACTION_LOAD_DEFAULT);
        }

        usleep(500000);
    }
}
