/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


CREATE OR REPLACE PROCEDURE Create_UhInfo
(OUT_idUHInfo OUT number,
 IN_idChanT varchar,
 IN_dNaturalFrequency varchar,
 IN_dDamping varchar,
 IN_dFullscale varchar,
 IN_dSensitivity varchar,
 IN_dAzm varchar,
 IN_dDip varchar,
 IN_iGain number,
 IN_iSensorType number
 )
 as

/*
	returns idUHinfo, or negative value in case of error
*/
Temp_idUHInfo   number;
Temp_idChanT   	number;
State   		number;

BEGIN

  /***********************************************/
  /* Check to see if this idChanT already exists  */
  /***********************************************/
	State := 1;
    select idChanT into Temp_idChanT from UH_Info_External
        where idChanT = IN_idChanT;

	State := 2;
	select idUHInfo into Temp_idUHInfo from UH_Info_External
        where idChanT = IN_idChanT;

    /*  Update the info */

	State := 3;
    update UH_Info_External
        set UH_Info_External.dNaturalFrequency = IN_dNaturalFrequency,
            UH_Info_External.dDamping = IN_dDamping,
            UH_Info_External.dFullscale = IN_dFullscale,
            UH_Info_External.dSensitivity = IN_dSensitivity,
            UH_Info_External.dAzm = IN_dAzm,
            UH_Info_External.dDip = IN_dDip,
            UH_Info_External.iGain = IN_iGain,
            UH_Info_External.iSensorType = IN_iSensorType
        where idChanT = Temp_idChanT;

    OUT_idUHInfo := Temp_idUHInfo;

EXCEPTION
    WHEN NO_DATA_FOUND THEN

	if  State = 1 then

	    /* Insert the new record */
		select UH_Info_ExternalSeq.NEXTVAL into Temp_idUHInfo from sys.dual;

	    Create_Core_idKey(Temp_idUHInfo);
		if Temp_idUHInfo <= 0 then
			OUT_idUHInfo := -100 + Temp_idUHInfo;
			return;
		end if;

		insert into UH_Info_External 
			(idUHInfo, idChanT, dNaturalFrequency, dDamping, dFullscale,
								dSensitivity, dAzm, dDip, iGain, iSensorType)
						values 
			(Temp_idUHInfo, IN_idChanT, IN_dNaturalFrequency, IN_dDamping,
					IN_dFullscale, IN_dSensitivity, 
					IN_dAzm, IN_dDip, IN_iGain, IN_iSensorType);

		OUT_idUHInfo := Temp_idUHInfo;
		return;
	end if;

	if State = 2 then
		OUT_idUHInfo := -1;
		return;
	end if;

	if State = 3 then
		OUT_idUHInfo := -2;
		return;
	end if;

END;
