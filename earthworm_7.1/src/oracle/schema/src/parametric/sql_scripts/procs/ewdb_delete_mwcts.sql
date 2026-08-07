/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_mwcts.sql,v $
/*     Revision 1.1  2005/05/24 21:35:51  davidk
/*     Added logic to delete mw based data.
/*                      */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/



CREATE OR REPLACE Function Delete_MWCTS
(IN_idMwCTS      number
) 
RETURN NUMBER

as

Temp            number;
State           number;
Temp_RetCode    number := 0;

begin

  State := 1;

  delete MwChanTS where idMwCTS = IN_idMwCTS;

  State := 10;
  return(0);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      return(1);
    else
      insert into test values('Delete_MwCTS_Exc ' || IN_idMwCTS, Temp, State);
      return(-1);
    end if;
END;

