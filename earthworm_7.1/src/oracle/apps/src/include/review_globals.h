/********************************************************************
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: review_globals.h,v 1.10 2003/01/30 23:12:57 lucky Exp $
 *
 *    Revision history:
 *     $Log: review_globals.h,v $
 *     Revision 1.10  2003/01/30 23:12:57  lucky
 *     *** empty log message ***
 *
 *     Revision 1.9  2002/05/28 19:33:56  lucky
 *     Added header and footer tag text
 *
 *     Revision 1.8  2002/05/28 17:24:05  lucky
 *     *** empty log message ***
 *
 *     Revision 1.7  2002/03/22 18:23:18  lucky
 *     Added retrieval of unpicked snippets on demand to speed things up
 *
 *     Revision 1.5  2002/02/19 16:39:37  lucky
 *     Added options for BackgroundColor, HeaderLogo, FooterLogo
 *
 *     Revision 1.4  2002/02/01 16:50:33  lucky
 *     Added SacBufferLen -- optional parameter to specify how long sac files are to be
 *
 *     Revision 1.3  2001/07/28 00:43:14  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.2  2001/07/25 20:58:44  lucky
 *     Added MakeLMPreferred
 *
 *     Revision 1.1  2001/07/01 21:55:17  davidk
 *     Initial revision
 *
 *
 ********************************************************************/

/* Environment stuff */
char  envEW_LOG[MAXPATH];        /* where variable EW_LOG will be stored */
char  envEW_PARAMS[MAXPATH];     /* where variable PARAMS will be stored */
char  DBservice[EQP_MAXWORD];    /* DBMS instance to interact with    */
char  DBuser[EQP_MAXWORD];       /* UserId to connect to database as  */
char  DBpassword[EQP_MAXWORD];   /* Password to datasource            */
char  EvtStructFile[MAXPATH];    /* Full path to the event struct binary file */
char  JavascriptFile[MAXPATH];   /* File containing Lomax Javascript */
char  SacFormat[MAXPATH];        /* Sparc or intel */
char  NetworkCode[EQP_MAXWORD];
char  WebDir[MAXPATH];
char  ReviewDir[MAXPATH];
char  WebTmpDir[MAXPATH];
char  ReviewTmpDir[MAXPATH];
char  HypoDir[MAXPATH];
char  HypoBin[MAXPATH];
char  HypoCfg[MAXPATH];
char  WebHost[MAXPATH];
char  AlarmRing[MAXPATH];
char  MyModuleID[MAXPATH];
char  MyInstID[MAXPATH];
char  *RevSrcs[EQP_MAXWORD];
char  LM_writeWA_configfile[MAXPATH];
char  LM_review_configfile[MAXPATH];
char  LM_progname[MAXPATH];
char  LM_outputfile[MAXPATH];
char  LM_method[EQP_MAXWORD];
int   ValidateQdds;
int   LM_LocalMagType;
int   NumRevSrcs;
int   NumArrivalPicksToShow;
int   NumAmplitudePicksToShow;
int   MakeLMPreferred;
long  SacBufferLen;
char  BackgroundColor[MAXPATH];
char  HeaderLogo[MAXPATH];
char  FooterLogo[MAXPATH];
char  HeaderTag[MAXPATH];
char  FooterTag[MAXPATH];
double  TaperLength;
double  LowFreqTaper1;
double  LowFreqTaper2;
double  HighFreqTaper1;
double  HighFreqTaper2;
int		Alarms_minPopulation;
int		Alarms_showPopulation;


/* Validate Qdds options */
double	MinDepth; 
double	MaxDepth; 
int		NumPhases; 
double	MaxGap; 
double	MaxDist; 
double	MaxRMS; 
double	MaxER0; 
double	MaxERH; 
double	MaxERZ; 
double	MinMag; 
int		NumMCTest; 
M_C_T	MC_Test[MAX_MCTEST];


SGOpt SeisGramOptions[MAX_SG2K_OPT];
int   NumSeisGramOptions;


int   DEBUG;                     /* debug flag */

int iNumTGDs;             /* Number of "seismic display program
                            links to add at bottom of page */
TraceGifDisplayStruct TGDS[MAX_TGDS];

