/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_mws_for_origin.sql,v $
/*     Revision 1.1  2005/05/24 21:35:51  davidk
/*     Added logic to delete mw based data.
/*
/*     Revision 1.3  2005/01/12 22:05:33  davidk
/*     Updated based on bugs and problems found in hydra_reaper.
/*
/*     Revision 1.2  2004/09/09 19:21:00  davidk
/*     Changed call to Delete_Pick() which now takes 1 parameter.
/*
/*     Revision 1.1  2004/09/07 19:31:15  davidk
/*     Reaper v2 09/02/2004.
/*                 */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/



CREATE OR REPLACE Function Delete_MWs_For_Origin
(IN_idOrigin      number
) 
RETURN NUMBER

as

Cursor Mw_Cursor(Cursor_idOrigin number) is
  select idMw from Mw
    where idOrigin=Cursor_idOrigin;

Temp            number;
State           number;
Temp_RetCode    number := 0;

begin

  State := 1;
  /* handle dependent magnitudes */
  select count(idMw) into Temp from Mw where idOrigin = IN_idOrigin;
  if(Temp > 0) then
    State := 2;
    for Curr in Mw_Cursor(IN_idOrigin) loop
      State := 3;
        Temp := Delete_Mw(Curr.idMw);
        if(Temp < 0) then
          return(-2);
        elsif(Temp > 0) then
          Temp_RetCode := Temp_RetCode + 1;
        end if;

    end loop;  /* for each mw */
  end if;      

  State := 10;
  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      insert into test values('Delete_MwsFOrig_FK ' || IN_idOrigin, Temp, State);
      return(1);
    else
      insert into test values('Delete_MwsFOrig_Exc ' || IN_idOrigin, Temp, State);
      return(-1);
    end if;
END;

