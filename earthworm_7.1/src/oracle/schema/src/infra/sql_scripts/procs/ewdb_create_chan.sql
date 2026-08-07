/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Chan
(OUT_idChan out number,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idChan:
                  >0  DB idChan
                  -1  Unknown Error

				  Others:  Undefined
*/
Temp_idChan     number;
Temp_idComment  number;

begin

  /**********************************/
  /* Deal with Comment Strings      */
  /**********************************/
  if IN_sComment IS NULL then
    Temp_idComment := 0;
  else
    Create_Comment(Temp_idComment, IN_sComment);
  end if;

  /**********************************/
  /* Get A New ChanID.              */
  /**********************************/
  select ChanSeq.NEXTVAL into Temp_idChan from sys.dual;

  Create_Core_idKey(Temp_idChan);
  if Temp_idChan <= 0 then
    OUT_idChan := Temp_idChan;
  end if;

  /**********************************/
  /* Insert new Chan Record         */
  /**********************************/
  insert into Chan(idChan,idComment)
    values(Temp_idChan,Temp_idComment);

  /**********************************/
  /* Set the idChan return value   */
  /**********************************/

  OUT_idChan := Temp_idChan;

EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idChan := -1;
END;
