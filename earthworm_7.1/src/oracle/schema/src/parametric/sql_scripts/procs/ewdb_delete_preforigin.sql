/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_preforigin.sql,v $
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



CREATE OR REPLACE Function Delete_PrefOrigin
(IN_idOrigin      number
) 
RETURN NUMBER

as

Cursor Mag_Cursor(Cursor_idOrigin number) is
  select idMag from Magnitude
    where idOrigin=Cursor_idOrigin;

Cursor OP_Cursor(Cursor_idOrigin number) is
  select idPick from OriginPick
    where idOrigin=Cursor_idOrigin;

Temp            number;
State           number;
Temp_RetCode    number := 0;

begin

  /* handle prefer record.  We have to delete the prefer record before
     we can delete the preferred magnitude record, and one of the magnitudes
	 for this origin SHOULD be the PREFERRED */
  State := 7;
  Delete Prefer where idPrefOrigin = IN_idOrigin;

  State := 1;
  /* handle dependent magnitudes */
  select count(idMag) into Temp from Magnitude where idOrigin = IN_idOrigin;
  if(Temp > 0) then
    State := 2;
    for Curr in Mag_Cursor(IN_idOrigin) loop
      State := 3;
        Temp := Delete_Magnitude(Curr.idMag);
        if(Temp < 0) then
          return(-2);
        elsif(Temp > 0) then
          Temp_RetCode := Temp_RetCode + 1;
        end if;

    end loop;  /* for each mag */
  end if;      


  State := 4;

  State := 5;
  /* handle originpicks and associated picks */
  Delete OriginPick where idOrigin = IN_idOrigin;

  State := 8;
  Delete MechFM where idOrigin = IN_idOrigin;

  State := 9;
  Delete Origin where idOrigin = IN_idOrigin;

  State := 10;
  return(Temp_RetCode);

EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if(Temp = -2292) THEN    /* Foreign Key constraint */
      insert into test values('Delete_PrefOrig_FK ' || IN_idOrigin, Temp, State);
      return(1);
    else
      insert into test values('Delete_PrefOrig_Exc ' || IN_idOrigin, Temp, State);
      return(-1);
    end if;
END;
