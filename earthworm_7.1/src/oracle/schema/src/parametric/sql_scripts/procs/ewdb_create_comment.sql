/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Comment
(OUT_idComment out number,
 IN_sComment in varchar
)
as
/* Return Codes for OUT_idComment:
                  >0  DB idComment
                  -1  Unknown Error
				  Others:  Undefined
*/

Temp_idComment Number;
begin

  /**********************************/
  /* Get A New CommentID.           */
  /**********************************/
  select CommentsSeq.NEXTVAL into Temp_idComment from sys.dual;

  Create_Core_idKey(Temp_idComment);
  if Temp_idComment <= 0 then
    OUT_idComment := Temp_idComment;
  end if;

  /**********************************/
  /* Insert new Comment Record        */
  /**********************************/
  insert into Comments(idComment,sComment,idNextComment)
    values(Temp_idComment,IN_sComment,0);

  /**********************************/
  /* Set the idComment return value   */
  /**********************************/

  OUT_idComment := Temp_idComment;

EXCEPTION
  /**********************************/
  /* We aren't expecting any excep's*/
  /**********************************/
  WHEN OTHERS THEN
    OUT_idComment := -1;
END;
