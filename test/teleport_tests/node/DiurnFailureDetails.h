#ifndef TELEPORT_DIURNFAILUREDETAILS_H
#define TELEPORT_DIURNFAILUREDETAILS_H


#include <src/crypto/uint256.h>
#include <src/mining/work.h>

class DiurnFailureDetails
{
public:
    uint160 calend_hash{0};
    uint160 mined_credit_message_hash{0};
    TwistWorkCheck check;

    DiurnFailureDetails() { }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(calend_hash);
        READWRITE(mined_credit_message_hash);
        READWRITE(check);
    )

    JSON(calend_hash, mined_credit_message_hash, check);

    DEPENDENCIES(calend_hash, mined_credit_message_hash);
};


#endif //TELEPORT_DIURNFAILUREDETAILS_H
