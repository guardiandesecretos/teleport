#include <src/credits/CreditInBatch.h>
#include <src/crypto/point.h>
#include <src/crypto/secp256k1point.h>
#include <src/vector_tools.h>
#include <src/credits/SignedTransaction.h>
#include <src/credits/creditsign.h>
#include <test/teleport_tests/currency/CryptoCurrencyAddress.h>
#include "Wallet.h"
#include "test/teleport_tests/node/credit/messages/MinedCreditMessage.h"
#include "test/teleport_tests/node/credit/structures/CreditSystem.h"
#include "test/teleport_tests/node/credit/structures/CreditTracker.h"

#include "log.h"
#define LOG_CATEGORY "Wallet.cpp"

void Wallet::SetCreditSystem(CreditSystem *credit_system_)
{
    credit_system = credit_system_;
}

void Wallet::SetCalendar(Calendar *calendar_)
{
    calendar = calendar_;
}

void Wallet::SetSpentChain(BitChain *spent_chain_)
{
    spent_chain = spent_chain_;
}

std::vector<CreditInBatch> Wallet::GetCredits()
{
    return credits;
}

Point Wallet::GetNewPublicKey()
{
    CBigNum private_key;
    private_key.Randomize(Secp256k1Point::Modulus());
    Point public_key(SECP256K1, private_key);
    keydata[public_key]["privkey"] = private_key;
    uint160 key_hash = KeyHash(public_key);
    keydata[key_hash]["pubkey"] = public_key;
    return public_key;
}

void Wallet::HandleCreditInBatch(CreditInBatch credit_in_batch)
{
    if (PrivateKeyIsKnown(credit_in_batch) and not HaveCreditInBatchAlready(credit_in_batch))
    {
        credits.push_back(credit_in_batch);
    }
}

bool Wallet::PrivateKeyIsKnown(CreditInBatch credit_in_batch)
{
    return PrivateKeyIsKnown(credit_in_batch.public_key);
}

bool Wallet::PrivateKeyIsKnown(Point public_key)
{
    return keydata[public_key].HasProperty("privkey");
}

bool Wallet::HaveCreditInBatchAlready(CreditInBatch credit_in_batch)
{
    return VectorContainsEntry(credits, credit_in_batch);
}

SignedTransaction Wallet::GetSignedTransaction(Point pubkey, uint64_t amount)
{
    log_ << "getting signed transaction paying to " << pubkey << "\n";
    return SignTransaction(GetUnsignedTransaction(pubkey, amount), keydata);
}

UnsignedTransaction Wallet::GetUnsignedTransaction(Point pubkey, uint64_t amount)
{
    UnsignedTransaction raw_tx;
    uint64_t amount_in = 0;

    for (auto credit : credits)
    {
        if (credit_system != NULL)
            credit_system->AddDiurnBranchToCreditInBatch(credit);

        raw_tx.inputs.push_back(credit);
        amount_in += credit.amount;
        if (amount_in >= amount)
            break;
    }

    log_ << "tx inputs: " << raw_tx.inputs << "\n";

    if (amount > amount_in)
    {
        raw_tx.inputs.resize(0);
        return raw_tx;
    }

    raw_tx.AddOutput(Credit(pubkey, amount));
    log_ << "GetUnsignedTransaction: pubkey of output is " << raw_tx.outputs.back().public_key << "\n";

    int64_t change = amount_in - amount;
    if (change > 0)
    {
        Point change_pubkey = GetNewPublicKey();
        raw_tx.outputs.push_back(Credit(change_pubkey, (uint64_t) change));
    }
    return raw_tx;
}

void Wallet::AddBatchToTip(MinedCreditMessage msg, CreditSystem* credit_system)
{
    msg.hash_list.RecoverFullHashes(credit_system->msgdata);
    RemoveTransactionInputsSpentInBatchFromCredits(msg, credit_system);
    AddNewCreditsFromBatch(msg, credit_system);
    SortCredits();
}

void Wallet::SortCredits()
{
    std::sort(credits.begin(), credits.end());
}

void Wallet::RemoveTransactionInputsSpentInBatchFromCredits(MinedCreditMessage& msg, CreditSystem* credit_system)
{
    for (auto hash : msg.hash_list.full_hashes)
        if (credit_system->MessageType(hash) == "tx")
        {
            SignedTransaction tx = credit_system->msgdata[hash]["tx"];
            for (auto &input : tx.rawtx.inputs)
            {
                if (VectorContainsEntry(credits, input))
                {
                    EraseEntryFromVector(input, credits);
                }
            }
        }
}

