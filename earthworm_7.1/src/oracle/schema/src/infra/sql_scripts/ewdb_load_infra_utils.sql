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
       co.dAzm, co.dDip, co.dLat, co.dLon, co.dElev, 
	   co.idCompT, co.tOff tOffCompT, co.tOn tOnCompT
 from chant ch, compt co
 where ch.idcompt=co.idcompt
;

CREATE OR REPLACE VIEW ALL_COMPT_INFO AS
select co.tOff,co.tOn, co.idComp, com.idSite,
       co.sSta, co.sComp, co.sNet, co.sLoc,
       co.dAzm, co.dDip, co.dLat, co.dLon, co.dElev, 
	   co.idCompT
 from  compt co, comp com
 where co.idComp = com.idComp
 ORDER BY co.sSta, co.sNet, co.sComp, co.sLoc, co.tOn
;

CREATE OR REPLACE VIEW ALL_CHANT_INFO AS
select * from ChanT;

CREATE OR REPLACE VIEW ALL_STATION_INFO_W_SITE AS
select asi.*, co.idSite
 from all_station_info asi, comp co
 where asi.idComp=co.idComp
;

CREATE OR REPLACE VIEW ALL_SITE_INFO AS
select * from Site
;

CREATE OR REPLACE VIEW ALL_COMP_INFO AS
select s.sSta, s.sNet, c.* 
 from Site s, Comp c
 where s.idSite = c.idSite
;

CREATE OR REPLACE VIEW ALL_SITET_INFO AS
select s.ssta, s.snet, s.idComment Site_idComment, st.* 
from Site s, SiteT st
where s.idSite=st.idSite
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
select asi.*, acc.dGain, acc.dSampRate, acc.idCTF,
       acc.sFuncDesc
 from ALL_ChanCTF acc, ALL_STATION_INFO asi
 where acc.idchant(+) = asi.idchant
;


CREATE OR REPLACE VIEW ALL_CHANNEL_INFO_W_SITE AS
select asctf.*, co.idSite
 from all_station_ctf asctf, comp co
 where asctf.idComp=co.idComp
;



/* 
 Loading Infrastructure stored procedures 
****************************************/

@ewdb_delete_pz_for_ctf
/

@ewdb_delete_ctf
/

@ewdb_delete_chanctf
/

@ewdb_delete_chant
/

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

/* new added as part of station maintenance */
@ewdb_delete_compt
/

@ewdb_delete_comp
/

@ewdb_delete_sitet
/

@ewdb_delete_site
/

@ewdb_get_chan_params
/

@ewdb_get_compt_params
/

@ewdb_get_sitet_params
/

@ewdb_modify_site_params
/

@ewdb_set_compt_params
/

@ewdb_split_sitet
/

@ewdb_update_chant_time
/

@ewdb_update_compt_time
/

@ewdb_update_sitet
/



