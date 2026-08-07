/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Or_Update_Comment
(OUT_idComment out number,
 IN_idComment number,
 IN_sComment varchar
)
as
/* Return Codes for OUT_idComment:
          >0  DB idComment
           0  NULL string; comment was deleted
          <0  SQL error code
*/
Temp_idComment        number;
Temp_sComment        varchar(4000);
Temp            number;

begin

  /* Right-trim any whitespace */
  Temp_sComment := RTRIM (IN_sComment);

  /* Check to see if this comment already exists     */
  select idComment into Temp_idComment from Comments
    where idComment = IN_idComment;

  /* If this is a zero-length comment, we'll delete the comment instead and
   * return 0 for the id
   */
  if (Temp_sComment IS NULL) THEN
    Delete Comments where idComment = Temp_idComment;
    OUT_idComment := 0;
  else
    /* Otherwise, update the comment */
    OUT_idComment := Temp_idComment;

    update Comments
      set Comments.sComment = Temp_sComment
      where idComment = Temp_idComment;
  end if;


EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /* This is a new comment; insert it */
    select CommentsSeq.NEXTVAL into Temp_idComment from sys.dual;

    insert into Comments 
      (idComment, sComment)
      values (Temp_idComment, Temp_sComment);

    OUT_idComment := Temp_idComment;

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Or_Update_Comment',IN_idComment,Temp);
    OUT_idComment := -1;

end;
