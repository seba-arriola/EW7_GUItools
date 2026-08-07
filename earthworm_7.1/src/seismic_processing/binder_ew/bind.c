
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: bind.c,v 1.7 2005/08/01 21:12:31 dietz Exp $
 *
 *    Revision history:
 *     $Log: bind.c,v $
 *     Revision 1.7  2005/08/01 21:12:31  dietz
 *     Added optional "EventIdFile" command to set the name of the file
 *     which will store the next valid eventid for this instance of
 *     binder (default = quake_id.d). This will allow multiple instances
 *     of binder to run on the same host.
 *
 *     Revision 1.6  2005/03/11 18:44:44  dietz
 *     cleaned up to fix some compilation warnings
 *
 *     Revision 1.5  2005/03/09 19:10:40  dietz
 *     Made pick and quake fifo lengths configurable with new optional cmds:
 *       pick_fifo_length  xxx  (default=1000)
 *       quake_fifo_length yyy  (default=100)
 *
 *     Revision 1.4  2004/10/21 16:49:27  dietz
 *     Modified to allow pick association with entire quake list (previously
 *     only attempted assoc with 10 most recent quakes). Changes required
 *     keeping track of the earliest pick sequence number associated with each
 *     quake so that a hypocenter update is not attempted if some of the
 *     supporting picks are no longer in the pick FIFO.
 *
 *     Revision 1.3  2004/05/14 23:35:37  dietz
 *     modified to work with TYPE_PICK_SCNL messages only
 *
 *     Revision 1.2  2002/05/31 21:45:17  dietz
 *     changed to use logit instead of fprintf when processing config file
 *
 *     Revision 1.1  2000/02/14 16:08:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 16:07:49  lucky
 *     Initial revision
 *
 *
 */

               /**************************************
                *      bind.c : The associator.      *
                **************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kom.h>
#include <site.h>
#include <tlay.h>
#include "bind.h"
#include "grid.h"
#include "hyp.h"

#define hypot(x,y) (sqrt((x)*(x) + (y)*(y)))

/* Function prototypes
 *********************/
void   logit( char *, char *, ... );  /* logit.c  sys-independent  */
void   grid_cart(GEO *, CART *);
int    grid_stack( long );
void   bndr_link( long, long );
void   bndr_quake2k( long );
int    bind_cull( long );
void   bind_kill( long );
int    bind_locate( long );
void   bind_assess( long );
void   bind_quakes( void );
int    bind_scavenge( long );
double bind_taper( double );
void   bind_touch( long );
void   bind_update( long );
void   bind_wt( HYP *, PIX * );
double hyp_dis( HYP *, PIX * );
double hyp_out( int, int, HYP *, int, PIX * );
int    hyp_l1( int, HYP *, int, PIX * );
int    assess_pcks( HYP *, int, PIX * );

#define ABS(X) (((X) >= 0) ? (X) : -(X))

extern char EventIdFile[];            /* declared in binder_ew.c */

#define MAXPHS 400
static int  initBind = 0;
static int  maxQuake = 100;
static int  maxPick = 1000;
static long nextID = 1;
static HYP  Hyp;

static int nPix;
static PIX Pix[MAXPHS];

/*static int nBig    = MAXPHS;  */    /* 950111:ldd. after a quake has nBig picks,  */
/*static int locPick =      1;  */    /*             locate it every locPick picks  */
#define MAXSET 10                     /* 950308:bfh. Made nBig and locPick arrays   */
static int nSet = 0;                  /*             to allow more flexibility in   */
static int nBig[MAXSET];              /*             deciding how often to update   */
static int locPick[MAXSET];           /*             a hypocenter.                  */

static int MinAssess = 6;             /* 960617:ldd  run assess_pck when an eq has  */
static int MaxAssess = 12;            /*             between MinAssess & MaxAssess  */
                                      /*             picks associated with it       */
#define MAXQ 20
static int nQ = 0;
static long lQ[MAXQ];

static int hypCode = 0;
static double rmsCut = 1.0;
static double rMax = 150.0;

#define MAXTPR 100
static int nTaper = 0;
static struct {
        double  r;              /* Distance (kilometers)                */
        double  wid;            /* Width (seconds)                      */
} Taper[MAXTPR];

