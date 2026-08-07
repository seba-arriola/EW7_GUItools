/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Site
(
 OUT_iRetCode OUT number,
 OUT_idSite   OUT number,
 IN_sSta          varchar,
 IN_sNet          varchar,
 IN_sComment      varchar
)

/******************************
 *   Return codes:
 *    0  :  SUCCESS
 *    2  :  Site record already exists
 *   -1  :  Unknown Error, see Debug table for details
 *   -3  :  Unknown DVOI error, see Debug table for details
 *  -1X  :  Error during Create_Core_idKey() where X is the error
 ******************************/
as

Temp_idSite        number;
Temp_idComment     number;
State              number;
Temp               number;

begin

  State := 0;

  select count(idSite) into Temp from Site
    where ssta = IN_sSta 
	  and sNet = IN_sNet;

  State := 1;

  if(Temp = 1) then
    /* Site already exists */
    select idSite into OUT_idSite from Site 
      where ssta = IN_sSta 
	    and sNet = IN_sNet;
	OUT_iRetCode := 2;
	return;
  end if;

  
  State := 2;

  /**********************************/
  /* Deal with Comment Strings      */
  /**********************************/
  if IN_sComment IS NULL then
    Temp_idComment := NULL;
  else
    Create_Comment(Temp_idComment, IN_sComment);
  end if;

  State := 3;


  /**********************************/
  /* Get A New SiteID.              */
  /**********************************/
  select SiteSeq.NEXTVAL into Temp_idSite from sys.dual;
 
  Create_Core_idKey(Temp_idSite);
  if Temp_idSite <= 0 then
    OUT_idSite := -1;
    OUT_iRetCode := -10 + Temp_idSite;
  end if;

  State := 4;


  insert into Site(idSite,sSta,sNet,idComment)
    values(Temp_idSite,IN_sSta,IN_sNet,Temp_idComment);

  State := 5;

  OUT_idSite := Temp_idSite;
  OUT_iRetCode := 0;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    OUT_idSite   := -1;
    OUT_iRetCode := -3;  /* unknown DVOI Exception */

  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('Create_Site',Temp,State);
    insert into test values('Create_Site2 (' || IN_sSta || ',' 
                            || IN_sNet || ')',Temp,State);

    OUT_idSite   := -1;
    OUT_iRetCode := -1;
END;

