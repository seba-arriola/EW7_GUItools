/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_load_core_utils.sql,v 1.30 2005/06/21 20:20:02 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_load_core_utils.sql,v $
/*     Revision 1.30  2005/06/21 20:20:02  davidk
/*     Added view ALL_CODADUR_INFO
/*
/*     Revision 1.29  2005/06/16 16:46:51  davidk
/*     Added several views as part of db cleanup.
/*
/*     Revision 1.28  2005/05/24 21:37:10  davidk
/*     Added fucntions required to delete mw based data.
/*
/*     Revision 1.27  2005/05/12 20:41:39  mark
/*     Added comments functions; added comments to ALL_EVENTS_INFO
/*
/*     Revision 1.26  2005/05/05 23:26:57  davidk
/*     Cleaned up ALL_EVENTS_INFO view.
/*     It was previously dependent upon there being exactly 1 ExternalEvent
/*     per DBEvent, and would function properly if there were more or less.
/*     Removed the ExternalEvent component, and returned
/*     sSourceEventID=NULL for backwards compatability with existing code.
/*
/*     Revision 1.25  2005/04/21 19:19:34  davidk
/*     Added optional ssource column to ALL_UNASSOC_PICK_INFO view.
/*
/*     Revision 1.24  2005/03/31 20:49:35  davidk
/*     Added missing call to create ewdb_create_mwfilter sql proc.
/*
/*     Revision 1.23  2005/03/31 18:52:20  davidk
/*     Changed the EWDB_MwStruct:
/*       Added dM0 (doh!)
/*       Changed dPercentCVLD to dPercentCLVD
/*
/*     Revision 1.22  2005/03/24 18:15:45  davidk
/*     Added 2nd time series to MwChanTS to overcome 2000 byte string limitation
/*     in OCI 7.3.
/*
/*     Revision 1.21  2005/03/23 06:23:13  davidk
/*     Added SQL Views and Procs for Mw.
/*
/*     Revision 1.20  2005/01/28 17:43:02  mark
/*     Added sSource to arrival info
/*
/*     Revision 1.19  2004/09/09 05:00:28  davidk
/*     Removed merge/coincidence sql.
/*     Updated for reaper v2.
/*
/*     Revision 1.18  2004/08/19 19:32:24  davidk
/*     Added view ALL_PICK_INFO
/*
/*     Revision 1.17  2004/08/10 21:49:07  davidk
/*     Updated the architecture for sql install scripts.
/*     *_util  scripts load sql procedures, functions, and views
/*     *_constants scripts populate constant-tables
/*     *_tables  scripts load/update table, columns, constraints, and indexes
/*     *_combination scripts perform one of the above tasks, but contain statements
/*     which are dependent upon more than one sub-schema, and must be run
/*     separately after all sub-schemas have been processed.
/*
/*     These changes fixed several install problems and meaningless error messages.
/*                   */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/


/*
  Creating core schema utility functions
*****************************************/
@ewdb_getti
/

/****************************************
  Creating Views for parametric schema 
*****************************************/

CREATE OR REPLACE VIEW Quake AS 
 select EV.idEvent, O.tOrigin, O.dLat dLatitude, O.dLon Longitude,
        O.dDepth, Ma.dMagavg dMagnitude
   from  Event EV, Origin O, Magnitude Ma, Prefer Pr
   where EV.idEvent = PR.idEvent
     and PR.idPrefOrigin = O.idOrigin (+)
     and PR.idPrefMag = MA.idMag (+)
     and EV.tiEventType = 0 /*Type = Quake */
     and EV.iDubiocity=0
;


CREATE OR REPLACE VIEW ALL_EVENTS_INFO AS
  select ev.idEvent, ev.tiEventType, ev.iDubiocity, ev.bArchived, ev.idComment, 
   ev.idInternalComment, ev.idContribMagComment,
   s.sSource, s.sHumanReadable, '' sSourceEventID,
   o.idOrigin, o.tOrigin, o.dLat, o.dLon, o.dDepth, m.dMagAvg dPrefMag
  from event ev, source s, origin o,
   prefer p, magnitude m
  where s.idSource=o.idSource
    AND p.idEvent=ev.idEvent
    AND p.idpreforigin=o.idorigin (+)
    AND p.idprefmag=m.idmag (+);


CREATE OR REPLACE VIEW ALL_EVENT_ORIGIN_INFO AS
	select unique
		ev.idEvent, s.sSource, s.sHumanReadable,
		o.idOrigin, o.tOrigin, o.dLat, o.dLon, o.dDepth, 
		o.iAssocPh, o.iUsedPh, o.iFixedDepth
	from event ev, source s, origin o, bind b, ewdb_tablelist ewdbt
	where  ewdbt.sTableName='Origin'
		AND ewdbt.idTable=b.tiCore
		AND b.idEvent=ev.idEvent
		AND b.idCore=o.idOrigin 
		AND s.idSource=o.idSource;