double OTconst1 = 2.0;          /* bfh 11-4-94:"variable" taper                 */
double OTconst2 = 2.0;          /*             proporional to estimated         */
double loc_DF_factor = 3.0;     /*             uncertianty in origin time.      */
double Npix = 5.0;              /*             (Adds onto Carls distance        */
double res1_OT;                 /*             taper.) ala Als's implimentation */
double res2_OT;                 /*             of Bill E. and Dave O's idea.    */

double s;                       /* 941212:bfh. SLANT DISTANCE.                  */
double rAvg_factor  = 10.0;     /* 941217:bfh.                                  */

double tdif_MIN     =  -1.0;    /* 950123:bfh. Read in MIN and MAX values for   */
double tdif_MAX     = 120.0;    /*             tdif from param. file: binder.d  */

#define MAXPH 10
static int nPh = 0;
static struct {
        int     iph;            /* Phase index                          */
        double  fac;            /* Weight factor                        */
} Ph[MAXPH];

#define MAXWT 5
static int nWt = 0;
static struct {
        char    wt;             /* Pick weight ('0' - '4')              */
        double  fac;            /* Weight factor                        */
} Wt[MAXWT];

/**********************************************************************
 * bind_init() : One time initialization, create quake and pick lists *
 **********************************************************************/
void bind_init( void )
{
        if(initBind)
                return;
        initBind = 1;

/* Allocate quake fifo */
        mQuake = (long) maxQuake;
/*      lQuake = 1; */                                          /* 950413:ldd. */
        lQuake = nextID;  /* eqs get sequential id#s between runs. 950413:ldd. */
        pQuake = (QUAKE *)calloc(mQuake, sizeof(QUAKE));
        if(!pQuake) {
                logit("et", "bind_init: Cannot allocate quake fifo; exiting!\n");
                exit(0);
        }
        iQuake = (long *)calloc(mQuake, sizeof(long));
        if(!iQuake) {
                logit("et", "bind_init: Cannot allocate quake list; exiting!\n");
                exit(0);
        }
        logit( "", "bind_init: quake fifo initialized (length=%ld)\n", mQuake );

/* Allocate pick fifo */
        mPick = (long) maxPick;
        lPick = 1;        /* first number of pick id sequence */
        pPick = (PICK *)calloc(mPick, sizeof(PICK));
        if(!pPick) {
                logit("et", "bind_init: Cannot allocate pick fifo; exiting!\n");
                exit(0);
        }
        iPick = (long *)calloc(mPick, sizeof(long));
        if(!iPick) {
                logit("et", "bind_init: Cannot allocate pick list; exiting!\n");
                exit(0);
        }
        logit( "", "bind_init: pick fifo initialized (length=%ld)\n", mPick );
}

/*************************************************************
 * bind_com: Process bind commands from binder's config file *
 *************************************************************/
