/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE Update_Comment_Next
(OUT_iErrCode out number,
 IN_idComment number,
 IN_idNextComment number
)
as
/* Return Codes for OUT_idComment:
				     0	All is OK
			 		  -1  Unknown error
            -2  Can't find comment ID in DB
                 
*/
Temp_idComment				number;
Temp						number;

begin

	/* Check to see if this comment already exists */
	select idComment into Temp_idComment from Comments
		where idComment = IN_idComment;

	update Comments
		set Comments.idNextComment = IN_idNextComment
		where idComment = Temp_idComment;

	OUT_iErrCode := 0;

EXCEPTION
	WHEN NO_DATA_FOUND THEN
		OUT_iErrCode := -2;

	WHEN OTHERS THEN
		Temp := SQLCODE;
		insert into test values('UpdateCommentNext',0,Temp);
		OUT_iErrCode := -1;
end;
