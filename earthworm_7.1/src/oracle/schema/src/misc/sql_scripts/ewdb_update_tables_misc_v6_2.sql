/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Updates and additions to parametric tables go here
**********************************************************/

/* 
  Place Tables
 *************************/
/*************************************************/
CREATE TABLE PlaceType(idPlaceType  NUMBER(13,0) NOT NULL,
                    iPlaceMajorType NUMBER(3,0) NOT NULL,
                    iPlaceMinorType NUMBER(3,0) NOT NULL,
                    sDescription    VARCHAR2(50) NOT NULL,
 CONSTRAINT idPlaceType PRIMARY KEY (idPlaceType),
 CONSTRAINT PlaceType_UK UNIQUE (iPlaceMajorType,iPlaceMinorType)
);
Create Sequence PlaceTypeSeq  START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;

/*************************************************/
CREATE TABLE Place (idPlace NUMBER(13,0) NOT NULL,
                    iPlaceMajorType NUMBER(3,0) NULL,
                    iPlaceMinorType NUMBER(3,0) NULL,
                    dLat NUMBER(10,6) NULL,
                    dLon NUMBER(10,6) NULL,
                    dElev NUMBER(7,2) NULL,
                    iPopulation NUMBER(13,0) NULL,
                    sName VARCHAR2(70) NOT NULL, 
                    sState VARCHAR2(5) NOT NULL, 
                    sCounty VARCHAR2(30) NULL,
                    sCountry VARCHAR2(30) NOT NULL,
 CONSTRAINT idPlace PRIMARY KEY (idPlace),
 CONSTRAINT Place_iPlaceType_FK FOREIGN KEY (iPlaceMajorType,iPlaceMinorType) 
   REFERENCES PlaceType (iPlaceMajorType,iPlaceMinorType),
 CONSTRAINT Place_UK UNIQUE (sName,dLat,dLon)
);
Create Sequence PlaceSeq      START WITH 1 MAXVALUE 999999999 CYCLE CACHE 2;


/*************************************************/

/* 
  Event Lock Tables
 *************************/

CREATE TABLE EventLock(idEvent NUMBER(13,0) NOT NULL,
                          idSource NUMBER(10,0) NOT NULL,
                          sMachineName VARCHAR2(25) NOT NULL, 
                          iLockTime NUMBER(10,0) NULL, 
                          sNote VARCHAR2(50) NULL,
 CONSTRAINT EventLock_idEvent PRIMARY KEY(idEvent)
);
