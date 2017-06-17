
/******************************************************************************
 * Fichier Module :igmpv3.c - An IGMPv3-router implementation
 ******************************************************************************
 * Fichier    : igmpv3.c
 * Description: Implementation de differentes routines du protocole IGMPv3-router
 *               en se basant sur le "draft-ietf-idmr-igmp-v3-07.txt", Mars 2001
 * Date       : May 18, 2000
 * Auteurs    : lahmadi@loria.fr
 *              Anis.Ben-Hellel@loria.fr
 * Last Modif : juin 10,  2002
 *
 *****************************************************************************/

#include "igmprt.h"

unsigned long upstream;
int forward_upstream;

#define GROUP_INIT    0
#define FIRST_JOIN_GROUP    1
#define LAST_LEAVE_GROUP	2
#define LAST_LEAVE_GROUP_HAD_SEND 3

igmp_mulsrc_t mulsrc;
extern int wan_index;
extern int lan_index;
extern int wan_igmp_socket;
extern igmp_router_t router;
extern int wan_igmp_version;
extern unsigned long lan_ipaddr, lan_netmask;
int wan_version_timer = IGMP_WAN_VERSION_TIMER;
int general_query_count = 0;

/*
 * int check_src_set(src,set)
 * check if a source is in a set
 *
 */
int check_src_set (
        struct in_addr src,
        igmp_src_t *set)
{
    igmp_src_t *sr;
    for (sr=set;sr;sr=(igmp_src_t *)sr->igmps_next){
        if (sr->igmps_source.s_addr == src.s_addr)
            return TRUE;
    }
    return FALSE;
}
/*
 * int check_src (
 *  struct in_addr src,
 *      struct in_addr *sources,    
 *  int numsrc) 
 */     
int check_src (     
        struct in_addr src, 
        struct in_addr *sources, int numsrc)
{
    int i;
    for (i=0;i< numsrc; i++){
        if (src.s_addr == sources[i].s_addr)
            return TRUE;
    }
    return FALSE;
}

/*
 * void igmp_group_handle_allow
 * handle an allow report for a group
 *
 */
void igmp_group_handle_allow(
            igmp_router_t *router,
            igmp_interface_t *ifp,
            igmp_group_t *gp,
            int numsrc,
            struct in_addr *sources)
{
    igmp_src_t *src;
    int i,num;
    struct in_addr set_src[MAX_ADDRS];
    igmp_group_t *gp1;
    membership_db* member;

    /* state INCLUDE(A+B)*/
    for (i=0;i<numsrc;i++) {
      src=igmp_group_src_add(gp,sources[i]);
          if (ifp->igmpi_addr.s_addr == upstream)
             k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,forward_upstream);
          else
			 k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,1);
    }

    /* (B) = GMI */
    num =0;
    gp1 = gp;
    for (src=gp->igmpg_sources;src;src=(igmp_src_t *)src->igmps_next){
      if (check_src(src->igmps_source,sources,numsrc) == TRUE){
        src->igmps_timer = IGMP_GMI;
        src->igmps_fstate= FALSE;
      }
      set_src[num].s_addr = src->igmps_source.s_addr;
      num = num+1;
    }
    gp = gp1;
    if (ifp->igmpi_addr.s_addr != upstream){
      member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
      set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
    }
}

/*
 * void igmp_group_handle_block
 * handle a block report for a group
 *
 */
void igmp_group_handle_block(
            igmp_router_t *router,
            igmp_interface_t *ifp,
            igmp_group_t *gp,
            int numsrc,
            struct in_addr *sources)
{
    igmp_src_t *sr,*old_src_set;
    struct in_addr set_src[MAX_ADDRS];
    int i,num;
    igmp_group_t *gp1;
    membership_db* member;

    old_src_set = gp->igmpg_sources;
    if (gp->igmpg_fmode ==IGMP_FMODE_INCLUDE){
        /* send Q(G,A*B) */
        num=0;
        gp1=gp;
        for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
            if ((check_src_set(sr->igmps_source,old_src_set) == TRUE)&& (check_src(sr->igmps_source,sources,numsrc) == TRUE)){
                set_src[num].s_addr = sr->igmps_source.s_addr;
                    num = num+1;
            }
        }
        send_group_src_specific_q(router,ifp,gp,set_src,num);
        gp=gp1;

        num=0;
        gp1=gp;
        for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
           if (check_src_set(sr->igmps_source,old_src_set) == TRUE) {
               set_src[num].s_addr=sr->igmps_source.s_addr;
               num = num+1;
				}
            }
        gp=gp1;
           if (ifp->igmpi_addr.s_addr != upstream){
          member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
          set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
        }       

    }else{
      /* EXCLUDE MODE*/
      /* add source to group source table*/
      for (i=0;i<numsrc;i++){
            sr=igmp_group_src_add(gp,sources[i]);
        /*(A-X-Y)=group_timer*/
            if ((check_src_set(sr->igmps_source,old_src_set) == FALSE) && (check_src(sr->igmps_source,sources,numsrc) == TRUE)){
            sr->igmps_timer= gp->igmpg_timer;
        sr->igmps_fstate = FALSE;
        }
    }
    /* send Q(G,A-Y) */
    num=0;
    gp1=gp;
    for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
        if ((check_src(sr->igmps_source,sources,numsrc) == TRUE) && (sr->igmps_timer == 0)){
             set_src[num].s_addr=sr->igmps_source.s_addr;
             num = num+1;
        }
    }
    gp=gp1;
    send_group_src_specific_q(router,ifp,gp,set_src,num);   

    num=0;
    gp1=gp;
    for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
      if (check_src_set(sr->igmps_source,old_src_set) == TRUE) {
        set_src[num].s_addr=sr->igmps_source.s_addr;
        num = num+1;
      }
    }
    gp=gp1;
    if (ifp->igmpi_addr.s_addr != upstream){
      member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
      set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
    }           
    }
}

/*
 * void igmp_group_toex()
 * Handle a to_ex{} report for a group
 */

void igmp_group_handle_toex(
                igmp_router_t *router,
                igmp_interface_t *ifp,
                igmp_group_t *gp,
                int numsrc,
                struct in_addr *sources)
{
    igmp_src_t *src,*sr,*old_src_set;
    struct in_addr set_src[MAX_ADDRS];
    igmp_group_t *gp1;
    int i,num,count=0;
    membership_db* member;
	igmp_rep_t *rep;

	for(rep=gp->igmpg_members;rep;rep=(igmp_rep_t *)rep->igmpr_next)
	{
		if((rep->igmpr_addr.s_addr != lan_ipaddr) && ((rep->igmpr_addr.s_addr & lan_netmask)
			    == (lan_ipaddr & lan_netmask)))
		{
			count++;
		}
	}
	if(count == 1)
		gp->igmpg_flags = FIRST_JOIN_GROUP; /* First join a group */
	else
		gp->igmpg_flags = GROUP_INIT;

