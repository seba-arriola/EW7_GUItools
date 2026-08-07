/*                                                          */
/*   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE */
/*   CHECKED IT OUT USING THE COMMAND CHECKOUT.             */
/*                                                          */

/***********************************************************
  Column Updates and additions to infra tables:
**********************************************************/


/***********************************************************
  Constraint additions to infra tables:
**********************************************************/


/***********************************************************
  Index additions to infra tables:
**********************************************************/
CREATE INDEX COMPT_SSTA ON  COMPT(sSta);
CREATE INDEX COMPT_tOff ON  COMPT(tOff);
CREATE INDEX CHANT_idCompT ON  ChanT(idCompT);

