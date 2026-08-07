/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE update_origin_by_xidExternal
(OUT_idOrigin out number,
 IN_sExternal_Table_Name varchar,
 IN_xidExternal varchar,
 IN_sSource varchar,
 IN_tOrigin number,
 IN_dLat number,
 IN_dLon number,
 IN_dDepth number,
 IN_iGap number,
 IN_dDmin number,
 IN_dRms number,
 IN_iAssocRd number,
 IN_iAssocPh number,
 IN_iUsedRd number,
 IN_iUsedPh number,
 IN_iE0Azm number,
 IN_iE0Dip number,
 IN_iE1Azm number,
 IN_iE1Dip number,
 IN_iE2Azm number,
 IN_iE2Dip number,
 IN_dE0 number,
 IN_dE1 number,
 IN_dE2 number,
 IN_dErLat number,
 IN_dErLon number,
 IN_dErZ number,
 IN_sComment varchar,
 IN_tMCI number,
 IN_iFixedDepth number
)
as
/* Return Codes for OUT_idOrigin:
          >0  DB idOrigin
          -1  Unknown Error
          -2  sExternal_Table_Name not found in EWDB_Tablelist
				  -3  Origin record identified by tiExternal/xidExternal not found
				  -4  Unknown NO_DATA_FOUND Exception
				  -11 Create_Comment failed for IN_sComment
          -12 Get_idSource() Function Failed for IN_sSource
          -3X Error in Check_External_Record_Validity(), X describes
               the error returned Check_External_Record_Validity()
				  Others:  Undefined
*/
Temp_idSource    Number;
Temp_CommentID   Number;
Temp_tiExternal  Number;
Temp_idOrigin    Number;
State            Number;
Temp             number;


  begin

    State := 0;

    /**********************************/
    /* Deal with Comment String       */
    /**********************************/
    if IN_sComment IS NULL then
      Temp_CommentID := 0;
    else
      Create_Comment(Temp_CommentID,IN_sComment);
      if Temp_CommentID < 1 then
        OUT_idOrigin := -11;
        return;
      end if;
    end if;

    State := 1;

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      OUT_idOrigin := -12;
      return;
    end if;

    State := 2;

    /**********************************/
    /* Check for Valid External Record*/
    /* Or NULL Link                   */
    /**********************************/
    Check_External_Record_Validity(Temp_tiExternal,IN_xidExternal,
        IN_sExternal_Table_Name);

    State := 3;

    /**************************************
    /* 0 means something was NULL, so we will
    /*   assume(oops!) that there is no external
    /*   record to connect with!
    **************************************/
    if Temp_tiExternal < 0 then
      OUT_idOrigin := -30 + Temp_tiExternal;
      return;
    end if;

    State := 4;

    if Temp_tiExternal = 0 then
      select idTable into Temp_tiExternal from EWDB_Tablelist
        where sTableName = IN_sExternal_Table_Name;
    end if;

    /************************************/
    /* OK, we now have a valid Source, 
    /*  a valid SourceOrigin, and we are
    /*  ready to update an origin.
    /************************************/

    State := 5;

    /**********************************/
    /* Lookup the idOrigin by tiExternal/xidExternal
    /**********************************/
    select idOrigin into Temp_idOrigin from Origin 
      where xidExternal = IN_xidExternal
        and tiExternal  = Temp_tiExternal;

    State := 6;

    update Origin 
      set tOrigin = IN_tOrigin, dLat = IN_dLat, dLon = IN_dLon, 
        dDepth = IN_dDepth, iGap = IN_iGap, dDMin = IN_dDMin, 
        dRms = IN_dRms, iAssocRd = IN_iAssocRd, iAssocPh = IN_iAssocPh, 
        iUsedRd = IN_iUsedRd, iUsedPh = IN_iUsedPh, 
        iE0Azm = IN_iE0Azm, iE0Dip =IN_iE0Dip, dE0 = IN_dE0,
        iE1Azm = IN_iE1Azm, iE1Dip =IN_iE1Dip, dE1 = IN_dE1,
        iE2Azm = IN_iE2Azm, iE2Dip =IN_iE2Dip, dE2 = IN_dE2,
        dErLat = IN_dErLat, dErLon = IN_dErLon,dErz = IN_dErz, 
		idComment = Temp_CommentID,
		tMCI = IN_tMCI, iFixedDepth=IN_iFixedDepth
      where idOrigin = Temp_idOrigin;


    State := 7;
   
    /**********************************/
    /* Set OUT_idOrigin to the idOrigin
    /* of the record we just updated.
    /**********************************/
    OUT_idOrigin := Temp_idOrigin;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/

  /**********************************/
  EXCEPTION
    WHEN NO_DATA_FOUND THEN
      if State = 4 then
        OUT_idOrigin := -2;  /* External TableName not found in EWDB_Tablelist */
      elsif State = 5 then
        OUT_idOrigin := -3;  /* Record with given tiExternal/xidExternal 
                                 not found in Origin table */
      else 
        OUT_idOrigin := -4;   /* Unknown NO_DATA_FOUND error */
      end if;
    WHEN OTHERS THEN
      Temp := SQLCODE;
      insert into test values('up_or_by_xid',Temp,State);
      OUT_idOrigin := -1;
  End;
