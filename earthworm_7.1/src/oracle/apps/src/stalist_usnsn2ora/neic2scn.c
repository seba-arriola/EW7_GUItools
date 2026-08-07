
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: neic2scn.c,v 1.2 2000/03/30 16:27:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: neic2scn.c,v $
 *     Revision 1.2  2000/03/30 16:27:16  davidk
 *     removed earthworm header file includes.  Changed return codes to
 *     P3DB_RETURN_XXX from EW_XXX.  Removed unused variables and added
 *     a function prototype.
 *
 *     Revision 1.1  2000/02/23 19:17:14  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/22 17:14:19  lucky
 *     Initial revision
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <p3db_ora_api.h>
#include "stalist_usnsn2ora.h"

/* Function prototype
   ******************/
int  IsComment (char []);
int  MatchNeic2SCN (char *, char *, char *, char *, NEIC2SCN *, int);
int GetNEICStaList (NEIC2SCN **Sta, int *Nsta, char *filename);


  /***************************************************************
   *                         GetNEICStaList()                    *
   *                                                             *
   *                     Read the station list                   *
   *                                                             *
   ***************************************************************/

int GetNEICStaList (NEIC2SCN **Sta, int *Nsta, char *filename)
{
   char    string[150];
   int     i;
   int     nsta;
   NEIC2SCN *sta;
   FILE    *fp;

	if ((Nsta == NULL) || (filename == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return P3DB_RETURN_FAILURE;
	}

/* Open the station list file
   **************************/
   if ((fp = fopen (filename, "r") ) == NULL)
   {
      logit( "et", "usnsn_loc2ora: Error opening station list file <%s>.\n",
             filename);
      return P3DB_RETURN_FAILURE;
   }

/* Count channels in the station file.
   Ignore comment lines and lines consisting of all whitespace.
   ***********************************************************/
   nsta = 0;
   while ( fgets( string, 130, fp ) != NULL )
      if ( !IsComment( string ) ) nsta++;

   rewind( fp );

/* Allocate the station list
   *************************/
   sta = (NEIC2SCN *) calloc( nsta, sizeof(NEIC2SCN) );
   if ( sta == NULL )
   {
      logit( "et", "usnsn_loc2ora: Cannot allocate the station array\n" );
      return P3DB_RETURN_FAILURE;
   }


/* Read stations from the station list file into the station array
   **************************************************************/
   i = 0;
   while ( fgets( string, 130, fp ) != NULL )
   {
      int ndecoded;

      if ( IsComment( string ) ) continue;
      ndecoded = sscanf( string,
              "%s%s%s%s", 
               sta[i].neic_sta,
               sta[i].sta,
               sta[i].comp,
               sta[i].net);
      if (ndecoded < 4)
      {
         logit( "e", "usnsn_loc2ora: Error decoding station file.\n" );
         logit( "e", "ndecoded: %d\n", ndecoded );
         logit( "e", "Offending line:\n" );
         logit( "e", "%s\n", string );
         return P3DB_RETURN_FAILURE;
      }
      i++;
   }
   fclose( fp );
   *Sta  = sta;
   *Nsta = nsta;
   return P3DB_RETURN_SUCCESS;
}


    /*********************************************************************
     *                             IsComment()                           *
     *                                                                   *
     *  Accepts: String containing one line from a pick_ew station list  *
     *  Returns: 1 if it's a comment line                                *
     *           0 if it's not a comment line                            *
     *********************************************************************/

int IsComment( char string[] )
{
   int i;

   for ( i = 0; i < (int)strlen( string ); i++ )
   {
      char test = string[i];

      if ( test!=' ' && test!='\t' && test!='\n' )
      {
         if ( test == '#'  )
            return 1;          /* It's a comment line */
         else
            return 0;          /* It's not a comment line */
      }
   }
   return 1;                   /* It contains only whitespace */
}


    /*********************************************************************
     *                         MatchNeic2SCN ()                          *
     *                                                                   *
     *********************************************************************/
int  MatchNeic2SCN (char *neic_sta, char *sta, char *comp, 
			char *net, NEIC2SCN *neic2scn, int nSta)
{

	int		i;

	if ((neic2scn == NULL) || (nSta < 0) || (neic_sta == NULL) ||
			(sta == NULL) || (comp == NULL) || (net == NULL))
	{
		logit ("", "Invalid arguments passed in.\n");
		return P3DB_RETURN_FAILURE;
	}


	/* Strip off any trailing spaces */
	i = 0;
	while ((i < 6) && (neic_sta[i] != ' '))
	{
		i = i + 1;
	}
	if (i != 6)
		neic_sta[i] = '\0';
	else
		neic_sta[i + 1] = '\0';


	/* check against the list */
	i = 0;
	while (i < nSta)
	{
		if (strcmp (neic2scn[i].neic_sta, neic_sta) == 0)
		{
			/* Found a Match! */
			strcpy (sta, neic2scn[i].sta);
			strcpy (comp, neic2scn[i].comp);
			strcpy (net, neic2scn[i].net);
			return P3DB_RETURN_SUCCESS;
		}

		else
		{
			i = i + 1;
		}
	}

	/* Not Found! */
	strcpy (sta, neic_sta);
	strcpy (comp, "???");
	strcpy (net, "???");


	return P3DB_RETURN_SUCCESS;

}
