/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *                                                          *
 *                                                          *
 *    Revision history:                                     *
 *                                                          *
 *     $Log: ewdb_delete_smmessages_before_time.sql,v $
 *     Revision 1.1  2001/07/14 08:00:30  davidk
 *     Initial revision
 *     *
 *                                                          *
 ************************************************************/

CREATE OR REPLACE PROCEDURE Delete_SMMessages_Before_Time
(OUT_RetCode OUT number,
 IN_tTime      number,
 IN_bForce     number
)
as

Temp_tiCore  number;

Cursor SMMessage_cursor(Cursor_tTime number) is
  select * from SMMessage
    where tLoad < Cursor_tTime;

  Temp             number := 0;
  Temp_RetCode     number;
  State            number;

begin
  State := 1;

  for Curr in SMMessage_cursor(IN_tTime) loop
    Temp_RetCode := Delete_SMMessage(Curr.idSMMessage, IN_bForce);
    if Temp_RetCode < 0 then
      OUT_RetCode := Temp_RetCode;
      return;
    end if;
  end loop;

  OUT_RetCode := 0;

EXCEPTION
  WHEN OTHERS THEN
    insert into test values('Delete_SMMessages_Before_Time',IN_tTime,State);
	OUT_RetCode := -1;
END;

