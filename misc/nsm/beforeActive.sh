#!/bin/bash
#
# This script is called before an instance transitions
# to active as a final confirmation that the instance
# should go active.
#
# Return "1" to confirm activation, anything else to
# abort activation and return to an idle state.  
#
# In addition to denying the transition, the resource will
# also be 'stunned' for 5 * the state transition time defined
# in the configuration.  For example: if the configured
# transition time is 5 seconds, the resource will be forced
# into a idle state for at least 25 seconds before it will
# again be considered for potential transition.

echo "1"
