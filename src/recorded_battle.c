#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "recorded_battle.h"
#include "main.h"
#include "pokemon.h"
#include "random.h"
#include "event_data.h"
#include "link.h"
#include "string_util.h"
#include "palette.h"
#include "save.h"
#include "alloc.h"
#include "util.h"
#include "task.h"
#include "text.h"
#include "battle_setup.h"
#include "frontier_util.h"
#include "constants/trainers.h"
#include "constants/rgb.h"

#define BATTLER_RECORD_SIZE 664
#define ILLEGAL_BATTLE_TYPES ((BATTLE_TYPE_LINK | BATTLE_TYPE_SAFARI | BATTLE_TYPE_FIRST_BATTLE                  \
                              | BATTLE_TYPE_WALLY_TUTORIAL | BATTLE_TYPE_ROAMER | BATTLE_TYPE_EREADER_TRAINER    \
                              | BATTLE_TYPE_KYOGRE_GROUDON | BATTLE_TYPE_LEGENDARY | BATTLE_TYPE_REGI            \
                              | BATTLE_TYPE_RECORDED | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SECRET_BASE        \
                              | BATTLE_TYPE_GROUDON | BATTLE_TYPE_KYOGRE | BATTLE_TYPE_RAYQUAZA))

struct PlayerInfo
{
    u32 trainerId;
    u8 name[PLAYER_NAME_LENGTH + 1];
    u8 gender;
    u16 battlerId;
    u16 language;
};

struct MovePp
{
    u16 moves[MAX_MON_MOVES];
    u8 pp[MAX_MON_MOVES];
};

struct RecordedBattleSave
{
    struct Pokemon playerParty[PARTY_SIZE];
    struct Pokemon opponentParty[PARTY_SIZE];
    u8 playersName[MAX_BATTLERS_COUNT][PLAYER_NAME_LENGTH + 1];
    u8 playersGender[MAX_BATTLERS_COUNT];
    u32 playersTrainerId[MAX_BATTLERS_COUNT];
    u8 playersLanguage[MAX_BATTLERS_COUNT];
    u32 rngSeed;
    u32 battleFlags;
    u8 playersBattlers[MAX_BATTLERS_COUNT];
    u16 opponentA;
    u16 opponentB;
    u16 partnerId;
    u16 field_4FA;
    u8 lvlMode;
    u8 frontierFacility;
    u8 frontierBrainSymbol;
    u8 battleScene:1;
    u8 textSpeed:3;
    u32 AI_scripts;
    u8 recordMixFriendName[PLAYER_NAME_LENGTH + 1];
    u8 recordMixFriendClass;
    u8 apprenticeId;
    u16 easyChatSpeech[6];
    u8 recordMixFriendLanguage;
    u8 apprenticeLanguage;
    u8 battleRecord[MAX_BATTLERS_COUNT][BATTLER_RECORD_SIZE];
    u32 checksum;
};

