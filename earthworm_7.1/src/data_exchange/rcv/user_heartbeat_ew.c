#include <stdio.h>
extern FILE *logout;
extern int   dbg;

user_heartbeat()
{
   if(dbg)  fprintf(logout,"Heartbeat !\n");
   fflush( logout );
}
