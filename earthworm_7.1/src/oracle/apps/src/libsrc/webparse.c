/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: webparse.c,v 1.7 2001/07/28 00:43:53 lucky Exp $
 *
 *    Revision history:
 *     $Log: webparse.c,v $
 *     Revision 1.7  2001/07/28 00:43:53  lucky
 *      State of the code after debugging and testing prior to v6.0 release freeze.
 *
 *     Revision 1.6  2001/03/28 19:12:41  lucky
 *     Increased the buffer sizes again up to twice the largest alarms message.
 *     We need this for the Alarms system.
 *
 *     Revision 1.5  2001/02/28 17:29:10  lucky
 *     Massive schema redesign and cleanup.
 *
 *     Revision 1.4  2000/07/20 17:46:15  lucky
 *     Increased WEBPARSE_STDIN_BUFFER_SIZE again, to accomodate Dewey-type events with
 *     circa 1000 phases
 *
 *     Revision 1.3  2000/06/21 22:55:34  lucky
 *      Cleaned up logit calls to make log files more readable.
 *
 *     Revision 1.2  2000/06/16 18:14:01  lucky
 *     Increased the size of WEBPARSE_STDIN_BUFFER_SIZE to accomodate large number of phases
 *     being passed in to the review stuff.
 *
 *     Revision 1.1  1999/11/09 17:41:53  lucky
 *     Initial revision
 *
 *     Revision 1.1  1999/11/09 17:41:24  lucky
 *     Initial revision
 *
 *
 */


/********************/
/*   HEADER section */
/********************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ewdb_apps_utils.h>
#include <webparse.h>

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

#define WEBPARSE_MAX_EXPRESSION_SIZE 2*ALARM_MSG_SIZE

/*
#define RB_ALL_EVENTS 0 
#define RB_ALL_REVIEWED_EVENTS 1 
#define RB_UNREVIEWED_EVENTS 2 
#define RB_REVIEWEDBY_EVENTS 3 
*/

#define WEBPARSE_BIG_BUFFER_SIZE 2*ALARM_MSG_SIZE
#define WEBPARSE_STDIN_BUFFER_SIZE 100000
#define WEBPARSE_MISC_BUFFER_SIZE 2*ALARM_MSG_SIZE


/* External functions */
void   logit_init( char *prog, short mid, int bufSize, int logflag );
void   logit( char *, char *, ... );          /* Function prototype  */
/* End external functions */

/********************/
/*   SOURCE section */
/********************/


char * Webparse_GetExpression(char * pBuffer, int * pExprLen)
{
  char * pCurr, * pCurrQ;

  /* DK 990126, changed GetExpression to handle '?' delimiters
     as well as '&', because of the way netscape browser handles
     server side image maps (image map href's) */

  /* find the next '&' or '?' delimiter in the buffer */
  pCurr=strstr(pBuffer,"&");
  if(pCurr)
  {
    pCurrQ=strstr(pBuffer,"?");
    /* if '?' is valid and closer than '&' */
    if(pCurrQ < pCurr && pCurrQ)
    {
      pCurr=pCurrQ;
    }
  }
  else
  {
    pCurr=strstr(pBuffer,"?");
  }

  if(pCurr)
    *pExprLen=pCurr-pBuffer;
  else
    *pExprLen=strlen(pBuffer);

  return(pBuffer);
}  /* End Webparse_GetExpression() */

