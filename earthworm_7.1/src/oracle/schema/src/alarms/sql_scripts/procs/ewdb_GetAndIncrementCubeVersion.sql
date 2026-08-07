/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */

CREATE OR REPLACE PROCEDURE GetAndIncrementCubeVersion
(OUT_iVersionNumber out number,
 IN_idEvent number
)
as
/* Return Codes for OUT_iVersionNumber:
                  >0  new Version Number 
                  -1  Unknown Error
*/
Temp_iVersionNumber			number;
Temp                    number;

begin

  /**************************************************/
  /* Check to see if this audit already exists      */
  /**************************************************/
	select iVersionNumber into Temp_iVersionNumber from CubeVersionNumber
		where idEvent = IN_idEvent;

	/* 
		If so, return current value then increment it 
	 */
	OUT_iVersionNumber := Temp_iVersionNumber;

	Temp_iVersionNumber := Temp_iVersionNumber + 1;

	update CubeVersionNumber
		set CubeVersionNumber.iVersionNumber = Temp_iVersionNumber
			where idEvent = IN_idEvent;

EXCEPTION
	WHEN NO_DATA_FOUND THEN

		/* Insert a new cube version record */
		select CubeVersionSeq.NEXTVAL into Temp_iVersionNumber from sys.dual;

		/* Return 0 (current value), set to 1 (next value) */
		insert into CubeVersionNumber 
			(idVersion, idEvent, iVersionNumber)
			values (Temp_iVersionNumber, IN_idEvent, 1);

		OUT_iVersionNumber := 0;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('IncCubeVersion_EX', IN_idEvent, Temp);
    OUT_iVersionNumber := -1;
end;