    old_src_set=(igmp_src_t *)gp->igmpg_sources;
    if (gp->igmpg_fmode == IGMP_FMODE_INCLUDE){
      for(i=0;i<numsrc;i++){
        src=igmp_group_src_add(gp,sources[i]);
        if ((check_src_set(src->igmps_source,old_src_set) == FALSE) && check_src(src->igmps_source,sources,numsrc) == TRUE) {
          src->igmps_timer=0;
            }
        if ((check_src_set(src->igmps_source,old_src_set) == TRUE) && check_src(src->igmps_source,sources,numsrc) == FALSE){
          igmp_src_cleanup(gp,src);
          k_proxy_del_mfc (router->igmprt_socket, src->igmps_source.s_addr, gp->igmpg_addr.s_addr);
        }       
      }
    }else{ /*EXCLUDE filter mode*/
      for (i=0;i<numsrc;i++){
        src=igmp_group_src_add(gp,sources[i]);
        /*(A-X-Y)=group timer*/
        if ((check_src_set(src->igmps_source,old_src_set) == FALSE) && (check_src(src->igmps_source,sources,numsrc) == TRUE)){
          src->igmps_timer= gp->igmpg_timer;
          src->igmps_fstate = FALSE;
        }
        /*delete( X-A) delete (Y-A)*/
        if ((check_src(src->igmps_source,sources,numsrc) == FALSE) && (check_src_set(src->igmps_source,old_src_set) == TRUE)){
            igmp_src_cleanup(gp,src);
        k_proxy_del_mfc (router->igmprt_socket, src->igmps_source.s_addr, gp->igmpg_addr.s_addr);
         }
      }
    }
    /*group timer = GMI*/  
    gp->igmpg_timer=IGMP_GMI;
    /* send querry Q(G,A-Y) or A*B */
    num=0;
    gp1=gp;
    for (sr=gp->igmpg_sources;sr!=NULL;sr=(igmp_src_t *)sr->igmps_next){
      if (sr->igmps_timer > 0) {
        set_src[num].s_addr=sr->igmps_source.s_addr;
        num = num+1;
      }
    }
    gp=gp1;
    send_group_src_specific_q(router,ifp,gp,set_src,num);
    /* group filter mode : EXCLUDE*/ 
    gp->igmpg_fmode = IGMP_FMODE_EXCLUDE;

    if (ifp->igmpi_addr.s_addr != upstream){
      member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
      set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
    }

    if(numsrc == 0)
    {
        k_proxy_chg_mfc(router->igmprt_socket,mulsrc.igmps_addr.s_addr,gp->igmpg_addr.s_addr,wan_index,1);
    }
	igmp_info_print(router);
}


/* 
 * void igmp_group_toin()
 * handle to_in{} report for a group
 */
void igmp_group_handle_toin(
                igmp_router_t *router,
                igmp_interface_t *ifp,
                igmp_group_t *gp,
                int numsrc,
				igmp_rep_t *srcrep,
                struct in_addr *sources)
{
  igmp_src_t *src,*old_src_set,*sr;
  struct in_addr set_src[MAX_ADDRS];
  int i,n,num,count=0,igmplen;
  igmp_group_t *gp1,*gp2;
  membership_db* member;
  igmp_rep_t *rep;

	if(!igmp_interface_group_lookup(ifp,gp->igmpg_addr))
	{
		return;
	}

	if (!igmp_group_rep_lookup(gp, srcrep->igmpr_addr))
	{
	    return;
	}

	igmp_group_rep_del(gp,srcrep->igmpr_addr);
	igmp_info_print(router);

	for(rep=gp->igmpg_members;rep;rep=(igmp_rep_t *)rep->igmpr_next)
	{
		if((rep->igmpr_addr.s_addr != lan_ipaddr) && ((rep->igmpr_addr.s_addr & lan_netmask)
			    == (lan_ipaddr & lan_netmask)))
		{
			count++;
		}
	}
	
	if(count == 0)
	{
		if(gp->igmpg_flags!=LAST_LEAVE_GROUP_HAD_SEND)
			gp->igmpg_flags = LAST_LEAVE_GROUP; /* Last leave a group */

		if(wan_igmp_version != IGMP_VERSION_1)
			k_proxy_del_mfc(router->igmprt_socket,mulsrc.igmps_addr.s_addr,gp->igmpg_addr.s_addr);
	}
	else
		gp->igmpg_flags = GROUP_INIT;

 
  old_src_set=(igmp_src_t *)gp->igmpg_sources;
  for (i=0;i<numsrc;i++){
      src=igmp_group_src_add(gp,sources[i]);
      /*(B) = GMI*/
      if ((check_src(src->igmps_source,sources,numsrc) == TRUE) && (check_src_set(src->igmps_source,old_src_set) == FALSE)){
    src->igmps_timer= IGMP_GMI;
    src->igmps_fstate = FALSE;
        if (ifp->igmpi_addr.s_addr == upstream)
		{
           k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,forward_upstream);
		}
        else
		{
           k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,1);
		}
    }
  }
  if (gp->igmpg_fmode == IGMP_FMODE_INCLUDE){
      /*send querry Q(G,A-B)*/
      gp1=gp;
      num=0;
      for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
        if ((check_src(sr->igmps_source,sources,numsrc) == FALSE) && (check_src_set(sr->igmps_source,old_src_set) == TRUE )){
        set_src[num].s_addr=sr->igmps_source.s_addr;
        num = num+1;
        }
      }
      gp =gp1;
      send_group_src_specific_q(router,ifp,gp,set_src,num);
      gp->igmpg_fmode=IGMP_FMODE_INCLUDE;

      gp1=gp;
      num = 0;
      for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
     set_src[num].s_addr=sr->igmps_source.s_addr;
     num = num+1;    
      }
      gp = gp1;

      if (ifp->igmpi_addr.s_addr != upstream){
    member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);        
    set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
      }     
      
  }else{
	/*send Q(G,X-A)*/
    gp1=gp;
    num = 0;
    for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
      if ((check_src(sr->igmps_source,sources,numsrc) == FALSE) && (check_src_set(sr->igmps_source,old_src_set) == TRUE ) && (sr->igmps_timer > 0)){
    set_src[num].s_addr=sr->igmps_source.s_addr;
    num = num+1;
      }      
    }
    gp=gp1;
    send_group_src_specific_q(router,ifp,gp,set_src,num);
    /*send Q(G)*/
    send_group_specific_query(router,ifp,gp);
    gp->igmpg_fmode=IGMP_FMODE_EXCLUDE;  

    gp1 = gp;
    num = 0;
    for (sr=gp->igmpg_sources;sr; sr=(igmp_src_t *)sr->igmps_next){
    set_src[num].s_addr=sr->igmps_source.s_addr;
        num = num+1;
    }
    gp = gp1;
	if (ifp->igmpi_addr.s_addr != upstream){
      member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src); 
      //set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);

    }

	}

 }
  