int bind_com( void )
{
        char *str;
        int   tmp;

        if(k_its("hypcode")) {
                hypCode = k_int();
                if ( hypCode < 0 || hypCode > 7 )
                {
                   logit("e", "bind_com: Invalid hypcode; exiting!\n" );
                   exit( -1 );
                }
                return 1;
        }

        if(k_its("pick_fifo_length")) {
                maxPick = k_int();
                return 1;
        }

        if(k_its("quake_fifo_length")) {
                maxQuake = k_int();
                return 1;
        }

        if(k_its("rmscut")) {
                rmsCut = k_val();
                return 1;
        }

        if(k_its("t_dif")) {                    /* 950123:bfh. added command */
                tdif_MIN = k_val();
                tdif_MAX = k_val();
                return 1;
        }

        if(k_its("r_max")) {
                rMax = k_val();
                return 1;
        }

        if(k_its("ravg_factor") || k_its("rAvg_Factor")) {  /* 941217:bfh. added command. */
                rAvg_factor = k_val();
                return 1;
        }

        if(k_its("taper")) {
                if(nTaper < MAXTPR) {
                        Taper[nTaper].r   = k_val();
                        Taper[nTaper].wid = k_val();
                        nTaper++;
                        return 1;
                }
                logit("e","bind_com: Too many <taper> commands!  MAXTPR = %d;"
                          " exiting!\n", MAXTPR );
                exit( 0 );
        }
        if(k_its("taper_ot") || k_its("taper_OT")) {        /* 941104:bfh. added command */
                OTconst1 = k_val();
                OTconst2 = k_val();
                return 1;
        }

        if(k_its("ph")) {
                if(nPh < MAXPH) {
                        Ph[nPh].iph = -1;
                        str = k_str();
                        if(!str) return 1;
                        if(k_its("P"))  Ph[nPh].iph = 0;
                        if(k_its("S"))  Ph[nPh].iph = 1;
                        if(k_its("Pn")) Ph[nPh].iph = 2;
                        if(k_its("Sn")) Ph[nPh].iph = 3;
                        if(k_its("Pg")) Ph[nPh].iph = 4;
                        if(k_its("Sg")) Ph[nPh].iph = 5;
                        if(Ph[nPh].iph < 0)
                                return 0;
                        Ph[nPh].fac = k_val();
                        nPh++;
                        return 1;
                }
                logit("e", "bind_com: Too many <ph> commands!  MAXPH = %d;"
                           " exiting!\n", MAXPH );
                exit( 0 );
        }

        if(k_its("wt")) {
                if(nWt < MAXWT) {
                        str = k_str();
                        if(!str) return 1;
                        Wt[nWt].wt = str[0];
                        Wt[nWt].fac = k_val();
                        nWt++;
                        return 1;
                }
                logit("e", "bind_com: Too many <wt> commands!  MAXWT = %d;"
                           " exiting!\n", MAXWT );
                exit( 0 );
        }

        if(k_its("locate_eq")) {              /* 950111:ldd. added command loc_big_eq */
                if(nSet < MAXSET) {           /* 950308:bfh. changed nBig, locPick to */
                   nBig[nSet]    = k_int();   /*             arrays                   */
                   locPick[nSet] = k_int();   /* 950421:ldd. changed command name     */
                /* logit( "",
                          "locate_eq -> eq with > %3d picks, locates every %3d picks\n",
                           nBig[nSet], locPick[nSet] ); */
                   nSet++;
                   return 1;
                }
                logit("e", "bind_com: Too many <locate_eq> commands!  MAXSET = %d;"
                           " exiting!\n", MAXSET );
                exit( 0 );
        }

        if(k_its("next_id")) {       /* add 1 to value read from EventIdFile to insure    */
                nextID = k_long();   /* no sequence# overlap (ie, if binder bombed after  */
                nextID++;            /* assigning the number but before writing the file) */
                return 1;            /* 950413:ldd.                                       */
        }

        if(k_its("assess_pk")) {     /* set the range of #picks for running */
                MinAssess = k_int(); /* WLE's resampling associator         */
                MaxAssess = k_int(); /* 960920:ldd.                         */
                if( MaxAssess < MinAssess ) {
                    tmp = MinAssess;
                    MinAssess = MaxAssess;
                    MaxAssess = tmp;
                }
                return 1;
        }

        return 0;
}

/***************************************************************
 * bind_pick(pick) : Insert pick into pick list, and associate *
 ***************************************************************/
