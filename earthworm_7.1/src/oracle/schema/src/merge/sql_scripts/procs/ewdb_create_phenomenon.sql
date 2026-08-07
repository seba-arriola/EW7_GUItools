/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Phenomenon
(OUT_idPh out number,
 IN_idPrefEvent number,
 IN_sSource varchar,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idPh:
          >0  DB idPh
          -1  Duplicate Unique Value Exception in Phenomena
          -2  Source Function Failed
                  -3  Unexpected Exception
                  -4  NO_DATA_FOUND Exception
                  -11 Create_Comment failed for IN_sComment
          -12 Get_idSource() Function Failed for IN_sSource
          -3X Error in Check_External_Record_Validity(), X describes
               the error returned Check_External_Record_Validity()
         -1XX Error in Set_Prefer(), XX describes the error returned
               by Set_Prefer()
                  Others:  Undefined
*/
Temp_idSource    Number;
Temp_CommentID   Number;
Temp_idPh     	 Number;
Temp_idPrefEvent     Number := IN_idPrefEvent;
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
        OUT_idPh := -11;
        return;
      end if;
    end if;

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      OUT_idPh := -12;
      return;
    end if;


    /**********************************/
    /* Get a new idPh        */
    /**********************************/
    select PhenomenaSeq.NEXTVAL into Temp_idPh from sys.dual;

    Create_Core_idKey(Temp_idPh);
    if Temp_idPh <= 0 then
        OUT_idPh := Temp_idPh;
    end if;


    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
     Check_idEvent_Validity (Temp_idPrefEvent);
     if Temp_idPrefEvent < 1 then
       OUT_idPh := -14;
       return;
     end if;

    /***************************************/
    /* Insert the new Phenomenon           */
    /***************************************/
     insert into Phenomena
         (idPh, idPrefEvent, idSource, idComment)
       values
         (Temp_idPh, Temp_idPrefEvent, Temp_idSource, Temp_CommentID);


    /**********************************************/
    /* Set OUT_idPh to the idPh
    /* of the record we just created.
    /**********************************************/
    OUT_idPh := Temp_idPh;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/
EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      OUT_idPh := -3;
    WHEN NO_DATA_FOUND THEN
      OUT_idPh := -4;
    WHEN OTHERS THEN
      Temp:=SQLCODE;
      insert into test values ('Create_Phenomenon_excep', Temp, IN_idPrefEvent);
      OUT_idPh := -1;

end;