/*
 * void igmp_group_handle_isex()
 *
 * Handle a is_ex{A} report for a group 
 * the report have only one source
 */
void
igmp_group_handle_isex(
    igmp_router_t* router,
    igmp_interface_t* ifp,
    igmp_group_t* gp,
    int numsrc,
    struct in_addr *sources)
{
    igmp_src_t *src,*sr,*old_src_set;
    int i;
    membership_db* member;

	/* In order to forward multicast packets automatically when gproxy run later than client joining a group */
	if(numsrc == 0)
	{
		if( general_query_count < 5)
		{
			k_proxy_chg_mfc(router->igmprt_socket,mulsrc.igmps_addr.s_addr,gp->igmpg_addr.s_addr,wan_index,1);
		}
	}

    /* Reset timer */
    gp->igmpg_timer = IGMP_GMI; /* ifp->igmpi_qi = GMI : GMI = (RBV * QI) + QRI */
    /* Do the v3 logic */
    old_src_set=gp->igmpg_sources;
    if (gp->igmpg_fmode == IGMP_FMODE_EXCLUDE) {
      /* $6.4.1: State = Excl(X,Y), Report = IS_EX(A) */
      for (i=0;i < numsrc;i++){
        src=igmp_group_src_add(gp,sources[i]);
        /* (A-X-Y) = GMI */
        if ((check_src_set(src->igmps_source,old_src_set) == FALSE) && (check_src(src->igmps_source,sources,numsrc) == TRUE)){
          src->igmps_timer = IGMP_GMI;
          src->igmps_fstate = FALSE;
        }
        else
          /* delete ( X-A)  delete (Y-A) */
          if ((check_src(src->igmps_source,sources,numsrc) == FALSE) && (check_src_set(src->igmps_source,old_src_set) == TRUE)){
        igmp_src_cleanup(gp,sr);
        k_proxy_del_mfc (router->igmprt_socket, sr->igmps_source.s_addr, gp->igmpg_addr.s_addr);
          }
      }   
    }
    else {
      /* $6.4.1: State = Incl(X,Y), Report = IS_EX(A) */
      for (i=0;i< numsrc; i++){
        src=igmp_group_src_add(gp,sources[i]);
        if ((check_src_set(src->igmps_source,old_src_set) == FALSE) && (check_src(src->igmps_source,sources,numsrc) == TRUE)){  
          /*(B-A) = 0*/
          src->igmps_timer = 0;
        }else{
          if ((check_src(src->igmps_source,sources,numsrc) == FALSE) && (check_src_set(src->igmps_source,old_src_set) == TRUE)){ 
        /* delete (A-B)*/
        igmp_src_cleanup(gp,src);
        k_proxy_del_mfc (router->igmprt_socket, src->igmps_source.s_addr, gp->igmpg_addr.s_addr);
          } 
        }
      }
    }
    gp->igmpg_fmode = IGMP_FMODE_EXCLUDE;

    if (ifp->igmpi_addr.s_addr != upstream){
      member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,numsrc,sources);         
      set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
    }  

	igmp_info_print(router); 
}

/*
 * void igmp_group_handle_isin()
 *
 * Handle a is_in{A} report for a group 
 * the report have only one source
 */
void
igmp_group_handle_isin(
    igmp_router_t* router,
    igmp_interface_t* ifp,
    igmp_group_t* gp,
    int numsrc,
    struct in_addr *sources)
{
    igmp_src_t *src,*sr; 
    struct in_addr *source;
    struct in_addr set_src[MAX_ADDRS];
    int i,num;
    igmp_group_t *gp1;
    membership_db* member;

    /* Do the v3 logic */
    for (i=0;i < numsrc;i++){
      /*(A) = GMI*/
      src=igmp_group_src_add(gp,sources[i]);
      if (check_src(src->igmps_source,sources,numsrc) == TRUE){
        src->igmps_timer = IGMP_GMI;
        src->igmps_fstate = FALSE;
            if (ifp->igmpi_addr.s_addr == upstream)
                k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,forward_upstream);
            else
                k_proxy_chg_mfc(router->igmprt_socket,sources[i].s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,1);
      }
    } 
    num=0;
    gp1=gp;
    for (sr=gp->igmpg_sources;sr;sr=(igmp_src_t *)sr->igmps_next){
      set_src[num].s_addr=sr->igmps_source.s_addr;
      num = num+1;
    }
    gp=gp1; 
    if (gp->igmpg_fmode == IGMP_FMODE_EXCLUDE) {
      /* $6.4.1: State = Excl(X,Y), Report = IS_IN(A) */
      gp->igmpg_fmode = IGMP_FMODE_EXCLUDE;

      if (ifp->igmpi_addr.s_addr != upstream){
        member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
        set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
      }       
    }
    else {
      /* $6.4.1: State = Incl(A), Report = IS_IN(B) */
      gp->igmpg_fmode = IGMP_FMODE_INCLUDE;

      if (ifp->igmpi_addr.s_addr != upstream){    
        member = (membership_db*)update_multi(router,gp->igmpg_addr,gp->igmpg_fmode,num,set_src);         
        set_source_filter(router,gp,upstream,member->membership.fmode,member->membership.numsources,member->membership.sources);
      }     
    }
}

/***************************************************************************/
/*                                     */
/*              Timer management routines                          */
/************************************************************************* */

/*
 * void igmprt_timer_querier(igmp_interface_t *ifp)
 * handle the other querier timer
 *
 */
void igmprt_timer_querier(igmp_interface_t *ifp)
{

    if (ifp->igmpi_oqp > 0)
        --ifp->igmpi_oqp;
    if (ifp->igmpi_oqp == 0)
        ifp->igmpi_isquerier == TRUE;
}


/*
 * void igmprt_timer_group(igmp_router_t* router, igmp_interface_t *ifp)
 *
 * handle the  groups timers for this router
 *
 */
