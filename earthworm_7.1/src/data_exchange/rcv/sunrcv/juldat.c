/*main()
{
	char mon[4];
	int yr,day;
	for (;;) {
		printf("Month : ");
		scanf("%s",mon);
		printf("day : ");
		scanf("%d",&day);
		printf("Year : ");
		scanf("%d",&yr);
		printf("%s %d, %d\n",mon,day,yr);
		printf("Jul day=%d\n",julday(mon,day,yr));
	}
}*/
	
/*main() 
{
	int yr,julian,mon,day;

	for(;;) {
		printf("year : ");
		scanf("%d",&yr);
		printf("julian : ");
		scanf("%d",&julian);
		datjul(yr,julian,&mon, &day);
		printf("mon=%d day=%d\n",mon,day);
	}
}	*/
/*
	From a Unix 3 character month in ascii, a day of month, and year,
	calculate the Julian day of year.  Parts stolen from K&R's C book.
*/
static char daytab[2][13] = {
	{0,31,28,31,30,31,30,31,31,30,31,30,31},
	{0,31,29,31,30,31,30,31,31,30,31,30,31}
};
int juldat(mon,day,yr)
int day,yr;
char *mon;
{
	char mn[4];
	int i,month,leap;
	for (i=0; i<3; i++) mn[i]=*(mon+i);				/* create string for comparison*/
	mn[3]=0;
	month=-1;
	if(strcmp(mn,"Jan") == 0) month=1;				/* calculate month of year */
	if(strcmp(mn,"Feb") == 0) month=2;
	if(strcmp(mn,"Mar") == 0) month=3;
	if(strcmp(mn,"Apr") == 0) month=4;
	if(strcmp(mn,"May") == 0) month=5;
	if(strcmp(mn,"Jun") == 0) month=6;
	if(strcmp(mn,"Jul") == 0) month=7;
	if(strcmp(mn,"Aug") == 0) month=8;
	if(strcmp(mn,"Sep") == 0) month=9;
	if(strcmp(mn,"Oct") == 0) month=10;
	if(strcmp(mn,"Nov") == 0) month=11;
	if(strcmp(mn,"Dec") == 0) month=12;
	leap= yr%4 == 0 && yr%100 != 0 || yr%400 == 0;	/* is it a leap year */
/*	printf("Mon=%s month=%d leap=%d\n",mn,month,leap);*/
	for (i=1; i< month; i++) day+=daytab[leap][i];	/* add up all the full months */
	return day;
}

/* convert the yr and julian day to a month and day*/
int datjul(yr,julian,mon,day)
		int yr,julian;
int *mon, *day;
{	int leap,j;
	int sum;
	leap= yr%4 == 0 && yr%100 != 0 || yr%400 == 0;	/* is it a leap year */
	sum=0;
	for(j=1; j<=12; j++) {
		if(sum < julian && sum+daytab[leap][j] >= julian) {
			*mon=j;
			*day=julian-sum;
			return 0;
		}
		sum += daytab[leap][j];
	}
}
