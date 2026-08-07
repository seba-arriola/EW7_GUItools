/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Pick
(OUT_idPick out number,
 IN_idChan number,
 IN_sSource varchar := NULL,
 IN_sSourcePickID varchar := NULL,
 IN_sPhase varchar,
 IN_tPhase number,
 IN_cMotion char,
 IN_cOnset char,
 IN_dSigma number
)
as


  /* Return Codes:
       -2       ERROR:  Error during Get_idSource() (retrieving the id of the Source)
       -4       ERROR:  Error during Get_idPick() (checking for matching pick)
       -5       ERROR:  Couldn't insert pick because it was a duplicate, but couldn't
                         find the duplicate.  (see debug table)
       >0       SUCCESS:  id of the new or matching Pick
  **********************************************/

Temp_ID          number;
Temp_idPick      number :=0;
Temp_idSource    number;
begin


  /**********************************/
  /* Check for a Matching Pick      */
  /**********************************/
  Temp_ID := Get_idPick(Temp_idPick, IN_sSource, IN_sSourcePickID);
  if(Temp_ID = 0) then
    /* pick already exists */
    OUT_idPick := Temp_idPick;
    return;
  elsif(Temp_ID != -3) then
    /* error during get idPick */
    OUT_idPick := -4;
    return;
  end if;

  /* Get_idPick returned -3 indicating that a matching pick was not found */

  /**********************************/
  /* Get the idSource               */
  /**********************************/
  Get_idSource(Temp_idSource,IN_sSource);
  if Temp_idSource < 1 then
    if(Temp_idSource = -3) then
      Temp_idSource := NULL;
    else
     OUT_idPick := -2;
     return;
    end if;
  end if;

  /**********************************/
  /* Get A New PickID.              */
  /**********************************/
  select PickSeq.NEXTVAL into Temp_idPick from sys.dual;

  Create_Core_idKey(Temp_idPick);
  if Temp_idPick <= 0 then
    OUT_idPick := Temp_idPick;
  end if;

  /**********************************/
  /* Insert new Pick Record         */
  /**********************************/
  insert into Pick(idPick, sPhase, tPhase, idChan, tiExternal,
                   idSource, xidExternal, cMotion, cOnset, dSigma)
    values(Temp_idPick, IN_sPhase, IN_tPhase, IN_idChan, NULL,
           Temp_idSource, IN_sSourcePickID, IN_cMotion, IN_cOnset, IN_dSigma);


  /**********************************/
  /* Set the idPick return value   */
  /**********************************/

  OUT_idPick := Temp_idPick;


EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    Temp_ID := Get_idPick(Temp_idPick, IN_sSource, IN_sSourcePickID);
    if(Temp_ID != 0) THEN
      insert into test values('Create_Pick_DVOI ' || IN_sSourcePickID, Temp_idSource, Temp_ID);
      OUT_idPick := -5;
    end if;

  WHEN OTHERS THEN
    /*  ??? */
    Temp_ID := SQLCODE;
    insert into test values('Create_Pick_ex', Temp_idPick, Temp_ID);
	  OUT_idPick := -1;
END;