void igmprt_timer_group(igmp_router_t* router,igmp_interface_t *ifp)
{
  igmp_group_t *gp;
  igmp_src_t   *src;
  int delete_gr;
  int flag,count=0;
  struct ip_mreq mreq;
  igmp_interface_t* upstream_interface, *ifp1;
  struct in_addr up,srctmp;;
  igmp_router_t* igmprt;

	  
  for(gp=ifp->igmpi_groups;gp;gp=gp->igmpg_next){
    
    /*decrement group timer*/
    if (gp->igmpg_timer > 0)
      --gp->igmpg_timer;
    /*handle group timer*/
    if (gp->igmpg_fmode == IGMP_FMODE_EXCLUDE){
      if ((gp->igmpg_timer == 0) && (ifp->igmpi_addr.s_addr == lan_ipaddr)){
			k_proxy_del_mfc(router->igmprt_socket,mulsrc.igmps_addr.s_addr,gp->igmpg_addr.s_addr);
    
    /* no more listeners to group */
    delete_gr = TRUE;
    for (src=gp->igmpg_sources;src;src=(igmp_src_t *)src->igmps_next){
      if (src->igmps_timer > 0){
        delete_gr = FALSE;     
      }else{
        igmp_src_cleanup(gp,src);
        k_proxy_del_mfc (router->igmprt_socket, src->igmps_source.s_addr, gp->igmpg_addr.s_addr);
      }
     
	}
    if (delete_gr == TRUE){
      /*all source timers expired*/
      LOG((LOG_DEBUG,"all source timer expired delete group\n"));

      /*igmp_group_cleanup(gp);*/  /* Del by lewis min, 12/15/2007 */

      igmprt = router;
      /*deleate group from the set of groups of upstream interface if all downstream memberships are down*/
      flag = TRUE;
	  for (ifp1 = router->igmprt_interfaces; ifp1; ifp1 = ifp1->igmpi_next)
	  {
        if ((igmp_interface_group_lookup(ifp1,gp->igmpg_addr) != NULL) && (ifp1->igmpi_addr.s_addr != upstream))
		{
			flag = FALSE;
		}
	  }
      if (flag == TRUE) {	 
        mreq.imr_multiaddr.s_addr=gp->igmpg_addr.s_addr;
        mreq.imr_interface.s_addr=upstream;
        if (VALID_ADDR(mreq.imr_multiaddr)) {		
          up.s_addr = upstream;	 
          upstream_interface = igmprt_interface_lookup(router,up);
          if (igmp_interface_group_lookup(upstream_interface,mreq.imr_multiaddr) != NULL) {
        if (setsockopt(router->igmprt_up_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &mreq, sizeof(mreq)) < 0) {
          perror("setsockopt - IP_DROP_MEMBERSHIP");
          //exit(1);
        }
		
          }
        } 
      }
      router = igmprt;
    }
    else{

      /*switch to INCLUDE filter-mode with source list = sources with timer running*/
      gp->igmpg_fmode = IGMP_FMODE_INCLUDE;
    } 
      }           
    }
  }
  igmprt_clear_timer_group(ifp);
}


      
/*
 * void igmprt_timer_source(igmp_router_t* router, igmp_interface_t *ifp)
 *
 * handle source timers
 */
void igmprt_timer_source (igmp_router_t* router,igmp_interface_t *ifp)
{
  igmp_group_t *gp;
  igmp_src_t   *src;
  int flag;
  struct ip_mreq mreq;
  igmp_interface_t* upstream_interface;
  struct in_addr up;
  igmp_router_t* igmprt;
  igmp_interface_t *ifp1;
  
  for (gp=ifp->igmpi_groups;gp;gp=gp->igmpg_next)
    for (src=gp->igmpg_sources;src;src=(igmp_src_t *)src->igmps_next){
      /*decrement source timer*/
      if (src->igmps_timer > 0) 
    --src->igmps_timer;
      /*handle source timer*/
      switch(gp->igmpg_fmode){
      case IGMP_FMODE_INCLUDE:
    if (src->igmps_timer > 0 ){
      /* suggest to forward traffic from source */
      /* TO DO: add a flag to the source record*/   
      if (src->igmps_fstate == FALSE){
        src->igmps_fstate= TRUE;
        //printf("forward traffic from source: %s\n",inet_ntoa(src->igmps_source));
        /*tell the kernel to forward traffic from this source*/
        k_proxy_chg_mfc(router->igmprt_socket,src->igmps_source.s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,1);
      }
    }else {
      if (src->igmps_timer == 0){
        /*suggest to stop forwarding traffic from source*/
        if (src->igmps_fstate == TRUE){
          src->igmps_fstate = FALSE;
          //printf("stop forwarding traffic from source, timer = 0: %s\n",inet_ntoa(src->igmps_source));
          /*tell the kernel to stop forwarding traffic from this source*/
          k_proxy_chg_mfc(router->igmprt_socket,src->igmps_source.s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,0);
        }
        igmp_src_cleanup(gp,src);
        k_proxy_del_mfc (router->igmprt_socket, src->igmps_source.s_addr, gp->igmpg_addr.s_addr);
        if (gp->igmpg_sources == NULL) {        
          /*delete group*/
          igmp_group_cleanup(gp);

          igmprt = router;
          /*deleate group from the set of groups of upstream interface if all downstream memberships are down*/
          flag = TRUE;
          for (ifp1 = igmprt->igmprt_interfaces; ifp1; ifp1 = ifp1->igmpi_next)
        if ((igmp_interface_group_lookup(ifp1,gp->igmpg_addr) != NULL) && (ifp1->igmpi_addr.s_addr != upstream))
          flag = FALSE;
          if (flag == TRUE) {
        mreq.imr_multiaddr.s_addr=gp->igmpg_addr.s_addr;
        mreq.imr_interface.s_addr=upstream;
        if (VALID_ADDR(mreq.imr_multiaddr)) {
          up.s_addr = upstream;
          upstream_interface = igmprt_interface_lookup(router,up);
          if (igmp_interface_group_lookup(upstream_interface,mreq.imr_multiaddr) != NULL) {
            if (setsockopt(router->igmprt_up_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void *) &mreq, sizeof(mreq)) < 0) { 
              perror("setsockopt - IP_DROP_MEMBERSHIP");
              //exit(1);
            }
          }
        }
          }
          router = igmprt;
        }                                 
      }
    }   
    if (gp->igmpg_sources == NULL)/*TODO: (*,G) state source addr = ADRESSE_NONE*/
      printf("not forward source\n");
    break;
      case IGMP_FMODE_EXCLUDE:
    if (src->igmps_timer > 0 ){
      if (src->igmps_fstate == FALSE){  
        src->igmps_fstate = TRUE;
        printf("suggest to forward traffic from src: %s\n",inet_ntoa(src->igmps_source));
        /*tell the kernel to forward traffic from this source */
        k_proxy_chg_mfc(router->igmprt_socket,src->igmps_source.s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,1);
      } 
    }
    else{
      if (src->igmps_timer == 0){
        if (src->igmps_fstate == TRUE){
          src->igmps_fstate = FALSE;
          printf("not forward traffic from src: %s\n",inet_ntoa(src->igmps_source));
          /*tell the kernel to stop forwarding traffic from this source*/
          k_proxy_chg_mfc(router->igmprt_socket,src->igmps_source.s_addr,gp->igmpg_addr.s_addr,ifp->igmpi_index,0);    
        }
      }
      if (gp->igmpg_sources == NULL)/*TODO: (*,G) state source addr = ADRESSE_NONE*/
        printf("forward traffic from all sources\n");   
      break;
    }
      }
    }
}

