/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: ewdb_get_idpick.sql,v 1.5 2004/03/31 19:04:42 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_get_idpick.sql,v $
 *     Revision 1.5  2004/03/31 19:04:42  davidk
 *     Improved debug logging in exception handling code.
 *
 *
 *
 */

CREATE OR REPLACE FUNCTION Get_idPick
(OUT_idPick out number,
 IN_sSource varchar,
 IN_sSourcePickID varchar
)
  RETURN NUMBER

as
/* 

   Return Codes:
	    0    SUCCESS
	   -1    ERROR:  see debugging table for details
		 -2    ERROR:  error looking up idSource from given source
		 -3    ERROR:  Pick with matching source/sourcepickID not found

	 
*/

Temp_idSource    Number;
Temp_idPick      Number;
Temp             Number;

begin

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      if(Temp_idSource = -3) then
        Temp_idSource := NULL;
      else
        return(-2);
      end if;
    end if;

    /*****************************************************************/
    /* See if we already know about this source/sourceid combination */
    /*****************************************************************/

    select idPick into Temp_idPick from Pick
        where idSource = Temp_idSource
          and xidExternal = IN_sSourcePickID;


    OUT_idPick := Temp_idPick;
		return(0);

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/
EXCEPTION
    WHEN NO_DATA_FOUND THEN

		  return(-3);

    WHEN OTHERS THEN
      Temp:=SQLCODE;
      insert into test values ('GetidPick_ex' || IN_sSource || ' ' || IN_sSourcePickID , Temp, Temp_idPick);
      return(-1);

end;

