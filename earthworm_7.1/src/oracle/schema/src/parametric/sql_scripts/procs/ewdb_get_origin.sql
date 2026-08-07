/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
       
CREATE OR REPLACE PROCEDURE Get_Origin 
(OUT_Retcode       out number,
 IN_idOrigin       in  number,
 OUT_xidExternal   out varchar,
 OUT_sSource       out varchar,
 OUT_sRealSource   out varchar,
 OUT_tOrigin       out number,
 OUT_dLat          out number,
 OUT_dLon          out number,
 OUT_dDepth        out number,
 OUT_iGap          out number,
 OUT_dDmin         out number,
 OUT_dRms          out number,
 OUT_iAssocRd      out number,
 OUT_iAssocPh      out number,
 OUT_iUsedRd       out number,
 OUT_iUsedPh       out number,
 OUT_iE0Azm        out number,
 OUT_iE0Dip        out number,
 OUT_dE0           out number,
 OUT_iE1Azm        out number,
 OUT_iE1Dip        out number,
 OUT_dE1           out number,
 OUT_iE2Azm        out number,
 OUT_iE2Dip        out number,
 OUT_dE2           out number,
 OUT_dErLat        out number,
 OUT_dErLon        out number,
 OUT_dErZ          out number, 
 OUT_tMCI          out number,
 OUT_iFixedDepth   out number,
 OUT_Comment       out varchar
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
  select xidExternal,idSource,tOrigin,dLat,dLon,dDepth,
         iGap,dDmin,dRms,iAssocRd,iAssocPh,iUsedRd,iUsedPh,
         iE0Azm,iE0Dip,dE0,
         iE1Azm,iE1Dip,dE1,
         iE2Azm,iE2Dip,dE2,
         dErLat,dErLon,dErZ,tMCI,
         iFixedDepth,idComment 
  into   OUT_xidExternal,Temp_idSource,OUT_tOrigin,OUT_dLat,OUT_dLon,OUT_dDepth,
         OUT_iGap,OUT_dDmin,OUT_dRms,
         OUT_iAssocRd,OUT_iAssocPh, OUT_iUsedRd,OUT_iUsedPh,OUT_iE0Azm,OUT_iE0Dip,
         OUT_dE0, OUT_iE1Azm,OUT_iE1Dip,OUT_dE1,OUT_iE2Azm,OUT_iE2Dip,OUT_dE2,
         OUT_dErLat,OUT_dErLon,OUT_dErZ,OUT_tMCI,
         OUT_iFixedDepth,Temp_idComment 
  from Origin 
  where idOrigin=IN_idOrigin;

  State := 1;
    
  if Temp_idComment = 0 then
    OUT_Comment := '';
  else
    select sComment into OUT_Comment from Comments where idComment=Temp_idComment;
  END IF;

  State := 2;

  if Temp_idSource IS NULL then
    OUT_sSource := '';
    OUT_sRealSource := '';
  else
    select sHumanReadable, sSource into OUT_sSource, OUT_sRealSource from Source where idSource=Temp_idSource;
  end if;

  OUT_Retcode := 0;
EXCEPTION
  WHEN NO_DATA_FOUND THEN
    if State = 0 then
      OUT_Retcode := -2;  /* invalid originid */
    elsif State = 1 then
      OUT_Comment := '';
      State := 2;

      if Temp_idSource IS NULL then
        OUT_sSource := '';
    	OUT_sRealSource := '';
      else
    	select sHumanReadable, sSource into OUT_sSource, OUT_sRealSource from Source where idSource=Temp_idSource;
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
    insert into test values('GetOrigin_ex',State,Temp);
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
    insert into test values('GetOrigin_ex2',State,Temp);
    OUT_Retcode := -1;
END;
     
   

