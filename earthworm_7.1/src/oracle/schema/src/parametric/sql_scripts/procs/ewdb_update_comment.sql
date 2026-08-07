/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


CREATE OR REPLACE PROCEDURE Update_Comment
(
 OUT_RetCode out number,
 IN_idCore number,
 IN_sCoreTableName varchar,
 IN_sComment varchar
)
 as
/* RETURN CODES:
      -1:     Unknown Exception see Debug Table
      -2:     Invalid record as described by (IN_idCore,IN_sCoreTableName)
      -3:     Unable to create comment record for IN_sComment
      -4:     Unable to update idComment for record

********************************************/
Temp           number;
Temp_RetCode   number;
State          number;
Temp_tiCore    number;
Temp_idCore    number;
Temp_idComment number;

BEGIN

  State := 1;
  Check_Record_Validity(Temp_tiCore,IN_idCore,IN_sCoreTableName);
  if Temp_tiCore <= 0 then
    OUT_RetCode := -2;
    return;
  end if;

  State := 2;
  Create_Comment(Temp_idComment,IN_sComment);
  if Temp_idComment <= 0 then
    OUT_RetCode := -3;
    return;
  end if;

  State := 3;
  Update_idComment(Temp,Temp_tiCore,Temp_idCore,Temp_idComment);
  if Temp <= 0 then
    OUT_RetCode := -4;
    return;
  end if;

  State := 4;
  Delete Comments where idComment=Temp;

  OUT_RetCode := 0;
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      OUT_RetCode := 0;
    else
      insert into test values('Update_Comment1_'|| IN_idCore,State,Temp);
      insert into test values('Update_Comment2_'|| IN_idCore,
                              Temp_idComment,Temp_tiCore);
      OUT_RetCode := -1;
    end if;
END;
