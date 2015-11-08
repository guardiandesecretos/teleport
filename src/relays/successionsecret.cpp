#include "flexnode/flexnode.h"


#include "log.h"
#define LOG_CATEGORY "successionsecret.h"


/*********************************
 *  DistributedSuccessionSecret
 */

    std::vector<Point> DistributedSuccessionSecret::Relays()
    {
        if (relay_number < INHERITANCE_START)
            return std::vector<Point>();

        if (relays.size() == 0)
        {
            relays = flexnode.RelayState().Executors(relay_number);
        }
        return relays;
    }

/*
 *  DistributedSuccessionSecret
 *********************************/