/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_load_merge_utils.sql,v 1.1 2004/09/09 06:04:10 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_load_merge_utils.sql,v $
/*     Revision 1.1  2004/09/09 06:04:10  davidk
/*     no message
/*                   */
/*     Revision 1.1  2002/06/18 16:10:35  lucky             */
/*     Initial revision                                     */
/*                                                          */
/************************************************************/


/*
  Creating merge schema utility functions
*****************************************/

/****************************************
  Creating Views for merge schema 
*****************************************/

CREATE OR REPLACE VIEW ALL_COINCIDENCE_EVENT_INFO AS
    select unique
        ev.idEvent, ev.tiEventType, s.sSource, s.sHumanReadable,
        c.idCoincidence, c.tCoincidence
    from event ev, source s, CoincidenceEvent c, bind b, ewdb_tablelist ewdbt
    where  ewdbt.sTableName='CoincidenceEvent'
        AND ewdbt.idTable=b.tiCore
        AND b.idEvent=ev.idEvent
        AND b.idCore=c.idCoincidence
        AND s.idSource=c.idSource;



CREATE OR REPLACE VIEW ALL_MERGE_INFO AS
    select unique
        mrg.idEvent, mrg.idPh, mrg.idMerge,
        o.tOrigin, o.dLat, o.dLon, o.dDepth, m.dMagAvg dPrefMag,
		ph.idPrefEvent, s.sSource, s.sHumanReadable
    from merge mrg, origin o, bind b, ewdb_tablelist ewdbt, 
			source s, phenomena ph, magnitude m, prefer p
    where  ewdbt.sTableName='Origin'
        AND ewdbt.idTable=b.tiCore
        AND b.idEvent=mrg.idEvent
        AND b.idCore=o.idOrigin
        AND ph.idPh=mrg.idPh
		AND p.idEvent=mrg.idEvent 
		AND p.idprefmag=m.idmag 
		AND p.idpreforigin=o.idOrigin 
        AND s.idSource=ph.idSource;



/*******************************************/


/****************************************
  Creating Stored Procedures for merge schema 
*****************************************/

/* Coincidence utils */

@ewdb_create_coincidence.sql
/

@ewdb_create_trigger.sql
/

@ewdb_delete_coincidence.sql
/

@ewdb_delete_trigger.sql
/


/* Merge utils */

@ewdb_create_phenomenon.sql
/

@ewdb_delete_phenomenon.sql
/

@ewdb_create_merge.sql
/

@ewdb_delete_merge.sql
/

@ewdb_update_merge_preference.sql
/
