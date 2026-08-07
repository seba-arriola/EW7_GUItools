/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_idSource
(OUT_idSource out number,
 IN_sSource in varchar,
 IN_iEWInstID number := null,
 IN_idInst number:= null
)
as
/* Return Codes for OUT_idBind:
          >0  DB idSource
          -1  Unknown Error
		      -2  Error getting idInst from IN_iEWInstID
          -3  sSource is null
          -8  Index problem in table Source.  SERIOUS!!!!!
				  Others:  Undefined
*/

Temp_idSource Number;
Temp_idInst   Number;
State         Number;
Temp          Number;
begin

begin

begin

  /**********************************/
  /* Check for Source Record        */
  /**********************************/

  State := 0;

  if(IN_sSource is null) then
    OUT_idSource:= -3;
    return;
  end if;
  if(IN_idInst is not null and IN_idInst > 0) then
    Temp_idInst := IN_idInst;

    State := 1;

    select idSource into OUT_idSource from Source 
      where sSource = IN_sSource
	    and idInst = IN_idInst;

  elsif(IN_iEWInstID is not null and IN_iEWInstID > 0) then

    State := 2;

    Temp := Get_idInst_From_EWInstID(Temp_idInst, IN_iEWInstID);
	  if(Temp <= 0) then
	    OUT_idSource := -2;
	    return;
	  end if;

    State := 3;

    select idSource into OUT_idSource from Source 
      where sSource = IN_sSource
	    and idInst = Temp_idInst;

  else
   
    State := 4;

    Temp_idInst := null;

    select count(idSource) into Temp from Source
      where sSource = IN_sSource;

    State := 5;

	  if(Temp = 1) then
        State := 6;

        select idSource into OUT_idSource from Source 
          where sSource = IN_sSource;
	  elsif(Temp > 1) then
        State := 7;
        select idSource into OUT_idSource from Source 
          where sSource = IN_sSource
	      and idInst is null;
    else
      State := 8;
      /* we've never heard of this source */
      select idSource into OUT_idSource from Source 
        where sSource = IN_sSource
          and idInst is null;
	  end if;
  end if;

  State := 9;
    

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    /**********************************/
    /* Not a known source, make it known!!
    /**********************************/

    /**********************************/
    /* Get A New SourceID.             */
    /**********************************/
    select SourceSeq.NEXTVAL into Temp_idSource from sys.dual;

    Create_Core_idKey(Temp_idSource);
    if Temp_idSource <= 0 then
      OUT_idSource := Temp_idSource;
    end if;


	/**********************************/
    /* Create  A New Source record.   */
    /**********************************/
    insert into Source(idSource,idInst,sSource,sHumanReadable,iSourceType,sNote,idComment)
     values(Temp_idSource,Temp_idInst,IN_sSource,IN_sSource,-1/* iSourceType unknown */,NULL,NULL);
    
    OUT_idSource := Temp_idSource;

  WHEN OTHERS THEN
    Temp:= SQLCODE;
	insert into test values('Get_idSource ' || State,Temp,Temp_idInst);
    OUT_idSource := -1;
END;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    select idSource into Temp_idSource from Source
     where sSource = IN_sSource
	   and idInst  = Temp_idInst;

    OUT_idSource := Temp_idSource;

  WHEN OTHERS THEN
    Temp:= SQLCODE;
  	insert into test values('Get_idSource_EXC ' || State,Temp,Temp_idInst);
	  insert into test values('Get_idSource_EXC 2 (' || IN_sSource || ')' ,Temp_idSource, Temp_idInst);
    OUT_idSource := -1;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    Temp:= SQLCODE;
	insert into test values('Get_idSource_NDF',Temp,Temp_idInst);
    OUT_idSource := -8;
  WHEN OTHERS THEN
    Temp:= SQLCODE;
	insert into test values('Get_idSource_EXC2 ' || State,Temp,Temp_idInst);
    OUT_idSource := -1;
END;