CREATE OR REPLACE VIEW ALL_ARRIVAL_INFO as
  select p.idPick, p.xidExternal, p.idChan, p.sPhase, p.tPhase, 
         p.cMotion, p.cOnset, p.dSigma, 
         op.idOriginPick, op.idOrigin, op.sPhase sCalcPhase, 
         op.tPhase tCalcPhase, op.dWeight, op.dDist, op.dAzm, 
         op.dTakeoff, op.trespick,
		 src.sSource 
  from pick p, originpick op, source src
  where op.idPick = p.idPick
    AND src.idSource = p.idSource;


CREATE OR REPLACE VIEW STAMAG_CODADUR_INFO as
  select mlink.idMagLink, mlink.idMag, mlink.idDatum idMeasurement,
         mlink.dMag, mlink.dWeight,
         cd.idtcoda, cd.idpick, cd.tCodaDurObs, cd.tCodaDurXtp,
         tc.idChan, tc.tCodaTermObs, tc.tCodaTermXtp,
         mt.iMagType
  from MagLink mlink, CodaDur cd, TCoda tc, MagType mt
  where mlink.idDatum=cd.idCodaDur
    AND cd.idTCoda = tc.idTCoda
    AND mt.sMagAbbrev = 'Md';


CREATE OR REPLACE VIEW STAMAG_PEAKAMP_INFO as
  select mlink.idMagLink, mlink.idMag, mlink.idDatum idMeasurement,
         mlink.dMag, mlink.dWeight, 
         ab.tiExternal, ab.xidExternal, ab.idChan, 
         ab.dPeakAmp1, ab.tAmp1, ab.tPeriod1,
         ab.dPeakAmp2, ab.tAmp2, ab.tPeriod2,
         ab.iMagType
  from MagLink mlink, PeakAmp ab, Magnitude m
  where mlink.idDatum=ab.idPeakAmp
    AND mlink.idMag=m.idMag;


CREATE OR REPLACE VIEW ALL_PEAKAMPS AS
SELECT 
	idPeakAmp, idChan, idPick, tiExternal, xidExternal, dPeakAmp1,tAmp1,
	tPeriod1,dPeakamp2,tAmp2,tPeriod2,
	idSource,iMagType,tStartInterval,tEndInterval 
FROM PeakAmp;

CREATE OR REPLACE VIEW ALL_PEAKAMPS_FOR_EVENT AS
select EV.idEvent, PA.*
   from  Event EV, Bind B, PeakAmp PA
   where EV.idEvent = B.idEvent
     and B.tiCore = getti('PeakAmp')
     and B.idCore = PA.idPeakAmp;


CREATE OR REPLACE VIEW ALL_PICK_INFO AS
SELECT * from Pick;


CREATE OR REPLACE VIEW ALL_MWCHAN_INFO AS
SELECT * from MwChan;

CREATE OR REPLACE VIEW ALL_MWCTS_INFO AS
SELECT * from MwChanTS;


CREATE OR REPLACE VIEW ALL_MWCHAN_INFO_W_TS AS
select mwc.*, 
       cts1.dTSStart Synth_dTSStart, cts1.dTSFreq Synth_dTSFreq, cts1.iTSLen Synth_iTSLen, 
       cts1.idMwFilter Synth_idMwFilter, cts1.idChan Synth_idChan, cts1.iType Synth_iType, cts1.sTS1 Synth_sTS, cts1.sTS2 Synth_sTS2,
       cts2.dTSStart Real_dTSStart, cts2.dTSFreq Real_dTSFreq, cts2.iTSLen Real_iTSLen, 
       cts2.idMwFilter Real_idMwFilter, cts2.idChan Real_idChan, cts2.iType Real_iType,  cts2.sTS1 Real_sTS, cts2.sTS2 Real_sTS2
 from ALL_MWCHAN_INFO mwc, ALL_MWCTS_INFO cts1, ALL_MWCTS_INFO cts2
 where mwc.idSynthTS = cts1.idMwCTS
   and mwc.idRealTS  = cts2.idMwCTS;


CREATE OR REPLACE VIEW ALL_MW_INFO AS
SELECT * from Mw;

CREATE OR REPLACE VIEW ALL_MWFILTER_INFO AS
SELECT * from MwFilter;

CREATE OR REPLACE VIEW ALL_MW_INFO_W_FILTER AS
SELECT MW.*, MWF.dLowCutHz, MWF.dLowTaperHz, MWF.dHighTaperHz, MWF.dHighCutHz
 from ALL_MW_INFO MW, ALL_MWFILTER_INFO MWF
 where MW.idMwFilter = MWF.idMwFilter;

CREATE OR REPLACE VIEW ALL_UNASSOC_PICK_INFO AS
select p.*, s.ssource
 from pick p, source s
 where p.idpick in
   (
    select idpick from pick
     MINUS
    select pk.idpick
     from originpick op, prefer p, event e, pick pk
     where p.idpreforigin = op.idorigin
       and  p.idevent = e.idevent
       and  e.iDubiocity = 0
       and  op.idpick = pk.idpick
   )
   and  p.idsource = s.idsource (+)
 order by idpick;

