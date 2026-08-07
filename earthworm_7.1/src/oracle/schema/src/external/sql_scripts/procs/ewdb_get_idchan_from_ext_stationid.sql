/*                                                          *
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE *
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.             *
 *
 *    $Id: ewdb_get_idchan_from_ext_stationid.sql,v 1.4 2004/03/17 17:59:16 davidk Exp $
 *
 *    Revision history:
 *     $Log: ewdb_get_idchan_from_ext_stationid.sql,v $
 *     Revision 1.4  2004/03/17 17:59:16  davidk
 *     Fixed various bugs while testing station_maintenance tool suite.
 *
 *     Revision 1.2  2003/12/13 01:56:36  davidk
 *     Rewrote a large chunk of the procedure.
 *     Fixed a problem where an SCNL and Channel would exist in the database,
 *     but had not been entered via the external mechanism, and so did
 *     not exist in Station_External.
 *     When the SCNL was inserted into Station_External, it caused a new
 *     Chan record to be created, instead of discovering the exsting one.
 *     The changed code now discovers the existing associated idchan.
 *
 *
 *
 ************************************************************/
CREATE OR REPLACE PROCEDURE Get_idChan_From_Ext_StationID
(OUT_idChan out number,
 IN_StationID number,
 IN_bUseParamsInsteadOfTable number := 0,
 IN_sSta varchar := NULL,
 IN_sComp varchar := NULL,
 IN_sNet  varchar := NULL,
 IN_sLoc  varchar := NULL
)
as
/* 
  Get_idChan_From_Ext_StationID()
  Return Codes for OUT_idChan:
      >0  DB idChan
      -1  Unknown Error
      -2  Create_Chan() failed!
      -3  IN_StationID not found in Station table!
      -4  CRITICAL index problem
          Others:  Undefined
*/
Temp_idChan   number;
Temp_sComment varchar(100);
Temp          number;
State         number;
Temp_sSta     varchar(10);
Temp_sComp    varchar(10);
Temp_sNet     varchar(10);
Temp_sLoc     varchar(10);

Temp_StationID number;
begin

  State := 0;
  /**************************************/
  /* Check for Station_External_2_idChan Record  */
  /**************************************/

  select count(idChan) into Temp from Station_2_idChan_External
   where StationID = IN_StationID;

  if(Temp = 0) then
    /* A matching Station_2_idChan_External record doesn't exist. */

    if (IN_bUseParamsInsteadOfTable = 1) then
      Temp_sSta := IN_sSta;
      Temp_sComp := IN_sComp;
      Temp_sNet := IN_sNet;
      Temp_sLoc := IN_sLoc;
    else
      State := 1;

      /* First, check to see that the StationID is valid. */
      Temp_StationID := Get_External_StationID(IN_sSta,IN_sComp,IN_sNet,IN_sLoc);

      if(Temp_StationID != IN_StationID) then
        /* we have a problem, the stationID's don't match so 
           the original must be invalid */
        /* StationID is bogus.  Return Error! */
        OUT_idChan := -3;
        return;
      end if;

      State := 2;

      select sta, chan, net, loc 
        into Temp_sSta, Temp_sComp, Temp_sNet, Temp_sLoc
       from Station_External
       where StationID = IN_StationID;

    end if;

    State := 3;

    /* DK 021704  Adding support / fixing bug for Location code*/
    select count(idChan) into Temp from ALL_STATION_INFO 
     where sSta  = Temp_sSta
       AND sComp = Temp_sComp
       AND sNet  = Temp_sNet
       AND (sLoc = Temp_sLoc OR (sLoc IS NULL and Temp_sLoc IS NULL));

    if Temp = 0 then
      State := 4;
      /* No Channel exists for the given Station External.  Create one */
      Temp_sComment := 
         'Created By Get_idChan_From_Ext_StationID() for StationID'
         || IN_StationID;
  
      State := 5;

      Create_Chan(Temp_idChan,Temp_sComment);
      if Temp_idChan <= 0 then
        OUT_idChan := -2;
        return;
      end if;
    else
      /* atleast one matching record already exists.  Retrieve the idChan */

      State := 7;

    /* DK 021704  Adding support / fixing bug for Location code*/
      select min(idChan) into Temp_idChan from ALL_STATION_INFO 
       where sSta  = Temp_sSta
         AND sComp = Temp_sComp
         AND sNet  = Temp_sNet
         AND (sLoc = Temp_sLoc OR (sLoc IS NULL and Temp_sLoc IS NULL));
    end if;

    State := 8;

    insert into Station_2_idChan_External(StationID,idChan)
     values (IN_StationID,Temp_idChan);
  else
    /* a Station_2_idChan_External already exists. */
      State := 9;
    /* grab the idchan from the matching Station_2_idChan_External record. */
    select idChan into Temp_idChan from Station_2_idChan_External
     where StationID = IN_StationID;
  end if;

  OUT_idChan := Temp_idChan;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    if State = 6 then
      /*  Some crudsucker beat us to the punch!  */
      /*  grab their idChan, and forget about ours */
      select count(idChan) into Temp from Station_2_idChan_External
       where StationID = IN_StationID;

      if(Temp = 0) then
        /* we've been hornswaggled.  something's screwed up. log
           error information and go home. */
        insert into test values('get_idchan_f_stax_xxxx',IN_StationID,State);
        OUT_idChan := -4;
      else
        select min(idChan) into OUT_idChan from Station_2_idChan_External
         where StationID = IN_StationID;
      end if;
    else
      insert into test values('get_idchan_f_stax_dvoi',IN_StationID,State);
      OUT_idChan := -1;
    end if;


  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('get_idchan_f_stax_ex',IN_StationID,Temp);
    OUT_idChan := -1;
END;

