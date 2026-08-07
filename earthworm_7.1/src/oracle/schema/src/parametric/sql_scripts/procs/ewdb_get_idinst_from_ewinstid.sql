/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE FUNCTION Get_idInst_From_EWInstID
(OUT_idInst out number,
 IN_iEWInstID number
)
RETURN Number

as
/* Return Codes :
           0  Success
          -1  Unknown Error
	  Others:  Undefined


   Note:  This function is pretty empty.  We cannot create a
   new Installation on the fly, because we need unique things
   like Installation Name and Network Code, in addition to the
   EWInstID.  There are not many installations, and it does not
   seem like an undue strain to require the operator to preload
   the installation information.
*/

Temp_idInst   Number;
Temp          Number;
begin

  /**********************************/
  /* Check for Inst Record          */
  /**********************************/

  select idInst into OUT_idInst from Installation 
   where iEWInstID = IN_iEWInstID;

  return(0);
    

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Not a known installation, too bad!!
    /**********************************/
	return(-2);

  WHEN OTHERS THEN
    Temp:= SQLCODE;
	insert into test values('Get_idInst',Temp,IN_iEWInstID);
    return(-1);
END;