CREATE OR REPLACE VIEW ALL_SOURCE_INFO AS
select * from source;

CREATE OR REPLACE VIEW ALL_MAG_INFO AS
select * from Magnitude;

CREATE OR REPLACE VIEW ALL_EXTERNAL_EVENT_INFO AS
 select e.idEvent, e.tiEventType, e.iDubiocity, e.bArchived, 
        ex.sSourceEventID, s.sSource, s.sHumanReadable,  
        c.sComment 
  from event e, bind b, externalevent ex, source s, comments c 
  where b.idEvent = e.idEvent 
        and b.tiCore  = getti('ExternalEvent') 
        and b.idCore  = ex.idExternalEvent 
        and ex.idSource = s.idSource 
        and ex.idComment = c.idComment (+) ;

CREATE OR REPLACE VIEW ALL_CODADUR_INFO AS
 select tc.tCodaTermObs, tc.tCodaTermXtp,  tc.idChan, 
       cd.tCodaDurObs, cd.tCodaDurXtp, cd.idTCoda, cd.idCodaDur, cd.idPick
  from TCoda tc, CodaDur cd
  where tc.idTCoda = cd.idTCoda;
/*******************************************/


/****************************************
  Creating Stored Procedures for parametric schema 
*****************************************/
@ewdb_get_idinst_from_ewinstid
/

@ewdb_check_external_record_validity
/

@ewdb_check_idevent_validity
/

@ewdb_check_record_validity
/

@ewdb_create_core_idkey
/

@ewdb_create_comment
/

@ewdb_delete_comment
/

@ewdb_update_comment_next
/

@ewdb_create_bind
/

@ewdb_create_codaamp
/

@ewdb_get_phase_info_from_pick
/

@ewdb_get_coda_term_times
/

@ewdb_create_codadur
/

@ewdb_get_idsource
/

@ewdb_create_externalevent
/

@ewdb_get_idextev_from_source_evid
/

@ewdb_create_event
/

@ewdb_set_prefer
/

@ewdb_create_magnitude
/

@ewdb_create_mechfm
/

@ewdb_get_idpick
/

@ewdb_create_pick
/

@ewdb_create_phase
/

@ewdb_create_arrival
/

@ewdb_create_pick_for_mech
/

@ewdb_get_mag_type_info
/

@ewdb_get_magnitude_type
/

@ewdb_create_sta_mag
/

@ewdb_create_tcoda
/


@ewdb_get_idamp
/

@ewdb_create_peakamp
/

@ewdb_delete_peakamp.sql
/

@ewdb_delete_codaamp.sql
/

@ewdb_delete_pick.sql
/

@ewdb_delete_tcoda.sql
/

@ewdb_delete_tcodas_before_time.sql
/

@ewdb_delete_codadur.sql
/

@ewdb_delete_data_before_time.sql
/

@ewdb_delete_preforigin.sql
/

/* 
   Delete Event will compile with errors because it requires 
   waveform deletion procedures and those do not get created 
   until the waveform script gets run later.  Please ignore
   the compile warning   DavidK 2000 03 20
************************************************************/
@ewdb_delete_event.sql
/

@ewdb_delete_externalevent.sql
/

@ewdb_delete_mwcts
/

@ewdb_delete_mwchan
/

@ewdb_delete_mw
/

@ewdb_delete_mws_for_origin
/

@ewdb_delete_magnitude.sql
/

@ewdb_delete_origin.sql
/

@ewdb_delete_peakamps_before_time.sql
/

@ewdb_delete_picks_before_time.sql
/

@ewdb_get_event_info
/

@ewdb_get_idevent_from_bind
/

@ewdb_get_idevent_from_externalid
/

@ewdb_get_idmag_from_sourceeventid
/

@ewdb_get_magnitude
/

@ewdb_get_origin
/

@ewdb_get_preferred_summary_info
/

@ewdb_vctonum
/

@ewdb_update_origin_by_xidexternal
/

@ewdb_update_event.sql
/

@ewdb_get_num_wavestations_for_event.sql
/

@ewdb_get_pick_byid.sql
/

@ewdb_create_unassoc_pick.sql
/

@ewdb_create_originpick.sql
/

@ewdb_create_base_origin_record
/

@ewdb_get_idorigin_from_source_data
/

@ewdb_create_origin
/

@ewdb_create_installation
/

@ewdb_create_source
/

@ewdb_get_event_id
/

@ewdb_get_pickid_by_extid
/


@ewdb_associate_mw_with_mag
/

@ewdb_create_mwfilter
/

@ewdb_get_mwfilter_by_id
/

@ewdb_get_mwfilter_by_props
/

@ewdb_create_mwchan
/

@ewdb_create_mwchants
/

@ewdb_create_mw
/



