/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: nsmp2ew.c,v 1.20 2004/03/01 18:02:58 luetgert Exp $
 *
 *    Revision history:
 *     $Log: nsmp2ew.c,v $
 *     Revision 1.20  2004/03/01 18:02:58  luetgert
 *     *** empty log message ***
 *
 *     Revision 1.19  2004/02/27 22:35:19  luetgert
 *     Fixed xml file write
 *
 *     Revision 1.18  2003/01/27 16:44:33  lombard
 *     The problems that prevented sm_nsmp2ew from running on sparc Solaris were
 *     fixed by changes Jim Luetgert made to the K2 header file. The original K2
 *     header file had some fields that were not byte-aligned for sparc hardware,
 *     which resulted in the program crashing.
 *
 *     Revision 1.17  2002/12/06 01:15:33  dietz
 *     added new instid argument to file2ew_ship call;
 *     changed fprintf(stderr...) calls to logit("e"...)
 *
 *     Revision 1.16  2002/02/25 21:29:01  lucky
 *     Fixed translation from box to SCN
 *
 *     Revision 1.15  2001/08/30 23:56:23  dietz
 *     changed a return code from -1 to 0 so non-EVT files are
 *     treated properly.
 *
 *     Revision 1.14  2001/05/03 23:07:35  dietz
 *     Changed to write TYPE_STRONGMOTIONII msgs.
 *     NOTE: sm_nsmp2ew DOES NOT run properly on Sparc Solaris!!!
 *
 *     Revision 1.13  2001/03/27 01:09:57  dietz
 *     Added support for reading heartbeat file contents.
 *     Currently file2ewfilter_hbeat() is a dummy function.
 *
 *     Revision 1.12  2001/03/13 21:30:10  dietz
 *     Jim tweaked the XML file writer a bit.
 *
 *     Revision 1.11  2001/03/09 17:14:03  dietz
 *     Jim fixed a bug in the response spectrum calculations
 *
 *     Revision 1.10  2001/02/08 16:36:02  dietz
 *      Added file2ewfilter_com function to read config file commands
 *     and file2ewfilter_shutdown() to cleanup before exit.
 *     NOTE: nsmp2ew.c has never worked properly on Sparc Solaris.
 *           It does work on NT and Intel Solaris.
 *
 *     Revision 1.9  2000/12/08 20:02:04  dietz
 *     Fixed a bug in which lat/lon was being written to the XML file
 *
 *     Revision 1.8  2000/12/08 18:39:15  dietz
 *     New feature (by Jim) to always write an XML file to the
 *     XML subdirectory of its input directory.
 *
 *     Revision 1.7  2000/11/27 21:34:17  dietz
 *     Added array bound checking in amaxper().
 *
 *     Revision 1.6  2000/11/21 22:04:01  dietz
 *     *** empty log message ***
 *
 *     Revision 1.5  2000/11/21 22:01:58  dietz
 *     New version from Jim Luetgert enables channel to SCNL mapping.
 *
 *     Revision 1.3  2000/11/03 21:33:03  dietz
 *     Replaces dummy functions with real functions which read K2-format .evt data
 *     files and calculate peak acceleration, velocity, etc.
 *     Written by Jim Luetgert, November,2000.
 *
 *     Revision 1.2  2000/10/20 18:47:14  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/09/26 22:59:05  dietz
 *     Initial revision
 *
 */

/*  nsmp2ew.c
 *  Read a K2-format .evt data file (from the NSMP system),
 *  create Earthworm strongmotion messages, and place them in a transport
 *  ring.  Returns number of TYPE_STRONGMOTION messages written to ring
 *  from file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <earthworm.h>
#include <chron3.h>
#include <kom.h>
#include <k2evt2ew.h>
#include <rw_strongmotionII.h>
#include "file2ew.h"


#ifdef _INTEL
/*  macro returns byte-swapped version of given 16-bit unsigned integer */
#define BYTESWAP_UINT16(var) \
               ((unsigned short)(((unsigned short)(var)>>(unsigned char)8) + \
               ((unsigned short)(var)<<(unsigned char)8)))

