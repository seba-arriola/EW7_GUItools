
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <ewdb_ora_api.h>


/*
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
**  IN THIS ARRAY, LONGER STRINGS MUST COME FIRST
**  And all elements must be unique
** !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/
static const char * PARM_TAGS[] =
{
    "-tss" /* start time ("string") */
  , "-tes" /* end time ("string") */
  , "-me"  /* max events db may return */
  , "-ts"  /* start time */
  , "-te"  /* end time */
  , "-dm"  /* minimum depth */
  , "-dx"  /* maximum depth */
  , "-mm"  /* minimum magnitude */
  , "-mx"  /* maximum magnitude */
  , "-am"  /* minimum latitude */
  , "-ax"  /* maximum latitude */
  , "-om"  /* minimum longitude */
  , "-ox"  /* maximum longitude */
  , "-o"   /* output file */
  , "-t"   /* test mode */
  , "-u"   /* db user login */
  , "-p"   /* db user password */
  , "-s"   /* db service */
};

enum PARM_ID
{
    TIME_START_STR  = 0
  , TIME_END_STR
  , MAX_EVENTS
  , TIME_START
  , TIME_END
  , DEPTH_MIN
  , DEPTH_MAX
  , MAGN_MIN
  , MAGN_MAX
  , LAT_MIN
  , LAT_MAX
  , LON_MIN
  , LON_MAX
  , OUT_FILE
  , TEST_MODE
  , DB_LOGIN
  , DB_PASSWORD
  , DB_SERVICE

  , ARG_TYPE_COUNT
};


/*
#define DEFAULT_END_DATE_STR "2050/12/31"
#define DEFAULT_END_TIME_STR "23:59:59"
#define DEFAULT_START_DATE_STR "1900/01/01"
#define DEFAULT_START_TIME_STR "00:00:00"
#define DEFAULT_MINLAT (-90.00)
#define DEFAULT_MAXLAT (90.00)
#define DEFAULT_MINLON (-180.00)
#define DEFAULT_MAXLON (180.00)
#define DEFAULT_MINZ (0.00)
#define DEFAULT_MAXZ (100.00)
#define DEFAULT_MINMAG (0.00)
#define DEFAULT_MAXMAG (10.00)
*/

static int SetParam( const int p_parmid, const char * p_value );
static int CheckParams();
static int ParseTime( const char * p_timestring , time_t * r_time );


static char g_outFilename[120];
static int g_maxReturnEvents = 0;
static char g_dbUser[50];
static char g_dbPasswd[50];
static char g_dbService[50];
static char g_SttTimeString[16];
static unsigned long g_SttTime;
static char g_EndTimeString[16];
static unsigned long g_EndTime;
static EWDB_EventListStruct g_MinEvent; /* min values for selection criteria */
static EWDB_EventListStruct g_MaxEvent; /* maxlues for selection criteria */
static char g_isTestMode = 0;

static EWDB_EventListStruct * g_EventBuffer = NULL;

/*
typedef struct _EWDB_EventListStruct
{
    EWDB_EventStruct Event;
    EWDBid  idOrigin;             
    double  dOT;             
    float   dLat;           
    float   dLon;          
    float   dDepth;       
    float   dPrefMag;  
} EWDB_EventListStruct;
*/