/*****************************************************************************
 *
 *                  Query Routines
 *
 *****************************************************************************/
/*
 * sch_query_t * igmp_sch_create ( struct in_addr gp)
 * create a new scheduled entry
 */
sch_query_t *igmp_sch_create( struct in_addr gp)
{
    sch_query_t *sh;
    if (sh=(sch_query_t *)malloc(sizeof(*sh))){
        sh->gp_addr.s_addr = gp.s_addr;
        sh->sch_next = NULL;
    }
    return sh;  
}
/*
 * void sch_group_specq_add(router,ifp,gp,sources,numsrc)
 * add a scheduled query entry
 */
void sch_group_specq_add(
        igmp_interface_t *ifp,
        struct in_addr gp,
        struct in_addr *sources,
        int numsrc)
{
    sch_query_t* sch;
    int i;
    if (numsrc != 0){
    /*create the schedule entry*/
    sch=igmp_sch_create(gp);
    /*set the retransmissions number*/
    sch->igmp_retnum = IGMP_DEF_RV - 1;
    sch->numsrc = numsrc;
    for (i=0;i<numsrc;i++)
        sch->sources[i].s_addr = sources[i].s_addr;
    sch->sch_next = (sch_query_t *)ifp->sch_group_query;
    ifp->sch_group_query = sch;
    }else
        return;
}
        

/*
 * void igmprt_membership_query()
 *
 * Send a membership query on the specified interface, to the specified group.
 * Include sources if they are specified and the router version of the 
 * interface is igmp version 3.
 */
void
igmprt_membership_query(igmp_router_t* igmprt, igmp_interface_t* ifp,
    struct in_addr *group, struct in_addr *sources, int numsrc, int SRSP)
{
    char buf[12], *pbuf = NULL;
    igmpv3q_t *query;
    int size;
    struct sockaddr_in sin;
    int i, igmplen, version = ifp->igmpi_version;

    assert(igmprt != NULL);
    assert(ifp != NULL);

	general_query_count++;
    /* Allocate a buffer to build the query */ 
    if (numsrc != 0 && version == IGMP_VERSION_3) {
        pbuf = (char*) malloc(sizeof(*query) + numsrc * sizeof(struct in_addr));
        if (pbuf == NULL)
            return;
        query = (igmpv3q_t *) pbuf;
    }
    else
        query = (igmpv3q_t *) buf;

    /* Set the common fields */
    query->igmpq_type = IGMP_MEMBERSHIP_QUERY;
    query->igmpq_group.s_addr = group->s_addr; 
    query->igmpq_cksum = 0;

    /* Set the version specific fields */
    switch (ifp->igmpi_version) {
        case IGMP_VERSION_1:
            igmplen = 8;
            query->igmpq_code = 0;
            break;
        case IGMP_VERSION_2:
            igmplen = 8;
            query->igmpq_code = ifp->igmpi_qri;
            break;
        case IGMP_VERSION_3:
            igmplen = 12;
            query->igmpq_code = ifp->igmpi_qri;
            if (SRSP == TRUE) /*set supress router-side Processing*/
                query->igmpq_misc=(ifp->igmpi_rv | 0x08);
            else
                query->igmpq_misc = ifp->igmpi_rv;
            
            query->igmpq_qqi = ifp->igmpi_qi;
            query->igmpq_numsrc = htons(numsrc);
            for (i=0; i<numsrc; i++){
                query->igmpq_sources[i].s_addr = sources[i].s_addr;
            }
            break;
        default:
            if (pbuf)
                free(pbuf);
            return;
    }

    /* Checksum */
    query->igmpq_cksum = in_cksum((unsigned short*) query, igmplen);
    /* Send out the query */
    size=sizeof(*query)+(numsrc-1)*sizeof(struct in_addr);
    sin.sin_family = AF_INET;
    if (group->s_addr == 0)
        sin.sin_addr.s_addr = htonl(INADDR_ALLHOSTS_GROUP);
    else
        sin.sin_addr.s_addr = group->s_addr;
    sendto(ifp->igmpi_socket, (void*) query, size, 0, 
        (struct sockaddr*)&sin, sizeof(sin));
	
    if (pbuf)
        free(pbuf);
}
/*
 * void send_group_specific_query()
 * send a query to a specific group
 *
 */
void send_group_specific_query(
        igmp_router_t *router,
        igmp_interface_t *ifp,
        igmp_group_t *gp)
{
    int SRSP=FALSE;
    if (gp->igmpg_timer > IGMP_TIMER_SCALE)
        SRSP = TRUE;
    else
        gp->igmpg_timer = IGMP_TIMER_SCALE;
    /*send a group specific query immediately*/
    igmprt_membership_query(router,ifp,&gp->igmpg_addr,NULL,0,SRSP);
    /*schedule retransmission*/
    sch_group_specq_add(ifp,gp->igmpg_addr,NULL,0); 
}

/*
 * void send_group_src_specific_q()
 * send a group and source specific query
 */
void send_group_src_specific_q(
            igmp_router_t *router,
            igmp_interface_t *ifp,
            igmp_group_t *gp,
            struct in_addr *sources,
            int numsrc)
{
    int i;
    igmp_src_t *src;

    if (gp != NULL){
    for (i=0;i < numsrc; i++){
        src=igmp_group_src_lookup(gp,sources[i]);
        if (src != NULL)
            src->igmps_timer = IGMP_TIMER_SCALE;
        else
           return;
    }
    /*schedule retransmission*/
    sch_group_specq_add(ifp,gp->igmpg_addr,sources,numsrc);
    }


}
/*
 * void sch_query_cleanup(ifp,sch)
 * cleanup a scheduled record query from an inteeface
 *
 */
void sch_query_cleanup(igmp_interface_t *ifp,
               sch_query_t *sch)
{
    sch_query_t *sh;
    if (sch != ifp->sch_group_query){
      for (sh=ifp->sch_group_query;sh->sch_next != sch;sh=sh->sch_next);
      sh->sch_next = sch->sch_next;
      free(sch);
    }else{ /*delete the head*/
      sh=ifp->sch_group_query;
      ifp->sch_group_query = sh->sch_next;
      free(sh);
    }
}   
/*
 * void construct_set()
 * construct two sets of sources with source timer lower than LMQI
 * et another with source timer greater than LMQI
 */