/*  macro returns byte-swapped version of given 32-bit unsigned integer */
#define BYTESWAP_UINT32(var) \
                 ((unsigned long)(((unsigned long)(var)>>(unsigned char)24) + \
        (((unsigned long)(var)>>(unsigned char)8)&(unsigned long)0x0000FF00)+ \
        (((unsigned long)(var)<<(unsigned char)8)&(unsigned long)0x00FF0000)+ \
                                 ((unsigned long)(var)<<(unsigned char)24)))
#endif INTEL

#ifdef _SPARC
#define BYTESWAP_UINT16(var) (var)
#define BYTESWAP_UINT32(var) (var)
#endif

#define PACKET_MAX_SIZE  3003      /* 2992 + 8 + 3, same as QT */
#define MAXTRACELTH 20000
#define GRAVITY 978.03      /* Gravity in cm/sec/sec */
#define SWAP(a,b) temp=(a);(a)=(b);(b)=temp
#define DECADE   315532800  /* Number of seconds betweeen 1/1/1970 and 1/1/1980 */

/* Functions in this source file
 *******************************/
int nsmp2ew( FILE *fp, char *fname );

/* Functions in other source files
 *********************************/
void logit( char *, char *, ... );         /* logit.c      sys-independent  */
int  GetType ( char *, unsigned char * );  /* getutil.c    sys-independent  */

/* Globals from file2ew.c
 ************************/
extern int Debug;
extern char NetworkName[];

/* Local globals
 ***************/
#define MAXCHAN  100             /* default maximum # channels            */
static int MaxChan = MAXCHAN;    /* configured max# chan (MaxChannel cmd) */
static int NchanNames = 0;	 /* number of names in this config file   */
CHANNELNAME *ChannelName = NULL; /* table of box/channel to SCNL          */

#define BUFLEN  65000           /* define maximum size for an event msg  */
static char MsgBuf[BUFLEN];     /* char string to hold output message    */
static unsigned char TypeSM2;   /* message type we'll produce            */

static int   Init = 0;     /* initialization flag */

/******************************************************************************
 * file2ewfilter_com() processes config-file commands to set up the           *
 *                     CSMIP-specific parameters                              *
 * Returns  1 if the command was recognized & processed                       *
 *          0 if the command was not recognized                               *
 * Note: this function may exit the process if it finds serious errors in     *
 *       any commands                                                         *
 ******************************************************************************/
int file2ewfilter_com( )
{
   char   *str;

/* Reset the maximum number of channels
 **************************************/
   if( k_its("MaxChannel") ) { /*optional*/
      MaxChan = k_int();
      if( ChannelName != NULL ) {
         logit( "e","nsmp2ew_com: must specify <MaxChan> before any"
                " <ChannelName> commands; exiting!\n" );
         exit( -1 );
      }
      return( 1 );
   }

/* Get the mappings from box id to SCNL name
 ********************************************/
   if( k_its("ChannelName") ) {  /*optional*/
   /* First one; allocate ChannelName struct */
      if( ChannelName == NULL ) {
         ChannelName = (CHANNELNAME *) malloc (MaxChan * sizeof(CHANNELNAME));
         if( ChannelName == NULL ) {
            logit( "e", "nsmp2ew_com: Error allocating %ld bytes for"
                   " %d chans; exiting!\n",
                   MaxChan*sizeof(CHANNELNAME), MaxChan );
            exit( -1 );
         }
      }
   /* See if we have room for another channel */
      if( NchanNames >= MaxChan ) {
         logit( "e", "nsmp2ew_com: Too many <ChannelName> commands"
                " in configfile; MaxChannel=%d; exiting!\n",
                (int) MaxChan );
         exit( -1 );
      }
   /* Get the box name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>SM_BOX_LEN ) { 
            logit( "e", "nsmp2ew_com: box name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].box, str);
      }
   /* Get the channel number */
      ChannelName[NchanNames].chan = k_int();
      if(ChannelName[NchanNames].chan > SM_MAX_CHAN){
         logit( "e", "nsmp2ew_com: Channel number %d greater "
                "than %d in <ChannelName> cmd; exiting\n",
		 ChannelName[NchanNames].chan,SM_MAX_CHAN);
         exit(-1);
      }
   /* Get the SCNL name */
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_STA_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: station name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].sta, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_CHAN_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: component name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].comp, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_NET_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: network name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].net, str);
      }
      if( ( str=k_str() ) ) {
         if(strlen(str)>TRACE_LOC_LEN ) { /* from trace_buf.h */
            logit( "e", "nsmp2ew_com: location name <%s> too long"
                   " in <ChannelName> cmd; exiting!\n", str );
            exit( -1 );
         }
         strcpy(ChannelName[NchanNames].loc, str);
      }
      NchanNames++;
      return( 1 );
   } 		

   
   return( 0 );
}

