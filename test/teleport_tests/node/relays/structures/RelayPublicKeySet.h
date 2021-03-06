#ifndef TELEPORT_RELAYPUBLICKEYSET_H
#define TELEPORT_RELAYPUBLICKEYSET_H


#include <src/crypto/point.h>
#include <test/teleport_tests/teleport_data/DataStore.h>

class Relay;

class RelayPublicKeySet
{
public:
    std::vector<std::vector<Point> > key_points;

    IMPLEMENT_SERIALIZE
    (
        READWRITE(key_points);
    );

    JSON(key_points);

    HASH160();

    std::vector<Point> PublicKeyTwentyFourths();

    void Populate(MemoryDataStore &keydata);

    void PopulateFirstElementOfEachRow(MemoryDataStore &keydata);

    void PopulateEachRowUsingTheSecretCorrespondingToTheFirstElement(MemoryDataStore &keydata);

    bool VerifyGeneratedPoints(MemoryDataStore &keydata);

    std::vector<Point> GenerateRowOfPointsAndStoreCorrespondingSecretParts(Point point, MemoryDataStore &keydata);

    Point GenerateReceivingPublicKey(Point point_corresponding_to_secret);

    CBigNum GenerateReceivingPrivateKey(Point point_corresponding_to_secret, MemoryDataStore &keydata);

    CBigNum Encrypt(CBigNum secret);

    CBigNum Decrypt(CBigNum encrypted_secret, MemoryDataStore &keydata, Point point_corresponding_to_secret);

    bool VerifyRowOfGeneratedPoints(uint64_t row_number, MemoryDataStore &keydata);

    CBigNum GenerateReceivingPrivateKeyQuarter(Point point_corresponding_to_secret, uint8_t key_quarter_position,
                                               MemoryDataStore &keydata);

    Point GenerateReceivingPublicKeyQuarter(Point point_corresponding_to_secret, uint8_t key_quarter_position);

    bool ValidateSizes();
};


#endif //TELEPORT_RELAYPUBLICKEYSET_H
