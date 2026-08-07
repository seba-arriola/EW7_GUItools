/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_CodaDur
(OUT_idCodaDur out number,
 IN_idTCoda number,
 IN_idPick number
)
as
/* Return Codes for OUT_idCodaDur:
                  >0  DB idEvent
                  -1  Unknown Error
                  -2  Incorrect Phase Type for Pick.  (Must be P-wave)
                  -6  Unknown DUP_VAL_ON_INDEX exception
                  -7  Index Problem on CodaDur table.  SERIOUS!!!!
                 -1X  Error from Check_Record_Validity(TCoda), 
                       X indicates error returned by C_R_V()
                 -2X  Error from Check_Record_Validity(Pick), 
                       X indicates error returned by C_R_V()
                 -3X  Error from Get_Phase_Info_From_Pick(), 
                       X indicates error returned by G_P_I_F_P()
                 -4X  Error from Get_Coda_Term_Times(), 
                       X indicates error returned by G_C_T_T()


				  Others:  Undefined
*/
Temp_ID            number;
State              number;
Temp_idCodaDur     number;
Temp_tCodaTermObs  number;
Temp_tCodaTermXtp  number;
Temp_tPhase        number;
Temp_sPhase        varchar(6);
Temp_tCodaDurObs   number;
Temp_tCodaDurXtp   number;
Temp_RetCode       number;

begin

begin

  /**********************************/
  /* Check for TCoda record validity 
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idTCoda,'TCoda');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idCodaDur := -10 + Temp_ID;
   return;
  end if;
    
  State := 1;

  /**********************************/
  /* Check for Pick record validity 
  /**********************************/

  Check_Record_Validity(Temp_ID,IN_idPick,'Pick');
  if not(Temp_ID > 0) then
   /* error, set return code and quit */
   OUT_idCodaDur := -20 + Temp_ID;
   return;
  end if;
    
  State := 2;


  /**********************************/
  /* Get tphase and sphase from pick
  /**********************************/
  Get_Phase_Info_From_Pick(Temp_RetCode,Temp_tPhase,Temp_sPhase,IN_idPick);
  if Temp_RetCode <= 0 then
    OUT_idCodaDur := -30 + Temp_RetCode;
    return;
  end if;
    
  State := 3;

  /**********************************/
  /* Get tObsTerm and tXtpTerm from TCoda
  /**********************************/

  Get_Coda_Term_Times(Temp_RetCode,Temp_tCodaTermObs,Temp_tCodaTermXtp,IN_idTCoda);
  if Temp_RetCode <= 0 then
    OUT_idCodaDur := -40 + Temp_RetCode;
    return;
  end if;

  State := 4;


  /**********************************/
  /* Check for proper phase type and do arithmetic
  /**********************************/

	/* Let the calling program do the checks of this nature LV 6/2001 */

/*
  if Temp_sPhase = 'P' then
    Temp_tCodaDurObs := Temp_tCodaTermObs - Temp_tPhase;
    Temp_tCodaDurXtp := Temp_tCodaTermXtp - Temp_tPhase;
  elsif Temp_sPhase = 'S' then
    Temp_tCodaDurObs := Temp_tCodaTermObs - Temp_tPhase;
    Temp_tCodaDurXtp := Temp_tCodaTermXtp - Temp_tPhase;
  else
    OUT_idCodaDur := -2;
    return;
  end if;
*/

  Temp_tCodaDurObs := Temp_tCodaTermObs - Temp_tPhase;
  Temp_tCodaDurXtp := Temp_tCodaTermXtp - Temp_tPhase;
  
  State := 5;

  /**********************************/
  /* Get A New CodaDurID.           */
  /**********************************/
  select CodaDurSeq.NEXTVAL into Temp_idCodaDur from sys.dual;

  Create_Core_idKey(Temp_idCodaDur);
  if Temp_idCodaDur <= 0 then
    OUT_idCodaDur := Temp_idCodaDur;
  end if;

  /**********************************/
  /* Insert new CodaDur Record      */
  /**********************************/
  insert into CodaDur(idCodaDur,idTCoda,idPick,tCodaDurObs,tCodaDurXtp)
    values(Temp_idCodaDur,IN_idTCoda,IN_idPick,Temp_tCodaDurObs,Temp_tCodaDurXtp);

  /**********************************/
  /* Set the idCodaDur return value   */
  /**********************************/

  OUT_idCodaDur := Temp_idCodaDur;

EXCEPTION
  WHEN DUP_VAL_ON_INDEX THEN
    /* Just to make sure that it wasn't from some subproc. */
    if State = 5 then
      select idCodaDur into Temp_idCodaDur from CodaDur 
       where idPick =  IN_idPick
         and idTCoda = IN_idTCoda;

      OUT_idCodaDur := Temp_idCodaDur;
    else
      OUT_idCodaDur := -6;
    end if;
    
  WHEN OTHERS THEN
    /*  ??? */
	  OUT_idCodaDur := -1;
    Temp_ID := SQLCODE;
    insert into test values('Create_CodaDur',State,Temp_ID);
END;

EXCEPTION
  WHEN NO_DATA_FOUND THEN
    OUT_idCodaDur := -7;
END;