EWRAM_DATA u32 gRecordedBattleRngSeed = 0;
EWRAM_DATA u32 gBattlePalaceMoveSelectionRngValue = 0;
EWRAM_DATA static u8 sBattleRecords[MAX_BATTLERS_COUNT][BATTLER_RECORD_SIZE] = {0};
EWRAM_DATA static u16 sRecordedBytesNo[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u16 sUnknown_0203C79C[4] = {0};
EWRAM_DATA static u16 sUnknown_0203C7A4[4] = {0};
EWRAM_DATA static u8 sUnknown_0203C7AC = 0;
EWRAM_DATA static u8 sLvlMode = 0;
EWRAM_DATA static u8 sFrontierFacility = 0;
EWRAM_DATA static u8 sFrontierBrainSymbol = 0;
EWRAM_DATA static MainCallback sCallback2_AfterRecordedBattle = NULL;
EWRAM_DATA u8 gUnknown_0203C7B4 = 0;
EWRAM_DATA static u8 sUnknown_0203C7B5 = 0;
EWRAM_DATA static u8 sBattleScene = 0;
EWRAM_DATA static u8 sTextSpeed = 0;
EWRAM_DATA static u32 sBattleFlags = 0;
EWRAM_DATA static u32 sAI_Scripts = 0;
EWRAM_DATA static struct Pokemon sSavedPlayerParty[PARTY_SIZE] = {0};
EWRAM_DATA static struct Pokemon sSavedOpponentParty[PARTY_SIZE] = {0};
EWRAM_DATA static u16 sPlayerMonMoves[2][MAX_MON_MOVES] = {0};
EWRAM_DATA static struct PlayerInfo sPlayers[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u8 sUnknown_0203CCD0 = 0;
EWRAM_DATA static u8 sRecordMixFriendName[PLAYER_NAME_LENGTH + 1] = {0};
EWRAM_DATA static u8 sRecordMixFriendClass = 0;
EWRAM_DATA static u8 sApprenticeId = 0;
EWRAM_DATA static u16 sEasyChatSpeech[6] = {0};
EWRAM_DATA static u8 sBattleOutcome = 0;

static u8 sRecordMixFriendLanguage;
static u8 sApprenticeLanguage;

// this file's functions
static u8 sub_8185278(u8 *arg0, u8 *arg1, u8 *arg2);
static bool32 CopyRecordedBattleFromSave(struct RecordedBattleSave *dst);
static void RecordedBattle_RestoreSavedParties(void);
static void CB2_RecordedBattle(void);

void sub_8184DA4(u8 arg0)
{
    s32 i, j;

    sUnknown_0203C7AC = arg0;
    sUnknown_0203CCD0 = 0;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        sRecordedBytesNo[i] = 0;
        sUnknown_0203C79C[i] = 0;
        sUnknown_0203C7A4[i] = 0;

        if (arg0 == 1)
        {
            for (j = 0; j < BATTLER_RECORD_SIZE; j++)
            {
                sBattleRecords[i][j] = 0xFF;
            }
            sBattleFlags = gBattleTypeFlags;
            sAI_Scripts = gBattleResources->ai->aiFlags;
        }
    }
}

void sub_8184E58(void)
{
    s32 i, j;

    if (sUnknown_0203C7AC == 1)
    {
        gRecordedBattleRngSeed = gRngValue;
        sFrontierFacility = VarGet(VAR_FRONTIER_FACILITY);
        sFrontierBrainSymbol = GetFronterBrainSymbol();
    }
    else if (sUnknown_0203C7AC == 2)
    {
        gRngValue = gRecordedBattleRngSeed;
    }

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
    {
        u8 linkPlayersCount;
        u8 text[30];

        gUnknown_0203C7B4 = GetMultiplayerId();
        linkPlayersCount = GetLinkPlayerCount();

        for (i = 0; i < MAX_BATTLERS_COUNT; i++)
        {
            sPlayers[i].trainerId = gLinkPlayers[i].trainerId;
            sPlayers[i].gender = gLinkPlayers[i].gender;
            sPlayers[i].battlerId = gLinkPlayers[i].id;
            sPlayers[i].language = gLinkPlayers[i].language;

            if (i < linkPlayersCount)
            {
                StringCopy(text, gLinkPlayers[i].name);
                StripExtCtrlCodes(text);
                StringCopy(sPlayers[i].name, text);
            }
            else
            {
                for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
                    sPlayers[i].name[j] = gLinkPlayers[i].name[j];
            }
        }
    }
    else
    {
        sPlayers[0].trainerId = (gSaveBlock2Ptr->playerTrainerId[0])
                                    | (gSaveBlock2Ptr->playerTrainerId[1] << 8)
                                    | (gSaveBlock2Ptr->playerTrainerId[2] << 16)
                                    | (gSaveBlock2Ptr->playerTrainerId[3] << 24);

        sPlayers[0].gender = gSaveBlock2Ptr->playerGender;
        sPlayers[0].battlerId = 0;
        sPlayers[0].language = gGameLanguage;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            sPlayers[0].name[i] = gSaveBlock2Ptr->playerName[i];
    }
}

void RecordedBattle_SetBattlerAction(u8 battlerId, u8 action)
{
    if (sRecordedBytesNo[battlerId] < BATTLER_RECORD_SIZE && sUnknown_0203C7AC != 2)
    {
        sBattleRecords[battlerId][sRecordedBytesNo[battlerId]++] = action;
    }
}

void RecordedBattle_ClearBattlerAction(u8 battlerId, u8 bytesToClear)
{
    s32 i;

    for (i = 0; i < bytesToClear; i++)
    {
        sRecordedBytesNo[battlerId]--;
        sBattleRecords[battlerId][sRecordedBytesNo[battlerId]] = 0xFF;
        if (sRecordedBytesNo[battlerId] == 0)
            break;
    }
}

u8 RecordedBattle_GetBattlerAction(u8 battlerId)
{
    // Trying to read past array or invalid action byte, battle is over.
    if (sRecordedBytesNo[battlerId] >= BATTLER_RECORD_SIZE || sBattleRecords[battlerId][sRecordedBytesNo[battlerId]] == 0xFF)
    {
        gSpecialVar_Result = gBattleOutcome = B_OUTCOME_PLAYER_TELEPORTED; // hah
        ResetPaletteFadeControl();
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
        SetMainCallback2(CB2_QuitRecordedBattle);
        return 0xFF;
    }
    else
    {
        return sBattleRecords[battlerId][sRecordedBytesNo[battlerId]++];
    }
}

u8 sub_81850D0(void)
{
    return sUnknown_0203C7AC;
}

u8 sub_81850DC(u8 *arg0)
{
    u8 i, j;
    u8 ret = 0;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        if (sRecordedBytesNo[i] != sUnknown_0203C79C[i])
        {
            arg0[ret++] = i;
            arg0[ret++] = sRecordedBytesNo[i] - sUnknown_0203C79C[i];

            for (j = 0; j < sRecordedBytesNo[i] - sUnknown_0203C79C[i]; j++)
            {
                arg0[ret++] = sBattleRecords[i][sUnknown_0203C79C[i] + j];
            }

            sUnknown_0203C79C[i] = sRecordedBytesNo[i];
        }
    }

    return ret;
}

