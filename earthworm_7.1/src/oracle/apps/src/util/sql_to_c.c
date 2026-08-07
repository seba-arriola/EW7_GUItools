#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

typedef struct _ParamStruct
{
  char   szParam[256];
  char * szCoreParam;
  int    iType;
  int    iSize;
  int    iInOut; // IN = 0, OUT = 1, IN OUT = 2
} ParamStruct;


char * strncpy_ew(char * dest, const char * source, size_t iLenIncludingNull);

# define OA_INT 1
# define OA_SHORT 2
# define OA_DOUBLE 3
# define OA_FLOAT 4
# define OA_SZ 5
# define OA_CHAR 6
# define OA_LVRAW 9
# define OA_EWDBID 10

#define  PARAM_INOUT_IN     0
#define  PARAM_INOUT_OUT    1
#define  PARAM_INOUT_INOUT  2

int main()
{
  FILE * fOut;

  /* vars to set */
  char szFunctionName[256];
  char szSQLFunctionName[256];
  int iNumParams;
  ParamStruct aParams[50];

  /* vars already set */
  char szFileName[256];
  char szDateTime[256];
  time_t tNow;
  int i;
  char szTypeStrings[][256] = {"","OA_INT","OA_SHORT","OA_DOUBLE","OA_FLOAT",
                               "OA_SZ","OA_CHAR","","","OA_LVRAW","OA_EWDBID"};



  char szInputBuffer[256], szParseBuffer[256];
  char *szTok, * szUnder;
  char cType;
  int j;

  /* parse the SQL Procedure call */
  /*
  CREATE OR REPLACE PROCEDURE Create_SiteT
  (
   OUT_idSiteT OUT number,
   IN_idSite number,
   IN_tOn number,
   IN_tOff number,
   IN_dLat number,
   IN_dLon number,
   IN_dElev number,
   IN_sComment varchar
  )
  */
ParseNextLine:
  fgets(szInputBuffer,sizeof(szInputBuffer),stdin);
  strncpy_ew(szParseBuffer, szInputBuffer, sizeof(szParseBuffer));
  szTok = strtok(szParseBuffer," \t\n");
  if(szTok && strcmp(szTok,"CREATE"))
    goto ParseNextLine;

  strtok(NULL," \t\n");  // get OR
  strtok(NULL," \t\n");  // get REPLACE
  szTok = strtok(NULL," \t\n");
  if(szTok && strcmp(szTok,"PROCEDURE"))
    goto ParseNextLine;
  szTok = strtok(NULL," \t\n");  // get szSQLFunctionName
  if(szTok)
    strncpy_ew(szSQLFunctionName, szTok, sizeof(szSQLFunctionName));

  fgets(szInputBuffer,sizeof(szInputBuffer),stdin);
  strncpy_ew(szParseBuffer, szInputBuffer, sizeof(szParseBuffer));
  szTok = strtok(szParseBuffer," \t\n");
  if(szTok && strcmp(szTok,"("))
    goto ParseNextLine;

  /* get all of the params */
  i = 0;

  while(fgets(szInputBuffer,sizeof(szInputBuffer),stdin))
  {
    strncpy_ew(szParseBuffer, szInputBuffer, sizeof(szParseBuffer));
    if(strlen(szParseBuffer) == 1)
      break;

    szTok = strtok(szParseBuffer," \t\n()");
    if(!szTok)
      continue;
    strncpy_ew(aParams[i].szParam, szTok, sizeof(aParams[i].szParam));
    szUnder = strchr(szTok,'_');
    if(szUnder)
    {
      cType = szUnder[1];
      aParams[i].szCoreParam = aParams[i].szParam + (&szUnder[1] - szTok);
    }
    else
      continue;
    if(cType == 'i')
    {
      if(szUnder[2] == 'd')  // id
        aParams[i].iType = OA_EWDBID;
      else
        aParams[i].iType = OA_INT;
    }
    else if(cType == 'd')
      aParams[i].iType = OA_DOUBLE;
    else if(cType == 't')
      aParams[i].iType = OA_DOUBLE;
    else if(cType == 'f')
      aParams[i].iType = OA_FLOAT;
    else if(cType == 'c')
      aParams[i].iType = OA_CHAR;
    else if(cType == 'b')
      aParams[i].iType = OA_INT;
    else if(cType == 's')
    {
      /*
      if(szUnder[2] == 'z')
        aParams[i].iType = OA_SZ;
      else
        aParams[i].iType = OA_SHORT;
      ***************************/
      aParams[i].iType = OA_SZ;
    }
    else
    {
      /* ?? OA_LVRAW */
      continue;
    }
  
    szTok = strtok(NULL," \t\n()");
    aParams[i].iInOut = PARAM_INOUT_IN;
    if(!szTok)
      continue;
    if(strcmp(szTok,"IN") == 0)
    {
      if((szTok = strtok(NULL," \t\n()")) && strcmp(szTok,"OUT") == 0)
        aParams[i].iInOut = PARAM_INOUT_INOUT;
      szTok = strtok(NULL," \t\n()");
    }
    if(strcmp(szTok,"OUT") == 0)
    {
      aParams[i].iInOut = PARAM_INOUT_OUT;
      szTok = strtok(NULL," \t\n()");
    }
    aParams[i].iSize = 0;
    i++;
  }  /* end while fgets(params) */

  iNumParams = i;

  for(i=0,j=0; i < strlen(szSQLFunctionName); i++)
  {
    if(szSQLFunctionName[i] != '_')
    {
      szFunctionName[j] = szSQLFunctionName[i];
      j++;
    }
  }
  szFunctionName[j] = 0x00;

  /*
  strcpy(aParams[0].szParam,"OUT_idSite");
  aParams[0].iSize = 0;
  aParams[0].iType = OA_INT;
  aParams[0].iInOut = PARAM_INOUT_OUT;

  strcpy(aParams[1].szParam,"IN_sSta");
  aParams[1].iSize = 0;
  aParams[1].iType = OA_SZ;
  aParams[1].iInOut = PARAM_INOUT_IN;

  strcpy(aParams[2].szParam,"IN_sNet");
  aParams[2].iSize = 0;
  aParams[2].iType = OA_SZ;
  aParams[2].iInOut = PARAM_INOUT_IN;

  strcpy(aParams[3].szParam,"IN_sComment");
  aParams[3].iSize = 0;
  aParams[3].iType = OA_SZ;
  aParams[3].iInOut = PARAM_INOUT_IN;
*/

  time(&tNow);
  strcpy(szDateTime,ctime(&tNow));
  strtok(szDateTime,"\n");

  sprintf(szFileName,"ewdb_api_%s.c",szFunctionName);
  fOut = fopen(szFileName, "w");


  fprintf(fOut,
    "/*\n"
    " *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE\n"
    " *   CHECKED IT OUT USING THE COMMAND CHECKOUT.\n"
    " *\n"
    " *    $Id: sql_to_c.c,v 1.1 2004/03/17 18:48:20 davidk Exp $\n"
    " *\n"
    " *    Revision history:\n"
    " *     $Log: sql_to_c.c,v $
    " *     Revision 1.1  2004/03/17 18:48:20  davidk
    " *     Initial revision
    " *\n"
    " */\n"
    "\n\n",
    szFunctionName, szDateTime, szFunctionName);

  fprintf(fOut,
    "#include <stdlib.h>\n"
    "#include <stdio.h>\n"
    "#include <ewdb_ora_api.h>\n"
    "#include <ewdb_cli_base.h>\n"
    "#include <ewdb_ew_oci_base.h>\n"
    "\n\n");

  fprintf(fOut,
    "static char SQL_STRING[] = \n"
    "     \"  Begin %s(%s => :%s",
    szSQLFunctionName, aParams[0].szParam, aParams[0].szParam);
  for(i=1; i < iNumParams; i++)
  {
    fprintf(fOut,
    ", \"\n           \"%s => :%s",
      aParams[i].szParam, aParams[i].szParam);
  }

  fprintf(fOut, 
    "); End;\";\n\n");

  fprintf(fOut,
    "static EWDB_OCI_SFS SQLParamsBindArray[] = \n"
    "{\n"
    "  {%d,%d,%d,%d,%d,%s,\":%s\"}",
    0,1,0,0,0,szTypeStrings[aParams[0].iType], aParams[0].szParam);
  
  for(i=1; i < iNumParams; i++)
  {
    fprintf(fOut,
      ",\n  {%d,%d,%d,%d,%d,%s,\":%s\"}",
      0,1,0,0,0,szTypeStrings[aParams[i].iType], aParams[i].szParam);
  }

  fprintf(fOut, 
    "\n};\n\n");

  fprintf(fOut, 
    "#define  NUM_FIELDS %d\n\n", iNumParams);

  fprintf(fOut, 
  "/* static variables */\n");
  for(i=0; i < iNumParams; i++)
  {
    if(aParams[i].iType == OA_DOUBLE || aParams[i].iType == OA_FLOAT)
    {
      fprintf(fOut,"static char sz%s[20];\n", aParams[i].szCoreParam);
    }
  }
  fprintf(fOut, "\n\n");
  fprintf(fOut, 
"/* Statement Struct for %s statement */\n", szFunctionName);
  fprintf(fOut, 
"static EWDB_OCIStatementStruct SSStatement;\n\n");

  fprintf(fOut, 
    "/********************************\n"
    "      FUNCTION PROTOTYPES\n"
    "********************************/\n"
    "int Prep%sExec(\n" 
    "                           EWDB_Cursor * ppCursor);\n"
    "int Post%sExec(EWDBid * pid);\n"
    "int Init%sStatement(char *statement, \n"
    "                                 EWDB_OCIStatementStruct *pSS);\n\n\n",
    szFunctionName, szFunctionName, szFunctionName);

  // ewdb_api_XXX() function
  fprintf(fOut, 
    "/* Describe Function ewdb_api_%s\n"
    "*********************************************************************/\n",
    szFunctionName);
  
  fprintf(fOut, 
    "int ewdb_api_%s()\n"
    "{\n\n",
    szFunctionName);

  fprintf(fOut, 
    "  EWDB_Cursor pCursor;\n"
    "  int rc;\n\n");
  
  fprintf(fOut, 
    "  if(<INPUT PARAM1> == NULL || <INPUT PARAM2> == NULL)\n"
    "  {\n"
    "    logit(\"\",\"ewdb_api_%s():Null pointer passed in!\\n\");\n"
    "    return(EWDB_RETURN_FAILURE);\n"
    "  }\n\n",
    szFunctionName);

  fprintf(fOut, 
    "  ewdb_base_SetLastOraAPIActionTime();\n\n");
  
  fprintf(fOut, 
    "  if(ewdb_base_Reconnect() != EWDB_RETURN_SUCCESS)\n"
    "  {\n"
    "    logit(\"\",\"ewdb_api_%s(): Could not reconnect to the database!\\n\");\n"
    "    return(EWDB_RETURN_FAILURE);\n"
    "  }\n\n",
    szFunctionName);

  fprintf(fOut, 
    "  if(Prep%sExec(&pCursor)\n"
    "      != EWDB_RETURN_SUCCESS)\n"
    "  {\n"
    "    logit(\"\",\"ewdb_api_%s():Prep%sExec() failed.\\n\");\n"
    "    return(EWDB_RETURN_FAILURE);\n"
    "  }\n\n",
    szFunctionName, szFunctionName, szFunctionName, szFunctionName);
  
  
  fprintf(fOut, 
    "  if(ewdb_base_SQLExecute(pCursor))\n"
    "  {\n"
    "    ewdb_base_ErrorReport(hEWDBC, pCursor,\"ewdb_api_%s():ewdb_base_SQLExecute\", 1);\n"
    "    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));\n"
    "  } \n\n",
    szFunctionName);


  fprintf(fOut, 
    "  /* Commit the transaction(all the previous inserts!)\n"
    "  ****************************************************/\n"
    "  if(ewdb_base_SQLCommit(hEWDBC))\n"
    "  {\n"
    "    ewdb_base_ErrorReport(hEWDBC, hEWDBC,\"ewdb_api_%s():ewdb_base_SQLCommit\",2);\n"
    "    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));\n"
    "  }\n\n",
    szFunctionName);

  fprintf(fOut, 
"  rc = Post%sExec(pid);\n\n",
    szFunctionName);

  fprintf(fOut, 
    "  ewdb_base_SetLastOraAPIActionTime();\n\n");
  
  fprintf(fOut, 
    "  if(rc == EWDB_RETURN_WARNING)\n"
    "  {\n"
    "    logit(\"\", \"ewdb_api_%s(): \"\n"
    "           \"ERROR:  Post%sExec() reports SQL Proc %%s returned error(%%d)!\\n\",\n"
    "          \"%s()\", *pid);\n"
    "    return(EWDB_RETURN_FAILURE);\n"
    "  }\n"
    "  else if(rc == EWDB_RETURN_FAILURE)\n"
    "  {\n"
    "    logit(\"\", \"ewdb_api_%s(): \"\n"
    "           \"ERROR:  Post%sExec failed!!\\n\");\n"
    "    return(ewdb_base_Disconnect(EWDB_RETURN_FAILURE));\n"
    "  }\n\n",
    szFunctionName, szFunctionName, szSQLFunctionName, szFunctionName, 
    szFunctionName);


  fprintf(fOut, 
    "  return(EWDB_RETURN_SUCCESS);\n"
    "}  /* end ewdb_api_%s() */\n\n\n",
    szFunctionName);
 

  // InitXXX() function
  fprintf(fOut, 
    "int Init%sStatement(char *statement,\n"
    "EWDB_OCIStatementStruct *pSS)\n"
    "{\n\n",
    szFunctionName);

  for(i=0; i<iNumParams; i++)
  {
    if(aParams[i].iType == OA_DOUBLE || aParams[i].iType == OA_FLOAT)
      fprintf(fOut, 
            "  pSS->FieldArray[%d].pVal = sz%s;\n",
            i,aParams[i].szCoreParam);
    else if (aParams[i].iType == OA_SZ)
      fprintf(fOut, 
            "  pSS->FieldArray[%d].pVal = %s;\n",
            i,aParams[i].szCoreParam);

    else
      fprintf(fOut, 
            "  pSS->FieldArray[%d].pVal = &%s;\n",
            i,aParams[i].szCoreParam);
  }

  fprintf(fOut, "\n");

  fprintf(fOut, 
    "  ewdb_base_RequestCursor(statement, pSS, 0);\n\n");
  
  fprintf(fOut, 
    "  return(EWDB_RETURN_SUCCESS);\n"
    "}  /* end Init%sStatement() */\n\n\n",
    szFunctionName);


  // PrepXXX() function
  fprintf(fOut, 
    "int Prep%sExec(EWDB_Cursor * ppCursor)\n"
    "{\n"
    "\n"
    "  SSStatement.NumOfFields = NUM_FIELDS;\n"
    "  SSStatement.FieldArray = SQLParamsBindArray;\n"
    "  SSStatement.RecordSize = 0;\n"
    "\n"
    "  // Copy user's vars to local vars here\n"
    "  <COPY> \n"
    "\n"
    "  if(Init%sStatement(SQL_STRING,\n"
    "                           &SSStatement) != EWDB_RETURN_SUCCESS)\n"
    "  {\n"
    "    logit(\"\", \"Call to Init%sStatement failed!\\n\");\n"
    "    return(EWDB_RETURN_FAILURE);\n"
    "  }\n"
    "\n"
    "  *ppCursor = SSStatement.pCda;\n"
    "  \n"
    "  return(EWDB_RETURN_SUCCESS);\n"
    "\n"
    "}  /* end Prep%sExec() */\n\n\n",
    szFunctionName, szFunctionName, szFunctionName, szFunctionName);

  // PostXXX() function
  fprintf(fOut, 
    "int Post%sExec(EWDBid * pid)\n"
    "{\n"
    "  EWDB_Cursor pCursor;\n"
    "  \n"
    "  *pid=%s;\n"
    "  \n"
    "  pCursor = SSStatement.pCda;\n"
    "  \n"
    "  ewdb_base_ReleaseCursor(pCursor);\n"
    "\n"
    "  if(%s <= 0)\n"
    "  {\n"
    "    return(EWDB_RETURN_WARNING);\n"
    "  }\n"
    "\n"
    "  return(EWDB_RETURN_SUCCESS);\n"
    "}  /* end Post%sExec() */\n\n\n",
    szFunctionName, aParams[0].szParam, aParams[0].szParam, szFunctionName);
 

  return(0);
}


char * strncpy_ew(char * dest, const char * source, size_t iLenIncludingNull);
{
  strncpy(dest,source,iLenIncludingNull - 1);
  dest[iLenIncludingNull - 1] = 0x00;

  return(dest);
}  /* end strncpy_ew() */

