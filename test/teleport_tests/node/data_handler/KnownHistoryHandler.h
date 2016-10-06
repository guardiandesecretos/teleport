
#ifndef TELEPORT_KNOWNHISTORYHANDLER_H
#define TELEPORT_KNOWNHISTORYHANDLER_H

#include <test/teleport_tests/node/DiurnFailureDetails.h>
#include "DataMessageHandler.h"


class KnownHistoryHandler
{
public:
    MemoryDataStore &msgdata, &creditdata;
    DataMessageHandler *data_message_handler;
    CreditSystem *credit_system;

    KnownHistoryHandler(DataMessageHandler *data_message_handler_) :
        data_message_handler(data_message_handler_),
        msgdata(data_message_handler_->msgdata),
        creditdata(data_message_handler_->creditdata),
        credit_system(data_message_handler_->credit_system)
    { }

    void HandleKnownHistoryMessage(KnownHistoryMessage known_history);

    KnownHistoryMessage GenerateKnownHistoryMessage();

    bool ValidateKnownHistoryMessage(KnownHistoryMessage known_history_message);

    void HandleKnownHistoryRequest(KnownHistoryRequest request);

    uint160 RequestKnownHistoryMessages(uint160 mined_credit_message_hash);

    uint160 RequestDiurnData(KnownHistoryMessage msg_hash, std::vector<uint32_t> requested_diurns, CNode *peer);

    void HandleDiurnDataMessage(DiurnDataMessage diurn_data_message);

    void HandlerDiurnDataRequest(DiurnDataRequest request);

    void RecordDiurnsWhichPeerKnows(KnownHistoryMessage known_history);

    bool CheckIfDiurnDataMatchesHashes(DiurnDataMessage diurn_data, KnownHistoryMessage history_message,
                                       DiurnDataRequest diurn_data_request);

    bool ValidateDataInDiurnDataMessage(DiurnDataMessage diurn_data_message);

    void TrimLastDiurnFromCalendar(Calendar &calendar, CreditSystem *credit_system);

    bool CheckSizesInDiurnDataMessage(DiurnDataMessage diurn_data_message);

    bool CheckForFailuresInProofsOfWorkInDiurn(Diurn &diurn, DiurnFailureDetails &failure_details,
                                               uint64_t scrutiny_time);
};


#endif //TELEPORT_KNOWNHISTORYHANDLER_H