void sub_81851A8(u8 *arg0)
{
    s32 i;
    u8 var1 = 2;
    u8 var2;

    if (!(gBattleTypeFlags & BATTLE_TYPE_LINK))
        return;

    for (i = 0; i < GetLinkPlayerCount(); i++)
    {
        if ((gLinkPlayers[i].version & 0xFF) != VERSION_EMERALD)
            return;
    }

    if (!(gBattleTypeFlags & BATTLE_TYPE_IS_MASTER))
    {
        for (var2 = *arg0; var2 != 0;)
        {
            u8 unkVar = sub_8185278(arg0, &var1, &var2);
            u8 unkVar2 = sub_8185278(arg0, &var1, &var2);

            for (i = 0; i < unkVar2; i++)
            {
                sBattleRecords[unkVar][sUnknown_0203C7A4[unkVar]++] = sub_8185278(arg0, &var1, &var2);
            }
        }
    }
}

static u8 sub_8185278(u8 *arg0, u8 *arg1, u8 *arg2)
{
    (*arg2)--;
    return arg0[(*arg1)++];
}

bool32 CanCopyRecordedBattleSaveData(void)
{
    struct RecordedBattleSave *dst = AllocZeroed(sizeof(struct RecordedBattleSave));
    bool32 ret = CopyRecordedBattleFromSave(dst);
    Free(dst);
    return ret;
}

static bool32 IsRecordedBattleSaveValid(struct RecordedBattleSave *save)
{
    if (save->battleFlags == 0)
        return FALSE;
    if (save->battleFlags & ILLEGAL_BATTLE_TYPES)
        return FALSE;
    if (CalcByteArraySum((void*)(save), sizeof(*save) - 4) != save->checksum)
        return FALSE;

    return TRUE;
}

static bool32 RecordedBattleToSave(struct RecordedBattleSave *battleSave, struct RecordedBattleSave *saveSection)
{
    memset(saveSection, 0, 0x1000);
    memcpy(saveSection, battleSave, sizeof(*battleSave));

    saveSection->checksum = CalcByteArraySum((void*)(saveSection), sizeof(*saveSection) - 4);

    if (TryWriteSpecialSaveSection(SECTOR_ID_RECORDED_BATTLE, (void*)(saveSection)) != 1)
        return FALSE;
    else
        return TRUE;
}