int bind_pick(PICK *pick)
{
        FILE  *idfp;
        TPHASE treg[10];
        char   tmplog[200];
        GEO geo;
        CART xyz_site;
        CART xyz_quake;
        long lquake;
        long l;
        long l1, l2;
        double tdif;
        double r;
        double resnrm;
        double resmin;
        double restpr;
        double res;
        double rhyp;
        double tres;
        double rmax;            /* Maximum dist of association  */
        int i;
        int iq;
        int ip;
        int is;
        int nph;
        int iph;

        bind_init();
        ip = lPick % mPick;
        pPick[ip]       = *pick;   /* structure copy */
        pPick[ip].lpick = lPick;   /* stash lPick for this pick */
        lPick++;

/* Try to associate with existing event */
        is = pick->site;
        geo.lat = (float) Site[is].lat;
        geo.lon = (float) Site[is].lon;
        geo.z   = (float) Site[is].elev;
        grid_cart(&geo, &xyz_site);
        l2 = lQuake;
/*      l1 = l2 - 10; */     /* originally only tries 10 most recent quakes */
        l1 = l2 - mQuake;    /* modified to try entire quake list           */
        if(l1 < 0)
                l1 = 0;
        iph = -1;
        resmin = 1000.0;
        for(l=l1; l<l2; l++) {
                iq = l % mQuake;
                if(pQuake[iq].npix == 0) /* Killed event? */
                        continue;
                tdif = pPick[ip].t - pQuake[iq].t;
/*              if(tdif < -1.0 || tdif > 120.0) continue;              950123:bfh. */
                if(tdif < tdif_MIN  ||  tdif > tdif_MAX) continue;  /* 950123:bfh. */
                geo.lat = (float) pQuake[iq].lat;
                geo.lon = (float) pQuake[iq].lon;
                geo.z   = (float) pQuake[iq].z;
                grid_cart(&geo, &xyz_quake);
                r = hypot(xyz_site.x-xyz_quake.x, xyz_site.y-xyz_quake.y);

/*              rmax = 3.0 * pQuake[iq].ravg;                            941104:bfh. */
                rmax = rAvg_factor * pQuake[iq].ravg;                 /* 941217:bfh. */
                if(rmax < 100.0)
                        rmax = 100.0;
                if(r > rmax)
                        continue;
                nph = t_region(r, xyz_quake.z, treg);

/*              restpr = bind_taper(r);    */                        /* 941104:bfh. */
/*----------following block replaces above commented line---------------941104:bfh. */
                Npix = pQuake[iq].npix;
                if( Npix <= 4.0 )
                         loc_DF_factor = 3.0;
                else
                         loc_DF_factor = sqrt( Npix/(Npix-4.0) );

                res1_OT = OTconst1 * pQuake[iq].rms * loc_DF_factor;

                s = sqrt(  ( pQuake[iq].z    * pQuake[iq].z    )
                         + ( pQuake[iq].dmin * pQuake[iq].dmin ) );

                if( s < 1.0 ) res2_OT = 0.0;
                else          res2_OT = OTconst2 * log10(s);

                restpr = bind_taper(r)
                        + res1_OT
                        + res2_OT;
/*----------------------------------------------------------------------end block---*/

                logit( "", "restpr = %.2f, r = %.2f, resmin = %.2f\n",
                       restpr, r, resmin);
                logit( "",
                      "restpr = dist-taper( %.2f ) + res1_OT( %.2f ) + res2_OT( %.2f )\n",
                       bind_taper(r), res1_OT, res2_OT);             /* 950421:ldd. */

                sprintf( tmplog, "  #%ld : ", l);
                for(i=0; i<nph; i++) {
                        res = pick->t - pQuake[iq].t - treg[i].t;    /* Travel-time residual. */
                        sprintf( tmplog+strlen(tmplog),
                                " %s(%.2f)", Phs[treg[i].phase], res);
                        resnrm = res;
                        if(resnrm < 0.0) resnrm = -resnrm;
                        if(resnrm > restpr)
                                continue;
                        if(resnrm < resmin) {
                                iph = treg[i].phase;
                                lquake = l ;
                                resmin = resnrm;
                                rhyp = r;
                                tres = res;
                        }
                }
                logit( "", "%s\n", tmplog );
        }
        if(iph > -1) {
                logit( "t",
                      "  bind : %8d %-5s %-3s %-2s %-2s %6.2f %6.2f - Assoc. as %s\n",
                       lquake, Site[is].name, Site[is].comp, 
                       Site[is].net, Site[is].loc,  
                       rhyp, tres, Phs[iph]);
                pPick[ip].quake = lquake;
                pPick[ip].phase = iph;
                bndr_link(lquake, lPick-1);
                bind_touch(lquake);
                bind_quakes();
                return 1;
        }

/* Hapless waif, try to stack and create new event. */
        if(!grid_stack(lPick-1))
                return 0;

/* A new event was created; update quake_id file...   950413:ldd. */
        idfp = fopen( EventIdFile, "w" );
        if ( idfp != (FILE *) NULL )
        {
                fprintf( idfp,
                        "# Next available earthquake sequence number:\n" );
                fprintf( idfp, "next_id %ld\n", lQuake );
                fclose( idfp );
        }

/* ...and process the new earthquake */
        bind_touch(lQuake-1);
        bind_quakes();
        return 1;
}

/******************************************************
 * bind_touch(lquake) : Schedule quake for relocation *
 ******************************************************/
void bind_touch(long lquake)
{
        int i;

        for(i=0; i<nQ; i++)
                if(lQ[i] == lquake)
                        return;
        if(nQ < MAXQ)
                lQ[nQ++] = lquake;
}

/************************************************
 * bind_quakes() : Relocate all modified quakes *
 ************************************************/
void bind_quakes( void )
{
        long lquake;

        while(nQ > 0) {
                nQ--;
                lquake = lQ[nQ];
                bind_update(lquake);
        }
}

/*******************************************************************
 * bind_update(lquake) : Locate, cull, and scavenge as appropriate *
 *******************************************************************/
