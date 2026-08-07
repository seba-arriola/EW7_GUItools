/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*    $Id: ewdb_update_tables_combination_constraints_working.sql,v 1.1 2004/08/10 21:49:07 davidk Exp $ */
/*                                                          */
/*    Revision history:                                     */
/*     $Log: ewdb_update_tables_combination_constraints_working.sql,v $
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

/***********************************************************
  COMBINATION CONSTRAINTS:
**********************************************************/

/***********************************************************
  Constraint additions to parametric tables:
**********************************************************/

/* THE FOLLOWING SCRIPTS REQUIRE THE CHAN TABLE */
ALTER TABLE CodaAmp 
 ADD( 
   CONSTRAINT CodaAmp_idChan FOREIGN KEY (idChan) 
     REFERENCES CHAN(idChan)
);

ALTER TABLE TCoda 
 ADD( 
   CONSTRAINT TCoda_idChan FOREIGN KEY (idChan) 
     REFERENCES CHAN(idChan)
);

ALTER TABLE PeakAmp 
 ADD( 
   CONSTRAINT PeakAmp_idChan FOREIGN KEY (idChan) 
     REFERENCES CHAN(idChan)
);

ALTER TABLE Pick
 ADD( 
   CONSTRAINT Pick_idChan FOREIGN KEY (idChan) 
     REFERENCES CHAN(idChan)
);

ALTER TABLE Ray
 ADD( 
   CONSTRAINT Ray_idChan1 FOREIGN KEY (idChan1) 
     REFERENCES CHAN(idChan),
   CONSTRAINT Ray_idChan2 FOREIGN KEY (idChan2) 
     REFERENCES CHAN(idChan),
   CONSTRAINT Ray_idChan3 FOREIGN KEY (idChan3) 
     REFERENCES CHAN(idChan)
);
