
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: sm_file2ew.h,v 1.4 2001/03/27 01:13:46 dietz Exp $
 *
 *    Revision history:
 *     $Log: sm_file2ew.h,v $
 *     Revision 1.4  2001/03/27 01:13:46  dietz
 *     *** empty log message ***
 *
 *     Revision 1.3  2001/02/08 16:36:02  dietz
 *     Generic definitions were moved into file2ew.h.  Only
 *     strong motion specifics remain.
 *
 *     Revision 1.2  2000/09/26 22:59:05  dietz
 *     *** empty log message ***
 *
 *     Revision 1.1  2000/02/14 19:19:05  lucky
 *     Initial revision
 *
 *
 */

/* 
 * sm_file2ew.h:
 *
 * LDD February2001
 */

#include <rw_strongmotionII.h>
#include "file2ew.h"

#define SM_BOX_LEN       25   /* maximum length of a box name               */
#define SM_MAX_CHAN      18   /* max number chans on one strongmotion box   */

/* Structure Definitions
 ***********************/
typedef struct _CHANNELNAME_ {
   char	box[SM_BOX_LEN];      /* Installation-assigned box name (or serial#) */
   int	chan;		      /* Channel number on this box                  */	
   char sta[TRACE_STA_LEN];   /* NTS: Site code as per IRIS SEED             */ 
   char comp[TRACE_CHAN_LEN]; /* NTS: Channel/Component code as per IRIS SEED*/ 
   char net[TRACE_NET_LEN];   /* NTS: Network code as per IRIS SEED          */
   char loc[TRACE_LOC_LEN];   /* NTS: Location code as per IRIS SEED         */
} CHANNELNAME;
