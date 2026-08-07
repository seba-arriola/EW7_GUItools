/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_PeakAmp
(OUT_idPeakAmp 		out number,
 IN_idChan 			in	number,
 IN_dPeakAmp1 		in	number,
 IN_tAmp1 			in	number,
 IN_tPeriod1 		in	number,
 IN_dPeakAmp2 		in	number,
 IN_tAmp2 			in	number,
 IN_tPeriod2 		in	number,
 IN_iMagType 		in	number,
 IN_sSource 		in	varchar,
 IN_sSourceAmpID 	in 	varchar,
 IN_idPick 			in	number,
 IN_tStartInterval 	in 	number,
 IN_tEndInterval 	in	number
)
as
/* Return Codes for OUT_idPeakAmp:
                  >0  DB idEvent
                  -1  Unknown Error

				  Others:  Undefined
*/
State            number;
Temp_idPeakAmp   number;
Temp_idSource    number;
Temp_ID          number;
Temp_idPick      number;

begin

  /**********************************/
  /* Check for an existing ampID    */
  /**********************************/
  State := 1;


  /**********************************/
  /* Handle idPick = 0 for NULL Pick*/
  /**********************************/
  State := 2;
  if IN_idPick = 0 then
    Temp_idPick := NULL;
  else 
    Temp_idPick := IN_idPick;
  end if;

  /**********************************/
  /* Get source information         */
  /**********************************/
  State := 3;
  Get_idSource (Temp_idSource,IN_sSource);

  if Temp_idSource < 1 then
    if(Temp_idSource = -3) then
      Temp_idSource := NULL;
    else
     OUT_idPeakAmp := -2;
     return;
    end if;
  end if;


  /**********************************/
  /* Get A New PeakAmpID.           */
  /**********************************/
  State := 4;
  select PeakAmpSeq.NEXTVAL into Temp_idPeakAmp from sys.dual;

  State := 5;
  Create_Core_idKey(Temp_idPeakAmp);
  if Temp_idPeakAmp <= 0 then
    OUT_idPeakAmp := Temp_idPeakAmp;
  end if;

  /**********************************/
  /* Insert new PeakAmp Record         */
  /**********************************/
  State := 6;

	insert into PeakAmp(idPeakAmp,idChan,idPick,dPeakAmp1,tAmp1,tPeriod1,
                      dPeakAmp2,tAmp2,tPeriod2,iMagType,idSource,xidExternal,
						tStartInterval, tEndInterval)
    values(Temp_idPeakAmp,IN_idChan,Temp_idPick,IN_dPeakAmp1,IN_tAmp1,IN_tPeriod1,
           IN_dPeakAmp2,IN_tAmp2,IN_tPeriod2,IN_iMagType,
           Temp_idSource,IN_sSourceAmpID,IN_tStartInterval,IN_tEndInterval);


  /**********************************/
  /* Set the idPeakAmp return value   */
  /**********************************/
  State := 7;

  OUT_idPeakAmp := Temp_idPeakAmp;

EXCEPTION
  WHEN OTHERS THEN
    Temp_ID := SQLCODE;
    insert into test values('ECPA_Ex  ' || IN_iMagType, IN_idChan, Temp_ID);
    insert into test values('ECPA_Ex2 ' || IN_iMagType, IN_idChan, State);
    
    /*  ??? */
	  OUT_idPeakAmp := -1;
END;
