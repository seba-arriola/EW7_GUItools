/*
 * getsubnet.c: Get the subnet and network trigger length
 *              1) Count the stations triggered in each subnet
 *              2) Determine subnet and network trigger status.
 *              3) Compute trigger duration
 */

/*******                                                        *********/
/*      Functions defined in this source file                           */
/*******                                                        *********/

/*      Function: getSubnet                                             */
/*                                                                      */
/*      Inputs:         pointer to the Network structure                */
/*                                                                      */
/*      Outputs:        triggered subnets are flagged                   */
/*                                                                      */
/*      Returns:        current network trigger duration                */
/*                      0 if network is not triggered                   */

/*******                                                        *********/
/*      System Includes                                                 */
/*******                                                        *********/
#include <stdio.h>

/*******                                                        *********/
/*      Earthworm Includes                                              */
/*******                                                        *********/
#include <earthworm.h>  /* logit                                        */

/*******                                                        *********/
/*      CarlSubTrig Includes                                            */
/*******                                                        *********/
#include "carlsubtrig.h"

/*******                                                        *********/
/*      Functions referenced in this source file                        */
/*******                                                        *********/

/*******                                                        *********/
/*      Function definitions                                            */
/*******                                                        *********/

/*      Function: GetSubnet                                             */
long GetSubnet( NETWORK *csuNet )
{
  STATION *station = csuNet->stations;
  SUBNET *subnet = csuNet->subnets;
  int active = 0;               /* Network trigger status               */
                                /* 0: not triggered                   	*/
                                /* 1: network triggered                 */
  int subStasTriggered, numSub;
  int isub, jsta;               /* array counters                       */
    
  numSub = 0;
  for ( isub = 0; isub < csuNet->nSub; isub++ )
  {
    subStasTriggered = 0;

    for ( jsta = 0; jsta < subnet[isub].nStas; jsta++ )
    {
		/* Changed by Eugene Lublinsky, 3/31/Y2K */
		/* triggerable flag added */
		if ( subnet[isub].triggerable[jsta] && (station[subnet[isub].stations[jsta]].countDown > 0) )
			subStasTriggered++;     /* station is triggerable for that subnet and is currently triggered */
    }

    if ( subStasTriggered >= subnet[isub].minToTrigger )
    {   /*  This subnet is active                               */
      subnet[isub].Triggered = 1;       /* reset when network triggers off */
      active = 1;               /* so the network is active             */
      numSub++;
    }
  }
  
  if ( active )
  {
    if ( numSub > csuNet->numSub ) csuNet->numSub = numSub;
    return ( csuNet->NetTrigDur + numSub * csuNet->subnetContrib );
  }
  else
    return ( 0 );
}
