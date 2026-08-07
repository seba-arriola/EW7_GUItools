/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Coincidence
(OUT_idCoincidence out number,
 IN_tCoincidence in number,
 IN_bBindToEvent number,
 IN_idEvent number,
 IN_sSource varchar,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idCoincidence:
          >0  DB idCoincidence
          -1  Duplicate Unique Value Exception in Coincidence
          -2  Source Function Failed
                  -3  Unexpected Exception
                  -4  NO_DATA_FOUND Exception
                  -11 Create_Comment failed for IN_sComment
          -12 Get_idSource() Function Failed for IN_sSource
          -14 IN_bBindToEvent set, and IN_idEvent invalid!
          -2X Error in Create_Bind(), X describes the error returned
               by Create_Bind()
          -3X Error in Check_External_Record_Validity(), X describes
               the error returned Check_External_Record_Validity()
         -1XX Error in Set_Prefer(), XX describes the error returned
               by Set_Prefer()
                  Others:  Undefined
*/
Temp_idSource    Number;
Temp_CommentID   Number;
Temp_idCoinc     Number;
Temp_idBind      Number;
Temp_idEvent     Number := IN_idEvent;
Temp             Number;

begin

    /**********************************/
    /* Deal with Comment String       */
    /**********************************/
    if IN_sComment IS NULL then
      Temp_CommentID := 0;
    else
      Create_Comment(Temp_CommentID,IN_sComment);
      if Temp_CommentID < 1 then
        OUT_idCoincidence := -11;
        return;
      end if;
    end if;

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      OUT_idCoincidence := -12;
      return;
    end if;


    /**********************************/
    /* Get a new idCoincidence        */
    /**********************************/
    select CoincSeq.NEXTVAL into Temp_idCoinc from sys.dual;

    Create_Core_idKey(Temp_idCoinc);
    if Temp_idCoinc <= 0 then
        OUT_idCoincidence := Temp_idCoinc;
    end if;


    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
    if IN_bBindToEvent = 1 then
      Check_idEvent_Validity (Temp_idEvent);
      if Temp_idEvent < 1 then
        OUT_idCoincidence := -14;
        return;
      end if;
    end if;

    /***************************************/
    /* Insert the new Coincidence record   */
    /***************************************/
     insert into CoincidenceEvent
         (idCoincidence, tCoincidence, idEvent, idSource, idComment)
       values
         (Temp_idCoinc, IN_tCoincidence, Temp_idEvent, Temp_idSource, Temp_CommentID);


    /***************************************/
    /* Bind our Coincidence to the Event   */
    /***************************************/
    if IN_bBindToEvent = 1 then
      Create_Bind (Temp_idBind, Temp_idEvent, 'CoincidenceEvent', Temp_idCoinc);
      if Temp_idBind < 1 then
         OUT_idCoincidence := -20 + Temp_idBind;
         return;
      end if;

    end if;

    /**********************************************/
    /* Set OUT_idCoincidence to the idCoincidence
    /* of the record we just created.
    /**********************************************/
    OUT_idCoincidence := Temp_idCoinc;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/
EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      OUT_idCoincidence := -3;
    WHEN NO_DATA_FOUND THEN
      OUT_idCoincidence := -4;
    WHEN OTHERS THEN
      Temp:=SQLCODE;
      insert into test values ('Create_Coinc_excep', Temp, IN_idEvent);
      OUT_idCoincidence := -1;

end;
