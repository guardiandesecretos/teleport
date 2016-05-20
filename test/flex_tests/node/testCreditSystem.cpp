#include <test/flex_tests/mined_credits/EncodedNetworkState.h>
#include "NetworkState.h"
#include <src/crypto/point.h>
#include <test/flex_tests/flex_data/MemoryDataStore.h>
#include <src/credits/SignedTransaction.h>
#include <src/base/util_time.h>
#include "gmock/gmock.h"
#include "CreditSystem.h"

using namespace ::testing;
using namespace std;


static uint64_t mined_credit_number{1};

MinedCredit SucceedingMinedCredit(MinedCredit& given_credit)
{
    MinedCredit successor;

    successor.network_state = given_credit.network_state;
    successor.network_state.previous_mined_credit_hash = given_credit.GetHash160();
    successor.network_state.batch_number += 1;
    successor.amount = mined_credit_number++;

    return successor;
}


class ACreditSystem : public Test
{
public:
    MemoryDataStore msgdata, creditdata;
    CreditSystem *credit_system;

    virtual void SetUp()
    {
        credit_system = new CreditSystem(msgdata, creditdata);
    }

    virtual void TearDown()
    {
        delete credit_system;
    }

    vector<MinedCredit> ChainStartingFromGivenMinedCredit(MinedCredit given_credit, uint64_t length)
    {
        vector<MinedCredit> chain_of_credits;
        MinedCredit mined_credit = SucceedingMinedCredit(given_credit);
        chain_of_credits.push_back(mined_credit);

        while (chain_of_credits.size() < length)
            chain_of_credits.push_back(SucceedingMinedCredit(chain_of_credits.back()));

        for (auto credit : chain_of_credits)
            credit_system->StoreMinedCredit(credit);

        return chain_of_credits;
    }

};

TEST_F(ACreditSystem, InitiallyReportsZeroForTheBatchWithTheMostWork)
{
    uint160 credit_hash_of_batch_with_the_most_work = credit_system->MostWorkBatch();
    ASSERT_THAT(credit_hash_of_batch_with_the_most_work, Eq(0));
}

TEST_F(ACreditSystem, ReportsTheCorrectBatchWithTheMostWork)
{
    uint160 hash1(123), hash2(23);

    creditdata[hash1].Location("total_work") = hash1;
    creditdata[hash2].Location("total_work") = hash2;

    uint160 credit_hash_of_batch_with_the_most_work = credit_system->MostWorkBatch();
    ASSERT_THAT(credit_hash_of_batch_with_the_most_work, Eq(hash1));
}

MinedCredit ExampleMinedCredit()
{
    MinedCredit credit;
    credit.amount = 1000;
    credit.keydata = Point(2).getvch();
    return credit;
}

TEST_F(ACreditSystem, StoresMinedCredits)
{
    auto credit = ExampleMinedCredit();

    credit_system->StoreMinedCredit(credit);
    MinedCredit recovered_mined_credit;
    recovered_mined_credit = creditdata[credit.GetHash160()]["mined_credit"];

    ASSERT_THAT(recovered_mined_credit, Eq(credit));;
}

TEST_F(ACreditSystem, RecordsTheTypeAsMinedCreditWhenItStoresAMinedCredit)
{
    auto credit = ExampleMinedCredit();

    credit_system->StoreMinedCredit(credit);
    string type = msgdata[credit.GetHash160()]["type"];

    ASSERT_THAT(type, Eq("mined_credit"));
}

SignedTransaction ExampleTransaction()
{
    SignedTransaction tx;
    tx.rawtx.outputs.push_back(Credit(Point(2).getvch(), 1000));
    return tx;
}

TEST_F(ACreditSystem, StoresTransactions)
{
    auto tx = ExampleTransaction();

    credit_system->StoreTransaction(tx);
    SignedTransaction recovered_transaction;
    recovered_transaction = creditdata[tx.GetHash160()]["tx"];

    ASSERT_THAT(recovered_transaction, Eq(tx));
}

MinedCreditMessage ExampleMinedCreditMessage()
{
    MinedCreditMessage mined_credit_message;
    mined_credit_message.mined_credit.amount = 2;
    return mined_credit_message;
}