bool32 MoveRecordedBattleToSaveData(void)
{
    s32 i, j;
    bool32 ret;
    struct RecordedBattleSave *battleSave, *savSection;
    u8 var;

    var = 0;
    battleSave = AllocZeroed(sizeof(struct RecordedBattleSave));
    savSection = AllocZeroed(0x1000);

    for (i = 0; i < PARTY_SIZE; i++)
    {
        battleSave->playerParty[i] = sSavedPlayerParty[i];
        battleSave->opponentParty[i] = sSavedOpponentParty[i];
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
        {
            battleSave->playersName[i][j] = sPlayers[i].name[j];
        }
        battleSave->playersGender[i] = sPlayers[i].gender;
        battleSave->playersLanguage[i] = sPlayers[i].language;
        battleSave->playersBattlers[i] = sPlayers[i].battlerId;
        battleSave->playersTrainerId[i] = sPlayers[i].trainerId;
    }

    battleSave->rngSeed = gRecordedBattleRngSeed;

    if (sBattleFlags & BATTLE_TYPE_LINK)
    {
        battleSave->battleFlags = (sBattleFlags & ~(BATTLE_TYPE_LINK | BATTLE_TYPE_20)) | BATTLE_TYPE_x2000000;

        if (sBattleFlags & BATTLE_TYPE_IS_MASTER)
        {
            battleSave->battleFlags |= BATTLE_TYPE_x80000000;
        }
        else if (sBattleFlags & BATTLE_TYPE_MULTI)
        {
            switch (sPlayers[0].battlerId)
            {
            case 0:
            case 2:
                if (!(sPlayers[gUnknown_0203C7B4].battlerId & 1))
                    battleSave->battleFlags |= BATTLE_TYPE_x80000000;
                break;
            case 1:
            case 3:
                if ((sPlayers[gUnknown_0203C7B4].battlerId & 1))
                    battleSave->battleFlags |= BATTLE_TYPE_x80000000;
                break;
            }
        }
    }
    else
    {
        battleSave->battleFlags = sBattleFlags;
    }

    battleSave->opponentA = gTrainerBattleOpponent_A;
    battleSave->opponentB = gTrainerBattleOpponent_B;
    battleSave->partnerId = gPartnerTrainerId;
    battleSave->field_4FA = gUnknown_0203C7B4;
    battleSave->lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    battleSave->frontierFacility = sFrontierFacility;
    battleSave->frontierBrainSymbol = sFrontierBrainSymbol;
    battleSave->battleScene = gSaveBlock2Ptr->optionsBattleSceneOff;
    battleSave->textSpeed = 2; // fast
    battleSave->AI_scripts = sAI_Scripts;

    if (gTrainerBattleOpponent_A >= TRAINER_RECORD_MIXING_FRIEND && gTrainerBattleOpponent_A < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        if (sBattleOutcome == B_OUTCOME_WON)
        {
            for (i = 0; i < 6; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_FRIEND].speechLost[i];
        }
        else
        {
            for (i = 0; i < 6; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_FRIEND].speechWon[i];
        }
        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_FRIEND].language;
    }
    else if (gTrainerBattleOpponent_B >= TRAINER_RECORD_MIXING_FRIEND && gTrainerBattleOpponent_B < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        if (sBattleOutcome == B_OUTCOME_WON)
        {
            for (i = 0; i < 6; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_FRIEND].speechLost[i];
        }
        else
        {
            for (i = 0; i < 6; i++)
                battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_FRIEND].speechWon[i];
        }
        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_FRIEND].language;
    }
    else if (gPartnerTrainerId >= TRAINER_RECORD_MIXING_FRIEND && gPartnerTrainerId < TRAINER_RECORD_MIXING_APPRENTICE)
    {
        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            battleSave->recordMixFriendName[i] = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].name[i];
        battleSave->recordMixFriendClass = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].facilityClass;

        battleSave->recordMixFriendLanguage = gSaveBlock2Ptr->frontier.towerRecords[gPartnerTrainerId - TRAINER_RECORD_MIXING_FRIEND].language;
    }

    if (gTrainerBattleOpponent_A >= TRAINER_RECORD_MIXING_APPRENTICE)
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_APPRENTICE].id;
        for (i = 0; i < 6; i++)
            battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_APPRENTICE].easyChatWords[i];
        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_A - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }
    else if (gTrainerBattleOpponent_B >= TRAINER_RECORD_MIXING_APPRENTICE)
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_APPRENTICE].id;
        for (i = 0; i < 6; i++)
            battleSave->easyChatSpeech[i] = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_APPRENTICE].easyChatWords[i];
        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[gTrainerBattleOpponent_B - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }
    else if (gPartnerTrainerId >= TRAINER_RECORD_MIXING_APPRENTICE)
    {
        battleSave->apprenticeId = gSaveBlock2Ptr->apprentices[gPartnerTrainerId - TRAINER_RECORD_MIXING_APPRENTICE].id;

        battleSave->apprenticeLanguage = gSaveBlock2Ptr->apprentices[gPartnerTrainerId - TRAINER_RECORD_MIXING_APPRENTICE].language;
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        for (j = 0; j < BATTLER_RECORD_SIZE; j++)
        {
            battleSave->battleRecord[i][j] = sBattleRecords[i][j];
        }
    }

    while (1)
    {
        ret = RecordedBattleToSave(battleSave, savSection);
        if (ret == TRUE)
            break;
        var++;
        if (var >= 3)
            break;
    }

    free(battleSave);
    free(savSection);
    return ret;
}