void construct_set( igmp_interface_t *ifp,
            sch_query_t *sch,
            struct in_addr *src_inf_lmqi,
            struct in_addr *src_sup_lmqi,
            int *numsrc1,
            int *numsrc2)
{
    igmp_src_t *src;
    igmp_group_t *gp;
    int i,numsr1,numsr2;
    /*src_sup_lmqi=NULL;
    src_inf_lmqi=NULL;*/
    
    numsr1 = numsr2 = 0;
    
    for (i=0;i < sch->numsrc; i++){
        /*lookup the group of the source*/
        if ((gp=igmp_interface_group_lookup(ifp,sch->gp_addr))== NULL){
            *numsrc1 = numsr1;
            *numsrc1 = numsr2;
            return;
        }
        /*lookup the record source in the group*/
        if ((src=igmp_group_src_lookup(gp,sch->sources[i]))==NULL){
            *numsrc1 = numsr1;
            *numsrc1 = numsr2;
            return;
        }
        if (src->igmps_timer > IGMP_TIMER_SCALE){
            src_sup_lmqi[numsr1].s_addr=src->igmps_source.s_addr;
            numsr1++;
        }else{
            src_inf_lmqi[numsr2].s_addr=src->igmps_source.s_addr;
            numsr2++;
        }
    }
    
    *numsrc1 =numsr1;
    *numsrc2 =numsr2;
}
/*
 * send_sh_query(router,ifp)
 * send scheduled query on an interface
 *
 */     
void send_sh_query(igmp_router_t *router,
           igmp_interface_t *ifp)
{
    sch_query_t *sch;
    struct in_addr src_inf_lmqi;
    struct in_addr src_sup_lmqi;
    int numsrc1;
    int numsrc2;
    igmp_group_t *gp;

    if (ifp->sch_group_query != NULL){
        for (sch=ifp->sch_group_query;sch;sch=sch->sch_next){
            /*trait query per query*/
            if (sch->numsrc == 0){
                /*group specifq query*/
                if (sch->igmp_retnum >0){
					/*another query yet*/
                    if ((gp=igmp_interface_group_lookup(ifp,sch->gp_addr))==NULL){
                        return;
                    }
                    if (gp->igmpg_timer > IGMP_TIMER_SCALE)
					{
						/* group timer > LMQI*/
                        igmprt_membership_query(router,ifp,&sch->gp_addr,NULL,0,1);
					}
                    else
					{
                        igmprt_membership_query(router,ifp,&sch->gp_addr,NULL,0,0);
					}
                    --sch->igmp_retnum; 
                }else{ /*number retransmission = 0*/
                    /*delete the query record*/
                    sch_query_cleanup(ifp,sch);
                }
            }else{
                /*group and source specifiq query*/
                if (sch->igmp_retnum > 0 ){
                    construct_set(ifp,sch,&src_inf_lmqi,&src_sup_lmqi,&numsrc1,&numsrc2);
                    /*send query of source with timer > LMQI*/
                    if (numsrc2 != 0){
                        igmprt_membership_query(router,ifp,&sch->gp_addr,&src_inf_lmqi,numsrc2,0);}
                    if (numsrc1  != 0){
                        igmprt_membership_query(router,ifp,&sch->gp_addr,&src_sup_lmqi,numsrc1,1);}

                    --sch->igmp_retnum;
                }else /*retransmission =0*/
                    sch_query_cleanup(ifp,sch); 

            }
        }
    }   

} 

/*
 * send out a membership report, version 1 and 2
 */
int send_membership_report_v12(igmp_router_t* igmprt, struct in_addr group, int version)
{
    igmpr_t *v2_report;
    char *pbuf;
    igmp_interface_t *ifp_1;
    igmp_group_t *gp_1;
    int igmplen=0,size=0;
	struct sockaddr_in sin;
    
    pbuf = (char*) malloc(sizeof(igmpr_t));
    if (pbuf == NULL){
        printf(" Can not allocate memoey for report packet\n");
        return -1;
        }

    v2_report = (igmpr_t *) pbuf;
	
    if (wan_igmp_version == IGMP_VERSION_1)
        v2_report->igmpr_type = IGMP_V1_MEMBERSHIP_REPORT;
    else 
		if (wan_igmp_version == IGMP_VERSION_2)
            v2_report->igmpr_type = IGMP_V2_MEMBERSHIP_REPORT;

    v2_report->igmpr_code = 0;
    igmplen += sizeof(igmpr_t);
 
	/* send igmpv2 membership report including all group report if we are queried */
	if(group.s_addr == 0)
	{
		for (ifp_1=igmprt->igmprt_interfaces; ifp_1;ifp_1=ifp_1->igmpi_next)
		{
			if(ifp_1->igmpi_addr.s_addr == lan_ipaddr)
				for (gp_1=ifp_1->igmpi_groups; gp_1 ; gp_1=gp_1->igmpg_next)
				{
					 if (gp_1->igmpg_addr.s_addr == 0){
						 continue;
						}
					/* We cannot send igmpv2 packet whose dst ip is "224.0.0.22"  or "224.0.0.2" to wan port when queried */
					 if ((gp_1->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS_V3)) || 
						 (gp_1->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS)))
					 {
						 continue;
					 }
					 if( gp_1->igmpg_members == NULL)
						 continue;
					/*Fill in group address*/
					memcpy(&(v2_report->igmpr_group),&(gp_1->igmpg_addr),4);
		            
					/* Checksum */
					v2_report->igmpr_cksum = 0;
					v2_report->igmpr_cksum = in_cksum((unsigned short*) v2_report, igmplen);
					/* Send out the report message */
					size = sizeof(*v2_report);
					sin.sin_family = AF_INET;
					memcpy(&sin.sin_addr,&(gp_1->igmpg_addr),4);
					
					sendto(wan_igmp_socket, (void*) v2_report, size, 0, (struct sockaddr*)&sin, sizeof(sin));
				}
		}
	}
	/* send igmp membership report to wan according to wan igmp version */
	else
	{
		if(wan_igmp_version == IGMP_VERSION_3)
			/*if(igmp_aggregation_timer == 0)*/
				send_membership_report_v12_to_v3(group,CHANGE_TO_EXCLUDE); /* v2 join group is equivalent to v3 type CHANGE_TO_EXCLUDE */
		else 
			if((wan_igmp_version == IGMP_VERSION_1) || (wan_igmp_version == IGMP_VERSION_2))
		{
			v2_report->igmpr_type = wan_igmp_version;
			/*Fill in group address*/
			if (group.s_addr == inet_addr(IGMP_ALL_ROUTERS_V3) || 
				 group.s_addr == inet_addr(IGMP_ALL_ROUTERS))
			{
				if (pbuf)
					free(pbuf);
				 return 0;
			}

			memcpy(&(v2_report->igmpr_group),&group,4);

			igmplen += 4;
			/* Checksum */
			v2_report->igmpr_cksum = 0;
			v2_report->igmpr_cksum = in_cksum((unsigned short*) v2_report, igmplen);
			/* Send out the leave group message */
			size = sizeof(*v2_report);
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = group.s_addr;

			sendto(wan_igmp_socket, (void*) v2_report, size, 0, (struct sockaddr*)&sin, sizeof(sin));

		}
	}
	
    if (pbuf)
        free(pbuf);
    return 0;
}