/*--------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
   char _workstring[120];

   int _a  /* start checking args after program name */
     , _p
     , _waitparm
     ;

   FILE * _out = NULL;

   int r_retc = 0;

   logit_init( "gencatalog", (short)0, 1024, 1 );


   if ( argc == 1 )
   {
      printf( "\n" );
      printf( "gencatalog -- lists events to standard out, or to a file.\n" );
      printf( "\n" );
      printf( "  usage:   gencatalog  <parameters>\n" );
      printf( "\n" );
      printf( "              tag     meaning\n" );
      printf( "  parameters:  -u   = database username (login)\n" );
      printf( "               -p   = database password\n" );
      printf( "               -s   = database service/name\n" );
      printf( "               -me  = max events database may return\n" );
      printf( "               -ts  = start time (seconds since Jan 1, 1970)\n" );
      printf( "            or -tss = start time (string: \"YYYYMMDDHHMMSS\")\n" );
      printf( "               -te  = end time (seconds since Jan 1, 1970)\n" );
      printf( "            or -tes = end time (string: \"YYYYMMDDHHMMSS\")\n" );
      printf( "               -am  = minimum latitude (fp +/-90.0)\n" );
      printf( "               -ax  = maximum latitude (fp, +/-90.0)\n" );
      printf( "               -om  = minimum longitude (fp, +/-180.0) (further west is lower)\n" );
      printf( "               -ox  = maximum longitude (fp, +/-180.0) (further east is higher)\n" );
      printf( "               -dm  = [OPTIONAL] minimum depth (fp), default:   0.0\n" );
      printf( "               -dx  = [OPTIONAL] maximum depth (fp), default: 500.0\n" );
      printf( "               -mm  = [OPTIONAL] minimum magnitude (fp), default:  0.0\n" );
      printf( "               -mx  = [OPTIONAL] maximum magnitude (fp), default: 10.0\n" );
      printf( "               -t   = [OPTIONAL] test mode flag [no value]\n" );
      printf( "                      (displays query criteria only)\n" );
      printf( "               -o   = [OPTIONAL] output file name, otherwise to stdout\n" );
      printf( "\n" );
      printf( "  example:\n" );
      printf( "\n" );
      printf( "      gencatalog -u db_name -p db_pass -s my_db \\\n" );
      printf( "      -me50 -tss20011101000000 -tes20011115235959 \\\n" );
      printf( "      -dm5.0 -dx 600.0 -mm 9.0 -mx4.5 -am 30.0N -ax 50.0N \\\n" );
      printf( "      -om122.0W -ox110.0W -t\n" );
      return 0;
   }

   g_EventBuffer = NULL;

   /*
   ** zero-out the criteria elements
   */
   g_EndTime = 0;
   g_SttTime = UINT_MAX;
   memset( &g_MinEvent, 0, sizeof(EWDB_EventListStruct) );
   memset( &g_MaxEvent, 0, sizeof(EWDB_EventListStruct) );

   /*
   ** List from all source
   */
   strcpy( g_MinEvent.Event.szSource, "*" );
   strcpy( g_MaxEvent.Event.szSource, "*" );

   g_outFilename[0] = 0;
   g_SttTimeString[0] = 0;
   g_EndTimeString[0] = 0;

   /* bad values */
   g_dbUser[0] = 0;
   g_dbPasswd[0] = 0;
   g_dbService[0] = 0;
   g_maxReturnEvents = 0;
   g_SttTime = -1; /* -1 is "missing" */
   g_EndTime = -1;
   g_MinEvent.dLat = -100.0;
   g_MaxEvent.dLat = -100.0;
   g_MinEvent.dLon = -200.0;
   g_MaxEvent.dLon = -200.0;
   g_MinEvent.dDepth =   0.0;
   g_MaxEvent.dDepth = 500.0;
   g_MinEvent.dPrefMag =  0.0;
   g_MaxEvent.dPrefMag = 10.0;


   _a = 1;
   _waitparm = -1;

   while ( _a < argc )
   {
      if ( _waitparm == -1 )
      {
         /* not currently waiting on another parm (after tag) */
         for ( _p = 0 ; _p < ARG_TYPE_COUNT ; _p++ )
         {
            if ( strncmp( argv[_a], PARM_TAGS[_p], strlen(PARM_TAGS[_p]) ) == 0 )
            {
               /* found matching tag text, leave loop */
               break;
            }
         }

         if ( _p == ARG_TYPE_COUNT )
         {
            printf( "Unrecognized tag: '%s'\n", argv[_a] );
            return -1;
         }

         if ( strlen(PARM_TAGS[_p]) < strlen(argv[_a]) )
         {
            /*
            ** Parameter value glued onto the end of the tag
            */

            strcpy( _workstring, argv[_a]+strlen(PARM_TAGS[_p]) );

/*
printf( "call SetParam(%d, %s) [1]  strlen(%s) < strlen(%s)\n"
      , _p
      , _workstring
      , PARM_TAGS[_p]
      , argv[_a]
      );
*/
            SetParam( _p, _workstring );
         }
         else
         {
            if ( _p == TEST_MODE )
            {
               /* no value for this tag */
               g_isTestMode = 1;
            }
            else
            {
               /* token consists solely of tag, next token should be value */
/*
printf( "call _waitparam = %d\n", _p );
*/
               _waitparm = _p;
            }
         }
      }
      else
      {
         if (   argv[_a][0] == '-'
             && _waitparm   != LAT_MIN
             && _waitparm   != LAT_MAX
             && _waitparm   != LON_MIN
             && _waitparm   != LON_MAX
            )
         {
            printf("Tag followed by another tag %s\n", PARM_TAGS[_waitparm] );
            return -1;
         }

/*
printf( "call SetParam(%d, %s) [2]\n", _waitparm, argv[_a] );
*/
         SetParam( _waitparm, argv[_a] );

         _waitparm = -1;
      }

      _a++;
   }

   if ( _waitparm != -1 )
   {
      printf( "incomplete command line, missing parameter value\n" );
      return -1;
   }

   if ( CheckParams() != 0 )
   {
      return -1;
   }


   if ( g_maxReturnEvents == 0 )
   {
      g_maxReturnEvents = 100;
   }


   /*
   ** Get output stream
   */
   if ( strlen(g_outFilename) == 0 )
   {
      _out = stdout;
   }
   else
   {
      if ( (_out = fopen(g_outFilename, "w")) == NULL )
      {
         printf( "Failed to open output file:\n%s\n", g_outFilename );
         return -1;
      }
   }


   if ( g_isTestMode == 1 )
   {
      fprintf( _out
             , "output file name: %s\n"
             , ( strlen(g_outFilename) == 0 ? "<none, to stdout>" : g_outFilename )
             );

      fprintf( _out
             , "db connect: %s : %s : %s\n"
             , g_dbUser
             , g_dbPasswd
             , g_dbService
             );

      fprintf( _out, "max rows from database: %d\n" , g_maxReturnEvents );

      fprintf( _out
             , "timespan: %s (%d) -- %s (%d)\n"
             , g_SttTimeString
             , g_SttTime
             , g_EndTimeString
             , g_EndTime
             );

      fprintf( _out, "latitude: %f --> %f\n", g_MinEvent.dLat, g_MaxEvent.dLat );

      fprintf( _out, "longitude: %f --> %f\n", g_MinEvent.dLon, g_MaxEvent.dLon );

      fprintf( _out, "depth: %f --> %f\n", g_MinEvent.dDepth, g_MaxEvent.dDepth );

      fprintf( _out, "magnitude: %f --> %f\n", g_MinEvent.dPrefMag, g_MaxEvent.dPrefMag );
   }
   else
   {
      /*
      ** not test mode, prepare storage space and database connection
      */

      if( (g_EventBuffer = (EWDB_EventListStruct *)malloc (g_maxReturnEvents* sizeof(EWDB_EventListStruct))) == NULL)
      {
         fprintf(stderr, "Unable to allocate space for retrieval of %d Events.\n", g_maxReturnEvents);
         r_retc = -1;
      }
      else
      {
         /*
         ** Initialize the Ora_API
         */
/*
         if ( ewdb_api_Init( g_dbUser, g_dbPasswd, g_dbService ) == EWDB_RETURN_FAILURE )
*/
         if ( ewdb_base_Init( g_dbUser, g_dbPasswd, g_dbService ) == EWDB_RETURN_FAILURE )
         {
            fprintf( stderr, "Database connection initialization failed.\n");
            r_retc = -1;
         }
         else
         {
            int _callstatus
              , _rowcount
              , _row
              ;

            /*
            ** Database call
            */
            if ( (_callstatus = ewdb_api_GetEventList(  g_EventBuffer
                                                     ,  g_maxReturnEvents
                                                     ,  g_SttTime
                                                     ,  g_EndTime
                                                     , &g_MinEvent
                                                     , &g_MaxEvent
                                                     , &_rowcount
                                                     )) == EWDB_RETURN_FAILURE )
            {
               fprintf( stderr, "Database call failed\n" );
               r_retc = -1;
            }
            else
            {
               EWDB_OriginStruct _origin;
               time_t      _otime;
               struct tm * _tstruct;
               float       _secs;
               double      _latdeg
                    ,      _londeg
                    ,      _latmin
                    ,      _lonmin
                    ;
               char        _lathemi = ' '
                  ,        _lonhemi = 'E'
                  ;
               char        _dmin[6];

               if ( _callstatus < 0 )
               {
                  fprintf( stderr
                         , "Insufficient space for %d matched rows, only returning %d (increase max events parameter [-me])\n"
                         , _callstatus * -1
                         , _rowcount
                         );
               }

               /*
               ** List events
               */
               for( _row = 0 ; _row < _rowcount ; _row++ )
               {

                  if ( ewdb_api_GetOrigin( g_EventBuffer[_row].idOrigin
                                         , &_origin
                                         ) == EWDB_RETURN_FAILURE )
                  {
                     fprintf( stderr
                            , "call to ewdb_api_GetOrigin() failed\n"
                            );
                     break;
                  }

                  _otime = (time_t)floor(g_EventBuffer[_row].dOT);
                  _tstruct = gmtime( &_otime );

                  _secs = (float)_tstruct->tm_sec
                        + ( g_EventBuffer[_row].dOT - floor(g_EventBuffer[_row].dOT) )
                        ;

                  if ( g_EventBuffer[_row].dLat < 0.0 )
                  {
                     g_EventBuffer[_row].dLat *= -1.0;
                     _lathemi = 'S';
                  }
                  else
                  {
                     _lathemi = ' ';
                  }
                  if ( g_EventBuffer[_row].dLon < 0.0 )
                  {
                     g_EventBuffer[_row].dLon *= -1.0;
                     _lonhemi = 'W';
                  }
                  else
                  {
                     _lonhemi = ' ';
                  }
                  _latdeg = floor( g_EventBuffer[_row].dLat );
                  _londeg = floor( g_EventBuffer[_row].dLon );

                  _latmin = (g_EventBuffer[_row].dLat - _latdeg) * 60.0;
                  _lonmin = (g_EventBuffer[_row].dLon - _londeg) * 60.0;

                  /*
                  **                                                                                         location remark
                  **                                                                             version / processing stage|
                  **                                                                                    event id          ||
                  **                                                                         auxilliary remarks|          ||
                  **                                                                   most common data source||          ||
                  **                                                                       quality code (A-D)|||          ||
                  **                                                                         analyst remarks||||          ||
                  **                                                                     vertical error    |||||          ||
                  **                                                               horizonal error    |    |||||          ||
                  **                                                 RMS travel time residual    |    |    |||||          ||
                  **                                            dist. to nearest station    |    |    |    |||||          ||
                  **                                              max. azimuthal gap   |    |    |    |    |||||          ||
                  **                               no. P&S times with weights > 0.1|   |    |    |    |    |||||          ||
                  **                                     Preferred Magnitude      ||   |    |    |    |    |||||          ||
                  **                                  Pref. magn. type code|      ||   |    |    |    |    |||||          ||
                  **                                           Depth      ||      ||   |    |    |    |    |||||          ||
                  **                                  Lon mins     |      ||      ||   |    |    |    |    |||||          ||
                  **                     Lon hemi('S' or ' ')|     |      ||      ||   |    |    |    |    |||||          ||
                  **                         Longitude deg  ||     |      ||      ||   |    |    |    |    |||||          ||
                  **                       Lat mins      |  ||     |      ||      ||   |    |    |    |    |||||          ||
                  **          Lat hemi('S' or ' ')|      |  ||     |      ||      ||   |    |    |    |    |||||          ||
                  **                 Latitude deg ||     |  ||     |      ||      ||   |    |    |    |    |||||          ||
                  **                            | ||     |  ||     |      ||      ||   |    |    |    |    |||||          ||
                  **        YYYYMMDD HHMMSS.SS  LLsll.ll LLLell.ll zzz.zz ?mm.mm  ?ggggddd.drr.rr?    vvv.v????IIIIIIIIII ?RRR
                  **        123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.12345678
                  */
                  if ( _origin.dDmin < 1000.0 )
                  {
                     sprintf( _dmin, "%5.1f", _origin.dDmin );
                  }
                  else
                  {
                     strcpy( _dmin, "*****" );
                  }

                  fprintf( _out
                         , "%04d%02d%02d %02d%02d%05.2f  %2.0f%c%5.2f %3.0f%c%5.2f %6.2f  %5.2f   %4d%s%5.2f     %5.1f    %010d     \n"
                         , _tstruct->tm_year + 1900
                         , _tstruct->tm_mon + 1
                         , _tstruct->tm_mday
                         , _tstruct->tm_hour
                         , _tstruct->tm_min
                         , _secs

                         , _latdeg
                         , _lathemi
                         , _latmin

                         , _londeg
                         , _lonhemi
                         , _lonmin

                         , g_EventBuffer[_row].dDepth
                         /*  ?  */
                         , g_EventBuffer[_row].dPrefMag

                         /*  ?  */
                         , _origin.iGap
                         , _dmin
                         , _origin.dRms

                         /*  ?  */
                         , _origin.dErZ
                         /*  ????  */
                         , g_EventBuffer[_row].Event.idEvent
                         /*  ?  ?  */
                         );

               }
            }

            ewdb_api_Shutdown();

         } /* good database connection */

         free( g_EventBuffer );

      } /* got storage space */

   } /* real, not test, mode -- want rows */

   if ( _out != NULL )
   {
      fclose( _out );
   }

   return 0;
}
/*---------------------------------------------------------------------------*/
int ParseTime( const char * p_timestring, time_t * r_time )
{
   int r_code = -2;  /* -2 is "error" */

   if ( p_timestring != NULL && strlen(p_timestring) == 14 )
   {
      struct tm _tmstruct;

      sscanf( p_timestring
            , "%4d%2d%2d%2d%2d%2d"
            , &_tmstruct.tm_year
            , &_tmstruct.tm_mon
            , &_tmstruct.tm_mday
            , &_tmstruct.tm_hour
            , &_tmstruct.tm_min
            , &_tmstruct.tm_sec
            );

      _tmstruct.tm_year -= 1900;
      _tmstruct.tm_mon  -= 1;
      _tmstruct.tm_isdst = -1;

      if ( (*r_time = mktime( &_tmstruct )) != -1 )
      {
         r_code = 0;
      }
   }
   
   return r_code;
}
/*-------------------------------------------------------------------------*/
int SetParam( const int p_parmid, const char * p_value )
{
   int r_code = 0;

   if ( p_value == NULL || strlen(p_value) == 0 )
   {
      fprintf( stderr, "SetParam(): value string (parm 2) NULL or zero-length\n" );
      return -1;
   }

   switch( p_parmid )
   {
     case OUT_FILE:
          strcpy( g_outFilename, p_value );
          break;
     case DB_LOGIN:
          strcpy( g_dbUser, p_value );
          break;
     case DB_PASSWORD:
          strcpy( g_dbPasswd, p_value );
          break;
     case DB_SERVICE:
          strcpy( g_dbService, p_value );
          break;
     case MAX_EVENTS:
          g_maxReturnEvents = atoi(p_value);
          break;
     case TIME_START:
          g_SttTime = atol(p_value);
          break;
     case TIME_START_STR:
          strcpy( g_SttTimeString , p_value );
          r_code = ParseTime(p_value, &g_SttTime);
          break;
     case TIME_END:
          g_EndTime = atol(p_value);
          break;
     case TIME_END_STR:
          strcpy( g_EndTimeString , p_value );
          r_code = ParseTime(p_value, &g_EndTime);
          break;
     case DEPTH_MIN:
          g_MinEvent.dDepth = (float)atof(p_value);
          break;
     case DEPTH_MAX:
          g_MaxEvent.dDepth = (float)atof(p_value);
          break;
     case MAGN_MIN:
          g_MinEvent.dPrefMag = (float)atof(p_value);
          break;
     case MAGN_MAX:
          g_MaxEvent.dPrefMag = (float)atof(p_value);
          break;
     case LAT_MIN:
          g_MinEvent.dLat = (float)atof(p_value);
          break;
     case LAT_MAX:
          g_MaxEvent.dLat = (float)atof(p_value);
          break;
     case LON_MIN:
          g_MinEvent.dLon = (float)atof(p_value);
          break;
     case LON_MAX:
          g_MaxEvent.dLon = (float)atof(p_value);
          break;
     default:
          fprintf( stderr, "SetParam(): unrecognized parameter %d\n", p_parmid );
          r_code = -1;
   }
   return r_code;
}
/*-------------------------------------------------------------------------*/
int CheckParams()
{
   int r_code = 0
     , _suppress
     ;

   if ( g_dbUser == NULL || strlen(g_dbUser) == 0 )
   {
      fprintf( stderr, "NULL or zero-length database login name (%s)\n"
             , PARM_TAGS[DB_LOGIN]
             );
      r_code = -1;
   }

   if ( g_dbPasswd == NULL || strlen(g_dbPasswd) == 0 )
   {
      fprintf( stderr, "NULL or zero-length database password (%s)\n"
             , PARM_TAGS[DB_PASSWORD]
             );
      r_code = -1;
   }

   if ( g_dbService == NULL || strlen(g_dbService) == 0 )
   {
      fprintf( stderr, "NULL or zero-length database/service name (%s)\n"
             , PARM_TAGS[DB_SERVICE]
             );
      r_code = -1;
   }


   /* ------------------------------------
   **         TIMES
   */
   _suppress = 0;

   if ( g_SttTime == -1 )
   {
      fprintf( stderr, "missing start time (%s|%s)\n"
            , PARM_TAGS[TIME_START]
            , PARM_TAGS[TIME_START_STR]
            );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_SttTime == -2 )
   {
      fprintf( stderr, "start time (%s) must be > zero or start time (%s) must be >= 19000101000001\n"
            , PARM_TAGS[TIME_START]
            , PARM_TAGS[TIME_START_STR]
            );
      r_code = -1;
      _suppress = 1;
   }

   if ( g_EndTime == -1 )
   {
      fprintf( stderr, "missing end time (%s|%s)\n"
            , PARM_TAGS[TIME_END]
            , PARM_TAGS[TIME_END_STR]
            );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_EndTime == -2 )
   {
      fprintf( stderr, "end time (%s) must be > zero or end time (%s) must be >= 19000101000001\n"
            , PARM_TAGS[TIME_END]
            , PARM_TAGS[TIME_END_STR]
            );
      r_code = -1;
      _suppress = 1;
   }

   if ( _suppress == 0 )
   {
      if ( g_EndTime < g_SttTime )
      {
         fprintf( stderr, "end time (%s|%s) before start time (%s|%s)\n"
               , PARM_TAGS[TIME_END]
               , PARM_TAGS[TIME_END_STR]
               , PARM_TAGS[TIME_START]
               , PARM_TAGS[TIME_START_STR]
               );
         r_code = -1;
      }
   }

   /* ------------------------------------
   **         LATITUDE
   */
   _suppress = 0;

   if ( g_MinEvent.dLat == -100.0 )
   {
      fprintf( stderr
             , "missing min latitude (%s)\n"
             , PARM_TAGS[LAT_MIN]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MinEvent.dLat < -90.0 || 90.0 < g_MinEvent.dLat )
   {
      fprintf( stderr
             , "min latitude (%s) must fall in range -90.0 -- 90.0\n"
             , PARM_TAGS[LAT_MIN]
             );
      r_code = -1;
      _suppress = 1;
   }

   if ( g_MaxEvent.dLat == -100.0 )
   {
      fprintf( stderr
             , "missing max latitude (%s)\n"
             , PARM_TAGS[LAT_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MaxEvent.dLat < -90.0 || 90.0 < g_MaxEvent.dLat )
   {
      fprintf( stderr
             , "max latitude (%s) must fall in range -90.0 -- 90.0\n"
             , PARM_TAGS[LAT_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }

   if ( _suppress == 0 )
   {
      if ( g_MaxEvent.dLat <= g_MinEvent.dLat )
      {
         fprintf( stderr
                , "max latitude (%s) must be greater than min latitude (%s)\n"
                , PARM_TAGS[LAT_MAX]
                , PARM_TAGS[LAT_MIN]
                );
         r_code = -1;
      }
   }

   /* ------------------------------------
   **         LONGITUDE
   */
   _suppress = 0;

   if ( g_MinEvent.dLon == -200.0 )
   {
      fprintf( stderr
             , "missing min longitude (%s)\n"
             , PARM_TAGS[LON_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MinEvent.dLon < -180.0 || 180.0 < g_MinEvent.dLon )
   {
      fprintf( stderr
             , "min longitude (%s) must fall in range -180.0 -- 180.0\n"
             , PARM_TAGS[LON_MIN]
             );
      r_code = -1;
   }
 
   if ( g_MaxEvent.dLon == -200.0 )
   {
      fprintf( stderr
             , "missing max longitude (%s)\n"
             , PARM_TAGS[LON_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MaxEvent.dLon < -180.0 || 180.0 < g_MaxEvent.dLon )
   {
      fprintf( stderr
             , "max longitude (%s) must fall in range -180.0 -- 180.0\n"
             , PARM_TAGS[LON_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }

   if ( _suppress == 0 )
   {
      if ( g_MaxEvent.dLon <= g_MinEvent.dLon )
      {
         fprintf( stderr
                , "max longitude (%s) must be greater than min longitude (%s)\n"
                , PARM_TAGS[LON_MAX]
                , PARM_TAGS[LON_MIN]
                );
         r_code = -1;
      }
   }


   /* ------------------------------------
   **         DEPTH
   */
   _suppress = 0;

   if ( g_MinEvent.dDepth == -1.0 )
   {
      fprintf( stderr
             , "missing min depth (%s)\n"
             , PARM_TAGS[DEPTH_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MinEvent.dDepth < 0.0 )
   {
      fprintf( stderr
             , "min longitude (%s) must be >= 0.0\n"
             , PARM_TAGS[DEPTH_MIN]
             );
      r_code = -1;
      _suppress = 1;
   }

   if ( g_MaxEvent.dDepth == -1.0 )
   {
      fprintf( stderr
             , "missing max depth (%s)\n"
             , PARM_TAGS[DEPTH_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }
   else if ( g_MaxEvent.dDepth < 0.0 )
   {
      fprintf( stderr
             , "max depth (%s) must be >= 0.0\n"
             , PARM_TAGS[DEPTH_MAX]
             );
      r_code = -1;
      _suppress = 1;
   }

   if ( _suppress == 0 )
   {
      if ( g_MaxEvent.dDepth <= g_MinEvent.dDepth )
      {
         fprintf( stderr
                , "max depth (%s) must be >= than min depth (%s)\n"
                , PARM_TAGS[DEPTH_MAX]
                , PARM_TAGS[DEPTH_MIN]
                );
         r_code = -1;
      }
   }


   /* ------------------------------------
   **         MAGNITUDE
   */

   if ( g_MaxEvent.dPrefMag < g_MinEvent.dPrefMag )
   {
      fprintf( stderr
             , "max magnitude (%s) must be >= than min magnitude (%s)\n"
             , PARM_TAGS[MAGN_MAX]
             , PARM_TAGS[MAGN_MIN]
             );
      r_code = -1;
   }

   return r_code;
}