static bool32 TryCopyRecordedBattleSaveData(struct RecordedBattleSave *dst, struct SaveSection *saveBuffer)
{
    if (TryReadSpecialSaveSection(SECTOR_ID_RECORDED_BATTLE, (void*)(saveBuffer)) != 1)
        return FALSE;

    memcpy(dst, saveBuffer, sizeof(struct RecordedBattleSave));

    if (!IsRecordedBattleSaveValid(dst))
        return FALSE;

    return TRUE;
}

static bool32 CopyRecordedBattleFromSave(struct RecordedBattleSave *dst)
{
    struct SaveSection *savBuffer = AllocZeroed(sizeof(struct SaveSection));
    bool32 ret = TryCopyRecordedBattleSaveData(dst, savBuffer);
    Free(savBuffer);

    return ret;
}

static void CB2_RecordedBattleEnd(void)
{
    gSaveBlock2Ptr->frontier.lvlMode = sLvlMode;
    gBattleOutcome = 0;
    gBattleTypeFlags = 0;
    gTrainerBattleOpponent_A = 0;
    gTrainerBattleOpponent_B = 0;
    gPartnerTrainerId = 0;

    RecordedBattle_RestoreSavedParties();
    SetMainCallback2(sCallback2_AfterRecordedBattle);
}

#define tFramesToWait data[0]

static void Task_StartAfterCountdown(u8 taskId)
{
    if (--gTasks[taskId].tFramesToWait == 0)
    {
        gMain.savedCallback = CB2_RecordedBattleEnd;
        SetMainCallback2(CB2_InitBattle);
        DestroyTask(taskId);
    }
}

static void SetVariablesForRecordedBattle(struct RecordedBattleSave *src)
{
    bool8 var;
    s32 i, j;

    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();

    for (i = 0; i < PARTY_SIZE; i++)
    {
        gPlayerParty[i] = src->playerParty[i];
        gEnemyParty[i] = src->opponentParty[i];
    }

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        for (var = FALSE, j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
        {
            gLinkPlayers[i].name[j] = src->playersName[i][j];
            if (src->playersName[i][j] == EOS)
                var = TRUE;
        }
        gLinkPlayers[i].gender = src->playersGender[i];
        gLinkPlayers[i].language = src->playersLanguage[i];
        gLinkPlayers[i].id = src->playersBattlers[i];
        gLinkPlayers[i].trainerId = src->playersTrainerId[i];

        if (var)
            ConvertInternationalString(gLinkPlayers[i].name, gLinkPlayers[i].language);
    }

    gRecordedBattleRngSeed = src->rngSeed;
    gBattleTypeFlags = src->battleFlags | BATTLE_TYPE_RECORDED;
    gTrainerBattleOpponent_A = src->opponentA;
    gTrainerBattleOpponent_B = src->opponentB;
    gPartnerTrainerId = src->partnerId;
    gUnknown_0203C7B4 = src->field_4FA;
    sLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    sFrontierFacility = src->frontierFacility;
    sFrontierBrainSymbol = src->frontierBrainSymbol;
    sBattleScene = src->battleScene;
    sTextSpeed = src->textSpeed;
    sAI_Scripts = src->AI_scripts;

    for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
    {
        sRecordMixFriendName[i] = src->recordMixFriendName[i];
    }

    sRecordMixFriendClass = src->recordMixFriendClass;
    sApprenticeId = src->apprenticeId;
    sRecordMixFriendLanguage = src->recordMixFriendLanguage;
    sApprenticeLanguage = src->apprenticeLanguage;

    for (i = 0; i < 6; i++)
    {
        sEasyChatSpeech[i] = src->easyChatSpeech[i];
    }

    gSaveBlock2Ptr->frontier.lvlMode = src->lvlMode;

    for (i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        for (j = 0; j < BATTLER_RECORD_SIZE; j++)
        {
            sBattleRecords[i][j] = src->battleRecord[i][j];
        }
    }
}