/******************************************************************************
 * file2ewfilter()   Do all the work to convert NSMP files to Earthworm msgs  *
 ******************************************************************************/
int file2ewfilter( FILE *fp, char *fname )
{
	K2InfoStruct	*pk2Info;
    char     whoami[50], xmldir[50], fnew[90], xname[150], *cptr;
    long     minute;
	struct Greg  g;
    FILE     *fx;
    int      i, nmsg;
    double   secs, sex, lat, lon;
    double   sec1970 = 11676096000.00;  /* # seconds between Carl Johnson's        */
                                        /* time 0 and 1970-01-01 00:00:00.0 GMT    */


/* Initialize variables
 **********************/
    sprintf(whoami, " %s: %s: ", "sm_file2ew", "nsmp2ew");
    if(!Init) 
		file2ewfilter_init();
		
	/* Allocate the k2Info structure
	 *******************************/
	if ((pk2Info = (K2InfoStruct *) 

	malloc (sizeof (K2InfoStruct))) == NULL) 
	{
		logit ("e", "Could not malloc k2Info structure (%d bytes).\n",
                       sizeof(K2InfoStruct) );
		return -1;
	}

	/* Parse the file 
	 ******************/
	if (k2evt2ew (fp, fname, pk2Info, ChannelName, NchanNames, 
                                NetworkName, Debug) != EW_SUCCESS)
	{
		logit ("e", "Call to k2evt2ew failed -- file not processed.\n");
		free (pk2Info);
		return -1;
	}
	
	if(strcmp(pk2Info->sm[0].net, "--")==0) 
	{
		logit ("e", "Call to k2evt2ew failed -- Station %s unknown.\n", pk2Info->sm[0].sta);
		free (pk2Info);
		return -1;
	}

	nmsg = 0;
	for (i = 0; i < pk2Info->head.rwParms.misc.nchannels; i++)
	{

		/* Build TYPE_SM msg from last SM_DATA structure and ship it
		***********************************************************/
		if (wr_strongmotionII( &pk2Info->sm[i], MsgBuf, BUFLEN ) != 0 ) 
		{
		    logit("e","nsmp2ew: Error building TYPE_STRONGMOTIONII msg\n");
            free (pk2Info);
            return( -1 );
        }
    
        if (file2ew_ship( TypeSM2, 0, MsgBuf, strlen(MsgBuf) ) != 0 ) 
        {
            logit("e","nsmp2ew: file2ew_ship error\n");
            free (pk2Info);
            return( -1 );
        }

        nmsg = nmsg + 1;
    }




/* Make sure XML subdirectory exists
 ***********************************/
    sprintf (xmldir, "xml");
    if( CreateDir( xmldir ) != EW_SUCCESS ) 
    {
       logit( "e", "%s: trouble creating XML directory: /%s\n",
                  whoami, xmldir );
    } 
    else 
    {
        strcpy(xname, fname);
        cptr = strstr(xname, ".evt");
        *cptr = 0;
        strcat(xname, ".xml");
        sprintf(fnew,"%s/%s",xmldir, xname );
        fx = fopen( fnew, "wb" );
        if( fx == NULL ) 
        {
            logit( "e", "%s: trouble opening XML file: %s\n", whoami, fnew );
            free (pk2Info);
            return( nmsg );
        } 
        else     
        {
            fprintf(fx, 
                "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?> \n");
            fprintf(fx, "<amplitudes agency=\"NSMP\"> \n");
            fprintf(fx, "<record> \n");
            fprintf(fx, "<timing> \n");
            fprintf(fx, "<reference zone=\"GMT\" quality=\"0.5\"> \n");

            secs = pk2Info->sm[0].t;
            secs += sec1970;
            minute = (long) (secs / 60.0);
            sex = secs - 60.0 * minute;
            i = sex;
/*
            j = (sex - i)*1000;
*/
            grg(minute, &g);
            fprintf(fx, "<year value=\"%4d\"/>",   g.year);
            fprintf(fx, "<month value=\"%2d\"/>",  g.month);
            fprintf(fx, "<day value=\"%2d\"/>",    g.day);
            fprintf(fx, "<hour value=\" %2d\"/>",  g.hour);
            fprintf(fx, "<minute value=\"%2d\"/>", g.minute);
            fprintf(fx, "<second value=\"%2d\"/>", i);
            fprintf(fx, "<msec value=\"0\"/> \n");
            fprintf(fx, "</reference> \n");
            fprintf(fx, "<trigger value=\"0\"/> \n");
            fprintf(fx, "</timing> \n");

            lat = pk2Info->head.rwParms.misc.latitude;
            lon = pk2Info->head.rwParms.misc.longitude;

            for(i = 0; i < pk2Info->head.rwParms.misc.nchannels; i++) 
            {
                fprintf(fx, 
                    "<station code=\"%s\" lat=\"%7.3f\" lon=\"%8.3f\" name=\"%s\"> \n",
                         pk2Info->sm[i].sta, lat, lon, pk2Info->head.rwParms.misc.comment);
                fprintf(fx, "<component name=\"%s\"> \n", pk2Info->sm[i].comp);
                fprintf(fx, 
                    "<acc value=\"%f\" units=\"cm/s/s\"/> \n", pk2Info->sm[i].pga);
                fprintf(fx, "<vel value=\"%f\" units=\"cm/s\"/> \n", pk2Info->sm[i].pgv);
                fprintf(fx, "<sa period=\"0.3\" value=\"%f\" units=\"cm/s/s\"/> \n", 
                                                    pk2Info->sm[i].rsa[0]);
                fprintf(fx, 
                    "<sa period=\"1.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
                                                    pk2Info->sm[i].rsa[1]);
                fprintf(fx, 
                    "<sa period=\"3.0\" value=\"%f\" units=\"cm/s/s\"/> \n", 
                                                    pk2Info->sm[i].rsa[2]);
                fprintf(fx, "</component> \n");
                fprintf(fx, "</station> \n\n");
            }
            fprintf(fx, "</record> \n");
            fprintf(fx, "</amplitudes> \n");
            fclose(fx);
        }
    }

    free (pk2Info);
    return( nmsg );
}


/******************************************************************************
 * file2ewfilter_init()  check arguments for NSMP data                        *
 ******************************************************************************/
int file2ewfilter_init( void )
{
   int ret=0;

/* Look up earthworm message type(s)
 ***********************************/
   if ( GetType( "TYPE_STRONGMOTIONII", &TypeSM2 ) != 0 ) {
      logit( "e",
             "nsmp2ew: Invalid message type <TYPE_STRONGMOTIONII>; exiting!\n" );
      ret = -1;
   }
   Init = 1;
   return ret;
}


/******************************************************************************
 * file2ewfilter_hbeat()  read heartbeat file, check for trouble              *
 ******************************************************************************/
int file2ewfilter_hbeat( FILE *fp, char *fname, char *system )
{
   return( 0 );
}


/******************************************************************************
 * file2ewfilter_shutdown()  free memory, other cleanup tasks                 *
 ******************************************************************************/
void file2ewfilter_shutdown( void )
{
   free( ChannelName );
   return;
}
