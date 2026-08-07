/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: Glock.h,v 1.2 2006/05/22 16:01:25 paulf Exp $
 *
 *    Revision history:
 *     $Log: Glock.h,v $
 *     Revision 1.2  2006/05/22 16:01:25  paulf
 *     added from hydra_proj, new version
 *
 *     Revision 1.1.1.1  2005/06/22 19:30:48  michelle
 *     new directory tree built from files in HYDRA_NEWDIR_2005-06-20 tagged hydra and earthworm projects
 *
 *     Revision 1.3  2004/11/01 06:33:21  davidk
 *     Modified to work with new hydra traveltime library.
 *     Cleaned up struct ITravelTime references.
 *
 *     Revision 1.2  2004/04/01 22:09:18  davidk
 *     v1.11 update
 *     Now utilizes OPCalc routines for calculating Origin/Pick association params.
 *
 *     Revision 1.4  2003/11/07 22:41:05  davidk
 *     Added functions described in previous description.
 *
 *     Revision 1.3  2003/11/07 22:38:59  davidk
 *     Added RCS Header.
 *     Added two member functions: Locate_InitOrigin() and CompareOrigins().
 *
 *
 **********************************************************/

 #ifndef GLOCK_H
#define GLOCK_H

#include <IGlint.h>


#define MAX_PCK 2000

struct ISolve;
class CGlock {
public:
// Attributes
	int			nIter;
//	int			nEq;
//	double		dRms;
	bool		bFree[4];	// True if parameter free
	ITravelTime	*pTT;
	IGlint		*pGlnt;
	ISolve		*pSlv;
	int			nPck;
	ORIGIN		*pOrg;
	PICK		*pPick[MAX_PCK];

// Methods
	CGlock();
	virtual ~CGlock();
	int Locate(char *ent, char *mask);
	int Locate(char *ent);
//	int Iterate(bool solve);
  int Iterate(bool solve, ORIGIN * poCurrent, ORIGIN * poNext);
  void Affinity();
protected:
  int  ProcessLocation(bool solve, ORIGIN * pOrg, PICK ** pPck, int nPck);
  void Locate_InitOrigin(ORIGIN * pOrigin, const char * idOrigin);
  int  CompareOrigins(ORIGIN * po1, ORIGIN * po2);
  int  AddPickCoefficientsForSolve(PICK * pPick, const ORIGIN * pOrg);
};

#endif
