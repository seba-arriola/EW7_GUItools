/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_mw.sql,v $
/*     Revision 1.1  2005/05/24 21:35:51  davidk
/*     Added logic to delete mw based data.
/*
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/



CREATE OR REPLACE Function Delete_MW
(IN_idMw      number
) 
RETURN NUMBER

as

Cursor MwChan_Cursor(Cursor_idMw number) is
  select idMwChan from MwChan
    where idMw=Cursor_idMw;

Temp            number;
State           number;
Temp_RetCode    number := 0;

begin

  State := 1;
  /* handle dependent magnitudes */
  select count(idMw) into Temp from MwChan where idMw = IN_idMw;
  if(Temp > 0) then
    State := 2;
    for Curr in MwChan_Cursor(IN_idMw) loop
      State := 3;
        Temp := Delete_MwChan(Curr.idMwChan);
        if(Temp < 0) then
          return(-2);
        elsif(Temp > 0) then
          Temp_RetCode := Temp_RetCode + 1;
        end if;

    end loop;  /* for each mw */
  end if;      

  delete Mw where idMw = IN_idMw;

  State := 10;
  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      insert into test values('Delete_Mw_FK ' || IN_idMw, Temp, State);
      return(1);
    else
      insert into test values('Delete_Mw_Exc ' || IN_idMw, Temp, State);
      return(-1);
    end if;
END;

