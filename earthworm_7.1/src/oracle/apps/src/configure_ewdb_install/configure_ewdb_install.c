#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <kom.h>
#include <errno.h>

typedef struct _KWS
{
  char szTerm[80];
  char szValue[80];
} KWS;
 
#define NUM_EW_ORACLE_BASE          0
#define NUM_EW_ORACLE_HOME          1
#define NUM_EW_ORACLE_SID           2
#define NUM_EW_ORACLE_DOMAIN        3
#define NUM_EW_ORACLE_CONTROL_DIR1  4
#define NUM_EW_ORACLE_CONTROL_DIR2  5
#define NUM_EW_ORACLE_CONTROL_DIR3  6
#define NUM_EW_ORACLE_LOG_DIR1      7
#define NUM_EW_ORACLE_LOG_DIR2      8
#define NUM_EW_ORACLE_LOG_DIR3      9
#define NUM_SYSTEM_PSWD            10
#define NUM_SYS_PSWD               11
#define NUM_EW_ROLLBACK_TABLESPACE 12 
#define NUM_EW_DATA_CORE           13
#define NUM_EW_DATA_INFRA          14
#define NUM_EW_DATA_WAVEFORM_1     15
#define NUM_EW_DATA_WAVEFORM_2     16
#define NUM_EW_DATA_WAVEFORM_3     17
#define NUM_EW_DATA_WAVEFORM_4     18
#define NUM_EW_DATA_WAVEFORM_5     19
#define NUM_EW_DATA_WAVEFORM_6     20
#define NUM_EW_SYSTEM_DIR          21
#define NUM_EW_UNDO_DIR            22

#define false 0
#define true  1

KWS KeyWords[] =
{
  {"EW_ORACLE_BASE",""},
  {"EW_ORACLE_HOME",""},
  {"EW_ORACLE_SID",""},
  {"EW_ORACLE_DOMAIN",""},
  {"EW_ORACLE_CONTROL_DIR1",""},
  {"EW_ORACLE_CONTROL_DIR2",""},
  {"EW_ORACLE_CONTROL_DIR3",""},
  {"EW_ORACLE_LOG_DIR1",""},
  {"EW_ORACLE_LOG_DIR2",""},
  {"EW_ORACLE_LOG_DIR3",""},
  {"SYSTEM_PSWD",""},
  {"SYS_PSWD",""},        
  {"EW_ROLLBACK_TABLESPACE",""},
  {"EW_DATA_CORE",""}, 
  {"EW_DATA_INFRA",""},
  {"EW_DATA_WAVEFORM_1",""}, 
  {"EW_DATA_WAVEFORM_2",""}, 
  {"EW_DATA_WAVEFORM_3",""}, 
  {"EW_DATA_WAVEFORM_4",""}, 
  {"EW_DATA_WAVEFORM_5",""}, 
  {"EW_DATA_WAVEFORM_6",""}, 
  {"EW_SYSTEM_DIR",""},
  {"EW_UNDO_DIR",""}
};

  char * files[] = 
  {
      "context.sql",
      "CreateDB.sql",
      "CreateDBCatalog.sql",
      "CreateDBFiles.sql",
      "dbinst.bat",
      "init.ora",
      "interMedia.sql",
      "JServer.sql",
      "ordinst.sql",
      "postDBCreation.sql",
      "xdb_protocol.sql",
      "ewdb_create_base_tablespaces.sql",
      "ewdb_create_rollbacks.sql",
      "ewdb_set_passwords.sql",
      ""
  };


int iNumKeyWords = 0;

char * szSID = "";