int Webparse_ParseExpression(char * pExpr, int ExprLen, char * pVar, char * pVal)
{

  char * pEnd, * pEquals, * pCurr;
  unsigned char Temp;
  unsigned int VCurr;
  char pTempExpr[WEBPARSE_MAX_EXPRESSION_SIZE];


  /* Initialize pVar and pVal to zero length strings */
  pVar[0]=pVal[0]=0;

  if(ExprLen >= WEBPARSE_MAX_EXPRESSION_SIZE)
  {
    logit("","Expression %s, of length %d is longer than "
               "MAX_EXPRESSION_SIZE - \0: %d.\n", 
          pExpr,ExprLen, WEBPARSE_MAX_EXPRESSION_SIZE);
    return(-1);
  }

  /* Copy pExpr to pTempExpr, make sure pTempExpr is NULL terminated */
  strncpy(pTempExpr,pExpr,ExprLen);
  pTempExpr[ExprLen]=0;
         
  if((pEquals=strstr(pTempExpr,"=")) == NULL)
  {
    /* This could be the imagemap coordinates.  If a comma is found, then
       say it is coordinates. */
    if((pEquals=strstr(pTempExpr,",")) == NULL)
    {
      logit("","ParseExpression: No '=' nor ',' found in expression: "
            "%s. of length %d\n",pExpr,ExprLen);
      return(-1);
    }
    else
    {
      /* Let it be a coordinate string */
      strcpy(pVar,"ClickCoords");
    }

  }  /* end if strstr("=") == NULL */

    /* Now we switch back to pExpr instead of our pTempExpr, for no other reason
     than that was the way it was originally coded, and it seems to work, so 
     no since going in and fartin' around with it.  DK 990126 */

   /* pEquals is unfortunately set based on pTempExpr, so we need to hack in a
      conversion to convert it to be relevant to pExpr */
  pEquals += (pExpr - pTempExpr);

  pEnd=pExpr+ ExprLen -1;
  pCurr=pExpr;
  VCurr=0;


  /* Process the Variable portion of the string, unless it has been 
     processed already. */
  if(!pVar[0])
  {
    while(pCurr < pEquals)
    {
      if(*pCurr == '%')
      {
        /* Convert Hex string to Integer */
        pCurr++;
        if(*pCurr >= '0' && *pCurr <= '9')
          Temp=*pCurr-'0';
        else if(*pCurr >= 'A' && *pCurr <= 'F')
          Temp=*pCurr-'A'+10;
        else
          return(-1);

        pCurr++;
        if(*pCurr >= '0' && *pCurr <= '9')
          Temp=Temp * 16 +  *pCurr-'0';
        else if(*pCurr >= 'A' && *pCurr <= 'F')
          Temp=(Temp * 16) + *pCurr-'A'+10;
        else
        {
          logit("","ParseExpression():Unexpected encoded char value.\n");
          return(-1);
        }

        pVar[VCurr++]=Temp;
      }
      else /* *pCurr != '%' */
      {
        pVar[VCurr++]=*pCurr;
      }
      
      pCurr++;
    }
    pVar[VCurr]=0;
    pCurr=pEquals+1;
  } /* End if(!pVar[0]) */

  /* Now process the Value portion of the expression */

  VCurr=0;
  while(pCurr <= pEnd)
  {
    if(*pCurr == '%')
    {
      /* Convert Hex string to Integer */
      pCurr++;
      if(*pCurr >= '0' && *pCurr <= '9')
        Temp=*pCurr-'0';
      else if(*pCurr >= 'A' && *pCurr <= 'F')
        Temp=*pCurr-'A'+10;
      else
        return(-1);

      pCurr++;
      if(*pCurr >= '0' && *pCurr <= '9')
        Temp=Temp * 16 +  *pCurr-'0';
      else if(*pCurr >= 'A' && *pCurr <= 'F')
        Temp=(Temp * 16) + *pCurr-'A'+10;
      else
      {
        logit("","ParseExpression():Unexpected encoded char value.\n");
        return(-1);
      }

      pVal[VCurr++]=Temp;
    }
    else /* *pCurr != '%' */
    {
      pVal[VCurr++]=*pCurr;
    }
    
    pCurr++;
  }

  pVal[VCurr]=0;
  return(0);

}  /* End Webparse_ParseExpression() */

  
int Webparse_GetAndProcessWebParams(void * pUserParams)
{
  /* GetAndProcessWebParams() retrieves parameters sent from the webserver,
     and calls the user function Webparse_Client_SetVars() to allow the 
     user to process the parameters. */

  char   pParamsBuffer[WEBPARSE_STDIN_BUFFER_SIZE];  
  /* This is the buffer used to read parameter
    strings from the web server.  If it is changed
    from an array to a pointer, then a sizeof() 
    call needs to be changed below, because it is
    used to indicate the size of the buffer. */

  int    ParamsBufferStrLen;  
  /* strlen() of params_string that is retrieved from webserver */

  int    i;       /*  Counter Variable */ 
  int    Curr=0;    /*  Current position in the buffer, (pBuffer[Curr])  */
  int    ExprLen; /*  Length of the current expression */
  char * pExpr;   /*  Pointer to the current expression */
  int    RetVal=0;/*  Function Return Code */


  char   szVar[WEBPARSE_MISC_BUFFER_SIZE];
  char   szVal[WEBPARSE_MISC_BUFFER_SIZE];
  /* Strings used to store Variable and Value pairs retrieved from
     the web params string via ParseExpression().  */
  


  /* Get the runtime parameters via cgi-bin interface */
  if(Webparse_GetDecodedWebString(pParamsBuffer, sizeof(pParamsBuffer)))
  {
    logit("","GetDecodedWebString() Failed!  Returning w/Error!\n");
    return(-1);
  }

  ParamsBufferStrLen=strlen(pParamsBuffer);

  /* Retrieve all of the expressions from the buffer,
     Parse each expression, and 
     Set the appropriate variable.
  ****************************************************/
  for(i=0; Curr < ParamsBufferStrLen  && RetVal == 0; i++)
  {
    /* Get the next expression and it's length */
    pExpr=Webparse_GetExpression(&pParamsBuffer[Curr],&ExprLen);
    
    if(pExpr == NULL)
      break;

    /* This could happen if there were consecutive '?' or something
       wierd caused by the combination of the way the browser creates
       a server side image map request (appends ?X,Y) to the map URL,
       no matter what it may be, and the way we set the URL as that
       of a cgi-bin program with get paramaters attached to the end. 
    */
    if(ExprLen == 0)
    {
      logit("","ExprLen from GetExpression() is 0.  Ignoring the current "
            "expression, and continuing on.\n");
  
      /* Move Curr to the beginning of the next expression */
      Curr+=ExprLen+1; /* Add 1 for the '&' expression separator */
      continue;
    }

    /* Parse the Expression into a Variable and a Value */
    if(RetVal=Webparse_ParseExpression(pExpr,ExprLen,szVar,szVal))
    {
      logit("","Webparse_ParseExpression() failed, returning %d.\n",RetVal);
      break;
    }

    /* Set the appropriate program variable */
    if(RetVal=Webparse_Client_SetVars(szVar,szVal,pUserParams))
    {
      logit("","Webparse_Client_SetVars() failed, returning %d.\n",RetVal);
      break;
    }

    /* Move Curr to the beginning of the next expression */
    Curr+=ExprLen+1; /* Add 1 for the '&' expression separator */
  }

  return(RetVal);
}  /* end Webparse_GetAndProcessWebParams() */

int Webparse_GetDecodedWebString(char * pBuffer, int BufferLen)
{

  int Max,i;
  char * pTemp;

  /* read from stdin: POST Method. */
  /*   could've use CONTENT_LENGTH param for fixed length*/
  if(fgets(pBuffer,BufferLen,stdin) == NULL)
  {
    /* Failed to read from stdin (POST Method) try to
    /* read QUERY_STRING env parameter: GET Method */
    if((pTemp=getenv("QUERY_STRING")) == NULL)
    {
      logit("","Read from QUERY_STRING failed.\n");
      logit("","Assuming that all is fine, just no params passed.\n");
      pBuffer[0]=0; /* Null terminate the string at the beginning */
    }
    else
    {
      strcpy(pBuffer,pTemp);
    }
  }  /* If stdin read failed. */

  /* Set RetVal, Curr(?), and Max(End of stdin) */
  Max=strlen(pBuffer);

  /* URL Decode part 1 (convert +'s to SPACE's) */
  for(i=0; i< Max; i++)
  {
    if(pBuffer[i]=='+')
      pBuffer[i]=' ';
  }

  return(0);
}

/* End Webparse_GetDecodedWebString() */

/* <END OF FILE> */