void Wallet::AddNewCreditsFromBatch(MinedCreditMessage& msg, CreditSystem* credit_system)
{
    CreditBatch batch = credit_system->ReconstructBatch(msg);
    log_ << "credit batch is " << PrettyPrint(batch) << "\n";
    for (auto &credit : batch.credits)
    {
        auto credit_in_batch = batch.GetCreditInBatch(credit);
        if (PrivateKeyIsKnown(credit_in_batch) and not VectorContainsEntry(credits, credit_in_batch))
        {
            log_ << "adding " << PrettyPrint(credit_in_batch) << "\n";
            credits.push_back(credit_in_batch);
        }
    }
    SortCredits();
}

uint64_t Wallet::Balance()
{
    uint64_t balance = 0;
    for (auto credit : credits)
        balance += credit.amount;
    return balance;
}

void Wallet::RemoveBatchFromTip(MinedCreditMessage msg, CreditSystem *credit_system)
{
    msg.hash_list.RecoverFullHashes(credit_system->msgdata);
    AddTransactionInputsSpentInRemovedBatchToCredits(msg, credit_system);
    RemoveCreditsFromRemovedBatch(msg, credit_system);
    std::sort(credits.begin(), credits.end());
}

void Wallet::AddTransactionInputsSpentInRemovedBatchToCredits(MinedCreditMessage& msg, CreditSystem* credit_system)
{
    for (auto hash : msg.hash_list.full_hashes)
        if (credit_system->MessageType(hash) == "tx")
        {
            SignedTransaction tx = credit_system->msgdata[hash]["tx"];
            for (auto &input : tx.rawtx.inputs)
                if (PrivateKeyIsKnown(input) and not VectorContainsEntry(credits, input))
                    credits.push_back(input);
        }
}

void Wallet::RemoveCreditsFromRemovedBatch(MinedCreditMessage& msg, CreditSystem* credit_system)
{
    CreditBatch batch = credit_system->ReconstructBatch(msg);
    for (auto &credit : batch.credits)
    {
        auto credit_in_batch = batch.GetCreditInBatch(credit);
        if (VectorContainsEntry(credits, credit_in_batch))
            EraseEntryFromVector(credit_in_batch, credits);
    }
}

void Wallet::SwitchAcrossFork(uint160 old_tip, uint160 new_tip, CreditSystem *credit_system)
{
    uint160 fork = credit_system->FindFork(old_tip, new_tip);
    while (old_tip != fork)
    {
        MinedCreditMessage msg = credit_system->msgdata[old_tip]["msg"];
        RemoveBatchFromTip(msg, credit_system);
        old_tip = msg.mined_credit.network_state.previous_mined_credit_message_hash;
    }
    std::vector<MinedCreditMessage> new_branch;
    while (new_tip != fork)
    {
        MinedCreditMessage msg = credit_system->msgdata[new_tip]["msg"];
        new_branch.push_back(msg);
        new_tip = msg.mined_credit.network_state.previous_mined_credit_message_hash;
    }
    std::reverse(new_branch.begin(), new_branch.end());
    for (auto &msg : new_branch)
        AddBatchToTip(msg, credit_system);
}

void Wallet::ImportPrivateKey(const CBigNum private_key)
{
    Point public_key(SECP256K1, private_key);
    log_ << "Importing private key for " << public_key << "\n";
    if (credit_system == nullptr)
        return;

    auto credits_paying_to_key = credit_system->tracker.GetCreditsPayingToRecipient(public_key);

    for (auto credit : credits_paying_to_key)
    {
        credit_system->AddDiurnBranchToCreditInBatch(credit);

        if (not calendar->ValidateCreditInBatch(credit))
            continue;
        if (spent_chain->Get(credit.position))
            continue;
        if (HaveCreditInBatchAlready(credit))
            continue;

        credits.push_back(credit);
        log_ << "imported credit: " << credit.json() << "\n";
    }
    SortCredits();
}

uint160 GetKeyHashFromAddress(std::string address_string)
{
    CBitcoinAddress address(address_string);
    CKeyID keyID;
    address.GetKeyID(keyID);
    vch_t hash_bytes(BEGIN(keyID), END(keyID));
    uint160 hash(hash_bytes);
    return hash;
}

std::string GetAddressFromPublicKey(Point public_key)
{
    std::string currency("TCR");
    return CryptoCurrencyAddress(currency, public_key).ToString();
}