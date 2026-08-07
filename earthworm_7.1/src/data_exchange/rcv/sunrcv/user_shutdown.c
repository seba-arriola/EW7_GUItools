/*
	This is a dummy user_shutdown routine.  If a real shutdown routine is
needed, replace this routine in its entirety
*/
#include <stdio.h>
user_shutdown()
{
	extern FILE *logout;
	fprintf(logout," USERSHUTDOWN called\n");
	return;
}
