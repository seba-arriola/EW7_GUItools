/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Origin
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
 IN_bBindToEvent number,
 IN_idEvent in number,
 IN_bSetPrefer number,
 IN_sComment varchar,
 IN_tMCI number,
 IN_iFixedDepth number,
 IN_iVersionNum number := NULL,
 IN_sSourceEventID varchar := NULL
)
as
/* Return Codes for OUT_idOrigin:
          >0  DB idOrigin
          -1  Unknown Exception
				  -4  NO_DATA_FOUND Exception
	 	     -11  Create_Comment failed for IN_sComment
         -14  IN_bBindToEvent set, and IN_idEvent invalid!
         -2X  Error in Create_Bind(), X describes the error returned
                by Create_Bind()
        -1XX  Error in Get_idOrigin_From_Source_Data(), XX describes the 
                error returned by Get_idOrigin_From_Source_Data()
        -2XX  Error in Set_Prefer(), XX describes the error returned
                by Set_Prefer()
	    Others: Undefined
*/

Temp_CommentID   Number;
Temp_tiExternal  Number := 0;  /* ignore sExternalTableName */
Temp_idOrigin    Number;
Temp_idBind      Number;
Temp_idEvent     Number := IN_idEvent;
Temp_idEvent2    Number;
Temp_idPrefer    Number;
Temp             Number;
State            Number;
Temp_RetCode     Number;

begin

  State := 1;

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


  State := 2;


    /**********************************/
    /* Get the idOrigin for our       *
     * Source, SourceEventID, and     *
     * Origin VersionNumber           */
    /**********************************/
    Temp_RetCode := Get_idOrigin_From_Source_Data(Temp_idOrigin,
                                                  Temp_idEvent2,
                                                  IN_sSource,
                                                  IN_sSourceEventID,
                                                  IN_iVersionNum,
                                                  Temp_tiExternal,
                                                  IN_xidExternal);

    if Temp_RetCode < 0 then
      OUT_idOrigin := -100 + Temp_RetCode;
      return;
    end if;

  State := 3;


    /************************************/
    /* OK, we now have a valid Source, 
    /*  a valid SourceOrigin, and we are
    /*  ready to create an origin.
    /************************************/

    if IN_bBindToEvent = 1 then /* if we are binding to an event */
      /**********************************/
      /* Deal With Event Issues         */
      /**********************************/
      Check_idEvent_Validity(Temp_idEvent);
      if Temp_idEvent < 1 then
        OUT_idOrigin := -14;
        return;
      end if;
    end if;  /* if IN_bBindToEvent = 1 */


  State := 4;


    /**********************************/
    /* Update the Origin record       */
    /**********************************/
     update origin set
          tOrigin = IN_tOrigin, dLat = IN_dLat, dLon = IN_dLon, 
          dDepth = IN_dDepth, iGap = IN_iGap, dDmin = IN_dDmin, 
          dRms = IN_dRms, iAssocRd = IN_iAssocRd, iAssocPh = IN_iAssocPh, 
          iUsedRd = IN_iUsedRd, iUsedPh = IN_iUsedPh,
          iE0Azm = IN_iE0Azm, iE0Dip = IN_IE0Dip, dE0 = IN_dE0,
          iE1Azm = IN_iE1Azm, iE1Dip = IN_IE1Dip, dE1 = IN_dE1,
          iE2Azm = IN_iE2Azm, iE2Dip = IN_IE2Dip, dE2 = IN_dE2,
          dErLat = IN_dErLat, dErLon = IN_dErLon, dErz = IN_dErz,
          idComment = Temp_CommentID, tMCI = IN_tMCI, iFixedDepth = IN_iFixedDepth
      where idOrigin = Temp_idOrigin;


  State := 5;


    if IN_bBindToEvent = 1 then
      /**********************************/
      /* Bind our Origin to the Event   */
      /**********************************/
      Create_Bind(Temp_idBind,Temp_idEvent,'Origin',Temp_idOrigin);
      if Temp_idBind < 1 then
         OUT_idOrigin := -20 + Temp_idBind;
         return;
      end if;


      State := 6;

      if IN_bSetPrefer = 1 then
        /**********************************/
        /* Set our as Preferred for the Event   */
        /**********************************/
        Set_Prefer(Temp_idPrefer,Temp_idEvent,1/*Origin*/,Temp_idOrigin);
        if Temp_idPrefer < 1 then
           OUT_idOrigin := -200 + Temp_idPrefer;
           return;
        end if;
      end if;

    end if; /* IN_bBindToEvent */

    

    State := 7;


    /**********************************/
    /* Set OUT_idOrigin to the idOrigin
    /* of the record we just created.
    /**********************************/
    OUT_idOrigin := Temp_idOrigin;

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/

  /**********************************/
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_idOrigin := -4;
  WHEN OTHERS THEN
    Temp:=SQLCODE;
    insert into test values('Create_Origin_excep ' || IN_idEvent,Temp,State);
    OUT_idOrigin := -1;
End;
