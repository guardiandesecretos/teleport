#ifndef FLEX_INITIALDATAHANDLER_H
#define FLEX_INITIALDATAHANDLER_H

#include "DataMessageHandler.h"


class InitialDataHandler
{
public:
    MemoryDataStore &msgdata, &creditdata;
    DataMessageHandler *data_message_handler;

    uint160 initial_difficulty{INITIAL_DIFFICULTY};
    uint160 initial_diurnal_difficulty{INITIAL_DIURNAL_DIFFICULTY};

    InitialDataHandler(DataMessageHandler *data_message_handler_) :
        data_message_handler(data_message_handler_),
        msgdata(data_message_handler_->msgdata),
        creditdata(data_message_handler_->creditdata)
    { }


    void HandleInitialDataRequestMessage(InitialDataRequestMessage request);

    void HandleInitialDataMessage(InitialDataMessage request);


    bool CheckSpentChainInInitialDataMessage(InitialDataMessage message);

    bool EnclosedMessagesArePresentInInitialDataMessage(InitialDataMessage &message);

    MemoryDataStore GetEnclosedMessageHashes(InitialDataMessage &message);

    bool InitialDataMessageMatchesRequestedCalendar(InitialDataMessage &initial_data_message);

    Calendar GetRequestedCalendar(InitialDataMessage &initial_data_message);

    bool InitialDataMessageMatchesCalendar(InitialDataMessage &message, Calendar calendar);

    bool SequenceOfMinedCreditMessagesIsValidAndHasValidProofsOfWork(std::vector<MinedCreditMessage> vector);

    bool SequenceOfMinedCreditMessagesIsValid(std::vector<MinedCreditMessage> msgs);

    bool SequenceOfMinedCreditMessagesHasValidProofsOfWork(std::vector<MinedCreditMessage> msgs);

    bool ValidateMinedCreditMessagesInInitialDataMessage(InitialDataMessage initial_data_message);

    void StoreDataFromInitialDataMessageInCreditSystem(InitialDataMessage &initial_data_message,
                                                       CreditSystem &credit_system);

    void TrimLastDiurnFromCalendar(Calendar &calendar, CreditSystem *credit_system);

    void SetMiningParametersForInitialDataMessageValidation(uint64_t number_of_megabytes, uint160 initial_difficulty,
                                                            uint160 initial_diurnal_difficulty);

    bool TipOfCalendarMatchesTipOfInitialDataMessage(Calendar &calendar, InitialDataMessage &message);

    void StoreMessageInCreditSystem(std::string type, vch_t content, CreditSystem &credit_system);

    bool ValidateInitialDataMessage(InitialDataMessage initial_data_message);

    void UseInitialDataMessageAndCalendar(InitialDataMessage initial_data_message, Calendar new_calendar);

    void MarkMinedCreditMessagesInInitialDataMessageAsValidated(InitialDataMessage initial_data_message);

};


#endif //FLEX_INITIALDATAHANDLER_H