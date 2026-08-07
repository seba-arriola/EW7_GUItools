#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/filio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h> 
#include <signal.h>
main()
{	struct tm *tmp;
	time_t tloc;
	printf("FIONBIO=%x\n",FIONBIO);
	tmp=gmtime( &(tloc=time(0)));
	printf("yr=%d yday=%d\n",tmp->tm_year, tmp->tm_yday);
}