/*
 * send out a membership report, version 3
 */
int send_membership_report_v3(int is_be_queried)
{
    igmp_report_t *v3_report;
    char *pbuf;
    igmp_interface_t *ifp1;
    igmp_group_t *gp1;
    int igmplen=0, recordnum=0;
    /* I suppose, the number of records is less than 64 */
    igmp_group_t records[64];
	struct sockaddr_in sin;
	igmp_grouprec_t  arecord;
    for (ifp1=router.igmprt_interfaces; ifp1;ifp1=ifp1->igmpi_next)
    {
		if(ifp1->igmpi_addr.s_addr == lan_ipaddr)
			for (gp1=ifp1->igmpi_groups; gp1; gp1=gp1->igmpg_next)
			{
				/* We should not send igmpv3 packet whose dst ip is "224.0.0.2" to wan port */
					recordnum++;
					/* TODO: may be copy all src and member to the array */
			}
    }
    if (recordnum > 0)
    {
		
        int i,j;
        pbuf = (char*) malloc(sizeof(igmp_report_t) + (recordnum-1)*8);
        if (pbuf == NULL){
            printf(" Can not allocate memoey for report packet\n");
            return -1;
        }

        v3_report = (igmp_report_t *) pbuf;
        v3_report->igmpr_type = IGMP_V3_MEMBERSHIP_REPORT;
        v3_report->igmpr_code = 0;
        v3_report->igmpr_rsv = 0;
		
        igmplen += sizeof(igmp_report_t);
		/* send igmpv3 membership report including all group report if we are queried */
		if(is_be_queried)
		{
			for (ifp1=router.igmprt_interfaces; ifp1;ifp1=ifp1->igmpi_next)
			{
				if(ifp1->igmpi_addr.s_addr == lan_ipaddr)
				{
					for (i=0,gp1=ifp1->igmpi_groups; gp1; gp1=gp1->igmpg_next,i++)
					{
						if(gp1->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS) || gp1->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS_V3))
							continue;
						if( gp1->igmpg_members == NULL)
							continue;
						arecord.igmpg_type = gp1->igmpg_type;
						arecord.igmpg_datalen = 0;
						arecord.igmpg_numsrc = 0;
						arecord.igmpg_group.s_addr = gp1->igmpg_addr.s_addr;
						
						memcpy(((char *)&(v3_report->igmpr_group[i])), &arecord, sizeof(igmp_grouprec_t));
						if(0!=i)
							igmplen += sizeof(igmp_grouprec_t);
					}
				}
			}
			v3_report->igmpr_numgrps = htons(i);
			/* Checksum */
			v3_report->igmpr_cksum = 0;
			v3_report->igmpr_cksum = in_cksum((unsigned short*) v3_report, igmplen);
			/* Send out the report message */
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = inet_addr(IGMP_ALL_ROUTERS_V3);
			sendto(wan_igmp_socket, (void*) v3_report, igmplen, 0, (struct sockaddr*)&sin, sizeof(sin));
		}
		/* send igmp membership report including group report only changed according to wan igmp version */
		else
		{
			if(wan_igmp_version == IGMP_VERSION_3)
			{
				for (ifp1=router.igmprt_interfaces; ifp1;ifp1=ifp1->igmpi_next)
				{
					if(ifp1->igmpi_addr.s_addr == lan_ipaddr)
					{
						for (j=0,gp1=ifp1->igmpi_groups; gp1; gp1=gp1->igmpg_next)
						{
							/* send report to wan when we first join or leave a group */
							if(gp1->igmpg_flags == FIRST_JOIN_GROUP || gp1->igmpg_flags == LAST_LEAVE_GROUP)
							{
								arecord.igmpg_type = gp1->igmpg_type;
								arecord.igmpg_datalen = 0;
								arecord.igmpg_numsrc = 0;
								arecord.igmpg_group.s_addr = gp1->igmpg_addr.s_addr;
								if(gp1->igmpg_flags== LAST_LEAVE_GROUP)
									gp1->igmpg_flags = LAST_LEAVE_GROUP_HAD_SEND;

								memcpy(((char *)&(v3_report->igmpr_group[j])), &arecord, sizeof(igmp_grouprec_t));
								if(0!=j)
									igmplen += sizeof(igmp_grouprec_t);	
								j++;

							}
							if(0==j)
							{
								if (pbuf)
									free(pbuf);
								return -1;
							}

						}
						v3_report->igmpr_numgrps = htons(j);
						/* Checksum */
						v3_report->igmpr_cksum = 0;
						v3_report->igmpr_cksum = in_cksum((unsigned short*) v3_report, igmplen);
						
					}
				}

				/* Send out the report message */
				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = inet_addr(IGMP_ALL_ROUTERS_V3);
				sendto(wan_igmp_socket, (void*) v3_report, igmplen, 0, (struct sockaddr*)&sin, sizeof(sin));
				
			}
			else
			{
				send_membership_report_v3_to_v12();
			}
			
		}

    }
	if (pbuf)
		free(pbuf);
    return 0;
}


/* 
 *	Change v12 membership report to v3 and send to wan port
 */
int send_membership_report_v12_to_v3(struct in_addr group, int type)
{
	igmp_report_t *igmpv3_report;
	char *p1buf;
    igmp_interface_t *ifp_2;
	int igmplen_12=0;
	igmp_grouprec_t  arecord;
	struct sockaddr_in sin;

	p1buf = (char*) malloc(sizeof(igmp_report_t));
        if (p1buf == NULL){
            printf(" Can not allocate memoey for report packet\n");
            return -1;
        }
	
	if (group.s_addr == inet_addr(IGMP_ALL_ROUTERS_V3) || 
		group.s_addr == inet_addr(IGMP_ALL_ROUTERS))
	{
			if (p1buf)
				free(p1buf);
			return 0;
	}

	igmpv3_report = (igmp_report_t *) p1buf;
    igmpv3_report->igmpr_type = IGMP_V3_MEMBERSHIP_REPORT;
    igmpv3_report->igmpr_code = 0;
    igmpv3_report->igmpr_rsv = 0;
    igmpv3_report->igmpr_cksum = 0;
	igmpv3_report->igmpr_numgrps = htons(1);
    
    igmplen_12 += 8;

	arecord.igmpg_type = type;
	arecord.igmpg_datalen = 0;
	arecord.igmpg_numsrc = 0;
	arecord.igmpg_group.s_addr = group.s_addr;

	memcpy(((char *)&(igmpv3_report->igmpr_group[0])), &arecord, 8);
	igmplen_12 += 8;

	igmpv3_report->igmpr_cksum = in_cksum((unsigned short*) igmpv3_report, igmplen_12);
        
    /* Send out the report message */
    sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(IGMP_ALL_ROUTERS_V3);

	sendto(wan_igmp_socket, (void*) igmpv3_report, igmplen_12, 0, (struct sockaddr*)&sin, sizeof(sin));
	
	if (p1buf)
		 free(p1buf);

	return 0;
}

