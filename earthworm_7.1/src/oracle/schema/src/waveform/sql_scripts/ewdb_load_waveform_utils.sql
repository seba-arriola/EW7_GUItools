/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */


/******************************
 Create waveform views
*******************************/

CREATE OR REPLACE VIEW ALL_WAVEFORM_DESCRIPTORS AS 
select *
 from waveformdesc
;

CREATE OR REPLACE VIEW ALL_WAVEFORM_DATA AS 
select wd.*, w.binSnippet
 from waveformdesc wd, waveform w
 where wd.idWaveform = w.idWaveform
;

CREATE OR REPLACE VIEW ALL_WAVEFORM_DESC_W_COMPT_INFO AS 
select wd.idWaveform, wd.idchan, wd.tStart, wd.tEnd, wd.iDataFormat, 
       wd.iByteLen,
       cot.tOff, cot.tOn, cot.sSta, cot.sComp, cot.sNet, cot.sLoc,
       cot.dLat, cot.dLon, cot.dElev, cot.dAzm, cot.dDip
 from waveformdesc wd, compt cot, chant cht
 where wd.idChan   = cht.idChan
   and wd.tStart   < cht.tOff
   and wd.tEnd     > cht.tOn
   and cht.idCompT = cot.idCompT
;

CREATE OR REPLACE VIEW ALL_WAVEFORM_DESC_BY_EVENT
AS
select awd.*, b.idEvent
from ALL_WAVEFORM_DESCRIPTORS awd, bind b, ewdb_tablelist p
where awd.idWaveform = b.idCore
and   b.tiCore=p.idTable
and   p.sTableName='WaveformDesc';

CREATE OR REPLACE VIEW ALL_WAVES_W_COMPT_BY_EVENT
AS
select awdwci.*, b.idEvent
from ALL_WAVEFORM_DESC_W_COMPT_INFO awdwci, bind b, ewdb_tablelist p
where awdwci.idWaveform = b.idCore
and   b.tiCore=p.idTable
and   p.sTableName='WaveformDesc';

/*** Create the concierge views ********/

CREATE OR REPLACE VIEW ALL_SNIPPET_REQUESTS AS 
select sr.*, 
       srs.tNextAttempt, srs.tInitialRequest,
       srs.iNumAttempts, srs.iNumAlreadyAtmptd, 
       srs.idWaveform, srs.iRetCode, 
       srs.iRequestGroup, srs.iLockTime, 
       srs.sNote,
		   cot.sSta, cot.sComp, cot.sNet, cot.sLoc
		from SnippetRequest sr, SnippetRetrievalSchedule srs,
		   CompT cot, ChanT cht
		where sr.idSnipReq = srs.idSnipReq
		  and sr.idChan    = cht.idChan
		  and cht.idCompT = cot.idCompT
		  and sr.tStart   <= cht.tOff
		  and sr.tEnd     >= cht.tOn;

/***********************************
 Create Waveform Stored Procedures
***********************************/

@ewdb_create_waveform_desc.sql
/

@ewdb_delete_actual_waveform.sql
/

@ewdb_delete_waveform.sql
/

@ewdb_delete_waveforms_before_time
/

@ewdb_get_waveformdesc
/

@ewdb_update_waveform_desc
/


/* 
 Create the concierge stored procedures 
**************************************/
@ewdb_reserve_snippet_requests.sql
/

@ewdb_create_snipreq.sql                
/

@ewdb_update_snipreq.sql
/

@ewdb_delete_snipreq.sql                
/

@ewdb_update_snipreq_controlparams.sql
/

@ewdb_lock_reserved_snip_req.sql        
/

@ewdb_lock_reserved_snip_reqs.sql       
/

@ewdb_release_reserved_snip_reqs.sql
/

@ewdb_check_for_matching_snippet
/

@ewdb_check_for_matching_snipreq
/

@ewdb_find_dup_snipreq_or_snippet
/

@ewdb_find_or_create_snipreq
/

@ewdb_delete_snippetrequest.sql                
/

@ewdb_delete_snipreqs_before_time.sql                
/


