/*
 * Program to control GPIO pins.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <epivers.h>
#include <shutils.h>
#include <bcmdevs.h>
#include <bcmparams.h>
#include <bcmgpio.h>

int main(int argc, char *argv[])
{
    int gpio_pin, gpio_value;

    if (argc < 2 || argc > 3)
    {
        printf("usage: set GPIO: gpio <pin> <value>\n");
        printf("       get GPIO: gpio <pin> \n");
        return -1;
    }
    
    gpio_pin = atoi(argv[1]);
    gpio_value = atoi(argv[2]);
    
    bcmgpio_connect(gpio_pin, BCMGPIO_DIRN_OUT);
    bcmgpio_out(1 << gpio_pin , gpio_value << gpio_pin);

    return 0;
}