void bind_update(long lquake)
{
        int kres;
        int iq;

        iq = lquake % mQuake;

        if( pQuake[iq].lpickfirst != 0  &&         /* if quake is not a new stack, and */   
            pQuake[iq].lpickfirst < lPick-mPick )  /* its 1st assoc pick fell off FIFO */
        {
           logit( "t", "###### Event %ld cannot be updated; supporting picks"
                       " no longer in FIFO ######\n", lquake );
           return;
        }

        kres = bind_locate(lquake);
        if(kres == -1) {                /* 950111:ldd. */
                goto scavenge;          /* 950421:ldd. */
        }                               /* 950111:ldd. */
        if(kres == 0
        || pQuake[iq].rms > rmsCut) {
                bind_kill(lquake);
                return;
        }

/* Cull */
        kres = bind_cull(lquake);
        if(kres < 1)
                goto scavenge;
        kres = bind_locate(lquake);
        if(kres == 0
        || pQuake[iq].rms > rmsCut) {
                bind_kill(lquake);
                return;
        }

/* Scavenge */
scavenge:
        kres = bind_scavenge(lquake);
        if(kres < 1)
                return;
        kres = bind_locate(lquake);
        if(kres == 0
        || pQuake[iq].rms > rmsCut) {
                bind_kill(lquake);
                return;
        }
}

/********************************************************
 * bind_locate(lquake) : Assemble data and locate quake *
 ********************************************************/
int bind_locate(long lquake)
{
        long l;
        long l1, l2;
        int  iq;
        int  ip;
        int  kres;
        int  locate;                   /* 950309:bfh. */
        int  i;                        /* 950309:bfh. */
        long lpickfirst = 0;               

/* Get most recent location for this event
 *****************************************/
        iq = lquake % mQuake;
        Hyp.t   = pQuake[iq].t;
        Hyp.lat = pQuake[iq].lat;
        Hyp.lon = pQuake[iq].lon;
        Hyp.z   = pQuake[iq].z;

/* Gather up all picks associated with this eq
 *********************************************/
        l2 = lPick;
        l1 = l2 - mPick;
        if(l1 < 0)
                l1 = 0;

        nPix = 0;
        for(l=l1; l<l2; l++) {
                ip = l % mPick;
                if(pPick[ip].quake != lquake)   continue;
                if(pPick[ip].lpick<lpickfirst || lpickfirst==0) lpickfirst = pPick[ip].lpick;
                Pix[nPix].id   = l;
                Pix[nPix].t    = pPick[ip].t;
                Pix[nPix].site = pPick[ip].site;
                Pix[nPix].ph   = pPick[ip].phase;
                Pix[nPix].ie   = pPick[ip].ie;
                Pix[nPix].fm   = pPick[ip].fm;
                Pix[nPix].wt   = pPick[ip].wt;
                bind_wt(&Hyp, &Pix[nPix]);
                Pix[nPix].flag = 'T';
                if(++nPix == MAXPHS)  break;
        }
        pQuake[iq].lpickfirst = lpickfirst;  /* reset quake's 1st assoc pick id */

        if(nPix < 4) {
                return 0;
        }

/*---------------------------------------------------Carl's original code--------*/
/*      kres = hyp_l1(hypCode, &Hyp, nPix, Pix);*/  /*these lines are Carl's     */
/*      if(kres < 1) {                          */  /*original code; replaced by */
/*              return 0;                       */  /*following block of code    */
/*      }                                       */  /*by ldd on 950111           */
/*      if(hypCode)                             */
/*              hyp_out(hypCode, lquake, &Hyp, nPix, Pix);      */
/*      pQuake[iq].t = Hyp.t;                   */
/*      pQuake[iq].lat = Hyp.lat;               */
/*      pQuake[iq].lon = Hyp.lon;               */
/*      pQuake[iq].z = Hyp.z;                   */
/*      pQuake[iq].rms = Hyp.rms;               */
/*      pQuake[iq].dmin = Hyp.dmin;             */
/*      pQuake[iq].ravg = Hyp.ravg;             */
/*      pQuake[iq].gap = Hyp.gap;               */
/*      pQuake[iq].npix = nPix;                 */
/*      pQuake[iq].nmod++;                      */
/*      bndr_quake2k(lquake);                   */
/*      return 1;                               */
/*----------------------------------------------------end original block---------*/

/*--------------------------------------------------- 950111:ldd start block-----*/
/*--------------------------------------------------- 950127:ldd rearranged -----*/
/*--------------------------------------------------- 950421:ldd added arrays ---*/
        locate = 1;
        if ( nSet>0  &&  nPix>nBig[0] ) {
                for( i=0; i < nSet-1; i++ ) {                /* find interval by */
                        if( nPix >= nBig[i+1] ) continue;    /* # picks assoc    */
                        break;
                }
                if( ( (nPix-nBig[i])%locPick[i] ) != 0 ) {   /*    locate eq?    */
                        locate =  0;                         /* we won't locate  */
                        kres   = -1;                         /* this eq now, but */
                        pQuake[iq].npix = nPix;              /* we'll update its */
                        pQuake[iq].nmod++;                   /* npk- & mod-count */
                }
        }
        if (locate) {
                bind_assess(lquake);  /*run WLE's acceptance tests; added 960920:ldd */
                if(nPix < 4) {
                    return 0;
                }
                kres = hyp_l1(hypCode, &Hyp, nPix, Pix);
                if(kres == 0) {
                        return 0;
                }
                if (hypCode)
                        hyp_out(hypCode, lquake, &Hyp, nPix, Pix);
                pQuake[iq].t    = Hyp.t;
                pQuake[iq].lat  = Hyp.lat;
                pQuake[iq].lon  = Hyp.lon;
                pQuake[iq].z    = Hyp.z;
                pQuake[iq].rms  = Hyp.rms;
                pQuake[iq].dmin = Hyp.dmin;
                pQuake[iq].ravg = Hyp.ravg;
                pQuake[iq].gap  = Hyp.gap;
                pQuake[iq].npix = nPix;
                pQuake[iq].nmod++;

        }
        bndr_quake2k(lquake);

        return (kres);
/*--------------------------------------------------- 950111:ldd end block------*/
}