void PlayRecordedBattle(void (*CB2_After)(void))
{
    struct RecordedBattleSave *battleSave = AllocZeroed(sizeof(struct RecordedBattleSave));
    if (CopyRecordedBattleFromSave(battleSave) == TRUE)
    {
        u8 taskId;

        RecordedBattle_SaveParties();
        SetVariablesForRecordedBattle(battleSave);

        taskId = CreateTask(Task_StartAfterCountdown, 1);
        gTasks[taskId].tFramesToWait = 128;

        sCallback2_AfterRecordedBattle = CB2_After;
        PlayMapChosenOrBattleBGM(FALSE);
        SetMainCallback2(CB2_RecordedBattle);
    }
    Free(battleSave);
}

#undef tFramesToWait

static void CB2_RecordedBattle(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTasks();
}

u8 GetRecordedBattleFrontierFacility(void)
{
    return sFrontierFacility;
}

u8 GetRecordedBattleFronterBrainSymbol(void)
{
    return sFrontierBrainSymbol;
}

void RecordedBattle_SaveParties(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        sSavedPlayerParty[i] = gPlayerParty[i];
        sSavedOpponentParty[i] = gEnemyParty[i];
    }
}

static void RecordedBattle_RestoreSavedParties(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        gPlayerParty[i] = sSavedPlayerParty[i];
        gEnemyParty[i] = sSavedOpponentParty[i];
    }
}

u8 GetActiveBattlerLinkPlayerGender(void)
{
    s32 i;

    for (i = 0; i < MAX_LINK_PLAYERS; i++)
    {
        if (gLinkPlayers[i].id == gActiveBattler)
            break;
    }

    if (i != MAX_LINK_PLAYERS)
        return gLinkPlayers[i].gender;

    return 0;
}

void sub_8185F84(void)
{
    sUnknown_0203C7B5 = 0;
}

void sub_8185F90(u16 arg0)
{
    sUnknown_0203C7B5 |= (arg0 & 0x8000) >> 0xF;
}

u8 sub_8185FAC(void)
{
    return sUnknown_0203C7B5;
}

u8 GetBattleSceneInRecordedBattle(void)
{
    return sBattleScene;
}

u8 GetTextSpeedInRecordedBattle(void)
{
    return sTextSpeed;
}

void RecordedBattle_CopyBattlerMoves(void)
{
    s32 i;

    if (GetBattlerSide(gActiveBattler) == B_SIDE_OPPONENT)
        return;
    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_x2000000))
        return;
    if (sUnknown_0203C7AC == 2)
        return;

    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        sPlayerMonMoves[gActiveBattler / 2][i] = gBattleMons[gActiveBattler].moves[i];
    }
}

#define ACTION_MOVE_CHANGE 6