TEST_F(ACreditSystem, StoresMinedCreditMessages)
{
    auto msg = ExampleMinedCreditMessage();

    credit_system->StoreMinedCreditMessage(msg);
    MinedCreditMessage recovered_msg;
    recovered_msg = creditdata[msg.GetHash160()]["msg"];

    ASSERT_THAT(recovered_msg, Eq(msg));
}

TEST_F(ACreditSystem, ReconstructsTheBatchRootOfAMinedCreditMessage)
{
    MinedCreditMessage msg;
    msg.mined_credit.network_state.batch_root = CreditBatch().Root();
    CreditBatch batch = credit_system->ReconstructBatch(msg);
    ASSERT_THAT(batch.Root(), Eq(msg.mined_credit.network_state.batch_root));
}

TEST_F(ACreditSystem, ReconstructsTheBatchRootOfAMinedCreditMessageWithAPreviousCreditHash)
{
    MinedCreditMessage msg;
    uint160 previous_mined_credit_hash = 1;
    msg.mined_credit.network_state.previous_mined_credit_hash = previous_mined_credit_hash;
    msg.mined_credit.network_state.batch_root = CreditBatch(previous_mined_credit_hash, 0).Root();
    CreditBatch batch = credit_system->ReconstructBatch(msg);
    ASSERT_THAT(batch.Root(), Eq(msg.mined_credit.network_state.batch_root));
}

class ACreditSystemWithAStoredTransaction : public ACreditSystem
{
public:
    MinedCreditMessage mined_credit_message;
    SignedTransaction tx;
    uint160 tx_hash;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        tx = ExampleTransaction();
        credit_system->StoreTransaction(tx);
        tx_hash = tx.GetHash160();
        mined_credit_message.hash_list.full_hashes.push_back(tx_hash);
        mined_credit_message.hash_list.GenerateShortHashes();
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithAStoredTransaction, CanRecoverTheFullHashesFromAMinedCreditMessage)
{
    ASSERT_TRUE(mined_credit_message.hash_list.RecoverFullHashes(msgdata));
}

TEST_F(ACreditSystemWithAStoredTransaction, CanReconstructTheCreditBatchFromAMinedCreditMessage)
{
    CreditBatch reconstructed_credit_batch = credit_system->ReconstructBatch(mined_credit_message);
    ASSERT_THAT(reconstructed_credit_batch.size(), Eq(1));
    ASSERT_THAT(reconstructed_credit_batch.credits[0], Eq(tx.rawtx.outputs[0]));
}

class ACreditSystemWithAStoredMinedCredit : public ACreditSystem
{
public:
    MinedCreditMessage mined_credit_message;
    MinedCredit mined_credit;
    uint160 credit_hash;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        mined_credit = ExampleMinedCredit();
        credit_system->StoreMinedCredit(mined_credit);
        credit_hash = mined_credit.GetHash160();
        mined_credit_message.hash_list.full_hashes.push_back(credit_hash);
        mined_credit_message.hash_list.GenerateShortHashes();
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithAStoredMinedCredit, CanReconstructTheCreditBatchFromAMinedCreditMessage)
{
    CreditBatch reconstructed_credit_batch = credit_system->ReconstructBatch(mined_credit_message);
    ASSERT_THAT(reconstructed_credit_batch.size(), Eq(1));
    ASSERT_THAT(reconstructed_credit_batch.credits[0], Eq((Credit)mined_credit));
}

class ACreditSystemWithStoredMinedCredits : public ACreditSystem
{
public:
    MinedCredit credit1, credit2, credit3, credit4, credit5;
    uint160 credit5_hash, credit4_hash, credit2_hash;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        credit2 = SucceedingMinedCredit(credit1);
        credit3 = SucceedingMinedCredit(credit2);
        credit4 = SucceedingMinedCredit(credit3);
        credit5 = SucceedingMinedCredit(credit2);

        credit_system->StoreMinedCredit(credit1);
        credit_system->StoreMinedCredit(credit2);
        credit_system->StoreMinedCredit(credit3);
        credit_system->StoreMinedCredit(credit4);
        credit_system->StoreMinedCredit(credit5);

        credit5_hash = credit5.GetHash160();
        credit4_hash = credit4.GetHash160();
        credit2_hash = credit2.GetHash160();
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithStoredMinedCredits, CanFindAFork)
{
    uint160 fork = credit_system->FindFork(credit4_hash, credit5_hash);
    ASSERT_THAT(fork, Eq(credit2_hash));
}


