
#ifndef _LANGUAGE_ASSEMBLY

/* Global SB handle */
extern sb_t *bcm947xx_sbh;
#define sbh bcm947xx_sbh

/* BSP UI initialization */
extern int ui_init_bcm947xxcmds(void);
#if CFG_ET
extern void et_addcmd(void);
#endif

#if CFG_WLU
extern void wl_addcmd(void);
#else
#endif

#endif
