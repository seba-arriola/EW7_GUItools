/*                                                          */
/*   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE DeleteComment
(OUT_iRetCode OUT number,
 IN_idComment number
)
as

Temp  number;

begin

	delete Comments where idComment=IN_idComment;

	OUT_iRetCode := 0;


EXCEPTION
  WHEN OTHERS THEN

		Temp := SQLCODE;
		insert into test values('DeleteComment',IN_idComment,Temp);
		OUT_iRetCode := -1;
END;

