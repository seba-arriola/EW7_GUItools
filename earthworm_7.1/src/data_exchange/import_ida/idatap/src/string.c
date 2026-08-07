#pragma ident "$Id: string.c,v 1.1 2004/03/17 21:18:34 lombard Exp $"
/*======================================================================
 *
 *  Misc. string operations.
 *
 *  util_strpad:
 *  Replace all characters from current end-of-string to specified
 *  length with constant character.  User is responsible for memory.
 *
 *  Pointer to begining of padded string is returned.
 *
 *----------------------------------------------------------------------
 *
 *  util_strtrm:
 *  Truncate a string, removing trailing blanks.  Truncated string
 *  is returned.
 *
 *====================================================================*/
#include "idatap.h"

char *util_strpad(char *input, int maxlen, int padchar)
{
int i;

    if (strlen(input) == maxlen) return input;
    for (i = strlen(input); i < maxlen-1; i++) input[i] = padchar;
    input[maxlen-1] = 0;

    return input;

}

char *util_strtrm(char *input)
{
int n;

    n = strlen(input) - 1;
    while (n >= 0 && input[n] == ' ') --n;
    input[++n] = 0;

    return input;

}

/* Revision History
 *
 * $Log: string.c,v $
 * Revision 1.1  2004/03/17 21:18:34  lombard
 * Initial revision
 *
 * Revision 1.2  2001/05/07 22:40:13  dec
 * ANSI function declarations
 *
 * Revision 1.1.1.1  2000/02/08 20:20:41  dec
 * import existing IDA/NRTS sources
 *
 */
