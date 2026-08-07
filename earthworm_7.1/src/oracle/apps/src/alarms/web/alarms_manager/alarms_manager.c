
/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: alarms_manager.c,v 1.6 2002/05/28 19:15:59 lucky Exp $
 *    Revision history:
 *
 */
  

#include <alarms.h>
#include <webparse.h>
#include <html_common.h>

#define		INIT_BUF_SIZE	100

/* qsort comparison routines */
static int RecipientList_CompareRecipients (const void *arg1, const void *arg2);
static int CritList_CompareCriteria (const void *arg1, const void *arg2);
static int FormList_CompareFormats (const void *arg1, const void *arg2);

/* other prototypes from functions in this file */
int html_recipient_manager(void);
int html_program_manager(void);
int html_format_manager(void);


int	main()
{

	/* Initialize some stuff
	 ***********************/
	DEBUG = 0;

	/* Read the configuration file (path hardcoded relative to executable)
	 *********************************************************************/
	ReadConfig ("../params/alarms.d");

	logit_init ("alarms_manager", 1, 1024, 1);  


	printf("Content-type: text/html\n\n");
	printf("<html>\n");
	printf("<HEAD><TITLE>Alarms Manager</TITLE></HEAD>\n");

	html_header (BackgroundColor, HeaderLogo, HeaderTag);

	printf("<center>\n");
	printf("<H1><FONT COLOR=blue>Earthworm Alarms Manager<br></FONT></H1>\n" );
	printf("</center>\n");

	/* Connect to the DB */
	if (ewdb_api_Init (DBuser, DBpassword, DBservice) != 0)
	{
		html_logit( "", "Trouble connecting to database; exiting!\n" );
		goto shutdown;
	}

	printf ("<BR><HR><BR>\n");
	if (html_recipient_manager () != EW_SUCCESS)
	{
		html_logit ("", "Call to html_recipient_manager failed.\n");
		goto shutdown;
	}
	printf ("<BR><HR><BR>\n");

	if (html_program_manager () != EW_SUCCESS)
	{
		html_logit ("", "Call to html_program_manager failed.\n");
		goto shutdown;
	}
	printf ("<BR><HR><BR>\n");

	if (html_format_manager () != EW_SUCCESS)
	{
		html_logit ("", "Call to html_format_manager failed.\n");
		goto shutdown;
	}
	printf ("<BR><HR><BR>\n");


shutdown:
	html_trailer (WebHost, FooterLogo, FooterTag);
	ewdb_api_Shutdown();
	logit("","alarms_manager: terminating\n" );

	return EW_SUCCESS;

}  /* end main() */



/********************* html_recipient_manager *********************/
int html_recipient_manager(void)
{

	int						i, NumRetr, NumFound;
	EWDB_AlarmsRecipientStruct			*pBuf;


	if ((pBuf = (EWDB_AlarmsRecipientStruct *) malloc (INIT_BUF_SIZE * 
								sizeof (EWDB_AlarmsRecipientStruct))) == NULL)
	{
		html_logit( "", "Can't malloc Recipient buffer. \n" );
		return EW_FAILURE;
	}

	
	
	/* Retrieve Recipient List */
	if (ewdb_api_GetAlarmsRecipientList (-1, pBuf, &NumFound, &NumRetr, 
									INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		html_logit( "", "Call to ewdb_api_GetAlarmsRecipientList failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Buffer too small: Retr=%d, Found=%d; allocating more.\n", 
																NumRetr, NumFound);

		free (pBuf);
		if ((pBuf = (EWDB_AlarmsRecipientStruct *) malloc (NumFound * 
									sizeof (EWDB_AlarmsRecipientStruct))) == NULL)
		{
			html_logit( "", "Can't malloc Recipient buffer. \n" );
			return EW_FAILURE;
		}

		/* Retrieve Recipient List */
		if (ewdb_api_GetAlarmsRecipientList (-1, pBuf, &NumFound, &NumRetr, 
											NumFound) == EWDB_RETURN_FAILURE)
		{
			html_logit( "", "Call to ewdb_api_GetAlarmsRecipientList failed.\n");
			return EW_FAILURE;
		}
	}
			
	/* quick sort the list into alphabetical order */
	qsort (pBuf, NumRetr, sizeof (EWDB_AlarmsRecipientStruct), RecipientList_CompareRecipients);


	printf ("<BR><BR><CENTER>\n");

	printf ("<STRONG>RECIPIENT MANAGER</STRONG><BR><P>\n");
	printf ("Select a recipient to modify, or select New Recipient to create a new recipient\n");

	/* Put up a list of recipients to chose from */

	printf ("<FORM NAME=\"AlarmsRecipientForm\" ACTION=\"alarms_process_recipient\" METHOD=POST>\n");

	printf ("<TABLE BORDER=0>\n");

	/* One-row table */
	printf ("<TR VALIGN=CENTER>\n");

	/* First column: list of recipients */ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<SELECT NAME=recipient SIZE=3>\n");

	printf ("<OPTION VALUE=%d>New Recipient\n", NEW_ENTRY_FLAG);
	for (i = 0; i < NumRetr; i++)
	{
		printf ("<OPTION VALUE=%u>%s\n", pBuf[i].idRecipient, pBuf[i].sDescription);
	}

	printf ("</SELECT>\n");

	/* Second column: submit button*/ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"submit\" NAME=\"submit\">\n");

	printf ("</TR></TABLE></FORM>\n");

	return EW_SUCCESS;
}  /* end html_recipient_manager() */



