/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Source
(OUT_RetCode out number,
 OUT_idSource out number,
 IN_sSource in varchar,
 IN_sHumanReadable varchar,
 IN_idInst number,  /* tried first */
 IN_iEWInstID number,  /* used if IN_idInst is invalid */
 IN_iSourceType number,
 IN_sNote varchar,
 IN_sComment varchar
)
as
/* Return Codes for OUT_RetCode:
           0  Success  (OUT_idSource contains the idSource of the new record)
		   1  Warning:  Existing Source record with duplicate attributes already exists.
          -1  Unknown Error
		  -2  Bad Installation ID:  (IN_idInst or IN_iEWInstID was invalid)
          -8  Key Constraint problem in table Source.  SERIOUS!!!!!
		 -1X  Error during Get_idInst_From_EWInstID().
		      X indicates the error returned by Get_idInst_From_EWInstID()
		 -2X  Error during creation of idInst.
		      X indicates the error returned by Create_Core_idKey
				  Others:  Undefined
*/
Temp_idSource Number;
Temp_idInst   Number;
Temp          Number;
begin
  begin

    /**********************************/
    /* Ensure we have a valid idInst. */
    /**********************************/
    if(IN_idInst > 0) then
      Temp_idInst := IN_idInst;
    else
      Temp := Get_idInst_From_EWInstID(Temp_idInst, IN_iEWInstID);
	  if(Temp != 0) then
	    OUT_RetCode := -10 + Temp;
	    return;
	  end if;
    end if;


    /**********************************/
    /* Get A New SourceID.             */
    /**********************************/

    select SourceSeq.NEXTVAL into Temp_idSource from sys.dual;

    Create_Core_idKey(Temp_idSource);
    if Temp_idSource <= 0 then
      OUT_RetCode := -20 + Temp_idSource;
    end if;


    /**********************************/
    /* Create  A New Source record.   */
    /**********************************/
    insert into Source(idSource,sSource,sHumanReadable,idInst,iSourceType,sNote,idComment)
     values(Temp_idSource,IN_sSource,IN_sHumanReadable,Temp_idInst,IN_iSourceType,IN_sNote,NULL);
  
    OUT_Retcode  := 0;
    OUT_idSource := Temp_idSource;

  EXCEPTION
    WHEN DUP_VAL_ON_INDEX THEN
      select idSource into Temp_idSource from Source
       where sSource = IN_sSource
	     and idInst  = Temp_idInst;

      OUT_idSource := Temp_idSource;
	  OUT_RetCode  := 1;
    WHEN OTHERS THEN
	  Temp := SQLCODE;
	  if(Temp = -2291) then
	    /* foreign key problem, must be a bad idInst */
	    insert into test values('Create_Source_FK' || IN_sSource, Temp_idInst, IN_iEWInstID);
	    OUT_RetCode := -2;
	  else
	    insert into test values('Create_Source_EXC ' || IN_sSource, Temp, Temp_idSource);
	    OUT_RetCode := -1;
	  end if;
  END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_RetCode := -8;
  WHEN OTHERS THEN
    OUT_RetCode := -1;
	Temp := SQLCODE;
	insert into test values('Create_Source_EXC2 ' || IN_sSource, Temp, Temp_idSource);
END;

