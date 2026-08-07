
/*
 *   This file is under RCS - do not modify unless you have
 *   checked it out using the command checkout.
 *
 *    $Id: getimage.c,v 1.4 2001/08/06 17:00:39 lucky Exp $
 *    Revision history:
 *
 *    $Log: getimage.c,v $
 *    Revision 1.4  2001/08/06 17:00:39  lucky
 *    Added support for NT map generation and display
 *
 *    Revision 1.3  2001/07/20 17:33:35  davidk
 *    Added a comment about using this program with older versions of webservers
 *    on NT.
 *
 *    Revision 1.2  2001/01/04 00:56:26  davidk
 *    Modified the program to work under NT too.  (Already works under unix)
 *
 *    Revision 1.1  2000/02/16 21:09:31  lucky
 *    Initial revision
 *
 *    Revision 1.1  1999/05/05 18:16:57  lucky
 *    Initial revision
 *
 *
 */
  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFERSIZE 100000
#ifdef _WINNT
#include <io.h>
#include <fcntl.h>
# define DEL_COMMAND "del"
# define DIR_SLASH '\\'
#else
# define DEL_COMMAND "rm"
# define DIR_SLASH '/'
#endif


int main(int argc, char ** argv)
{
  char * penv, *ptemp, del_command[100], fname[100], *pBuffer;
  int pid,bytesread1,bytesread2;
  FILE * fin;

  if(!(pBuffer=malloc(BUFFERSIZE)))
    return(-1);

#ifdef _WINNT
	/* 
	 * Open the stdout pipe in binary mode. This allows
	 * apache and other older webservers to display map
	 * gifs and other images. Thanks to Carol Bryan for
	 * this solution.
	 */
	_setmode(_fileno(stdout), _O_BINARY);
#endif _WINNT

  penv=getenv("QUERY_STRING");
  ptemp=strstr(penv,"GL_IMAGE_ID");
  if(ptemp)
  {
    ptemp+=strlen("GL_IMAGE_ID");
    if(ptemp[0] == '%')
      ptemp+=3;
    else
      ptemp+=1;
    if(pid=atoi(ptemp)) /* pid NOT NULL '='*/
    {
      sprintf(fname,"..%chtml%cimage_%d.gif",DIR_SLASH,DIR_SLASH,pid);
      if(fin=fopen(fname,"rb")) /* fin NOT NULL '='*/
      {
        /* Send header of reply back to web server */
        bytesread1 = fread(pBuffer, sizeof( char ), BUFFERSIZE, fin);        
        if(!bytesread1)
        {
        printf("Content-type: text/html\n\n");
        printf("Cannot load picture.\n",penv);
        return(-1);
        }

        printf("Content-type: image/gif\n\n");
        bytesread2 = fwrite(pBuffer, sizeof( char ), bytesread1, stdout); 
        if(bytesread1 != bytesread2)
        {
          /* Houston, we have a problem! */
          /* Don't know what to do about it.  */
        }
        else  /* if(bytesread1 != bytesread2) */
        { 
        /* If we got here, good, we are pretty happy */
          fclose(fin);
          sprintf(del_command,"%s %s",DEL_COMMAND,fname);
          system(del_command);
        }
      }
      else  /* if(fin=fopen())*/
      {
        /* Send header of reply back to web server */
        printf("Content-type: text/html\n\n");
        printf("Invalid GL_IMAGE_ID: %s.  Cannot load picture.  Image not found.\n",penv);
        return(-1);
      }
    }
    else  /* if(pid=atoi(penv)) */
    {
    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("Invalid GL_IMAGE_ID: %s.  Cannot load picture.\n",penv);
    return(-1);
    }
  }
  else /* if(penv) */
  {
    /* Send header of reply back to web server */
    printf("Content-type: text/html\n\n");
    printf("Invalid GL_IMAGE_ID.  Cannot load picture.\n");
    printf("Argc= %d.\n",argc);
    return(-1);
  }


  return(0);
}  /* end of main */