int ReadScriptVariables()
{
  int j;
  char * szToken;
  char szOracleDataDir[512];

  if(!k_open("script_variables"))
  {
    fprintf(stderr, "Could open \"script_variables\" file!\n");
    return(-1);
  }
  
  while(k_rd())        /* Read next line from active file  */
  {  
    szToken = k_str();         /* Get the first token from line */
    
                               /* Ignore blank lines & comments 
    *******************************/
    if( !szToken )           continue;
    if( szToken[0] == '#' )  continue;
    
    /* Process anything else as a command 
    ************************************/
    for(j=0; j < iNumKeyWords; j++)
    {
      if(strcmp(KeyWords[j].szTerm,szToken) == 0)
      {
        /* we've got a match, do the substitution */
        strcpy(KeyWords[j].szValue, k_str());
        break;
      }
    }
    if(j>=iNumKeyWords)
      fprintf(stderr, "Unrecognized keyword (%s)\n", k_str());
  }

  if(KeyWords[NUM_EW_ORACLE_BASE].szValue[0] == 0x00)  /* oracle base */
    strcpy(KeyWords[NUM_EW_ORACLE_BASE].szValue, "c:\\oracle");
  if(KeyWords[NUM_EW_ORACLE_HOME].szValue[0] == 0x00)  /* oracle home */
    strcpy(KeyWords[NUM_EW_ORACLE_HOME].szValue, "ora92");
  if(KeyWords[NUM_EW_ORACLE_SID].szValue[0] == 0x00)  /* oracle sid */
    strcpy(KeyWords[NUM_EW_ORACLE_SID].szValue, "ewdb");
  szSID = KeyWords[NUM_EW_ORACLE_SID].szValue;

  if(KeyWords[NUM_EW_ORACLE_DOMAIN].szValue[0] == 0x00)  /* oracle domain */
    strcpy(KeyWords[NUM_EW_ORACLE_DOMAIN].szValue, "usgs");

  sprintf(szOracleDataDir, "%s\\oradata\\%s", KeyWords[0].szValue, KeyWords[2].szValue);

  if(KeyWords[NUM_EW_ORACLE_CONTROL_DIR1].szValue[0] == 0x00)  /* control dir 1 */
    strcpy(KeyWords[NUM_EW_ORACLE_CONTROL_DIR1].szValue, szOracleDataDir);
  if(KeyWords[NUM_EW_ORACLE_CONTROL_DIR2].szValue[0] == 0x00)  /* control dir 2 */
    strcpy(KeyWords[NUM_EW_ORACLE_CONTROL_DIR2].szValue, KeyWords[4].szValue);
  if(KeyWords[NUM_EW_ORACLE_CONTROL_DIR3].szValue[0] == 0x00)  /* control dir 3 */
    strcpy(KeyWords[NUM_EW_ORACLE_CONTROL_DIR3].szValue, KeyWords[4].szValue);

  if(KeyWords[NUM_EW_ORACLE_LOG_DIR1].szValue[0] == 0x00)  /* log dir 1 */
    strcpy(KeyWords[NUM_EW_ORACLE_LOG_DIR1].szValue, szOracleDataDir);
  if(KeyWords[NUM_EW_ORACLE_LOG_DIR2].szValue[0] == 0x00)  /* log dir 2 */
    strcpy(KeyWords[NUM_EW_ORACLE_LOG_DIR2].szValue, KeyWords[7].szValue);
  if(KeyWords[NUM_EW_ORACLE_LOG_DIR3].szValue[0] == 0x00)  /* log dir 3 */
    strcpy(KeyWords[NUM_EW_ORACLE_LOG_DIR3].szValue, KeyWords[7].szValue);

  if(KeyWords[NUM_SYSTEM_PSWD].szValue[0] == 0x00) /* system password */
    strcpy(KeyWords[NUM_SYSTEM_PSWD].szValue, "earthworm");
  if(KeyWords[NUM_SYS_PSWD].szValue[0] == 0x00) /* sys password */
    strcpy(KeyWords[NUM_SYS_PSWD].szValue, "earthworm1");  
  if(KeyWords[NUM_EW_ROLLBACK_TABLESPACE].szValue[0] == 0x00)  /* rollback tablespace */
    strcpy(KeyWords[NUM_EW_ROLLBACK_TABLESPACE].szValue, "UNDOTBS1");

  if(KeyWords[NUM_EW_DATA_CORE].szValue[0] == 0x00)  /* core datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_CORE].szValue, szOracleDataDir);

  if(KeyWords[NUM_EW_DATA_INFRA].szValue[0] == 0x00)  /* infra datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_INFRA].szValue, szOracleDataDir);

  if(KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue[0] == 0x00)  /* waveform(1) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue, szOracleDataDir);
  if(KeyWords[NUM_EW_DATA_WAVEFORM_2].szValue[0] == 0x00)  /* waveform(2) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_2].szValue, KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
  if(KeyWords[NUM_EW_DATA_WAVEFORM_3].szValue[0] == 0x00)  /* waveform(3) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_3].szValue, KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
  if(KeyWords[NUM_EW_DATA_WAVEFORM_4].szValue[0] == 0x00)  /* waveform(4) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_4].szValue, KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
  if(KeyWords[NUM_EW_DATA_WAVEFORM_5].szValue[0] == 0x00)  /* waveform(5) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_5].szValue, KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
  if(KeyWords[NUM_EW_DATA_WAVEFORM_6].szValue[0] == 0x00)  /* waveform(6) datafile dir */
    strcpy(KeyWords[NUM_EW_DATA_WAVEFORM_6].szValue, KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
 
  if(KeyWords[NUM_EW_SYSTEM_DIR].szValue[0] == 0x00)  /* system db dir */
    strcpy(KeyWords[NUM_EW_SYSTEM_DIR].szValue, szOracleDataDir);

  if(KeyWords[NUM_EW_UNDO_DIR].szValue[0] == 0x00)  /* undo db dir */
    strcpy(KeyWords[NUM_EW_UNDO_DIR].szValue, szOracleDataDir);

  return(0);
    
}  /*ReadScriptVariables() */

int ResolveScriptVariables(int iNumKeyWords);

int main(int argc, char ** argv)
{

  int i,j;
  FILE * fIn, * fOut;
  char szNewFile[512], szIn[512], szOut[512], szVar[512];
  char *szPtrIn, *szPtrNext, *szPtrEnd;
  int bChanged;

  iNumKeyWords = sizeof(KeyWords)/sizeof(KWS);

  /* tasks
    1)  Read script-variables file
    2)  Create SID subdirectory
    3)  For each file in list, read the file and write out a 
        matching one in the SID subdirectory
   ************************************/

  /* Task 1: Read script-variables file*/
  ReadScriptVariables();

  if(!strcmp(szSID,""))
  {
    fprintf(stderr,"Error reading script variables\n");
    return(-1);
  }

  ResolveScriptVariables(iNumKeyWords);

  /* Task 2: Create SID subdirectory*/
  if(_mkdir(szSID) && errno != EEXIST)
  {
    fprintf(stderr,"Error creating directory (%s)\n");
    return(-1);
  }

  /* Task 3: Parse templates and create files in SID subdirectory*/
  for(i=0; files[i][0] != 0; i++)
  {
    fIn = fopen(files[i],"r");
    if(!fIn)
    {
      fprintf(stderr, "Could not open file (%s) for reading.\n", files[i]);
      continue;
    }

    if(strcmp(files[i],"dbinst.bat") == 0)
      sprintf(szNewFile, "%s\\%s.bat", szSID, szSID);
    else if(strcmp(files[i],"init.ora") == 0)
      sprintf(szNewFile, "%s\\init%s.ora", szSID, szSID);
    else
      sprintf(szNewFile, "%s\\%s", szSID, files[i]);

    fOut = fopen(szNewFile,"w");
    if(!fOut)
    {
      fprintf(stderr, "Could not open file (%s) for writing.\n", szNewFile);
      continue;
    }

    if(strcmp(files[i],"dbinst.bat") == 0)
    {
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_CONTROL_DIR1].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_CONTROL_DIR2].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_CONTROL_DIR3].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_LOG_DIR1].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_LOG_DIR2].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_ORACLE_LOG_DIR3].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_CORE].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_INFRA].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_1].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_2].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_3].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_4].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_5].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_DATA_WAVEFORM_6].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_SYSTEM_DIR].szValue);
      fprintf(fOut,"mkdir %s\n", KeyWords[NUM_EW_UNDO_DIR].szValue);
    }
    
    while(fgets(szIn,sizeof(szIn),fIn))
    {
      
      while(1)
      {
        bChanged = false;
        szPtrIn = szIn;
        szOut[0] = 0x00;
        while(szPtrNext = strstr(szPtrIn,"$("))
        {
          /* copy everything upto szPtrNext*/
          strncat(szOut,szPtrIn, szPtrNext-szPtrIn);
          
          szPtrIn = szPtrNext;
          
          szPtrEnd = strchr(szPtrNext,')');
          
          /* if there's no terminating ')' in the string*/
          /* then there are no more legal variables*/
          if(!szPtrEnd)
            break;
          
          /* make sure there's an actual variable name */
          if((szPtrEnd-szPtrNext-2) > 0)
          {
            strncpy(szVar,szPtrNext+2,szPtrEnd-szPtrNext-2);
            szVar[szPtrEnd-szPtrNext-2] = 0x00;
            for(j=0; j < iNumKeyWords; j++)
            {
              if(strcmp(KeyWords[j].szTerm,szVar) == 0)
              {
                /* we've got a match, do the substitution */
                strcat(szOut,KeyWords[j].szValue);
                szPtrIn = szPtrEnd + 1;
                /* record that we made a change */
                bChanged = true;
                break;  /* dk 121203 */
              }
            }
          }
          if(szPtrIn < szPtrEnd)
          {
            /* we didn't find a match, copy the full variable */
            /* to the output string */
            strncat(szOut,szPtrIn,szPtrEnd - szPtrIn + 1);
            szPtrIn = szPtrEnd + 1;
          }
        }  /* end while variables "$(" left */
        /* copy the remaining source string */
        strcat(szOut,szPtrIn);

        if(!bChanged)
          break;

        strcpy(szIn,szOut);
      }  /* end while(1) */
      fputs(szOut,fOut);
    }  /* while there is input file left to read. */
    fclose(fIn);
    fclose(fOut);
  }  /* for each file */

  return(0);

} /* end main() */




