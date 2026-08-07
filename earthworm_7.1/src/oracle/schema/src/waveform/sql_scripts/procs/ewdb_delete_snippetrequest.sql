/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_delete_snippetrequest.sql,v $
/*     Revision 1.1  2004/09/07 19:31:15  davidk
/*     Reaper v2 09/02/2004.
/*             */
/*                                                          */
/*                                                          */
/*                                                          */
/************************************************************/


CREATE OR REPLACE FUNCTION Delete_SnippetRequest
(IN_idSnipReq number) RETURN number
as


Temp                   number;
State                  number;


begin

  State := 1;
  Delete SnippetRetrievalSchedule where idSnipReq = IN_idSnipReq;

  State := 2;
  Delete SnippetRequest where idSnipReq = IN_idSnipReq;

  return(0);
EXCEPTION
  WHEN OTHERS THEN
    Temp := SQLCODE;
    if Temp = -2292 then
      insert into test values('Delete_SnipReq_FK ' || IN_idSnipReq,State,Temp);
      return(1);
    else
      insert into test values('Delete_SnippetRequest',IN_idSnipReq,Temp);
      return(-1);
    end if;
END;