void sub_818603C(u8 arg0)
{
    s32 battlerId, j, k;

    if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_x2000000))
        return;

    for (battlerId = 0; battlerId < gBattlersCount; battlerId++)
    {
        if (GetBattlerSide(battlerId) != B_SIDE_OPPONENT) // player's side only
        {
            if (arg0 == 1)
            {
                for (j = 0; j < MAX_MON_MOVES; j++)
                {
                    if (gBattleMons[battlerId].moves[j] != sPlayerMonMoves[battlerId / 2][j])
                        break;
                }
                if (j != MAX_MON_MOVES) // player's mon's move has been changed
                {
                    RecordedBattle_SetBattlerAction(battlerId, ACTION_MOVE_CHANGE);
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        for (k = 0; k < MAX_MON_MOVES; k++)
                        {
                            if (gBattleMons[battlerId].moves[j] == sPlayerMonMoves[battlerId / 2][k])
                            {
                                RecordedBattle_SetBattlerAction(battlerId, k);
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                if (sBattleRecords[battlerId][sRecordedBytesNo[battlerId]] == ACTION_MOVE_CHANGE)
                {
                    u8 ppBonuses[MAX_MON_MOVES];
                    u8 array1[MAX_MON_MOVES];
                    u8 array2[MAX_MON_MOVES];
                    struct MovePp movePp;
                    u8 array3[(MAX_MON_MOVES * 2)];
                    u8 var;

                    RecordedBattle_GetBattlerAction(battlerId);
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        ppBonuses[j] = ((gBattleMons[battlerId].ppBonuses & ((3 << (j << 1)))) >> (j << 1));
                    }
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        array1[j] = RecordedBattle_GetBattlerAction(battlerId);
                        movePp.moves[j] = gBattleMons[battlerId].moves[array1[j]];
                        movePp.pp[j] = gBattleMons[battlerId].pp[array1[j]];
                        array3[j] = ppBonuses[array1[j]];
                        array2[j] = (gDisableStructs[battlerId].mimickedMoves & gBitTable[j]) >> j;
                    }
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        gBattleMons[battlerId].moves[j] = movePp.moves[j];
                        gBattleMons[battlerId].pp[j] = movePp.pp[j];
                    }
                    gBattleMons[battlerId].ppBonuses = 0;
                    gDisableStructs[battlerId].mimickedMoves = 0;
                    for (j = 0; j < MAX_MON_MOVES; j++)
                    {
                        gBattleMons[battlerId].ppBonuses |= (array3[j]) << (j << 1);
                        gDisableStructs[battlerId].mimickedMoves |= (array2[j]) << (j);
                    }

                    if (!(gBattleMons[battlerId].status2 & STATUS2_TRANSFORMED))
                    {
                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            ppBonuses[j] = ((GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PP_BONUSES, NULL) & ((3 << (j << 1)))) >> (j << 1));
                        }
                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            movePp.moves[j] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_MOVE1 + array1[j], NULL);
                            movePp.pp[j] = GetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PP1 + array1[j], NULL);
                            array3[j] = ppBonuses[array1[j]];
                        }
                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            SetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_MOVE1 + j, &movePp.moves[j]);
                            SetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PP1 + j, &movePp.pp[j]);
                        }
                        var = 0;
                        for (j = 0; j < MAX_MON_MOVES; j++)
                        {
                            var |= (array3[j]) << (j << 1);
                        }
                        SetMonData(&gPlayerParty[gBattlerPartyIndexes[battlerId]], MON_DATA_PP_BONUSES, &var);
                    }

                    gChosenMoveByBattler[battlerId] = gBattleMons[battlerId].moves[*(gBattleStruct->chosenMovePositions + battlerId)];
                }
            }
        }
    }
}

u32 GetAiScriptsInRecordedBattle(void)
{
    return sAI_Scripts;
}

void sub_8186444(void)
{
    sUnknown_0203CCD0 = 1;
}

bool8 sub_8186450(void)
{
    return (sUnknown_0203CCD0 == 0);
}

void sub_8186468(u8 *dst)
{
    s32 i;

    for (i = 0; i < 8; i++)
        dst[i] = sRecordMixFriendName[i];

    dst[7] = EOS;
    ConvertInternationalString(dst, sRecordMixFriendLanguage);
}

u8 GetRecordedBattleRecordMixFriendClass(void)
{
    return sRecordMixFriendClass;
}

u8 GetRecordedBattleApprenticeId(void)
{
    return sApprenticeId;
}

u8 GetRecordedBattleRecordMixFriendLanguage(void)
{
    return sRecordMixFriendLanguage;
}

u8 GetRecordedBattleApprenticeLanguage(void)
{
    return sApprenticeLanguage;
}

void RecordedBattle_SaveBattleOutcome(void)
{
    sBattleOutcome = gBattleOutcome;
}

u16 *GetRecordedBattleEasyChatSpeech(void)
{
    return sEasyChatSpeech;
}
