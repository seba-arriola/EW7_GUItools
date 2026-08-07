
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: webparse.h,v 1.2 1999/11/09 16:55:39 lucky Exp $
 *    Revision history:
 *
 *    $Log: webparse.h,v $
 *    Revision 1.2  1999/11/09 16:55:39  lucky
 *    *** empty log message ***
 *
 *    Revision 1.1  1999/10/18 15:56:20  davidk
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:22:00  lucky
 *    Initial revision
 *
 *
 */
  

/* webparse.h */


/* functions defined in webparse.c */
int Webparse_ParseExpression(char * pExpr, int ExprLen, char * pVar, char * pVal);
char * Webparse_GetExpression(char * pBuffer, int * pExprLen);
int Webparse_GetDecodedWebString(char * pBuffer, int BufferLen);

/* User Callout function SetVars, passes the user a Variable, it's string value,
   and a pointer to User Defined Params, so that the user can take whatever
   desired action. */
int Webparse_Client_SetVars(char * szVar, char * szVal, void * pUserParams);

int Webparse_GetAndProcessWebParams(void * pUserParams);

/* end functions defined in webparse.c */
