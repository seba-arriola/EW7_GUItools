/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Column Updates and additions to waveform tables:
**********************************************************/


/***********************************************************
  Constraint additions to waveform tables:
**********************************************************/
/* THE FOLLOWING CONSTRAINTS REQUIRE THE CHAN TABLE */

ALTER TABLE SnippetRequest 
 ADD( 
   CONSTRAINT SnippetRequest_idChan FOREIGN KEY (idChan) 
     REFERENCES CHAN(idChan)
);


/*************************************************/
/* THE FOLLOWING CONSTRAINTS REQUIRE THE EVENT TABLE */

ALTER TABLE SnippetRequest 
 ADD( 
   CONSTRAINT SnippetRequest_idEvent FOREIGN KEY (idEvent) 
     REFERENCES EVENT(idEvent)
);


/*************************************************/
/* THE FOLLOWING CONSTRAINTS REQUIRE THE WAVEFORMDESC TABLE */

ALTER TABLE SnippetRetrievalSchedule 
 ADD( 
   CONSTRAINT SnippetRS_idWaveform FOREIGN KEY (idWaveform) 
     REFERENCES WAVEFORMDESC(idWaveform)
);

/*************************************************/
/* THE FOLLOWING CONSTRAINTS REQUIRE THE SNIPPETREQUEST TABLE */

ALTER TABLE SnippetRetrievalSchedule 
 ADD( 
   CONSTRAINT SnippetRS_FK_idSnipReq FOREIGN KEY (idSnipReq) 
     REFERENCES SNIPPETREQUEST(idSnipReq)
);



/***********************************************************
  Index additions to waveform tables:
**********************************************************/
/* Create the concierge indeces ********/
CREATE INDEX SnippetRequest_idChan ON  SnippetRequest(idChan) ;
CREATE INDEX SnippetRequest_idEvent ON  SnippetRequest(idEvent) ;
CREATE INDEX SnippetRS_tNextAttempt ON SnippetRetrievalSchedule(tNextAttempt);
CREATE INDEX SnippetRS_iLockTime ON SnippetRetrievalSchedule(iLockTime);