class ACreditSystemWithABigFork : public ACreditSystem
{
public:
    vector<MinedCredit> common_part, branch1, branch2;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        uint64_t length_of_common_part = 50;
        uint64_t length_of_branch1 = 20;
        uint64_t length_of_branch2 = 30;

        MinedCredit starting_credit;
        credit_system->StoreMinedCredit(starting_credit);
        common_part = ChainStartingFromGivenMinedCredit(starting_credit, length_of_common_part);
        branch1 = ChainStartingFromGivenMinedCredit(common_part.back(), length_of_branch1);
        branch2 = ChainStartingFromGivenMinedCredit(common_part.back(), length_of_branch2);
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithABigFork, CanFindTheFork)
{
    uint160 hash1 = branch1.back().GetHash160();
    uint160 hash2 = branch2.back().GetHash160();

    uint160 fork = credit_system->FindFork(hash1, hash2);

    ASSERT_THAT(fork, Eq(common_part.back().GetHash160()));
}


class ACreditSystemWithTwoNonIntersectingChains : public ACreditSystem
{
public:
    vector<MinedCredit> chain1, chain2;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();

        MinedCredit starting_credit1, starting_credit2;

        starting_credit1.amount = 1;
        starting_credit2.amount = 2;

        credit_system->StoreMinedCredit(starting_credit1);
        credit_system->StoreMinedCredit(starting_credit2);

        chain1 = ChainStartingFromGivenMinedCredit(starting_credit1, 10);
        chain2 = ChainStartingFromGivenMinedCredit(starting_credit2, 15);
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithTwoNonIntersectingChains, GivesZeroForTheFork)
{
    uint160 fork = credit_system->FindFork(chain1.back().GetHash160(), chain2.back().GetHash160());
    ASSERT_THAT(fork, Eq(0));
}

class ACreditSystemWithTransactions : public ACreditSystem
{
public:
    SignedTransaction tx1, tx2;

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        CreditInBatch input1, input2;
        input1.position = 1;
        input2.position = 2;

        tx1.rawtx.inputs.push_back(input1);
        tx2.rawtx.inputs.push_back(input2);

        credit_system->StoreTransaction(tx1);
        credit_system->StoreTransaction(tx2);
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

MinedCreditMessage MinedCreditMessageContaining(SignedTransaction tx)
{
    MinedCreditMessage msg;
    msg.hash_list.full_hashes.push_back(tx.GetHash160());
    msg.hash_list.GenerateShortHashes();
    return msg;
}

TEST_F(ACreditSystemWithTransactions, DeterminesTheCreditsSpentBetweenOneMinedCreditAndAnother)
{
    auto msg1 = MinedCreditMessageContaining(tx1);
    auto msg2 = MinedCreditMessageContaining(tx2);

    msg2.mined_credit = SucceedingMinedCredit(msg1.mined_credit);

    credit_system->StoreMinedCreditMessage(msg1);
    credit_system->StoreMinedCreditMessage(msg2);

    uint160 credit_hash1 = msg1.mined_credit.GetHash160();
    uint160 credit_hash2 = msg2.mined_credit.GetHash160();

    auto spent = credit_system->GetPositionsOfCreditsSpentBetween(credit_hash1, credit_hash2);

    ASSERT_FALSE(spent.count(1));
    ASSERT_TRUE(spent.count(2));
}

class ACreditSystemWithAFork : public ACreditSystem
{
public:
    vector<MinedCreditMessage> common_part;
    vector<MinedCreditMessage> branch1;
    vector<MinedCreditMessage> branch2;
    vector<SignedTransaction> transactions;
    uint64_t number_of_credits{0};
    uint64_t number_of_mined_credits{0};

    MinedCredit SucceedingMinedCredit(MinedCredit mined_credit)
    {
        MinedCredit succeeding_mined_credit;
        succeeding_mined_credit.network_state.previous_mined_credit_hash = mined_credit.GetHash160();
        succeeding_mined_credit.network_state.batch_number = mined_credit.network_state.batch_number;
        succeeding_mined_credit.amount = number_of_mined_credits++; // prevent exact duplicates
        return succeeding_mined_credit;
    }

