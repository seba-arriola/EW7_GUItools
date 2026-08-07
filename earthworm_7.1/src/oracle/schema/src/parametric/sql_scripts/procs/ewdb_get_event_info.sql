/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Event_Info 
(OUT_Retcode       out number,
 IN_idEvent        in  number,
 OUT_tiEventType   out number,
 OUT_iDubiocity    out number,
 OUT_bArchived     out number,
 OUT_sComment      out varchar,
 OUT_idComment	   out number,
 OUT_idInternalComment  out number,
 OUT_idContribComment   out number
)
as


/*******************************
     Return Codes:
         0:       Success
        -1:       Unknown Error
        -2:       Invalid idEvent value
        -4:       Unknown State Error
********************************/

State          number :=0;
Temp           number;


begin
  select tiEventType,iDubiocity,bArchived,idComment,idInternalComment,idContribMagComment
  into   OUT_tiEventType,OUT_iDubiocity,OUT_bArchived,OUT_idComment,OUT_idInternalComment,
			OUT_idContribComment
  from Event 
  where idEvent = IN_idEvent;

  State := 1;
    
  if OUT_idComment = 0 then
    OUT_sComment := '';
  else
    select sComment into OUT_sComment from Comments where idComment=OUT_idComment;
  end if;

  State := 2;

  OUT_Retcode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_Retcode := -2;  /* invalid idEvent */
    elsif State = 1 then
      OUT_sComment := '';
      OUT_Retcode := 0;

    else
      OUT_Retcode := -4;  /* unknown internal state error */
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Get_Event_Info_ex',State,Temp);
    OUT_Retcode := -1;
END;

     
