/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Updates and additions to parametric tables go here
**********************************************************/


/*
 Added index Bind_tiCore to improve performance on tiCore searches.
 Specifically added to improve query speed for strong_motion data.
*********************************************************************/
CREATE INDEX Bind_tiCore ON Bind(tiCore);

ALTER TABLE PICK ADD(idSource NUMBER(13,0) NULL,  
                     CONSTRAINT PICK_idSOURCE FOREIGN KEY (idSource) 
                       REFERENCES SOURCE(IDSOURCE));


/* Author related changes */
CREATE TABLE Installation (idInst NUMBER(13,0) NOT NULL,
                           iEWInstID NUMBER(3,0) NOT NULL,
                           sShortName VARCHAR2(20) NOT NULL, 
					                 sFullName VARCHAR2(200) NULL, 
					                 sPrimaryNetCode VARCHAR2(5) NULL,
					                 sAltNetCodes VARCHAR2(20) NULL,
					                 sPrimaryContact VARCHAR2(100) NULL,
					                 sNote VARCHAR2(255) NULL,
 CONSTRAINT Installation_iEWInstID PRIMARY KEY (iEWInstID),
 CONSTRAINT idInst UNIQUE (idInst),
 CONSTRAINT Installation_sShortName UNIQUE(sShortName),
 CONSTRAINT Installation_NetCode UNIQUE (sPrimaryNetCode)
);


ALTER TABLE Source ADD(idInst NUMBER(13,0) NULL,
					             iSourceType Number(2,0) NULL,
					             sNote VARCHAR2(255) NULL,
                       CONSTRAINT Source_idInst FOREIGN KEY (idInst) 
                         REFERENCES Installation (idInst),
                       CONSTRAINT Source_UK UNIQUE (sSource,idInst)
                      );


/*
Recognized source types
-1  unknown
0-9 automatic
10 - 19 human

defaults  are
-1 - unknown type
 0 - automatic module
10 - human reviewer
*/



/* Add starting location fields to Origin table */
alter table Origin add dLatStart NUMBER(10,6) NULL;
alter table Origin add dLonStart NUMBER(10,6) NULL;
alter table Origin add dDepthStart NUMBER(7,3) NULL;


/* Add source and source ID to PeakAmp table */
alter table PeakAmp add (idSource NUMBER(13,0) NULL,
                     CONSTRAINT PeakAmp_idSOURCE FOREIGN KEY (idSource)
                       REFERENCES SOURCE(IDSOURCE));


