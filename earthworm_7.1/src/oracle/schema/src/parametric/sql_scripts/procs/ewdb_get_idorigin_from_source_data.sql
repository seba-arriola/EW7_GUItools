/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE FUNCTION Get_idOrigin_From_Source_Data
(OUT_idOrigin out number,
 OUT_idEvent out number,
 IN_sSource varchar,
 IN_sSourceEventID varchar,
 IN_iVersionNum number,
 IN_tiExternal number,
 IN_xidExternal varchar
) return NUMBER
as
/* Return Codes:
         -1X  Error in Create_Core_idKey(), X describes the error returned
                by Create_Core_idKey()
           0  Valid matching origin already exists.
           1  Origin did not exist.  Origin was created and validated.  
           2  Origin created but not validated.  Either IN_sSourceEventID or 
                IN_iVersionNum was invalid.
          -1  Unknown Exception
          -2  Error in Get_idSource()
          -3  Error Creating Origin Record with NULL version number.
          -4  Error Creating Origin Record with valid version number.
	    Others: Undefined
*/

Temp_idOrigin    Number;
Temp             Number;
State            Number;
Temp_idSource    Number;
Temp_sSourceEventID varchar(50);
Temp_iVersionNum Number;
Temp_idEvent     Number;
Temp_idExternalEvent 
                 Number;
Temp_tiOrigin    Number;
begin

  State := 0;

  /**********************************/
  /* Get the idSource               */
  /**********************************/
  Get_idSource(Temp_idSource,IN_sSource);
  if Temp_idSource < 1 then
    insert into test values('Get_idOFSidSource ' || IN_sSource,Temp_idSource,0);
    return(-2);
  end if;

  State := 1;

  if(IN_sSourceEventID IS NULL or IN_sSourceEventID ='') then
    Temp_sSourceEventID := NULL;
  else 
    Temp_sSourceEventID := IN_sSourceEventID;
  end if;

  if(IN_iVersionNum IS NULL or IN_iVersionNum = 0) then
    Temp_iVersionNum := NULL;
  else 
    Temp_iVersionNum := IN_iVersionNum;
  end if;

  State := 11;

  if(Temp_sSourceEventID IS NULL or Temp_iVersionNum IS NULL) then
    /* we can't provide any validation.  both of these values need
       to be valid in order to enforce the 
       key (idSource, sSourceEventID, iVersionNum) 
     */
    State := 12;



    State := 13;

    Create_Base_Origin_Record(Temp_idOrigin, Temp_idSource, NULL, IN_tiExternal, IN_xidExternal);
    if(Temp_idOrigin < 0) then
      return(-3);
    else
      OUT_idOrigin := Temp_idOrigin;
      return(2);
    end if;
  end if;


  State := 2;

  select idExternalEvent into Temp_idExternalEvent
    from ExternalEvent
    where idSource = Temp_idSource 
      and sSourceEventID = IN_sSourceEventID;

  State := 3;

  select idEvent into Temp_idEvent from Bind 
    where idCore = Temp_idExternalEvent
      and tiCore = getti('ExternalEvent');

  State := 4;

  OUT_idEvent := Temp_idEvent;
  Temp_tiOrigin := getti('Origin');

  select o.idOrigin into OUT_idOrigin 
    from Origin o, Bind b
    where b.idEvent = Temp_idEvent
      and b.tiCore = Temp_tiOrigin
      and b.idCore = o.idOrigin
      and o.iVersionNum = IN_iVersionNum;


  State := 5;

  return(0);

  /**********************************/
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if(State = 4) then
      /* this is the expected case */
      Create_Base_Origin_Record(Temp_idOrigin, Temp_idSource, Temp_iVersionNum, 
                                IN_tiExternal, IN_xidExternal);
      if(Temp_idOrigin < 0) then
        return(-4);
      else
        OUT_idOrigin := Temp_idOrigin;
        return(0);
      end if;
    else
      Temp:=SQLCODE;
      insert into test values('Get_idOFS_NDF ' || IN_sSourceEventID,Temp,State);
      return(-1);
    end if;

  WHEN OTHERS THEN
    Temp:=SQLCODE;
    insert into test values('Get_idOFS ' || IN_sSourceEventID,Temp,State);
    return(-1);
End;
