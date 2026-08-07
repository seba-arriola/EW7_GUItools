/* config.c  config file for the quick review system */

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: EQRconfig.c,v 1.8 2002/10/08 18:14:52 lucky Exp $
 *    Revision history:
 *
 *    $Log: EQRconfig.c,v $
 *    Revision 1.8  2002/10/08 18:14:52  lucky
 *    Added nearest town option to alarm formats
 *
 *    Revision 1.7  2002/06/28 21:06:22  lucky
 *    Lucky's pre-departure checkin. Most changes probably had to do with bug fixes
 *    in connection with the GIOC scaffold.
 *
 *    Revision 1.6  2002/05/28 17:25:41  lucky
 *    *** empty log message ***
 *
 *    Revision 1.5  2001/09/10 20:46:16  lucky
 *    *** empty log message ***
 *
 *    Revision 1.4  2001/08/07 16:53:30  lucky
 *    Pre v6.0 checkin
 *
 *    Revision 1.3  2001/07/28 00:43:53  lucky
 *     State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *    Revision 1.2  2001/07/20 17:40:05  lucky
 *    State of the code after much of v6.0 testing and debugging
 *
 *    Revision 1.1  2001/07/01 21:55:21  davidk
 *    Initial revision
 *
 *    Revision 1.3  2001/06/26 17:37:03  lucky
 *    State of the code after all Utah specs have been met
 *
 *    Revision 1.2  2001/06/21 21:26:25  lucky
 *    State of the code after LocalMag review was implemented and at least partially
 *    tested. The new Lomax applet (june 18 version) has been incorporated. ML's can
 *    be added, deleted, modified, as well as the other types of picks.
 *
 *    Revision 1.1  2001/02/28 17:33:07  lucky
 *    Initial revision
 *
 *    Revision 1.1  2001/02/28 17:30:23  lucky
 *    Initial revision
 *
 *
 *
 */
  
/*************************************************************************
  This is standard config file reading code, adapted
  from earthworm config files.  It uses the kom.c libraries from 
  earthworm (or wherever earthworm may have lifted them from).  
  Davidk 19990419 
*************************************************************************/


/* include the normal system stuff*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* include earthworm headers */
#include <ewdb_ora_api.h>
#include <kom.h>

/* include our own header file */
#include <review.h>

/* include the review "globals" file */
#include <review_globals.h>
/*****************************************************************************
 *  ReadConfig() processes command file(s) using kom.c functions;            *
 *                  Exits if any errors are encountered 	             *
 *****************************************************************************/
#define ncommand  23          /* # of required commands you expect to process */

int ReadConfig( char *configfile )
{
   char     init[ncommand]; /* init flags, one byte for each required command */
   int      nmiss;          /* number of required commands that were missed   */
   char    *com;
   int      nfiles;
   int      success;
   int      i;	
   char*    str;

/* Set to zero one init flag for each required command 
 *****************************************************/   
   for( i=0; i<ncommand; i++ )  init[i] = 0;


	NumRevSrcs = 0;
	iNumTGDs = 0;
	NumArrivalPicksToShow = 0;
	NumAmplitudePicksToShow = 0;
	MakeLMPreferred = FALSE;
	SacBufferLen = -1;
	TaperLength = 0.0;
	LowFreqTaper1 = 0.05;
	LowFreqTaper2 = 0.1;
	HighFreqTaper1 = 0.45;
	HighFreqTaper2 = 0.5;

	ValidateQdds = 0;
	MinDepth = 10000000.0;
	MaxDepth = 10000000.0;
	NumPhases = 0;
	MaxGap = 1000000000.0;
	MaxDist = 1000000000.0;
	MaxRMS = 1000000.0;
	MaxER0 = 1000000.0;
	MaxERZ = 1000000.0;
	MaxERH = 1000000.0;
	NumMCTest = 0;
    strcpy (BackgroundColor, "notset");
    strcpy (HeaderLogo, "notset");
    strcpy (FooterLogo, "notset");
    strcpy (HeaderTag, "notset");
    strcpy (FooterTag, "notset");

	Alarms_minPopulation = 1;
	Alarms_showPopulation = FALSE;

	NumSeisGramOptions = 0;


/* Open the main configuration file 
 **********************************/
   nfiles = k_open( configfile ); 
   if ( nfiles == 0 ) {
	printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n", 
                configfile );
	exit( -1 );
   }

