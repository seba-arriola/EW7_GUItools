/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_mwchan.sql,v $
/*     Revision 1.1  2005/05/24 21:35:51  davidk
/*     Added logic to delete mw based data.
/*
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/



CREATE OR REPLACE Function Delete_MWChan
(IN_idMwChan      number
) 
RETURN NUMBER

as

Temp            number;
State           number;
Temp_RetCode    number := 0;
Temp_idSynthTS  number;
Temp_idRealTS   number;
begin

  State := 1;

  select idSynthTS, idRealTS into Temp_idSynthTS, Temp_idRealTS 
   from MwChan where idMwChan = IN_idMwChan;

  if(Temp_idSynthTS IS NOT NULL) THEN 
    Temp := Delete_MwCTS(Temp_idSynthTS);
  end if;

  if(Temp_idRealTS IS NOT NULL) THEN 
    Temp := Delete_MwCTS(Temp_idRealTS);
  end if;

  delete MwChan where idMwChan = IN_idMwChan;

  State := 10;
  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      insert into test values('Delete_MwChan_FK ' || IN_idMwChan, Temp, State);
      return(-3);
    else
      insert into test values('Delete_MwChan_Exc ' || IN_idMwChan, Temp, State);
      return(-1);
    end if;
END;

