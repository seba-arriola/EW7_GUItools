/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */



CREATE OR REPLACE PROCEDURE Create_Base_Origin_Record
(OUT_idOrigin out  number,
 IN_idSource       number,
 IN_iVersionNum    number,
 IN_tiExternal     number,
 IN_xidExternal    varchar
) 
as

  /*   Return Codes for OUT_idOrigin:
        >0      Success;  new idOrigin
        -1      Unknown Error
        -2      Duplicate Existing Origin Record
       -1X      Error in Create_Core_idKey() X indicates
                  the error returned by that proc.
   ******************************************/

Temp_idOrigin   number;
Temp            number;

begin
  /**********************************/
  /* Get a new idOrigin             */
  /**********************************/
  select OriginSeq.NEXTVAL into Temp_idOrigin from sys.dual;

  Create_Core_idKey(Temp_idOrigin);
  if Temp_idOrigin <= 0 then
    OUT_idOrigin := -10 + Temp_idOrigin;
    return;
  end if;

  insert into origin(idOrigin,idSource,tOrigin,dLat,dLon,iVersionNum,
                     tiExternal,xidExternal)
    values(Temp_idOrigin, IN_idSource, 0,0,0, IN_iVersionNum, 
           IN_tiExternal, IN_xidExternal);

  OUT_idOrigin := Temp_idOrigin;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    OUT_idOrigin := 1;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Base_Origin_Record' || IN_idSource, Temp, Temp_idOrigin);
    OUT_idOrigin := -1;
END;