/* Process all command files
 ***************************/
   while(nfiles > 0)   /* While there are command files open */
   {
        while(k_rd())        /* Read next line from active file  */
        {  
	    com = k_str();         /* Get the first token from line */

        /* Ignore blank lines & comments
         *******************************/
            if( !com )           continue;
            if( com[0] == '#' )  continue;

        /* Open a nested configuration file 
         **********************************/
            if( com[0] == '@' ) {
               success = nfiles+1;
               nfiles  = k_open(&com[1]);
               if ( nfiles != success ) {
                  printf("ReadConfig: Error opening command file %s; exiting!\n<br>\n",
                           &com[1] );
                  exit( -1 );
               }
               continue;
            }

        /* Process anything else as a command 
         ************************************/
  /*0*/     if( k_its("Logfiledir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) 
                {
                    sprintf( envEW_LOG, "EW_LOG=%s", str );  
                    if( putenv( envEW_LOG ) != 0 )  /*set environment variable for logit*/
                    {
                       printf("ReadConfig: putenv: unable to set "
                              "EW_LOG environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }


					/* set the Timezone variable */
					/******/
					if( putenv( "TZ=GMT" ) != 0 ) 
                    {
                       printf("ReadConfig: putenv: unable to set TZ environmental variable\n<br>\n");
                       exit( -1 );
                    }
					/******/
					
                } else {
                    printf("ReadConfig: Bad Logfiledir command in %s:\n"
                           "            pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[0] = 1;
            }

  /*1*/     else if( k_its("DBservice") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBservice, str );
                } else {
                    printf("ReadConfig: Bad DBservice command in %s:\n"
                           "            string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[1] = 1;
            }

  /*2*/     else if( k_its("DBuser") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBuser, str );
                } else {
                    printf("ReadConfig: Bad DBuser command in %s:\n"
                           "            username \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[2] = 1;
            }

  /*3*/     else if( k_its("DBpassword") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( DBpassword, str );
                } else {
                    printf("ReadConfig: Bad DBpassword command in %s:\n"
                           "            passwd \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[3] = 1;
            }
  /*4*/     else if( k_its("ReviewDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( ReviewDir, str );
                    sprintf( ReviewTmpDir, "%s%ctmp", ReviewDir, DIR_SLASH);
                } else {
                    printf("ReadConfig: Bad ReviewDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[4] = 1;
            }
  /*5*/     else if( k_its("PathToHypoBin") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoBin, str );
                } else {
                    printf("ReadConfig: Bad PathToHypoBin command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[5] = 1;
            }
  /*6*/     else if( k_its("HypoConfig") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoCfg, str );
                } else {
                    printf("ReadConfig: Bad HypoConfig command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[6] = 1;
            }

  /*7*/     else if( k_its("HypoinverseDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( HypoDir, str );
                } else {
                    printf("ReadConfig: Bad HypoinverseDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[7] = 1;
            }
  /*8*/     else if( k_its("JavascriptFile") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( JavascriptFile, str );
                } else {
                    printf("ReadConfig: Bad JavascriptFile command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[8] = 1;
            }
  /*9*/     else if( k_its("SacFormat") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( SacFormat, str );
                } else {
                    printf("ReadConfig: Bad SacFormat command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[9] = 1;
            }
  /*10*/     else if( k_its("WebDir") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebDir, str );
                    sprintf( WebTmpDir,"%s/tmp", WebDir );
                } else {
                    printf("ReadConfig: Bad WebDir command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[10] = 1;
            }
  /*11*/     else if( k_its("WebHost") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( WebHost, str );
                } else {
                    printf("ReadConfig: Bad WebHost command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[11] = 1;
            }
  /*12*/     else if( k_its("ReviewSource") ) 
             {
                if (NumRevSrcs >= MAX_REV_SRCS)
                {
                    fprintf( stderr,
                            "Too many <ReviewSource> commands in <%s>", configfile );
                    fprintf( stderr, "; max=%d; exitting!\n", (int) MAX_REV_SRCS );
                    exit( -1 );
                }

                if ((str = k_str()) == NULL)
  		        {
                    printf("ReadConfig: Bad ReviewSource command in %s:\n"
                           "            name is NULL; exiting!\n<br>\n",
                             configfile);
        		    exit (-1);
	        	}
                if( strlen(str) < EQP_MAXWORD ) 
		        {
                    if ((RevSrcs[NumRevSrcs] = malloc (EQP_MAXWORD)) == NULL)
  		            {
                        printf("ReadConfig: Call to malloc failed; exitting.\n");
        		        exit (-1);
	        	    }

                    strcpy( RevSrcs[NumRevSrcs], str );
                } else {
                    printf("ReadConfig: Bad ReviewSource command in %s:\n"
                           "            name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                NumRevSrcs = NumRevSrcs + 1;
                init[12] = 1;
            }
  /*13*/    else if( k_its("AlarmRing") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( AlarmRing, str );
                } else {
                    printf("Bad AlarmRing command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[13] = 1;
            }
  /*14*/    else if( k_its("MyModuleID") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( MyModuleID, str );
                } else {
                    printf("Bad MyModuleID command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[14] = 1;
            }
  /*15*/    else if( k_its("MyInstID") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( MyInstID, str );
                } else {
                    printf("Bad MyInstID command in %s:\n"
                           "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[15] = 1;
            }
  /*16*/    else if( k_its("EwParams") ) {
                str = k_str();
                if( strlen(str) < MAXPATH )
                {
                    sprintf( envEW_PARAMS, "EW_PARAMS=%s", str );
                    if( putenv( envEW_PARAMS ) != 0 )
                    {
                       printf("putenv: unable to set "
                              "EW_PARAMS environment variable; exiting!\n<br>\n" );
                       exit( -1 );
                    }
                } else {
                    printf("Bad EwParams command in %s:\n"
                             "pathname \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[16] = 1;
            }
  /*17*/    else if( k_its("LM_progname") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( LM_progname, str );
                } else {
                    printf("ReadConfig: Bad LM_progname command in %s:\n"
                            "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[17] = 1;
            }
  /*18*/    else if( k_its("LM_outputfile") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( LM_outputfile, str );
                } else {
                    printf("ReadConfig: Bad LM_outputfile command in %s:\n"
                            "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[18] = 1;
            }
  /*19*/    else if( k_its("LM_writeWA_configfile") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( LM_writeWA_configfile, str );
                } else {
                    printf("ReadConfig: Bad LM_writeWA_configfile command in %s:\n"
                            "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[19] = 1;
            }
  /*20*/    else if( k_its("LM_review_configfile") ) {
                str = k_str();
                if( strlen(str) < MAXPATH ) {
                    strcpy( LM_review_configfile, str );
                } else {
                    printf("ReadConfig: Bad LM_review_configfile command in %s:\n"
                            "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, MAXPATH );
                    exit( -1 );
                }
                init[20] = 1;
            }
  /*21*/    else if( k_its("LM_method") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( LM_method, str );
                } else {
                    printf("ReadConfig: Bad LM_method command in %s:\n"
                            "name \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }

				if (strcmp (LM_method, "Peak2Peak") == 0)
					LM_LocalMagType = MAGTYPE_LOCAL_PEAK2PEAK;
				else if (strcmp (LM_method, "Zero2Peak") == 0)
					LM_LocalMagType = MAGTYPE_LOCAL_ZERO2PEAK;
				else
                {
                    printf ("ReadConfig: Bad LM_method command in %s:\n"
                            "method <%s> invalid; exiting!\n<br>\n", configfile, str);
                    exit( -1 );
                }

                init[21] = 1;
            }
  /*22*/    else if( k_its("NetworkCode") ) {
                str = k_str();
                if( strlen(str) < EQP_MAXWORD ) {
                    strcpy( NetworkCode, str );
                } else {
                    printf("ReadConfig: Bad NetworkCode command in %s:\n"
                            "string \"%s\" too long; maxchar=%d; exiting!\n<br>\n",
                             configfile, str, EQP_MAXWORD );
                    exit( -1 );
                }
                init[22] = 1;
            }



			/* Optional configuration commands */

            else if( k_its("Debug") ) {
                DEBUG=1;
            }

			/* Nearest town options */
            else if( k_its("minPopulation") ) 
			{
				Alarms_minPopulation = k_int ();
			}
            else if( k_its("showPopulation") ) 
			{
				Alarms_showPopulation = k_int ();
			}

			/* Qdds validation options */
            else if( k_its("MinDepthTest") ) 
			{
				MinDepth = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("MaxDepthTest") ) 
			{
				MaxDepth = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("NphTest") ) 
			{
				NumPhases = k_int ();
                ValidateQdds = 1;
			}
            else if( k_its("GapTest") ) 
			{
				MaxGap = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("DminTest") ) 
			{
				MaxDist = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("RmsTest") ) 
			{
				MaxRMS = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("MaxE0Test") ) 
			{
				MaxER0 = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("MaxERZTest") ) 
			{
				MaxERZ = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("MaxERHTest") ) 
			{
				MaxERH = k_val ();
                ValidateQdds = 1;
            }
            else if( k_its("NcodaTest") ) 
			{
				if (NumMCTest >= MAX_MCTEST)
				{
                    fprintf ( stderr, 
							"Max number of NcodatTest lines exceeded.\n");
                }
				else
				{
					MC_Test[NumMCTest].Mag = k_val ();
					MC_Test[NumMCTest].MinCodas = k_int ();

					NumMCTest = NumMCTest + 1;
					ValidateQdds = 1;
				}
			}

            /* BackgroundColor
            *******************/
		      else if( k_its("BackgroundColor") )
      		{
       		   strcpy(BackgroundColor,k_str());
      		}

            /* HeaderLogo
            *******************/
		      else if( k_its("HeaderLogo") )
      		{
		          strcpy(HeaderLogo,k_str());
      		}

            /* FooterLogo
            *******************/
      		else if( k_its("FooterLogo") )
      		{
       		   strcpy(FooterLogo,k_str());
      		}
            /* HeaderTag
            *******************/
      		else if( k_its("HeaderTag") )
      		{
       		   strcpy(HeaderTag,k_str());
      		}
            /* FooterTag
            *******************/
      		else if( k_its("FooterTag") )
      		{
       		   strcpy(FooterTag,k_str());
      		}



            /* Applet options
            *******************/
      		else if( k_its("SeisGramOption") )
      		{
                if (NumSeisGramOptions >= MAX_SG2K_OPT)
                {
                    fprintf( stderr,
                            "Too many <SeisGramOption> commands in <%s>", configfile );
                    fprintf( stderr, "; max=%d; exitting!\n", (int) MAX_SG2K_OPT );
                    exit( -1 );
                }

				/* Read the command parameter */
                if ((str = k_str()) == NULL)
  		        {
                    fprintf(stderr,"Bad SeisGramOption command in %s:\n"
                                  " param is NULL; exiting!\n<br>\n", configfile);
        		    exit (-1);
	        	}
                if( strlen(str) < MAX_SG2K_PARAM ) 
                    strcpy (SeisGramOptions[NumSeisGramOptions].param, str );
 				else 
				{
                    fprintf (stderr, "Bad SeisGramOption command in %s:\n"
                                   " name \"%s\" too long; maxchar=%d; exiting!\n\n",
                             configfile, str, MAX_SG2K_PARAM );
                    exit( -1 );
                }

				/* Read the command value */
                if ((str = k_str()) == NULL)
  		        {
                    fprintf(stderr,"Bad SeisGramOption command in %s:\n"
                                  " value is NULL; exiting!\n<br>\n", configfile);
        		    exit (-1);
	        	}
                if( strlen(str) < MAX_SG2K_VALUE ) 
                    strcpy (SeisGramOptions[NumSeisGramOptions].value, str );
 				else 
				{
                    fprintf (stderr, "Bad SeisGramOption command in %s:\n"
                                   " name \"%s\" too long; maxchar=%d; exiting!\n\n",
                             configfile, str, MAX_SG2K_VALUE);
                    exit( -1 );
                }

				NumSeisGramOptions = NumSeisGramOptions + 1;
      		}


			/* Taper settings for conversion to ground motion*/
            else if (k_its ("TaperLength")) 
			{
                TaperLength = k_val();
            }

            else if (k_its ("LowFreqTaper1")) 
			{
                LowFreqTaper1 = k_val();
            }

            else if (k_its ("LowFreqTaper2")) 
			{
                LowFreqTaper2 = k_val();
            }

            else if (k_its ("HighFreqTaper1")) 
			{
                HighFreqTaper1 = k_val();
            }

            else if (k_its ("HighFreqTaper2")) 
			{
                HighFreqTaper2 = k_val();
            }


            else if (k_its ("SacBufferLen")) {
                SacBufferLen = k_long();
            }

            else if (k_its ("ShowArrivalPicks")) {
                NumArrivalPicksToShow = k_int();
            }

            else if (k_its ("ShowAmplitudePicks")) {
                NumAmplitudePicksToShow = k_int();
            }

            else if (k_its ("MakeLMPreferred")) {
                MakeLMPreferred = TRUE;
            }
            else if( k_its("WaveformLinks") ) 
            {

              if(iNumTGDs < MAX_TGDS)
              {

                str = k_str();
                strncpy(TGDS[iNumTGDs].szProgramName,str,TGD_STRING_LEN);
                TGDS[iNumTGDs].szProgramName[TGD_STRING_LEN-1]=0;

                str = k_str();

				strncpy(TGDS[iNumTGDs].szDescription,str,TGD_DESCRIPTION_LEN);
                TGDS[iNumTGDs].szDescription[TGD_DESCRIPTION_LEN-1]=0;

                TGDS[iNumTGDs].bUseSeparateWindow=k_int();

                if(TGDS[iNumTGDs].bUseSeparateWindow)
                {
                  str = k_str();
                  strncpy(TGDS[iNumTGDs].szTargetName,str,TGD_STRING_LEN);
                  TGDS[iNumTGDs].szTargetName[TGD_STRING_LEN-1]=0;
                }

                iNumTGDs++;
              }
              else
              {
                printf("ReadConfig: Too many TGDs!!!  Max %d allowed\n<br>\n", MAX_TGDS);
              }
            }


         /* Unknown command
          *****************/ 
	    else {
                printf( "ReadConfig: \"%s\" Unknown command in %s.\n<br>\n", 
                         com, configfile );
                continue;
            }

        /* See if there were any errors processing the command 
         *****************************************************/
            if( k_err() ) {
               printf("ReadConfig: Bad %s command in %s; exiting!\n<br>\n",
                        com, configfile );
               exit( -1 );
            }
	}
	nfiles = k_close();
   }

/* After all files are closed, check init flags for missed commands
 ******************************************************************/
   nmiss = 0;
   for ( i=0; i<ncommand; i++ )  if( !init[i] ) nmiss++;
   if ( nmiss ) {
       printf( "ReadConfig: ERROR, no " );
       if ( !init[0] )  printf( "Logfiledir "     );
       if ( !init[1] )  printf( "DBservice "      );
       if ( !init[2] )  printf( "DBuser "         );
       if ( !init[3] )  printf( "DBpassword "     );
       if ( !init[4] )  printf( "ReviewDir "    );
       if ( !init[5] )  printf( "PathToHypoBin "    );
       if ( !init[6] )  printf( "HypoConfig "    );
       if ( !init[7] )  printf( "HypoinverseDir "    );
       if ( !init[8] )  printf( "JavascriptFile "    );
       if ( !init[9] )  printf( "SacFormat "    );
       if ( !init[10] )  printf( "WebDir "    );
       if ( !init[11] )  printf( "WebHost "    );
       if ( !init[12] )  printf( "ReviewSource "    );
       if ( !init[13] )  printf( "AlarmRing "    );
       if ( !init[14] )  printf( "MyModuleID "    );
       if ( !init[15] )  printf( "MyInstID "    );
       if ( !init[16] )  printf( "EwParams "    );
       if ( !init[17] )  printf( "LM_progname "    );
       if ( !init[18] )  printf( "LM_outputfile "    );
       if ( !init[19] )  printf( "LM_writeWA_configfile "    );
       if ( !init[20] )  printf( "LM_review_configfile "    );
       if ( !init[21] )  printf( "LM_method "    );
       if ( !init[22] )  printf( "NetworkCode "    );
       printf( "command(s) in %s; exiting!\n<br>\n", configfile );
       exit( -1 );
   }

   return( 0 );
}


