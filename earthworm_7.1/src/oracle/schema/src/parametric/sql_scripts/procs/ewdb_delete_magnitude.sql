/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Delete_Magnitude
 (IN_idMag number) RETURN number
as


Temp               number;
Temp_iMagType      number;
Temp_sMagType      varchar(5);
Temp_idOrigin      number;


begin
  select iMagtype, idOrigin into Temp_iMagType, Temp_idOrigin 
   from Magnitude where idMag = IN_idMag;

  select sMagAbbrev into Temp_sMagType 
   from MagType where iMagType = Temp_iMagType;



  if(Temp_sMagType = 'Mw') THEN
    Temp:=Delete_MWs_For_Origin(Temp_idOrigin);
  else
    Delete MagLink where idMag = IN_idMag;
  end if;

  Delete Magnitude where idMag = IN_idMag;
  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      return(1);
    else
      insert into test values('Delete_Magnitude',IN_idMag,Temp);
      return(-1);
    end if;
END;