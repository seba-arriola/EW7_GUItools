/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
       
CREATE OR REPLACE PROCEDURE Get_PickId_ByExtId 
(OUT_idPick        out number,
 IN_xidExternal    in  varchar
)
as


/*******************************
     Return Codes:
         idPick:  Success
        -1:       Unknown Error
********************************/

Temp           number;

begin


  select idPick 
  into   OUT_idPick 
  from Pick 
  where xidExternal = IN_xidExternal;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
      OUT_idPick := -1;  
END;
