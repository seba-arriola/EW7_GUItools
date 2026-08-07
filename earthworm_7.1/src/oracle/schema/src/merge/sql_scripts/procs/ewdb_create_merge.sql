/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Merge
(OUT_idMerge out number,
 IN_idPh number,
 IN_idEvent number,
 IN_sSource varchar,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idMerge:
          >0  DB idMerge
          -1  Duplicate Unique Value Exception in Merge
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
Temp_idMerge     Number;
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
        OUT_idMerge := -11;
        return;
      end if;
    end if;

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      OUT_idMerge := -12;
      return;
    end if;


    /**********************************/
    /* Get a new idMerge        */
    /**********************************/
    select MergeSeq.NEXTVAL into Temp_idMerge from sys.dual;

    Create_Core_idKey(Temp_idMerge);
    if Temp_idMerge <= 0 then
        OUT_idMerge := Temp_idMerge;
    end if;



    /**********************************/
    /* Deal With Event Issues         */
    /**********************************/
    Check_idEvent_Validity (Temp_idEvent);
      if Temp_idEvent < 1 then
        OUT_idMerge := -14;
        return;
      end if;

    /***************************************/
    /* Insert the new Merge record   */
    /***************************************/
     insert into Merge
         (idMerge, idPh, idEvent, idSource, idComment)
       values
         (Temp_idMerge, IN_idPh, Temp_idEvent, Temp_idSource, Temp_CommentID);


    /**********************************************/
    /* Set OUT_idMerge to the idMerge
    /* of the record we just created.
    /**********************************************/
    OUT_idMerge := Temp_idMerge;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/
EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      OUT_idMerge := -3;
    WHEN NO_DATA_FOUND THEN
      OUT_idMerge := -4;
    WHEN OTHERS THEN
      Temp:=SQLCODE;
      insert into test values ('Create_Merge_excep', Temp, IN_idEvent);
      OUT_idMerge := -1;

end;
