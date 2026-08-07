
#
#                    hyp2000_mgr's Configuration File
#
MyModuleId MOD_EQPROC   # Label outgoing messages with this module id.
                        # This program is part of a mega-module which is
                        # started by eqproc or eqprelim.  All child
                        # processes of eqproc/eqprelim should use the
                        # same module id.

RingName   HYPO_RING    # Write output to this transport ring

LogFile    2            # 0=no log; 1=errors; 2=errors & hypocenters

SourceCode W            # Label summary cards with this character to 
                        # identify them as coming from Earthworm
