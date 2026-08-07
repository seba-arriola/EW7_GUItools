#include <math.h>
#include <Debug.h>
#include <string.h>


# define _OPCALC_SO_EXPORT  __declspec( dllexport)

#include <opcalc.h>
#include <opcalc_const.h>

// Lib Global variables - File Scope
static ITravelTime * pTT = 0x00;      // file-scope master traveltime pointer

//---------------------------------------------------------------------------------------Zerg
// Calculate the OPCalc_BellCurve function of x.
// The height of the curve ranges from 0 to 1.
// The curve is centered at 0, with a range from -1 to 1,
// such that OPCalc_BellCurve(0) = 1 and OPCalc_BellCurve(-1) = OPCalc_BellCurve(1) = 0.
// OPCalc_BellCurve is usually called as OPCalc_BellCurve(x/w), 
// where x is the deviation from the mean, and w is the half-width
// of the distrubution desired.
// OPCalc_BellCurve(-w) = OPCalc_BellCurve(w) = 0.0. 
// dBellCurve/dX(0) = dBellCurve/dX(1) = 0.0.
// OPCalc_BellCurve is symetric about 0.0, and is roughly gaussian with a
// half-width of 1. For x < -1 and x > 1, OPCalc_BellCurve is defined as 0.
// In Carl terms, OPCalc_BellCurve is defined as Zerg.
double OPCalc_BellCurve(double xx) 
{
	double x = xx;
	if(x < 0.0)
		x = -x;
	if(x > 1.0)
		return 0.0;
	return 2.0*x*x*x - 3.0*x*x + 1.0;
}


int OPCalc_CalcOriginAffinity(ORIGIN * pOrg, PICK ** ppPick, int nPck) 
{
//	PICK *pck;
	int i;
  int iRetCode;
  double dAffinitySum = 0.0;  // Sum of usable-pick affinities
  int    iAffinityNum = 0;    // Number of usable-picks

  if(nPck < 1)
    return(-1);

  // Log the calculated values
	CDebug::Log(DEBUG_MINOR_INFO,"Affinity(): dDelMin:%.2f dDelMod:%.2f dDelMax:%.2f\n",
		pOrg->dDelMin, pOrg->dDelMod, pOrg->dDelMax);

	// Gap affinity
		// DK CHANGE 011504.  Changing the range of the
    //                    dAffGap statistic
    //         pOrg->dAffGap = 1.0+OPCalc_BellCurve(pOrg->dGap/360.0);
    // dAffGap range is 0.0 - 2.0
	if(pOrg->nEq < OPCALC_nMinStatPickCount)
		pOrg->dAffGap = 1.0;
  else
    pOrg->dAffGap = 4 * OPCalc_BellCurve(pOrg->dGap/360.0); 
//    pOrg->dAffGap = 3 * OPCalc_BellCurve(pOrg->dGap/360.0); 

  if(pOrg->dAffGap > 2.0)
    pOrg->dAffGap = 2.0;
  else if(pOrg->dAffGap < 0.5 && pOrg->dAffGap > 0.1)
    pOrg->dAffGap = 0.5;  // don't let AffGap drop below 0.5 unless 
                          //the Gap is AWFUL!!!!



	// nEq affinity (macho statistic)
	if(pOrg->nEq < OPCALC_nMinStatPickCount)
		pOrg->dAffNumArr = 1.0;
	else
		pOrg->dAffNumArr = log10((float)(pOrg->nEq));

	for(i=0; i<nPck; i++) 
  {
    iRetCode = OPCalc_CalcPickAffinity(pOrg, ppPick[i]);
	  CDebug::Log(DEBUG_MINOR_INFO,"Affinity(): Pck[%d] Delta:%.2f Aff:%.2f\n", i, ppPick[i]->dDelta, ppPick[i]->dAff);

    if(iRetCode < 0)
    {
   	  CDebug::Log(DEBUG_MINOR_ERROR,
                  "OPCalc_CalcOriginAffinity(): Fatal Error(%d) calculating"
                  " affinity for pick[%d]\n",
                  iRetCode,i);
      return(-1);
    }
    if(iRetCode > 0)
      continue;

    dAffinitySum+=ppPick[i]->dAff;
    iAffinityNum++;
  }

  if(iAffinityNum > 0)
  {
    pOrg->dAff = dAffinitySum / iAffinityNum;
  }
  else
  {
    pOrg->dAff = 0;
  }
  return(0);
}  // end OPCalc_CalcOriginAffinity()