/********************************************************************
 *  bind_assess()  takes the HYP and PIX structures filled in by    *
 *  bind_locate and runs Bill Ellsworth's acceptance tests on them. *
 *  If some picks are rejected they are removed from the structure, *
 *  a new trial hypocenter is determined (placed in HYP), the       *
 *  accepted picks are re-weighted, and the rejected picks are      *
 *  officially "unlinked" (freed to restack or reassociate).        *
 *                                                 added 960920:ldd *
 ********************************************************************/
void bind_assess( long lquake )
{
   HYP tmphyp;     /* hypocenter returned by assess_pck */
   int nreject;
   int np;
   int i;
   int ip;
   int iq;

   iq = lquake % mQuake;

/* Should we run the pick-assessment now?
 ***************************************/
   if( MaxAssess == 0   ) return;       /* turned off in config file  */

   if( nPix < MinAssess ) return;       /* not enough picks yet       */

   if( nPix > MaxAssess  &&             /* too many picks AND         */
       pQuake[iq].assessed  ) return;   /* it's already been assessed */

/* OK, go for it
 ***************/
   tmphyp  = Hyp;
   nreject = assess_pcks( &tmphyp, nPix, Pix );

/* Log results
 *************/
   if( nreject == 0 ) {
        logit("", "  bind :  All picks accepted by assess_pcks\n" );
        if( pQuake[iq].assessed ) return;
   }

   if( nreject < 0 ) {
        logit("", "  bind :  Too few qualifying picks to complete assess_pcks\n" );
        nreject = ABS(nreject)-1;  /* correct out the error code to get */
                                   /* the #picks rejected as glitches   */
   }
   else {
        logit("", "  bind :  Starting hypocenter from assess_pcks\n" );
   }
   pQuake[iq].assessed = 1;

/* Update stuff if some picks were rejected or
   if we want to use trial hypocenter from assess_pcks
 *****************************************************/
   Hyp = tmphyp;                    /* use assess_pck's hypocenter */
   np  = 0;
   for(i=0; i<nPix; i++)
   {
        if( Pix[i].flag == 'T' ||   /* re-weight accepted picks */
            Pix[i].flag == '?'   )
        {
            bind_wt(&Hyp, &Pix[i]);
            if(np<i) Pix[np] = Pix[i];
            np++;
        }
        else                        /* throw away rejected pcks */
        {
            ip = Pix[i].id % mPick;
            pPick[ip].quake = 0;
            bndr_link(0, Pix[i].id);
        }
   }
   nPix = np;

   return;
}

/*************************************************
 * bind_cull(lquake) : Delete outlying arrivals  *
 *************************************************/
