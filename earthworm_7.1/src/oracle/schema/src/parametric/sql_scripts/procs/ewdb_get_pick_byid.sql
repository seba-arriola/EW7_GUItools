/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
       
CREATE OR REPLACE PROCEDURE Get_PickById 
(OUT_Retcode       out number,
 IN_idPick         in  number,
 OUT_xidExternal   out varchar,
 OUT_idChan        out number,
 OUT_sPhase        out varchar,
 OUT_tPhase        out number,
 OUT_cMotion       out varchar,
 OUT_cOnset        out varchar,
 OUT_dSigma        out number,
 OUT_sSource       out varchar
)
as


/*******************************
     Return Codes:
         0:       Success
        -1:       Unknown Error
        -2:       Invalid idOrigin value
        -4:       Unknown State Error
         1:       Warning, idSource is Corrupt in Origin record
********************************/

Temp_idComment number;
Temp_idSource  number;
State          number :=0;
Temp           number;

begin

begin
  select xidExternal,idSource,sPhase,tPhase,cMotion,cOnset,idChan,dSigma
  into   OUT_xidExternal,Temp_idSource,OUT_sPhase,OUT_tPhase,OUT_cMotion,
         OUT_cOnset,OUT_idChan,OUT_dSigma
  from Pick 
  where idPick=IN_idPick;

  State := 1;
    
  if Temp_idSource IS NULL then
    OUT_sSource := '';
  else
    select sHumanReadable into OUT_sSource from Source where idSource=Temp_idSource;
  end if;
  
  State := 2;

  OUT_Retcode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_Retcode := -2;  /* invalid pickid */
    elsif State = 1 then
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
    insert into test values('GetPick_ex',State,Temp);
    OUT_Retcode := -1;
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 1 then
      OUT_Retcode :=  1;  /* warning corrupt sourceid in the origin table! */
      OUT_sSource := '';
    end if;
  WHEN OTHERS THEN
    Temp := SQLCODE;
    insert into test values('GetPick_ex2',State,Temp);
    OUT_Retcode := -1;
END;
/     
   
show errors
