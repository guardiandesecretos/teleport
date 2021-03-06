#ifndef TELEPORT_CALEND_H
#define TELEPORT_CALEND_H


#include "test/teleport_tests/node/credit/messages/MinedCreditMessage.h"
#include "test/teleport_tests/node/historical_data/messages/DiurnMessageData.h"

class CreditSystem;

class Calend : public MinedCreditMessage
{
public:
    Calend () {}

    Calend(MinedCreditMessage msg);

    uint160 DiurnRoot() const;

    uint160 TotalCreditWork();

    DiurnMessageData GenerateDiurnMessageData(CreditSystem *credit_system);
};


#endif //TELEPORT_CALEND_H
