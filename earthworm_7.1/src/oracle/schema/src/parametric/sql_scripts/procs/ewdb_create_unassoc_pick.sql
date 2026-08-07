/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

CREATE OR REPLACE PROCEDURE Create_Unassoc_Pick
(OUT_idPick out number,
 IN_idChan number,
 IN_sSource varchar,
 IN_sSourcePickID varchar,
 IN_sPhase varchar,
 IN_tPhase number,
 IN_cMotion char,
 IN_cOnset char,
 IN_dSigma number
)
as

Temp_ID          number;
Temp_idPick      number :=0;

begin

  Create_Pick(Temp_idPick,
              IN_idChan,
              IN_sSource,
              IN_sSourcePickID,
              IN_sPhase,
              IN_tPhase,
              IN_cMotion,
              IN_cOnset,
              IN_dSigma
             );

   OUT_idPick := Temp_idPick;


EXCEPTION
  WHEN OTHERS THEN
    /*  ??? */
    Temp_ID := SQLCODE;
    insert into test values('Create_Unassoc_Pick', Temp_idPick, Temp_ID);
	  OUT_idPick := -1;
END;