    SignedTransaction TransactionWithOneInput()
    {
        SignedTransaction tx;
        CreditInBatch credit_in_batch;
        credit_in_batch.position = number_of_credits++;
        tx.rawtx.inputs.push_back(credit_in_batch);
        transactions.push_back(tx);
        return tx;
    }

    vector<MinedCreditMessage> ChainOfMinedCreditMessagesStartingFrom(MinedCreditMessage msg, uint64_t length)
    {
        vector<MinedCreditMessage> chain;
        while (chain.size() < length)
        {
            auto previous_mined_credit = chain.size() > 0 ? chain.back().mined_credit : msg.mined_credit;
            MinedCreditMessage next_msg;
            next_msg.mined_credit = SucceedingMinedCredit(previous_mined_credit);
            auto next_tx = TransactionWithOneInput();
            credit_system->StoreTransaction(next_tx);
            next_msg.hash_list.full_hashes.push_back(next_tx.GetHash160());
            next_msg.mined_credit.network_state.batch_size = 1;
            next_msg.mined_credit.network_state.batch_offset = previous_mined_credit.network_state.batch_offset
                                                                + previous_mined_credit.network_state.batch_size;
            next_msg.hash_list.GenerateShortHashes();
            credit_system->StoreMinedCreditMessage(next_msg);
            chain.push_back(next_msg);
        }
        return chain;
    }

    void SetBatchOffset(MinedCreditMessage& msg, uint32_t offset)
    {
        msg.mined_credit.network_state.batch_offset = offset;
        credit_system->StoreMinedCreditMessage(msg);
    }

    virtual void SetUp()
    {
        ACreditSystem::SetUp();
        MinedCreditMessage start;
        SetBatchOffset(start, 5);

        common_part = ChainOfMinedCreditMessagesStartingFrom(start, 5); // spends 0, 1, 2, 3, 4
        branch1 = ChainOfMinedCreditMessagesStartingFrom(common_part.back(), 5); // spends 5, 6, 7, 8, 9
        branch2 = ChainOfMinedCreditMessagesStartingFrom(common_part.back(), 5); // spends 10, 11, 12, 13, 14
    }

    virtual void TearDown()
    {
        ACreditSystem::TearDown();
    }
};

TEST_F(ACreditSystemWithAFork, DeterminesTheCreditsSpentAndUnspentWhenSwitchingAcrossTheFork)
{
    set<uint64_t> spent, unspent;
    auto hash1 = branch1.back().mined_credit.GetHash160();
    auto hash2 = branch2.back().mined_credit.GetHash160();

    bool succeeded = credit_system->GetSpentAndUnspentWhenSwitchingAcrossFork(hash1, hash2, spent, unspent);

    ASSERT_THAT(succeeded, Eq(true));
    ASSERT_THAT(unspent, Eq(set<uint64_t>{5, 6, 7, 8, 9}));
    ASSERT_THAT(spent, Eq(set<uint64_t>{10, 11, 12, 13, 14}));
}

TEST_F(ACreditSystemWithAFork, GetsTheSpentChainOfOneProngStartingFromTheSpentChainOfTheOther)
{
    BitChain spent_chain1, spent_chain2;
    auto hash1 = branch1.back().mined_credit.GetHash160();
    auto hash2 = branch2.back().mined_credit.GetHash160();

    for (uint32_t i = 0; i < 15; i++)
    {
        spent_chain1.Add();
        spent_chain2.Add();
        if (i < 5 or i >= 10)
            spent_chain2.Set(i);
        if (i < 10)
            spent_chain1.Set(i);
    }

    BitChain recovered_spent_chain2 = credit_system->GetSpentChainOnOtherProngOfFork(spent_chain1, hash1, hash2);
    ASSERT_THAT(recovered_spent_chain2, Eq(spent_chain2));
    ASSERT_FALSE(recovered_spent_chain2 == spent_chain1);
}

TEST_F(ACreditSystemWithAFork, GetsTheSpentChainOfAGivenCreditHashByReplayingTransactionsAndCredits)
{
    auto hash1 = branch1.back().mined_credit.GetHash160();
    BitChain empty_spent_chain;
    BitChain original_spent_chain = credit_system->GetSpentChainOnOtherProngOfFork(empty_spent_chain, 0, hash1);

    BitChain recovered_spent_chain = credit_system->GetSpentChain(hash1);
    ASSERT_THAT(recovered_spent_chain, Eq(original_spent_chain));
}
