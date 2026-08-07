/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Preferred_Summary_Info 
(OUT_Retcode       out number,
 IN_idEvent        in  number,
 OUT_idOrigin      out number,
 OUT_idMag         out number,
 OUT_idMech        out number
)
as


/*******************************
     Return Codes:
         0:       Success
        -1:       Unknown Error
         1:       No Prefer record found
********************************/

Temp           number;


begin
  select idPrefOrigin,idPrefMag,idPrefMech
   into  OUT_idOrigin,OUT_idMag,OUT_idMech
   from Prefer 
   where idEvent = IN_idEvent;

  OUT_Retcode := 0;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_Retcode := 1;
    return;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_PSI_ex',0,Temp);
    OUT_Retcode := -1;
END;

     