/********************* html_program_manager *********************/
int html_program_manager(void)
{

	int						i, NumRetr, NumFound;
	EWDB_AlarmsCritProgramStruct	*pBuf;


	if ((pBuf = (EWDB_AlarmsCritProgramStruct *) malloc (INIT_BUF_SIZE * 
								sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
	{
		html_logit( "", "Can't malloc Program buffer. \n" );
		return EW_FAILURE;
	}

	
	/* Retrieve Program List */
	if (ewdb_api_GetAlarmsCriteriaList (-1, pBuf, &NumFound, &NumRetr, 
											INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		html_logit( "", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Buffer too small: Retr=%d, Found=%d; allocating more.\n", 
																NumRetr, NumFound);

		free (pBuf);
		if ((pBuf = (EWDB_AlarmsCritProgramStruct *) malloc (NumFound * 
									sizeof (EWDB_AlarmsCritProgramStruct))) == NULL)
		{
			html_logit( "", "Can't malloc Program buffer. \n" );
			return EW_FAILURE;
		}

		/* Retrieve Program List */
		if (ewdb_api_GetAlarmsCriteriaList (-1, pBuf, &NumFound, 
								&NumRetr, NumFound) == EWDB_RETURN_FAILURE)
		{
			html_logit( "", "Call to ewdb_api_GetAlarmsCriteriaList failed.\n");
			return EW_FAILURE;
		}
	}

	/* quick sort the list into alphabetical order */
	qsort (pBuf, NumRetr, sizeof (EWDB_AlarmsCritProgramStruct), CritList_CompareCriteria);
			

	printf ("<BR><BR><CENTER>\n");

	printf ("<STRONG>CRITERIA PROGRAM MANAGER</STRONG><BR><P>\n");
	printf ("Select a program to modify, or select New Program to create a new program\n");

	/* Put up a list of programs to chose from */

	printf ("<FORM NAME=\"AlarmsCritForm\" ACTION=\"alarms_process_prog\" METHOD=POST>\n");

	printf ("<TABLE BORDER=0>\n");

	/* One-row table */
	printf ("<TR VALIGN=CENTER>\n");

	/* First column: list of programs */ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<SELECT NAME=crit SIZE=3>\n");

	printf ("<OPTION VALUE=%d>New Program\n", NEW_ENTRY_FLAG);
	for (i = 0; i < NumRetr; i++)
	{
		printf ("<OPTION VALUE=%u>%s\n", pBuf[i].idCritProgram, pBuf[i].sProgDescription);
	}

	printf ("</SELECT>\n");

	/* Second column: submit button*/ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"submit\" NAME=\"submit\">\n");

	printf ("</TR></TABLE></FORM>\n");

	return EW_SUCCESS;
} /* html_program_manager() */


/********************* html_format_manager *********************/
int html_format_manager(void)
{

	int						i, NumRetr, NumFound;
	EWDB_AlarmsFormatStruct		*pBuf;


	if ((pBuf = (EWDB_AlarmsFormatStruct *) malloc (INIT_BUF_SIZE * 
								sizeof (EWDB_AlarmsFormatStruct))) == NULL)
	{
		html_logit( "", "Can't malloc Format buffer. \n" );
		return EW_FAILURE;
	}

	
	/* Retrieve Formats */
	if (ewdb_api_GetAlarmsFormats (-1, pBuf, &NumFound, &NumRetr, 
											INIT_BUF_SIZE) == EWDB_RETURN_FAILURE)
	{
		html_logit( "", "Call to ewdb_api_GetAlarmsFormats failed.\n");
		return EW_FAILURE;
	}

	if (NumRetr < NumFound)
	{
		logit ("", "Buffer too small: Retr=%d, Found=%d; allocating more.\n", 
																NumRetr, NumFound);

		free (pBuf);
		if ((pBuf = (EWDB_AlarmsFormatStruct *) malloc (NumFound * 
									sizeof (EWDB_AlarmsFormatStruct))) == NULL)
		{
			html_logit( "", "Can't malloc Format buffer. \n" );
			return EW_FAILURE;
		}

		/* Retrieve Formats */
		if (ewdb_api_GetAlarmsFormats (-1, pBuf, &NumFound, 
								&NumRetr, NumFound) == EWDB_RETURN_FAILURE)
		{
			html_logit( "", "Call to ewdb_api_GetAlarmsFormats failed.\n");
			return EW_FAILURE;
		}
	}

	/* quick sort the list into alphabetical order */
	qsort (pBuf, NumRetr, sizeof (EWDB_AlarmsFormatStruct), FormList_CompareFormats);


	printf ("<BR><BR><CENTER>\n");

	printf ("<STRONG>FORMAT MANAGER</STRONG><BR><P>\n");
	printf ("Select a format to modify, or select New Format to create a new format\n");

	/* Put up a list of formats to chose from */

	printf ("<FORM NAME=\"FormatsForm\" ACTION=\"alarms_process_format\" METHOD=POST>\n");

	printf ("<TABLE BORDER=0>\n");

	printf ("<TR VALIGN=CENTER>\n");

	/* list of formats */ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<SELECT NAME=format SIZE=3>\n");

	printf ("<OPTION VALUE=%d>New Format\n", NEW_ENTRY_FLAG);
	for (i = 0; i < NumRetr; i++)
	{
		printf ("<OPTION VALUE=%u>%s\n", pBuf[i].idFormat, pBuf[i].sDescription);
	}

	printf ("</SELECT>\n");

	/* submit button*/ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"submit\" NAME=\"submit\">\n");

	/* preview button*/ 
	printf ("<TD ALIGN=CENTER>\n");
	printf ("<INPUT TYPE=\"submit\" VALUE=\"preview\" NAME=\"preview\">\n");
	printf ("</TR>\n");

	printf ("</TR></TABLE></FORM>\n");

	return EW_SUCCESS;

}  /* html_format_manager() */



/**********************************************************************/
static int RecipientList_CompareRecipients(const void *arg1, const void *arg2)
{

	int	retval;
    EWDB_AlarmsRecipientStruct  *par1 = (EWDB_AlarmsRecipientStruct *) arg1;
    EWDB_AlarmsRecipientStruct  *par2 = (EWDB_AlarmsRecipientStruct *) arg2;

	retval = strcmp (par1->sDescription, par2->sDescription);

    if (retval < 0)
        return (-1);
    else if (retval == 0)
       return (0);
    else
        return (1);
} /* RecipientList_CompareRecipients() */


/**********************************************************************/
static int CritList_CompareCriteria(const void *arg1, const void *arg2)
{

	int	retval;
    EWDB_AlarmsCritProgramStruct  *par1 = (EWDB_AlarmsCritProgramStruct *) arg1;
    EWDB_AlarmsCritProgramStruct  *par2 = (EWDB_AlarmsCritProgramStruct *) arg2;

	retval = strcmp (par1->sProgDescription, par2->sProgDescription);

    if (retval < 0)
        return (-1);
    else if (retval == 0)
       return (0);
    else
        return (1);
}  /* end CritList_CompareCriteria() */


/**********************************************************************/
static int FormList_CompareFormats(const void *arg1, const void *arg2)
{

	int	retval;
    EWDB_AlarmsFormatStruct  *par1 = (EWDB_AlarmsFormatStruct *) arg1;
    EWDB_AlarmsFormatStruct  *par2 = (EWDB_AlarmsFormatStruct *) arg2;

	retval = strcmp (par1->sDescription, par2->sDescription);

    if (retval < 0)
        return (-1);
    else if (retval == 0)
       return (0);
    else
        return (1);
} /* end FormList_CompareFormats() */

