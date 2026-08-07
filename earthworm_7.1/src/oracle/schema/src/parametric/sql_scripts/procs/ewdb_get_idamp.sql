

CREATE OR REPLACE FUNCTION Get_idAmp
(OUT_idPeakAmp out number,
 IN_sSource varchar,
 IN_sSourceAmpID varchar
)
  RETURN NUMBER

as
/* 

   Return Codes:
	    0    SUCCESS
	   -1    ERROR:  see debugging table for details
		 -2    ERROR:  error looking up idSource from given source
		 -3    ERROR:  Pick with matching source/sourcepickID not found

	 
*/

Temp_idSource    Number;
Temp_idPeakAmp   Number;
Temp             Number;

begin

    /**********************************/
    /* Get the idSource               */
    /**********************************/
    Get_idSource(Temp_idSource,IN_sSource);
    if Temp_idSource < 1 then
      if(Temp_idSource = -3) then
        Temp_idSource := NULL;
      else
        return(-2);
      end if;
    end if;

    /*****************************************************************/
    /* See if we already know about this source/sourceid combination */
    /*****************************************************************/

    select idPeakAmp into Temp_idPeakAmp from PeakAmp
        where idSource = Temp_idSource
          and xidExternal = IN_sSourceAmpID;


    OUT_idPeakAmp := Temp_idPeakAmp;
		return(0);

  /**********************************/
  /* End of Main Procedure.         */
  /**********************************/
EXCEPTION
    WHEN NO_DATA_FOUND THEN

		  return(-3);

    WHEN OTHERS THEN
      Temp:=SQLCODE;
      insert into test values ('GetidPeakAmp' || IN_sSource , Temp, Temp_idPeakAmp);
      return(-1);

end;