int bind_cull(long lquake)
{
        GEO geo;
        CART xyz_quake;
        CART xyz_site;
        double r;
        double dtdr, dtdz;
        double tres;
        double restpr;
        long l, l1, l2;
        int ip;
        int iq;
        int is;
        int ncull;

        ncull = 0;
        iq = lquake % mQuake;
        geo.lat = (float) pQuake[iq].lat;
        geo.lon = (float) pQuake[iq].lon;
        geo.z   = (float) pQuake[iq].z;
        grid_cart(&geo, &xyz_quake);
        l2 = lPick;
        l1 = l2 - mPick;
        if(l1 < 0)
                l1 = 0;
        for(l=l1; l<l2; l++) {
                ip = l % mPick;
                if(pPick[ip].quake != lquake)
                        continue;
                is = pPick[ip].site;
                geo.lat = (float) Site[is].lat;
                geo.lon = (float) Site[is].lon;
                geo.z   = (float) Site[is].elev;
                grid_cart(&geo, &xyz_site);
                r = hypot(xyz_site.x-xyz_quake.x, xyz_site.y-xyz_quake.y);

/*              restpr = bind_taper(r);                                 941107:bfh. */
/*------following block replaces above commented line ------------------941107:bfh. */
                Npix = pQuake[iq].npix;
                if( Npix <= 4.0 )
                         loc_DF_factor = 3.0;
                else
                         loc_DF_factor = sqrt( Npix/(Npix-4.0) );

                res1_OT = OTconst1 * pQuake[iq].rms * loc_DF_factor;

                s = sqrt(  ( pQuake[iq].z    * pQuake[iq].z    )
                         + ( pQuake[iq].dmin * pQuake[iq].dmin ) );

                if( s < 1.0 ) res2_OT = 0.0;
                else          res2_OT = OTconst2 * log10(s);

                restpr = bind_taper(r)
                        + res1_OT
                        + res2_OT;
/* -------------------------------------------------------------------end block-----*/

                tres = pPick[ip].t - pQuake[iq].t
                        - t_phase(pPick[ip].phase, r, pQuake[iq].z,
                                &dtdr, &dtdz);
                if(tres < -restpr || tres > restpr) {
                        logit( "t", "  bind : %8d %-5s %-3s %-2s %-2s %6.2f %6.2f - Culled.\n",
                               lquake, Site[is].name, Site[is].comp, 
                               Site[is].net, Site[is].loc, r, tres);
                        pPick[ip].quake = 0;
                        bndr_link(0, l);
                        ncull++;
                }
        }
        return ncull;
}

/************************************************************************
 * bind_scavenge(lquake) : Scavenge picks from waifs and smaller quakes *
 ************************************************************************/
int bind_scavenge(long lquake)
{
        TPHASE treg[10];
        GEO geo;
        CART xyz_quake;
        CART xyz_site;
        double r;
        double tres;
        double res;
        double resnrm;
        double resmin;
        double restpr;
        double rhyp;
        double rmax;
        int i;
        long l, l1, l2;
        int iph;
        int nph;
        int ip;
        int iq;
        int jq;
        int is;
        int nscav;

        nscav = 0;
        iq = lquake % mQuake;
        geo.lat = (float) pQuake[iq].lat;
        geo.lon = (float) pQuake[iq].lon;
        geo.z   = (float) pQuake[iq].z;
        grid_cart(&geo, &xyz_quake);
        l2 = lPick;
        l1 = l2 - mPick;
        if(l1 < 0)
                l1 = 0;
        for(l=l1; l<l2; l++) {
                ip = l % mPick;
                if(pPick[ip].quake == lquake)
                        continue;
                if(pPick[ip].quake != 0) {                       /* bug fix:       */
                        jq = pPick[ip].quake % mQuake;           /* enclosed these */
                        if(pQuake[jq].npix >= pQuake[iq].npix)   /* lines in if{}  */
                                continue;                        /*    950515:ldd. */
                }
                is = pPick[ip].site;
                geo.lat = (float) Site[is].lat;
                geo.lon = (float) Site[is].lon;
                geo.z   = (float) Site[is].elev;
                grid_cart(&geo, &xyz_site);
                r = hypot(xyz_site.x-xyz_quake.x, xyz_site.y-xyz_quake.y);
/*              rmax = 3.0 * pQuake[iq].ravg;                          941104:bfh. */
                rmax = rAvg_factor * pQuake[iq].ravg;               /* 941217:bfh. */
                if(rmax < 100.0)
                        rmax = 100.0;
                if(r > rmax)
                        continue;
                nph = t_region(r, xyz_quake.z, treg);
                iph = -1;
                resmin = 1000.0;

/*              restpr = bind_taper(r);                                941107:bfh. */
/*-------following block replaces above commented line ----------------941107:bfh. */
                Npix = pQuake[iq].npix;
                if( Npix <= 4.0 )
                         loc_DF_factor = 3.0;
                else
                         loc_DF_factor = sqrt( Npix/(Npix-4.0) );

                res1_OT = OTconst1 * pQuake[iq].rms * loc_DF_factor;

                s = sqrt(  ( pQuake[iq].z    * pQuake[iq].z    )
                         + ( pQuake[iq].dmin * pQuake[iq].dmin ) );

                if( s < 1.0 ) res2_OT = 0.0;
                else          res2_OT = OTconst2 * log10(s);

                restpr = bind_taper(r)
                        + res1_OT
                        + res2_OT;
/* -------------------------------------------------------------------end block----*/

                for(i=0; i<nph; i++) {
                        res = pPick[ip].t - pQuake[iq].t - treg[i].t;
                        resnrm = res;
                        if(resnrm < 0.0) resnrm = -resnrm;
                        if(resnrm > restpr)
                                continue;
                        if(resnrm < resmin) {
                                iph = treg[i].phase;
                                resmin = resnrm;
                                rhyp = r;
                                tres = res;
                        }
                }
                if(iph >= 0) {
                        logit( "t", "  bind : %8d %-5s %-3s %-2s %-2s %6.2f %6.2f - Scavenged as %s\n",
                                lquake, Site[is].name, Site[is].comp, 
                                Site[is].net, Site[is].loc, 
                                rhyp, tres, Phs[iph]);
                        if(pPick[ip].quake != 0)
                                bind_touch(pPick[ip].quake);
                        pPick[ip].quake = lquake;
                        pPick[ip].phase = iph;
                        bndr_link(lquake, l);
                        nscav++;
                }
        }
        return nscav;
}

