/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: shake_dtd.h,v 1.1 2001/03/22 18:14:39 lombard Exp $
 *
 *    Revision history:
 *     $Log: shake_dtd.h,v $
 *     Revision 1.1  2001/03/22 18:14:39  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * The shakemap earthquake.dtd and stationlist.dtd as strings
 * These are hard-coded here, but so is the XML-writing code
 * in Build_SM_File. If the DTD changes, you will need to
 * change both this file and the XML writer.
 * Pete Lombard; March 2001.
 */

#ifndef SHAKE_DTD_H
#define SHAKE_DTD_H

#define EARTHQUAKE_DTD "<!ELEMENT  earthquake EMPTY>\n\
<!ATTLIST earthquake\n\
  id            ID      #REQUIRED\n\
  lat           CDATA   #REQUIRED\n\
  lon           CDATA   #REQUIRED\n\
  mag           CDATA   #REQUIRED\n\
  year          CDATA   #REQUIRED\n\
  month         CDATA   #REQUIRED\n\
  day           CDATA   #REQUIRED\n\
  hour          CDATA   #REQUIRED\n\
  minute        CDATA   #REQUIRED\n\
  second        CDATA   #REQUIRED\n\
  timezone      CDATA   #REQUIRED\n\
  depth         CDATA   #REQUIRED\n\
  locstring     CDATA   #REQUIRED\n\
  pga           CDATA   #REQUIRED\n\
  pgv           CDATA   #REQUIRED\n\
  sp03          CDATA   #REQUIRED\n\
  sp10          CDATA   #REQUIRED\n\
  sp30          CDATA   #REQUIRED\n\
  created       CDATA   #REQUIRED\n\
>\n"

#define STALIST_DTD "<!ELEMENT stationlist (station+)>\n\
<!ATTLIST stationlist\n\
  created	CDATA	#REQUIRED\n\
>\n\
\n\
<!ELEMENT station (comp+)>\n\
<!ATTLIST station\n\
  code		CDATA 			#REQUIRED\n\
  name		CDATA 			#REQUIRED\n\
  insttype	CDATA 			#REQUIRED\n\
  lat		CDATA 			#REQUIRED\n\
  lon		CDATA 			#REQUIRED\n\
  source	(SCSN|CDMG|NSMP) 	'SCSN'\n\
  commtype	(DIG|ANA) 		'DIG'\n\
>\n\
\n\
<!ELEMENT comp (acc,vel,psa*)>\n\
<!ATTLIST comp\n\
  name          CDATA  #REQUIRED\n\
  originalname  CDATA  #IMPLIED\n\
>\n\
\n\
<!ELEMENT acc EMPTY>\n\
<!ELEMENT vel EMPTY>\n\
<!ELEMENT psa03 EMPTY>\n\
<!ELEMENT psa10 EMPTY>\n\
<!ELEMENT psa30 EMPTY>\n\
<!ATTLIST acc\n\
  value  CDATA         #REQUIRED\n\
  flag   CDATA        ''\n\
>\n\
<!ATTLIST vel\n\
  value CDATA          #REQUIRED\n\
  flag  CDATA         ''\n\
>\n\
<!ATTLIST psa03\n\
  value CDATA          #REQUIRED\n\
  flag  CDATA         ''\n\
>\n\
<!ATTLIST psa10\n\
  value CDATA          #REQUIRED\n\
  flag  CDATA         ''\n\
>\n\
<!ATTLIST psa30\n\
  value CDATA          #REQUIRED\n\
  flag  CDATA         ''\n\
>\n"

#endif
