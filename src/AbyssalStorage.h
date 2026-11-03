#ifndef ABYSSAL_STORAGE_H
#define ABYSSAL_STORAGE_H

#include "DatabaseEnvFwd.h"
#include "DataMap.h"
#include "Define.h"
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

class Player;
struct ItemTemplate;

struct PendingDeposit
{
    uint32 itemEntry;
    uint32 count;
};

// Per-player transient state stored via DataMap
struct AbyssalPlayerData : public DataMap::Base
{
    bool autoStoreEnabled = true;
    bool isMaterializing = false; // true while materializing items (suppress auto-deposit)
    // item GUID -> how many of its count came from the vault (a new stack can merge into one the player had)
    std::unordered_map<uint32, uint32> materializedItems;
    std::vector<PendingDeposit> pendingDeposits; // deferred auto-deposits
    uint32 pendingCrafts = 0;    // remaining crafts in a multi-craft batch
    uint32 pendingSpellId = 0;   // spell ID for multi-craft batch
};

AbyssalPlayerData* GetAbyssalData(Player* player);
AbyssalPlayerData* FindAbyssalData(Player* player); // nullptr if the player never used the vault

class AbyssalStorageMgr
{
public:
    static AbyssalStorageMgr* instance();

    // Enabled, and not a random bot (random bots don't need a vault)
    bool IsVaultUser(Player* player);

    void LoadAccountData(uint32 accountId);
    bool IsAccountLoaded(uint32 accountId);

    // Vault rows are written into trans; commit it with CommitWithInventory() so the vault and
    // the player's bags are saved together
    void DepositItem(uint32 accountId, uint32 itemEntry, uint32 count, CharacterDatabaseTransaction trans);
    bool WithdrawItem(uint32 accountId, uint32 itemEntry, uint32 count, CharacterDatabaseTransaction trans);
    void CommitWithInventory(Player* player, CharacterDatabaseTransaction trans);
    uint32 GetItemCount(uint32 accountId, uint32 itemEntry);
    std::unordered_map<uint32, uint32> GetAllItems(uint32 accountId);

    bool IsVaultable(ItemTemplate const* itemTemplate) const;
    bool ShouldAutoStore(Player* player, ItemTemplate const* itemTemplate);
    bool IsItemRequiredByActiveQuest(Player* player, uint32 itemId);
    uint32 GetQuestReservedCount(Player* player, uint32 itemId);

    // Messaging helpers
    void SendAddonMessage(Player* player, std::string const& message);
    void SendFullSync(Player* player);
    void SendItemUpdate(Player* player, uint32 itemEntry, uint32 count);
    void SendItemDelete(Player* player, uint32 itemEntry);

    bool IsEnabled() const { return _enabled; }
    void SetEnabled(bool enabled) { _enabled = enabled; }

private:
    AbyssalStorageMgr() = default;

    // accountId -> (itemEntry -> count). Accounts stay cached until shutdown: several characters of
    // one account (the player and their altbots) can be online at once, and all share this copy.
    std::unordered_map<uint32, std::unordered_map<uint32, uint32>> _storage;
    std::mutex _storageMutex;
    bool _enabled = true;
};

#define sAbyssalStorageMgr AbyssalStorageMgr::instance()

#endif // ABYSSAL_STORAGE_H