/***********************************************************
 * bind_kill(lquake) : Delete quake and disassociate picks *
 ***********************************************************/
void bind_kill(long lquake)
{
        long l, l1, l2;
        int ip;
        int iq;

        logit( "t", "###### Event %d killed. ######\n", lquake);
        l2 = lPick;
        l1 = l2 - mPick;
        if(l1 < 0)
                l1 = 0;
        for(l=l1; l<l2; l++) {
                ip = l % mPick;
                if(pPick[ip].quake != lquake)
                        continue;
                pPick[ip].quake = 0;
                bndr_link(0, l);
        }

        iq = lquake % mQuake;
        pQuake[iq].lpickfirst = 0;
        pQuake[iq].npix       = 0;
        bndr_quake2k(lquake);
}

/*******************************************************************
 * bind_wt: Assign weight to pick based on phase, wt, and distance *
 *******************************************************************/
void bind_wt(HYP *hyp, PIX *pix)
{
        double w;
        double r;
        double fac;
        int i;

        w = 1.0;
        if(nPh < 1)
                goto taper;
        for(i=0; i<nPh; i++) {
                if(pix->ph == Ph[i].iph) {
                        w *= Ph[i].fac;
                        goto taper;
                }
        }
        goto null;

taper:
        r = hyp_dis(hyp, pix);
        if(r > rMax)
                goto null;
        if(nTaper < 1)
                goto wt;
        fac = Taper[0].wid / bind_taper(r);
        w *= fac * fac;

wt:
        if(nWt < 1)
                goto pau;
        for(i=0; i<nWt; i++) {
                if(pix->wt == Wt[i].wt) {
                        w *= Wt[i].fac;
                        goto pau;
                }
        }
        goto null;

pau:
        pix->wt100 = (int) (100.0 * w);
        return;

null:
        pix->wt100 = 0;
        return;
}

/*******************************************************************
 * bind_taper:  return taper value (seconds) based on distance (r) *
 *******************************************************************/
double bind_taper( double r )
{
        double wid;
        int i;

        if(nTaper < 1)
                return 1.0;
        if(r < Taper[0].r)
                return Taper[0].wid;
        if(r > Taper[nTaper-1].r)
                return Taper[nTaper-1].wid;
        for(i=1; i<nTaper; i++) {
                if(r >= Taper[i].r)
                        continue;
                wid = Taper[i-1].wid
                    + (Taper[i].wid - Taper[i-1].wid)
                    * (r - Taper[i-1].r)
                    / (Taper[i].r - Taper[i-1].r);
                return wid;
        }
        return Taper[0].wid;
}

