/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE FUNCTION Create_Installation
(OUT_idInst out number,
 IN_iEWInstID number,
 IN_sShortName varchar,
 IN_sFullName varchar,
 IN_sPrimaryNetCode varchar,
 IN_sAltNetCodes varchar,
 IN_sPrimaryContact varchar,
 IN_sNote varchar
)
RETURN Number

as
/* Return Codes :
           0  Success
          -1  Unknown Error
		  -2  Existing installation with matching iEWInstID
		  -3  Existing installation with matching name
		  -4  Existing installation with matching Primary Network Code
	  Others:  Undefined


*/
Temp          Number;
Temp_idInst   Number;
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