int ResolveScriptVariables(int iNumKeyWords)
{
  int i,j;
  int bChanged;
  char * szIn, szOut[512], szVar[512];
  char *szPtrIn, *szPtrNext, *szPtrEnd;

  for(i=0; i < iNumKeyWords; i++)
  {
    szIn = KeyWords[i].szValue;
    while(1)
    {
      bChanged = false;
      szPtrIn = szIn;
      szOut[0] = 0x00;
      while(szPtrNext = strstr(szPtrIn,"$("))
      {
        /* copy everything upto szPtrNext */
        strncat(szOut,szPtrIn, szPtrNext-szPtrIn);
        
        szPtrIn = szPtrNext;
        
        szPtrEnd = strchr(szPtrNext,')');
        
        /* if there's no terminating ')' in the string */
        /* then there are no more legal variables */
        if(!szPtrEnd)
          break;
        
        /* make sure there's an actual variable name */
        if((szPtrEnd-szPtrNext-2) > 0)
        {
          strncpy(szVar,szPtrNext+2,szPtrEnd-szPtrNext-2);
          szVar[szPtrEnd-szPtrNext-2] = 0x00;
          for(j=0; j < iNumKeyWords; j++)
          {
            if(strcmp(KeyWords[j].szTerm,szVar) == 0)
            {
              /* we've got a match, do the substitution */
              strcat(szOut,KeyWords[j].szValue);
              szPtrIn = szPtrEnd + 1;
              /* record that we made a change */
              bChanged = true;
            }
          }
        }
        if(szPtrIn < szPtrEnd)
        {
          /* we didn't find a match, copy the full variable */
          /* to the output string */
          strncat(szOut,szPtrIn,szPtrEnd - szPtrIn + 1);
          szPtrIn = szPtrEnd + 1;
        }
      }  /* end while variables "$(" left */
      /* copy the remaining source string */
      strcat(szOut,szPtrIn);
      
      if(!bChanged)
        break;
      
      strcpy(szIn,szOut);
    }  /* end while(1) */
  } /* end for iNumKeyWords */
  return(0);
}  /* end ResolveScriptVariables() */
