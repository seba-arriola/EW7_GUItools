/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Bind
(OUT_idBind out number,
 IN_idEvent in number,
 IN_sCoreTableName in varchar,
 IN_idCore in number
)
as
/* Return Codes for OUT_idBind:
                  >0  DB idEvent
                  -1  Unknown Error
				  -2  Invalid eventID
				  -3  Invalid core table name
				  -4  Invalid core table id
				  -5  Bind Sequence Select Error
				  -6  Exception in Check_Record_Validity()
          -7  Unknown DUP_VAL_ON_INDEX exception!
          -9  Index Problem on Bind.  SERIOUS!!!
         -1X  Error in Check_Record_Validity(), X describes
               the error returned C_R_V()
				  Others:  Undefined
*/

Temp_idBind Number;
Temp_ID Number;
Temp_Result Number;
State   Number := 0;
Temp_idEvent     Number := IN_idEvent;
begin

begin

  /**********************************/
  /* Check for idEvent validity     */
  /**********************************/

  Check_idEvent_Validity(Temp_idEvent);
  if Temp_idEvent < 1 then
    OUT_idBind := -2;
    return;
  end if;

  State := 1;

  /**********************************/
  /* Check for core record validity   */
  /**********************************/
  Check_Record_Validity(Temp_ID,IN_idCore,IN_sCoreTableName);
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idBind := -10 + Temp_ID;
   return;
  end if;
    
  State := 2;

  /**********************************/
  /* Get A New BindID.              */
  /**********************************/
  select BindSeq.NEXTVAL into Temp_idBind from sys.dual;

  Create_Core_idKey(Temp_idBind);
  if Temp_idBind <= 0 then
    OUT_idBind := Temp_idBind;
  end if;

  /**********************************/
  /* Insert new Bind Record         */
  /**********************************/
  insert into bind(idBind,idEvent,tiCore,idCore)
    values(Temp_idBind,IN_idEvent,Temp_ID,IN_idCore);

  /**********************************/
  /* Set the idBind return value   */
  /**********************************/

  OUT_idBind := Temp_idBind;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
	  /*  idEvent not found */
	  OUT_idBind := -2;
	elsif State = 2 then
	  /*  Error selecting from BindSeq?  Huh??? */
	  OUT_idBind := -5;
	end if;

  WHEN DUP_VAL_ON_INDEX THEN
    if State = 2 then
      /**********************************/
      /* Be reasonably sure that it was */
      /* caused by our Bind insert :<)   */
      /**********************************/
      select idBind into Temp_idBind from Bind 
       where idEvent = IN_idEvent
         and tiCore  = Temp_ID
         and idCore  = IN_idCore;

      OUT_idBind := Temp_idBind;
    else
      OUT_idBind := -7;
    end if;

  WHEN OTHERS THEN
    if State = 1 then
	  /* Problem occurred  during Check_Record_Validity() */ 
    OUT_idBind := -6;
	else
	OUT_idBind := -1;
	end if;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
      /**********************************/
      /* There must be some screw up with Bind and it's keys, because
      /* we got a duplicate_key exception but we weren't able to find
      /* a record with that key.  Throw up our hands and go home!! 
      /**********************************/
   OUT_idBind := -9;
END;
