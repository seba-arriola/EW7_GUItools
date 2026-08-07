/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */
/*                                                          */


/* 
 Creating Infrastructure views
****************************************/
CREATE OR REPLACE VIEW ALL_STATION_INFO AS
select ch.idChan, ch.tOff,ch.tOn,ch.idChanT, co.idComp,
       co.sSta, co.sComp, co.sNet, co.sLoc,
       co.dAzm, co.dDip, co.dLat, co.dLon, co.dElev
 from chant ch, compt co
 where ch.idcompt=co.idcompt
;


CREATE OR REPLACE VIEW ALL_CTF AS
select * from CookedTF
;


CREATE OR REPLACE VIEW ALL_PZ AS
select * from PolesAndZeroes
;


CREATE OR REPLACE VIEW ALL_CHANCTF AS
select cht.*,chctf.dGain,chctf.dSampRate,ctf.*
 from ChanT cht, ChanCTF chctf, CookedTF ctf
 where cht.idChanT = chctf.idChanT
   and chctf.idCTF = ctf.idCTF
;


CREATE OR REPLACE VIEW ALL_STATION_CTF AS
select acc.*,asi.idComp,
       asi.sSta, asi.sComp, asi.sNet, asi.sLoc,
       asi.dAzm, asi.dDip, asi.dLat, asi.dLon, asi.dElev
 from ALL_ChanCTF acc, ALL_STATION_INFO asi
 where acc.idchant = asi.idchant
;


/* 
 Creating Infrastructure indeces 
****************************************/
CREATE INDEX COMPT_SSTA ON  COMPT(sSta);
CREATE INDEX COMPT_tOff ON  COMPT(tOff);
CREATE INDEX CHANT_idCompT ON  ChanT(idCompT);


/* 
 Loading Infrastructure stored procedures 
****************************************/

@ewdb_create_chant
/

@ewdb_set_chan_params
/

@ewdb_update_chant_for_clipped_compt
/

@ewdb_create_compt
/

@ewdb_create_site
/

@ewdb_get_idsite
/

@ewdb_create_comp
/

@ewdb_get_idcomp
/

@ewdb_set_comp_params
/

@ewdb_create_sitet
/

@ewdb_set_site_params
/

@ewdb_create_poleorzero
/

@ewdb_add_polesandzeroes_for_ctf
/

@ewdb_assoc_comp_w_chan
/

@ewdb_assoc_ctf
/

@ewdb_create_chan
/

@ewdb_create_comp_from_scnl
/

@ewdb_create_compt_for_scnlt
/

@ewdb_create_ctf
/

@ewdb_delete_pz_for_ctf
/

@ewdb_get_comp_params
/

@ewdb_get_idchant
/

@ewdb_update_chant
/

@ewdb_update_chants_for_timerange
/

@ewdb_update_ctf
/


/*********************************************************************/
/* Cooked Inf      */
/* Doing Cooked Inf as a set of 2 strings(Poles,Zeroes)       */
/* in the form of (R1 I1 R2 I2 R3 I3 R4 I4)      */
/*       */
/* 1) Create a CookedTF, and maybe assoc it with a ChanT.      */
/*     CreateCTF(OUT idCTF,OUT idChanCTF,sFuncDesc,Poles,Zeroes,      */
/*								idChanT,dGain,dSampRate)              */
/*     CreateCoreCTF(OUT idCTF,sFuncDesc,Poles,Zeroes)              */
/*     CreatePoleOrZero(OUT idPZ,idCTF,cPZType,dReal,dImaginary)              */
/*     AssocCTF(OUT idChanCTF,idCTF,idChanT,dGain,dSampRate)              */
/*               */
/*               */
/* 2) Assoc a ChanT with a CookedTF (insert or update)              */
/*     {Handled in #1 by AssocCTF}              */
/*               */
/* 3) Give me the CookedTF              */
/*     select * from ALL_CTF where idCTF=XXX;              */
/*     select * from PolesAndZeroes where idCTF=XXX;              */
/*               */
/* 4) Give me the cookedTF w/Gain by idChan,T              */
/*     select * from ALL_ChanCTF               */
/*      where idChan = XXX              */
/*        and tOn   <= T               */
/*        and tOff  >= T              */
/*     select * from PolesAndZeroes where idCTF=XXX;              */
/*               */
/* 5) Give me all the station info by SCNLT (CompT,CookedTF w/Gain)              */
/*     select * from ALL_STATION_CTF               */
/*      where idChan = XXX              */
/*        and tOn   <= T               */
/*        and tOff  >= T              */
/*     select * from PolesAndZeroes where idCTF=XXX;              */
/*               */
/* 6) Update a CookedTF by idCTF              */
/*     UpdateCTF(idCTF,sFuncDesc,Poles,Zeroes)              */
/*     DeletePZForCTF(idCTF)              */
/*  */
