/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_sac.h,v 1.1 2000/12/19 18:31:25 lombard Exp $
 *
 *    Revision history:
 *     $Log: lm_sac.h,v $
 *     Revision 1.1  2000/12/19 18:31:25  lombard
 *     Initial revision
 *
 *
 *
 */

/*
 * lm_sac.h
 */

#ifndef LM_SAC_H
#define LM_SAC_H

#include "lm_config.h"
#include "lm_util.h"

int getAmpFromSAC(EVENT *, LMPARAMS *, DATABUF *);
int isMatch(char *, char *);
int initSACBuf( long );
void initSACSave(EVENT *, LMPARAMS *);
void saveSACWATrace( DATABUF *, STA *, COMP3 *, EVENT *, LMPARAMS *);
void termSACSave(EVENT *, LMPARAMS *);
int getSACStaDist(double *, double *, double *);

#endif