/*
 * Change v3 membership report to v12 and send to wan port
 */
int send_membership_report_v3_to_v12()
{
	igmpr_t *v12_report;
    char *p2buf;
    igmp_interface_t *ifp2;
    igmp_group_t *gp2;
	int igmplen2=0, recordnum2=0;
	struct sockaddr_in sin;
	
	for (ifp2=router.igmprt_interfaces; ifp2;ifp2=ifp2->igmpi_next)
    {
		if(ifp2->igmpi_addr.s_addr == lan_ipaddr)
			for (gp2=ifp2->igmpi_groups; gp2; gp2=gp2->igmpg_next)
			{
				if ((gp2->igmpg_addr.s_addr != 0) && (recordnum2 < 64) && (gp2->igmpg_members != NULL)){
					recordnum2++;
					/* TODO: may be copy all src and member to the array */
				}
			}
    }
	
	if (recordnum2 > 0)
    {
        p2buf = (char*) malloc(sizeof(igmpr_t));
        if (p2buf == NULL){
            printf(" Can not allocate memoey for report packet\n");
            return -1;
        }

        v12_report = (igmpr_t *) p2buf;
        v12_report->igmpr_type = wan_igmp_version;
        v12_report->igmpr_code = 0;
        igmplen2 += 8;
		for (ifp2=router.igmprt_interfaces; ifp2;ifp2=ifp2->igmpi_next)
		{
			if(ifp2->igmpi_addr.s_addr == lan_ipaddr)
				for (gp2=ifp2->igmpi_groups; gp2; gp2=gp2->igmpg_next)
				{
					if (gp2->igmpg_addr.s_addr == 0){
						continue;
						}
					/* We cannot send igmpv12 packet whose dst ip is "224.0.0.22" and "224.0.0.2" to wan port */
					if (gp2->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS_V3) ||
						gp2->igmpg_addr.s_addr == inet_addr(IGMP_ALL_ROUTERS))
					{
						 continue;
					}
					/*Fill in group address*/
					/* First join a group */
					if(gp2->igmpg_flags == FIRST_JOIN_GROUP) 
					{
						memcpy(&(v12_report->igmpr_group),&(gp2->igmpg_addr),4);
						memcpy(&sin.sin_addr,&(gp2->igmpg_addr),4);
					}
					else
						/* Last leave a group */
						if(gp2->igmpg_flags == LAST_LEAVE_GROUP)
						{
							if(wan_igmp_version == IGMP_VERSION_2)
							{
								v12_report->igmpr_type = IGMP_V2_LEAVE_GROUP;
								memcpy(&(v12_report->igmpr_group),&(gp2->igmpg_addr),4);
								sin.sin_addr.s_addr = inet_addr(IGMP_ALL_ROUTERS);
							}
						}
		            
					/* Checksum */
        			v12_report->igmpr_cksum = 0;
					v12_report->igmpr_cksum = in_cksum((unsigned short*) v12_report, igmplen2);
					/* Send out the report message */
					sin.sin_family = AF_INET;
					
					sendto(wan_igmp_socket, (void*) v12_report, igmplen2, 0, (struct sockaddr*)&sin, sizeof(sin));
				}
		}
	}

	if(p2buf)
		free(p2buf);

	return 0;
}

/*
 * send out a v2 leave group message
 */
int send_leave_group_v2(struct in_addr group)
{
	igmpr_t *v2_report;
	igmp_interface_t* ifp;
    char *pbuf;
    int igmplen=0,size;
	struct sockaddr_in sin;
    
    pbuf = (char*) malloc(sizeof(igmpr_t));
    if (pbuf == NULL){
        printf(" Can not allocate memoey for report packet\n");
        return -1;
        }
    v2_report = (igmpr_t *) pbuf;
	if (wan_igmp_version == IGMP_VERSION_2)
		v2_report->igmpr_type = IGMP_V2_LEAVE_GROUP;
	else
	{
		if (pbuf)
			free(pbuf);
		return 0;
	}
    v2_report->igmpr_code = 0;
    v2_report->igmpr_cksum = 0;
    igmplen += 4;
 
	/*Fill in group address*/
    memcpy(&(v2_report->igmpr_group),&group,4);
    
    igmplen += 4;
	/* Checksum */
	v2_report->igmpr_cksum = in_cksum((unsigned short*) v2_report, igmplen);
    /* Send out the leave group message */
    size = sizeof(*v2_report);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(IGMP_ALL_ROUTERS);
			
	sendto(wan_igmp_socket, (void*) v2_report, size, 0, (struct sockaddr*)&sin, sizeof(sin));

    if (pbuf)
        free(pbuf);

    return 0;
}


/*
 * void receive_membership_query()
 * handle a reception of membership query
 */
void receive_membership_query(igmp_router_t* igmprt,
				  igmp_interface_t *ifp,
                  struct in_addr gp,
                  struct in_addr *sources,
                  u_long src_query,
                  int numsrc,
			      int srsp,
                  int version)
{
    igmp_group_t *gr;
    igmp_src_t *src;
    int i;
	struct in_addr group={0};

    if (src_query < ifp->igmpi_addr.s_addr){ /* another querier is present with lower IP adress*/
        ifp->igmpi_oqp=IGMP_OQPI;/*set the Other-Querier-Present timer to OQPI*/
        ifp->igmpi_isquerier = FALSE;
    }
    if ( srsp == FALSE){ /* Supress Router-Side Processing flag not set */
        gr=igmp_interface_group_lookup(ifp,gp);
        if (gr != NULL){
            if (numsrc == 0){
                /*group specific query*/
                /* group timer is lowered to LMQI*/
                gr->igmpg_timer = IGMP_TIMER_SCALE;
            }else{
                /*group and source specific query*/
                for (i=0;i < numsrc; i++){
                    src=igmp_group_src_lookup(gr,sources[i]);
                    if (src != NULL)
                        src->igmps_timer= IGMP_TIMER_SCALE;
                }
        
            }
		}
      }

	 wan_version_timer = IGMP_WAN_VERSION_TIMER;

     switch(version)
		{
		case IGMP_VERSION_1:
			wan_igmp_version = IGMP_VERSION_1;
			send_membership_report_v12(igmprt,group,version);
			break;
		case IGMP_VERSION_2:
			wan_igmp_version = IGMP_VERSION_2;
			send_membership_report_v12(igmprt,group,version);
			break;
		case IGMP_VERSION_3:
			wan_igmp_version = IGMP_VERSION_3;
			send_membership_report_v3(TRUE);
			break;
		default:
			break;
		}
}






 
