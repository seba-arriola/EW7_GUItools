/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Get_Magnitude 
(OUT_Retcode       out number,
 IN_idMag          in  number,
 OUT_sSource       out varchar,
 OUT_dMagAvg       out number,
 OUT_iNumMags      out number,
 OUT_dMagErr       out number,
 OUT_iMagType      out number,
 OUT_idOrigin      out number
)
as


/*******************************
     Return Codes:
         0:       Success
        -1:       Unknown Error
        -2:       Invalid idOrigin value
        -4:       Unknown State Error
         1:       Warning, idSource is Corrupt in Magnitude record
********************************/

Temp_idSource  number;
State          number :=0;
Temp           number;

begin

begin
  select idSource,dMagAvg,
         iNumMags,dMagErr,iMagType,idOrigin
  into   Temp_idSource,OUT_dMagAvg,
         OUT_iNumMags,OUT_dMagErr,OUT_iMagType,OUT_idOrigin
  from Magnitude
  where idMag=IN_idMag;

  State := 1;

  State := 2;

  if Temp_idSource IS NULL then
    OUT_sSource := '';
  else
    select sHumanReadable into OUT_sSource from Source where idSource=Temp_idSource;
  end if;


  OUT_Retcode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_Retcode := -2;  /* invalid originid */
    elsif State = 1 then
      OUT_Retcode := 0;

      State := 2;
      if Temp_idSource IS NULL then
        OUT_sSource := '';
      else
        select sHumanReadable into OUT_sSource from Source where idSource=Temp_idSource;
      end if;
      OUT_Retcode := 0;

    elsif State = 2 then
      OUT_Retcode :=  1;  /* warning corrupt sourceid in the origin table! */
      OUT_sSource := '';

    else
      OUT_Retcode := -4;  /* unknown internal state error */
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('GetMagnitude_ex',State,Temp);
    OUT_Retcode := -1;
END;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 2 then
      OUT_Retcode :=  1;  /* warning corrupt sourceid in the origin table! */
      OUT_sSource := '';
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('GetMagnitude_ex2',State,Temp);
    OUT_Retcode := -1;
END;