int OPCalc_CalcPickAffinity(ORIGIN * pOrg, PICK * pPick) 
{

  double dDistRatio;  // ratio between distance of current pick and median distance
  double dMedianDistEst;

  int iRetCode;

	// Do full calculations for picks not already associated with the
  // origin.  (distance, azimuth, traveltime, TakeOffAngle)
	if(pPick->iState == GLINT_STATE_WAIF) 
  {
    iRetCode = OPCalc_CalculateOriginPickParams(pPick,pOrg);
    if(iRetCode)
      return(iRetCode);
	}

	// Residual affinity
  // Range   0.0 - 2.0  where 0.0 is no residual and 2.0 is 
  //         any absolute residual >= 10 seconds (OPCALC_dResidualCutOff)
	pPick->dAffRes = 2.0 * OPCalc_BellCurve(pPick->tRes/(pPick->ttt.dResidWidth));

	// Distance affinity
  if(OPCALC_fAffinityStatistics & AFFINITY_USE_DISTANCE)
  {

    // hack in a calculation that increases the value used for the median distance if the number of 
    // phases is large.
    if(pOrg->nPh > (pOrg->dDelMod*2.0))
      dMedianDistEst = (double) (pOrg->nPh/2);
    else
      dMedianDistEst = pOrg->dDelMod;
    
    // Calculate the distance of this pick relative
    // to the distance of median pick used in the solution
    if(pOrg->nEq < OPCALC_nMinStatPickCount) 
    {
        dDistRatio = pPick->dDelta / (OPCALC_dCutoffMultiple*dMedianDistEst * 1.5);
    } 
    else 
    {
        dDistRatio = pPick->dDelta / (OPCALC_dCutoffMultiple*dMedianDistEst);
    }
    // Distance Affinity
    //       Penalize picks (based upon distance) when
    //       they are more than dCutoffMultiple times the 
    //       median pick distance from the Origin.
    // Range   0.0 - 2.0  where 0.0 is beyond the Modal distance cutoff and 2.0 is 
    //         a pick at the hypocenter.
    pPick->dAffDis = 2 * OPCalc_BellCurve(dDistRatio);

    if(pOrg->nEq < 4)
      pPick->dAffDis = 1.0;
  }
  else
  {
    pPick->dAffDis = 1.0;
  }

  if(OPCALC_fAffinityStatistics & AFFINITY_USE_PPD)
  {
    // Pick Probability Affinity
    // Calculate a Pick Probability Affinity statistic.
    // Range   0.5 - 5.0 (highest observed value is 3.0)
    // where the value is ((log 2 (X+1)) + 1) / 2
    // where X is the number of times that a phase was picked
    // within the Pick's travel time entry per 10000 picks.
    pPick->dAffPPD = pPick->ttt.dAssocStrength / 2.0;
    if(pPick->dAffPPD > 2.0)
      pPick->dAffPPD = 2.0;
  }
  else
  {
    pPick->dAffPPD = 1.0;
  }

	// Composite affinity statistic
	pPick->dAff = pOrg->dAffGap * pOrg->dAffNumArr * pPick->dAffRes * pPick->dAffDis * pPick->dAffPPD;

  return(0);
}  // end CalcPickAffinity()


void OPCalc_SetTravelTimePointer(ITravelTime * pTravTime)
{
  pTT = pTravTime;
}


int OPCalc_CalculateOriginPickParams(PICK * pPick, ORIGIN * pOrg)
{
  TTEntry   ttt;
  TTEntry * pttt;
  double    dDistKM;

  if(!(pTT && pPick && pOrg))
  {
		CDebug::Log(DEBUG_MINOR_ERROR,"OPCalc_CalculateOriginPickParams(): Null Pointer: "
                                  "pPick(%u) pOrg(%u) pTT(%u)\n", 
                pPick, pOrg, pTT);
    return(-1);
  }

  if(pPick->dLat < -90.0 || pPick->dLat > 90.0 || pPick->dLon < -180.0 || pPick->dLon > 180.0)
    return(-1);

	geo_to_km_deg(pOrg->dLat, pOrg->dLon, pPick->dLat, pPick->dLon, 
                &dDistKM, &pPick->dDelta, &pPick->dAzm);
	pttt = pTT->TBest(pOrg->dZ, pPick->dDelta, pPick->dT - pOrg->dT, &ttt);
	if(!pttt) 
  {
    pPick->bTTTIsValid = false;
    pPick->tRes = 9999.9;
    pPick->ttt.szPhase = Phases[PHASE_Unknown].szName;

    return(1);
  }

	pPick->dTrav = pttt->dTPhase;
	pPick->dToa = pttt->dTOA;
	pPick->tRes = (pPick->dT - pOrg->dT) - pttt->dTPhase;
  strcpy(pPick->sPhase, pttt->szPhase);
  memcpy(&pPick->ttt, pttt, sizeof(TTEntry));
  pPick->bTTTIsValid = true;
  CDebug::Log(DEBUG_MINOR_INFO,"Iterate(): Eq[%d] d:%.2f z:%.2f tobs:%.2f azm:%.2f ttt:%d\n", 
              pOrg->nEq, pPick->dDelta, pOrg->dZ, pPick->dT - pOrg->dT, pPick->dAzm, pttt);

  return(0);
}  // end OPCalc_CalculateOriginPickParams()


