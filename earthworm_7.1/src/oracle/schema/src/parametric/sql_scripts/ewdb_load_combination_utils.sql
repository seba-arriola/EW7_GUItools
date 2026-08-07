/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_load_combination_utils.sql,v 1.2 2005/01/28 17:43:02 mark Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_load_combination_utils.sql,v $
/*     Revision 1.2  2005/01/28 17:43:02  mark
/*     Added sSource to arrival info
/*
/*     Revision 1.1  2004/08/10 21:49:07  davidk
/*     Updated the architecture for sql install scripts.
/*     *_util  scripts load sql procedures, functions, and views
/*     *_constants scripts populate constant-tables
/*     *_tables  scripts load/update table, columns, constraints, and indexes
/*     *_combination scripts perform one of the above tasks, but contain statements
/*     which are dependent upon more than one sub-schema, and must be run
/*     separately after all sub-schemas have been processed.
/*
/*     These changes fixed several install problems and meaningless error messages.
/*   */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/

/***********************************************
 COMBINATION VIEWS
 **********************************************/


/***********************************************
 VIEWS THAT REQUIRE Parametric and Channel Data 
 **********************************************/

CREATE OR REPLACE VIEW ALL_ARRIVAL_W_CHAN_INFO AS
select AAI.idPick, AAI.xidExternal, AAI.sPhase, AAI.tPhase, 
         AAI.cMotion, AAI.cOnset, AAI.dSigma,
         AAI.idOriginPick, AAI.idOrigin, AAI.sCalcPhase, 
         AAI.tCalcPhase, AAI.dWeight, AAI.dDist, AAI.dAzm dArrivalAzm, 
         AAI.dTakeoff, AAI.tResPick, AAI.sSource, ASI.*
  from ALL_ARRIVAL_INFO AAI, ALL_STATION_INFO ASI
  where AAI.idChan = ASI.idChan;


CREATE OR REPLACE VIEW ALL_PEAKAMPS_FOR_EVENT_W_CHAN AS
select APA.idEvent, APA.idPeakAmp, APA.iMagType,
        APA.dPeakAmp1, APA.tPeriod1, APA.tAmp1,
        APA.dPeakAmp2, APA.tPeriod2, APA.tAmp2,
        ASI.*
  from ALL_PEAKAMPS_FOR_EVENT APA, ALL_STATION_INFO ASI
  where APA.idChan = ASI.idChan
    and APA.tAmp1 >= ASI.tOn
    and APA.tAmp1 <= ASI.tOff;


CREATE OR REPLACE VIEW ALL_PEAKAMPS_W_CHAN_INFO AS
select APA.idPeakAmp, APA.iMagType,
        APA.dPeakAmp1, APA.tPeriod1, APA.tAmp1,
        APA.dPeakAmp2, APA.tPeriod2, APA.tAmp2,
        ASI.*
  from ALL_PEAKAMPS APA, ALL_STATION_INFO ASI
  where APA.idChan = ASI.idChan
    and APA.tAmp1 >= ASI.tOn
    and APA.tAmp1 <= ASI.tOff;


CREATE OR REPLACE VIEW ALL_ARRIVAL_W_CHAN_INFO AS
select AAI.idPick, AAI.xidExternal, AAI.sPhase, AAI.tPhase, 
         AAI.cMotion, AAI.cOnset, AAI.dSigma, 
         AAI.idOriginPick, AAI.idOrigin, AAI.sCalcPhase, 
         AAI.tCalcPhase, AAI.dWeight, AAI.dDist, AAI.dAzm dArrivalAzm, 
         AAI.dTakeoff, AAI.tResPick, ASI.*
  from ALL_ARRIVAL_INFO AAI, ALL_STATION_INFO ASI
  where AAI.idChan = ASI.idChan;

CREATE OR REPLACE VIEW ALL_AMPS_FOR_ORIGIN_BY_PICK as
select   AAI.idPick, AAI.xidExternal xidPick, AAI.sPhase, AAI.tPhase, 
         AAI.cMotion, AAI.cOnset, AAI.dSigma, 
         AAI.idOriginPick, AAI.idOrigin, AAI.sCalcPhase, 
         AAI.tCalcPhase, AAI.dWeight, AAI.dDist, AAI.dAzm dArrivalAzm, 
         AAI.dTakeoff, AAI.tResPick,
         pa.idPeakAmp, pa.tiExternal, pa.xidExternal,
         pa.dPeakAmp1, pa.tAmp1, pa.tPeriod1,
         pa.dPeakAmp2, pa.tAmp2, pa.tPeriod2,
         pa.iMagType, pa.tStartInterval, pa.tEndInterval,
         s.sSource,
         ASI.*
  from ALL_ARRIVAL_INFO AAI, ALL_PEAKAMPS pa, ALL_STATION_INFO ASI, source s
  where aai.idpick = pa.idpick
    and pa.idChan = ASI.idChan
    and pa.tAmp1 <= ASI.tOff
    and pa.tAmp1 >= ASI.tOn
    and pa.idSource = s.idSource;


CREATE OR REPLACE VIEW ALL_AMPS_FOR_ORIGIN_BY_MAG as
select m.idOrigin, m.idmag, ml.idmaglink, 
         ml.dMag, ml.dWeight,
         pa.idPeakAmp, pa.tiExternal, pa.xidExternal,
         pa.dPeakAmp1, pa.tAmp1, pa.tPeriod1,
         pa.dPeakAmp2, pa.tAmp2, pa.tPeriod2,
         pa.iMagType, pa.tStartInterval, pa.tEndInterval,
         ASI.*
 from Magnitude m, MagLink ml, PeakAmp pa, ALL_STATION_INFO ASI  
 where m.idmag    = ml.idmag
   AND ml.idDatum = pa.idPeakAmp
   and pa.idChan  = ASI.idChan;

