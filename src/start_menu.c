#include "global.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_pyramid_bag.h"
#include "bg.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_obj_lock.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_effect.h"
#include "field_player_avatar.h"
#include "field_specials.h"
#include "field_weather.h"
#include "field_screen_effect.h"
#include "frontier_pass.h"
#include "frontier_util.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "item_menu.h"
#include "link.h"
#include "load_save.h"
#include "main.h"
#include "map_name_popup.h"
#include "menu.h"
#include "new_game.h"
#include "option_menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "play_time.h"
#include "pokedex.h"
#include "pokenav.h"
#include "safari_zone.h"
#include "save.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "start_menu.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "trainer_card.h"
#include "window.h"
#include "constants/songs.h"
#include "rom_8011DC0.h"
#include "union_room.h"
#include "constants/rgb.h"

// Menu actions
enum
{
    MENU_ACTION_POKEDEX,
    MENU_ACTION_POKEMON,
    MENU_ACTION_BAG,
    MENU_ACTION_POKENAV,
	MENU_ACTION_WAIT,
    MENU_ACTION_PLAYER,
    MENU_ACTION_SAVE,
    MENU_ACTION_OPTION,
    MENU_ACTION_EXIT, //options past this point can't be registered - this is option 8
    MENU_ACTION_RETIRE_SAFARI,
    MENU_ACTION_PLAYER_LINK,
    MENU_ACTION_REST_FRONTIER,
    MENU_ACTION_RETIRE_FRONTIER,
    MENU_ACTION_PYRAMID_BAG
};

// Wait status
enum
{
    WAIT_IN_PROGRESS,
	WAIT_DONE,
	WAIT_RETURN
};

// Save status
enum
{
    SAVE_IN_PROGRESS,
    SAVE_SUCCESS,
    SAVE_CANCELED,
    SAVE_ERROR
};

// Clock strings
const u8 *gAMPMLookup[2] = { gText_TimeAM, gText_TimePM };
const u8 *gDayLookup[7] = { gText_TimeMonday, gText_TimeTuesday, gText_TimeWednesday, gText_TimeThursday, gText_TimeFriday, gText_TimeSaturday, gText_TimeSunday };
const u8 *gDNStatusLookup[4] = { gText_TimeDawn, gText_TimeDay, gText_TimeDusk, gText_TimeNight };
const u8 *gEarlyLateLookup[2] = { gText_TimeEarly, gText_TimeLate };
const u8 *gSeasonLookup[4] = { gText_TimeSpring, gText_TimeSummer, gText_TimeFall, gText_TimeWinter };
extern const u8 gText_TimeDNSpacer[];
extern const u8 gText_TimeDaySpacer[];
extern const u8 gText_TimeSeasonSpacer[];

// IWRAM common
bool8 (*gMenuCallback)(void);

// EWRAM
EWRAM_DATA static u8 sSafariBallsWindowId = 0;
EWRAM_DATA static u8 sBattlePyramidFloorWindowId = 0;
EWRAM_DATA static u8 sNuzlockeWindowId = 0;
EWRAM_DATA static u8 sClockWindowId = 0;
EWRAM_DATA static u8 sTimeLeftWindowId = 0;
EWRAM_DATA static u8 sWaitListWindowId = 0;
EWRAM_DATA static u8 sStartMenuCursorPos = 0;
EWRAM_DATA static u8 sStartMenuScroll = 0;
EWRAM_DATA static u8 sNumStartMenuActions = 0;
EWRAM_DATA static u8 sCurrentStartMenuActions[10] = {0};
EWRAM_DATA static u8 sUnknown_02037619[2] = {0};

EWRAM_DATA static u8 (*sDialogCallback)(void) = NULL;
EWRAM_DATA static u8 sSaveDialogTimer = 0;
EWRAM_DATA static bool8 sSavingComplete = FALSE;
EWRAM_DATA static u8 sSaveInfoWindowId = 0;

// Menu action callbacks
static bool8 StartMenuPokedexCallback(void);
static bool8 StartMenuPokemonCallback(void);
static bool8 StartMenuBagCallback(void);
static bool8 StartMenuPokeNavCallback(void);
static bool8 StartMenuWaitCallback(void);
static bool8 StartMenuPlayerNameCallback(void);
static bool8 StartMenuSaveCallback(void);
static bool8 StartMenuOptionCallback(void);
static bool8 StartMenuExitCallback(void);
static bool8 StartMenuSafariZoneRetireCallback(void);
static bool8 StartMenuLinkModePlayerNameCallback(void);
static bool8 StartMenuBattlePyramidRetireCallback(void);
static bool8 StartMenuBattlePyramidBagCallback(void);

// Menu callbacks
static bool8 WaitStartCallback(void);
static bool8 WaitCallback(void);
static bool8 SaveStartCallback(void);
static bool8 SaveCallback(void);
static bool8 BattlePyramidRetireStartCallback(void);
static bool8 BattlePyramidRetireReturnCallback(void);
static bool8 BattlePyramidRetireCallback(void);
static bool8 HandleStartMenuInput(void);

// Wait dialog callbacks
static u8 WaitTryToWaitCallback(void);
static u8 WaitCantWaitCallback(void);
static u8 WaitDoWaitMenuCallback(void);

// Save dialog callbacks
static u8 SaveConfirmSaveCallback(void);
static u8 SaveYesNoCallback(void);
static u8 SaveConfirmInputCallback(void);
static u8 SaveFileExistsCallback(void);
static u8 SaveConfirmOverwriteDefaultNoCallback(void);
static u8 SaveConfirmOverwriteCallback(void);
static u8 SaveOverwriteInputCallback(void);
static u8 SaveSavingMessageCallback(void);
static u8 SaveDoSaveCallback(void);
static u8 SaveSuccessCallback(void);
static u8 SaveReturnSuccessCallback(void);
static u8 SaveErrorCallback(void);
static u8 SaveReturnErrorCallback(void);
static u8 BattlePyramidConfirmRetireCallback(void);
static u8 BattlePyramidRetireYesNoCallback(void);
static u8 BattlePyramidRetireInputCallback(void);

// Task callbacks
static void StartMenuTask(u8 taskId);
static void SaveGameTask(u8 taskId);
static void sub_80A0550(u8 taskId);
static void sub_80A08A4(u8 taskId);

// Some other callback
static bool8 sub_809FA00(void);

// Script
extern const u8 EventScript_NoRegisteredMenuOption[];

static const struct WindowTemplate sSafariBallsWindowTemplate = {0, 1, 1, 9, 4, 0xF, 8};

static const u8* const sPyramindFloorNames[] =
{
    gText_Floor1,
    gText_Floor2,
    gText_Floor3,
    gText_Floor4,
    gText_Floor5,
    gText_Floor6,
    gText_Floor7,
    gText_Peak
};

static const struct WindowTemplate sPyramidFloorWindowTemplate_2 = {0, 1, 1, 0xA, 4, 0xF, 8};
static const struct WindowTemplate sPyramidFloorWindowTemplate_1 = {0, 1, 1, 0xC, 4, 0xF, 8};

// Clock window
static const struct WindowTemplate sInfoWindowTemplate = {0, 2, 17, 26, 2, 0xF, 8};

// Window to count fainted mons in Nuzlocke mode
static const struct WindowTemplate sNuzlockeWindowTemplate = {0, 1, 1, 6, 4, 0xF, 62};

// "Time left" wait window
static const struct WindowTemplate sTimeLeftWindowTemplate = {0, 1, 1, 7, 4, 0xF, 62};

// Wait list window
static const struct WindowTemplate sWaitListWindowTemplate = {0, 23, 1, 28, 10, 0xF, 62};

static const struct MenuAction sStartMenuItems[] =
{
    {gText_MenuPokedex, {.u8_void = StartMenuPokedexCallback}},
    {gText_MenuPokemon, {.u8_void = StartMenuPokemonCallback}},
    {gText_MenuBag, {.u8_void = StartMenuBagCallback}},
    {gText_MenuPokenav, {.u8_void = StartMenuPokeNavCallback}},
	{gText_MenuWait, {.u8_void = StartMenuWaitCallback}},
    {gText_MenuPlayer, {.u8_void = StartMenuPlayerNameCallback}},
    {gText_MenuSave, {.u8_void = StartMenuSaveCallback}},
    {gText_MenuOption, {.u8_void = StartMenuOptionCallback}},
    {gText_MenuExit, {.u8_void = StartMenuExitCallback}},
    {gText_MenuRetire, {.u8_void = StartMenuSafariZoneRetireCallback}},
    {gText_MenuPlayer, {.u8_void = StartMenuLinkModePlayerNameCallback}},
    {gText_MenuRest, {.u8_void = StartMenuSaveCallback}},
    {gText_MenuRetire, {.u8_void = StartMenuBattlePyramidRetireCallback}},
    {gText_MenuBag, {.u8_void = StartMenuBattlePyramidBagCallback}}
};

static const struct BgTemplate sUnknown_085105A8[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static const struct WindowTemplate sUnknown_085105AC[] =
{
    {0, 2, 0xF, 0x1A, 4, 0xF, 0x194},
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sSaveInfoWindowTemplate = {0, 1, 1, 0xE, 0xA, 0xF, 8};

// Local functions
static void BuildStartMenuActions(void);
static void AddStartMenuAction(u8 action);
static void BuildNormalStartMenu(void);
static void BuildSafariZoneStartMenu(void);
static void BuildLinkModeStartMenu(void);
static void BuildUnionRoomStartMenu(void);
static void BuildBattlePikeStartMenu(void);
static void BuildBattlePyramidStartMenu(void);
static void BuildMultiBattleRoomStartMenu(void);
static void ShowSafariBallsWindow(void);
static void ShowPyramidFloorWindow(void);
static void ShowNuzlockeWindow(void);
static void CopyHourStrings(void);
static void ShowClockWindow(void);
static void RemoveExtraStartMenuWindows(void);
static bool32 PrintStartMenuActions(s8 *pIndex, u32 count);
static bool32 InitStartMenuStep(void);
static void InitStartMenu(u8 step);
static void CreateStartMenuTask(TaskFunc followupFunc);
static void InitSave(void);
static u8 RunWaitCallback(void);
static u8 RunSaveCallback(void);
static void ShowWaitMessage(const u8 *message, u8 (*waitCallback)(void));
static void ShowSaveMessage(const u8 *message, u8 (*saveCallback)(void));
static void sub_80A0014(void);
static void HideSaveInfoWindow(void);
static void SaveStartTimer(void);
static bool8 SaveSuccesTimer(void);
static bool8 SaveErrorTimer(void);
static void InitBattlePyramidRetire(void);
static void sub_80A03D8(void);
static bool32 sub_80A03E4(u8 *par1);
static void sub_80A0540(void);
static void ShowSaveInfoWindow(void);
static void RemoveSaveInfoWindow(void);
static void HideStartMenuWindow(void);

void SetDexPokemonPokenavFlags(void) // unused
{
    FlagSet(FLAG_SYS_POKEDEX_GET);
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKENAV_GET);
}

static void BuildStartMenuActions(void)
{
    sNumStartMenuActions = 0;

    if (IsUpdateLinkStateCBActive() == TRUE)
    {
        BuildLinkModeStartMenu();
    }
    else if (InUnionRoom() == TRUE)
    {
        BuildUnionRoomStartMenu();
    }
    else if (GetSafariZoneFlag() == TRUE)
    {
        BuildSafariZoneStartMenu();
    }
    else if (InBattlePike())
    {
        BuildBattlePikeStartMenu();
    }
    else if (InBattlePyramid())
    {
        BuildBattlePyramidStartMenu();
    }
    else if (InMultiBattleRoom())
    {
        BuildMultiBattleRoomStartMenu();
    }
    else
    {
        BuildNormalStartMenu();
    }
}

static void AddStartMenuAction(u8 action)
{
    AppendToList(sCurrentStartMenuActions, &sNumStartMenuActions, action);
}

static void BuildNormalStartMenu(void)
{
    if (FlagGet(FLAG_SYS_POKEDEX_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_POKEDEX);
    }
    if (FlagGet(FLAG_SYS_POKEMON_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_POKEMON);
    }

    AddStartMenuAction(MENU_ACTION_BAG);

    if (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_POKENAV);
    }

	AddStartMenuAction(MENU_ACTION_WAIT);
    AddStartMenuAction(MENU_ACTION_PLAYER);
	
	// No save option on Deadlocke mode
	if (gSaveBlock2Ptr->nuzlockeMode < NUZLOCKE_MODE_DEADLOCKE)
	{
		AddStartMenuAction(MENU_ACTION_SAVE);
	}
	
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildSafariZoneStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_RETIRE_SAFARI);
    AddStartMenuAction(MENU_ACTION_POKEDEX);
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);
	AddStartMenuAction(MENU_ACTION_WAIT);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildLinkModeStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);

    if (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_POKENAV);
    }

    AddStartMenuAction(MENU_ACTION_PLAYER_LINK);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildUnionRoomStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_BAG);

    if (FlagGet(FLAG_SYS_POKENAV_GET) == TRUE)
    {
        AddStartMenuAction(MENU_ACTION_POKENAV);
    }

    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildBattlePikeStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEDEX);
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildBattlePyramidStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PYRAMID_BAG);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_REST_FRONTIER);
    AddStartMenuAction(MENU_ACTION_RETIRE_FRONTIER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void BuildMultiBattleRoomStartMenu(void)
{
    AddStartMenuAction(MENU_ACTION_POKEMON);
    AddStartMenuAction(MENU_ACTION_PLAYER);
    AddStartMenuAction(MENU_ACTION_OPTION);
    AddStartMenuAction(MENU_ACTION_EXIT);
}

static void ShowSafariBallsWindow(void)
{
    sSafariBallsWindowId = AddWindow(&sSafariBallsWindowTemplate);
    PutWindowTilemap(sSafariBallsWindowId);
    DrawStdWindowFrame(sSafariBallsWindowId, FALSE);
    ConvertIntToDecimalStringN(gStringVar1, gNumSafariBalls, STR_CONV_MODE_RIGHT_ALIGN, 2);
    StringExpandPlaceholders(gStringVar4, gText_SafariBallStock);
    AddTextPrinterParameterized(sSafariBallsWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sSafariBallsWindowId, 2);
}

static void ShowPyramidFloorWindow(void)
{
    if (gSaveBlock2Ptr->frontier.curChallengeBattleNum == 7)
        sBattlePyramidFloorWindowId = AddWindow(&sPyramidFloorWindowTemplate_1);
    else
        sBattlePyramidFloorWindowId = AddWindow(&sPyramidFloorWindowTemplate_2);

    PutWindowTilemap(sBattlePyramidFloorWindowId);
    DrawStdWindowFrame(sBattlePyramidFloorWindowId, FALSE);
    StringCopy(gStringVar1, sPyramindFloorNames[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
    StringExpandPlaceholders(gStringVar4, gText_BattlePyramidFloor);
    AddTextPrinterParameterized(sBattlePyramidFloorWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sBattlePyramidFloorWindowId, 2);
}

// Color themes for each nuzlocke mode
const u8 gGreen[] = _("{COLOR GREEN}");
const u8 gBlue[] = _("{COLOR BLUE}");
const u8 gRed[] = _("{COLOR RED}"); //also used for coloring info pane stuff & for coloring the registered option text

// Creates the window to show the number of Pokemon lost in nuzlocke mode
static void ShowNuzlockeWindow(void)
{
    sNuzlockeWindowId = AddWindow(&sNuzlockeWindowTemplate);
    PutWindowTilemap(sNuzlockeWindowId);
    DrawStdWindowFrame(sNuzlockeWindowId, FALSE);
	// Start string with relevant text colors (above) to reflect the nuzlocke mode you're on
	switch(gSaveBlock2Ptr->nuzlockeMode)
	{
		case NUZLOCKE_MODE_NUZLOCKE:
			StringExpandPlaceholders(gStringVar4, gGreen);
			break;
		case NUZLOCKE_MODE_HARDLOCKE:
			StringExpandPlaceholders(gStringVar4, gBlue);
			break;
		case NUZLOCKE_MODE_DEADLOCKE:
			StringExpandPlaceholders(gStringVar4, gRed);
			break;
	}
	// Append "FAINTED:\n"
    StringAppend(gStringVar4, gText_Fainted);
	// Convert fainted counter to string, store in gStringVar1
	ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->nuzlockeCounter, STR_CONV_MODE_LEFT_ALIGN, 5);
	// Append the counter to the gStringVar4
	StringAppend(gStringVar4, gStringVar1);
	// Finally print
    AddTextPrinterParameterized(sNuzlockeWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sNuzlockeWindowId, 2);
}

// Copies hour strings to gStringVar2
static void CopyHourStrings(void)
{
	int hour;
	bool8 isPM = FALSE; //FALSE = AM, TRUE = PM
	bool8 isLate = FALSE; //FALSE = early season, TRUE = late season
	
	// Get current hour
	hour = gSaveBlock2Ptr->timeHour;
	// If hour = 0, add 12 for 12AM (midnight)
	if (hour == 0)
		hour += 12;
	else
	{
		// If 1pm or after, subtract 12 then set it to PM
		if (hour > 12)
		{
			hour -= 12;
			isPM = TRUE;
		}
		// If 12pm or earlier, enable PM
		else if (hour == 12)
			isPM = TRUE;
		// Otherwise is below 12pm, do nothing
	}
	// Finally convert to integer, store in gStringVar1
	ConvertIntToDecimalStringN(gStringVar1, hour, STR_CONV_MODE_RIGHT_ALIGN, 2);
	// Then append to gStringVar2
	StringAppend(gStringVar2, gStringVar1);
	// Then append AM or PM
	StringAppend(gStringVar2, gAMPMLookup[isPM]);
	// Append day/night status spacer
	StringAppend(gStringVar2, gText_TimeDNSpacer);
	// Append day/night status
	StringAppend(gStringVar2, gDNStatusLookup[gSaveBlock2Ptr->dayNightStatus]);
}
	
// Creates the information pane at the bottom which displays time, weather etc
static void ShowClockWindow(void)
{
	int hour;
	bool8 isPM = FALSE; //FALSE = AM, TRUE = PM
	bool8 isLate = FALSE; //FALSE = early season, TRUE = late season
	
    sClockWindowId = AddWindow(&sInfoWindowTemplate);
    PutWindowTilemap(sClockWindowId);
    DrawStdWindowFrame(sClockWindowId, FALSE);
	
	// Begin by turning hour red
	StringExpandPlaceholders(gStringVar4, gRed);
	// Copy hour strings to gStringVar2
	CopyHourStrings();
	// Append day spacer
	StringAppend(gStringVar4, gText_TimeDaySpacer);
	// Append day
	StringAppend(gStringVar4, gDayLookup[gSaveBlock2Ptr->timeDay]);
	// Append season spacer
	StringAppend(gStringVar4, gText_TimeSeasonSpacer);
	// If Friday-Sunday & Week 1, it's late season
	if (gSaveBlock2Ptr->timeDay > TIME_DAY_THURSDAY
	 && gSaveBlock2Ptr->timeWeek == TIME_WEEK_1)
	{
		isLate = TRUE;
		StringAppend(gStringVar4, gEarlyLateLookup[isLate]);
	}
	// If Monday-Wednesday & Week 0, it's early season
	else if (gSaveBlock2Ptr->timeDay < TIME_DAY_THURSDAY
	 && gSaveBlock2Ptr->timeWeek == TIME_WEEK_0)
	{
		StringAppend(gStringVar4, gEarlyLateLookup[isLate]);
	}
	// Make season text red
	StringAppend(gStringVar4, gRed);
	// Append season
	StringAppend(gStringVar4, gSeasonLookup[gSaveBlock2Ptr->timeSeason]);
	
	// Print text
    AddTextPrinterParameterized(sClockWindowId, 1, gStringVar4, 0, 1, 0xFF, NULL);
    CopyWindowToVram(sClockWindowId, 2);
}

static void RemoveExtraStartMenuWindows(void)
{
	// Remove clock window
	ClearStdWindowAndFrameToTransparent(sClockWindowId, FALSE);
    RemoveWindow(sClockWindowId);
		
    if (GetSafariZoneFlag())
    {
        ClearStdWindowAndFrameToTransparent(sSafariBallsWindowId, FALSE);
        CopyWindowToVram(sSafariBallsWindowId, 2);
        RemoveWindow(sSafariBallsWindowId);
    }
    if (InBattlePyramid())
    {
        ClearStdWindowAndFrameToTransparent(sBattlePyramidFloorWindowId, FALSE);
        RemoveWindow(sBattlePyramidFloorWindowId);
    }
	if (gSaveBlock2Ptr->nuzlockeMode != NUZLOCKE_MODE_OFF)
	{
		ClearStdWindowAndFrameToTransparent(sNuzlockeWindowId, FALSE);
        RemoveWindow(sNuzlockeWindowId);
	}
}

static bool32 PrintStartMenuActions(s8 *pIndex, u32 count)
{
    s8 index = *pIndex;
	bool8 red = FALSE;

    do
    {
		// is it the player's name being printed?
        if (sStartMenuItems[sCurrentStartMenuActions[index + sStartMenuScroll]].func.u8_void == StartMenuPlayerNameCallback)
        {
			// make it red if trainer card is registered
			if (gSaveBlock2Ptr->startMenuRegister == sCurrentStartMenuActions[index + sStartMenuScroll])
				red = TRUE;
			PrintPlayerNameOnWindow(GetStartMenuWindowId(), sStartMenuItems[sCurrentStartMenuActions[index + sStartMenuScroll]].text, 8, (index << 4) + 1, red);
        }
		// any other menu option
        else
        {
			// make red if registered
			if (gSaveBlock2Ptr->startMenuRegister == sCurrentStartMenuActions[index + sStartMenuScroll])
			{
				StringExpandPlaceholders(gStringVar4, gRed);
				StringAppend(gStringVar4, sStartMenuItems[sCurrentStartMenuActions[index + sStartMenuScroll]].text);
			}
			else
				StringExpandPlaceholders(gStringVar4, sStartMenuItems[sCurrentStartMenuActions[index + sStartMenuScroll]].text);
            AddTextPrinterParameterized(GetStartMenuWindowId(), 1, gStringVar4, 8, (index << 4) + 1, 0xFF, NULL);
        }

        index++;
        if (index >= sNumStartMenuActions)
        {
            *pIndex = index;
            return TRUE;
        }

        count--;
    }
    while (count != 0);

    *pIndex = index;
    return FALSE;
}

static bool32 InitStartMenuStep(void)
{
    s8 value = sUnknown_02037619[0];
	u8 length;

    switch (value)
    {
    case 0:
		// Stop clock updating when menu is opened
		gMain.stopClockUpdating = TRUE;
        sUnknown_02037619[0]++;
        break;
    case 1:
        BuildStartMenuActions();
        sUnknown_02037619[0]++;
        break;
    case 2:
        sub_81973A4();
		// Create start menu window
        DrawStdWindowFrame(sub_81979C4(sNumStartMenuActions), FALSE); // Never more than 7 options
        sUnknown_02037619[1] = 0;
        sUnknown_02037619[0]++;
        break;
    case 3:
        if (GetSafariZoneFlag())
            ShowSafariBallsWindow();
        else if (InBattlePyramid())
            ShowPyramidFloorWindow();
		// Show nuzlocke window if in Nuzlocke mode
		else if (gSaveBlock2Ptr->nuzlockeMode != NUZLOCKE_MODE_OFF)
			ShowNuzlockeWindow();
		// Always show info window
		ShowClockWindow();
        sUnknown_02037619[0]++;
        break;
    case 4:
        if (PrintStartMenuActions(&sUnknown_02037619[1], 2))
            sUnknown_02037619[0]++;
        break;
    case 5:
		// Create no more than 7 options
		if (sNumStartMenuActions > 7)
			length = 7;
		else
			length = sNumStartMenuActions;
        sStartMenuCursorPos = sub_81983AC(GetStartMenuWindowId(), 1, 0, 1, 16, length, sStartMenuCursorPos);
        CopyWindowToVram(GetStartMenuWindowId(), TRUE);
        return TRUE;
    }

    return FALSE;
}

static void InitStartMenu(u8 step)
{
    sUnknown_02037619[0] = step; // Step = 4 when updating the start menu whilst scrolling
    sUnknown_02037619[1] = 0;
    while (!InitStartMenuStep())
        ;
}

static void StartMenuTask(u8 taskId)
{
    if (InitStartMenuStep() == TRUE)
        SwitchTaskToFollowupFunc(taskId);
}

static void CreateStartMenuTask(TaskFunc followupFunc)
{
    u8 taskId;

    sUnknown_02037619[0] = 0;
    sUnknown_02037619[1] = 0;
    taskId = CreateTask(StartMenuTask, 0x50);
    SetTaskFuncWithFollowupFunc(taskId, StartMenuTask, followupFunc);
}

static bool8 sub_809FA00(void)
{
    if (InitStartMenuStep() == FALSE)
    {
        return FALSE;
    }

    sub_80AF688();
    return TRUE;
}

void sub_809FA18(void) // Called from field_screen.s
{
    sUnknown_02037619[0] = 0;
    sUnknown_02037619[1] = 0;
    gFieldCallback2 = sub_809FA00;
}

void sub_809FA34(u8 taskId) // Referenced in field_screen.s and rom_8011DC0.s
{
    struct Task* task = &gTasks[taskId];

    switch(task->data[0])
    {
    case 0:
        if (InUnionRoom() == TRUE)
            var_800D_set_xB();

        gMenuCallback = HandleStartMenuInput;
        task->data[0]++;
        break;
    case 1:
        if (gMenuCallback() == TRUE)
            DestroyTask(taskId);
        break;
    }
}

void ShowStartMenu(void) // Called from overworld.c and field_control_avatar.s
{
    if (!IsUpdateLinkStateCBActive())
    {
        FreezeEventObjects();
        sub_808B864();
        sub_808BCF4();
    }
    CreateStartMenuTask(sub_809FA34);
    ScriptContext2_Enable();
}

static bool8 HandleStartMenuInput(void)
{
    if (gMain.newKeys & DPAD_UP)
    {
        PlaySE(SE_SELECT);
		// If cursor is at the top & menu is scrolled down, decrease scroll counter by 1
		if (sStartMenuCursorPos == 0
		 && sStartMenuScroll != 0)
		{
			sStartMenuScroll--;
			// Clear start menu
			FillWindowPixelBuffer(GetStartMenuWindowId(), PIXEL_FILL(1));
			// Update text
			InitStartMenu(4);
		}
		// Try to move cursor up
		else
			sStartMenuCursorPos = Menu_MoveCursorNoWrapAround(-1);
    }

    if (gMain.newKeys & DPAD_DOWN)
    {
        PlaySE(SE_SELECT);
		// If cursor is at the bottom & last menu item isn't exit, increase scroll counter by 1
		if (sStartMenuCursorPos > 5
		 && sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll]].func.u8_void != StartMenuExitCallback)
		{
			sStartMenuScroll++;
			// Clear start menu
			FillWindowPixelBuffer(GetStartMenuWindowId(), PIXEL_FILL(1));
			// Update text
			InitStartMenu(4);
		}
		// Try to move cursor down
		else
			sStartMenuCursorPos = Menu_MoveCursorNoWrapAround(1);
    }

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if (sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll]].func.u8_void == StartMenuPokedexCallback)
        {
            if (GetNationalPokedexCount(0) == 0)
                return FALSE;
        }

        gMenuCallback = sStartMenuItems[sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll]].func.u8_void;

        if (gMenuCallback != StartMenuSaveCallback
		 && gMenuCallback != StartMenuWaitCallback
         && gMenuCallback != StartMenuExitCallback
         && gMenuCallback != StartMenuSafariZoneRetireCallback
         && gMenuCallback != StartMenuBattlePyramidRetireCallback)
        {
           FadeScreen(1, 0);
        }

        return FALSE;
    }
	
	// Try to register option when select is pressed
	if (gMain.newKeys & SELECT_BUTTON)
    {
		// Exit, retire (safari zone & pyramid), pyramid bag and link player name cannot be registered. These are options 0x8 onwards
        if (sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll] > 7)
        {
			// Give player feedback by playing a sound effect
			PlaySE(SE_BOO);
        }
		else
		{
			// Clear registered option if select is pressed over the same option that is registered
			if (gSaveBlock2Ptr->startMenuRegister == sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll])
			{
				gSaveBlock2Ptr->startMenuRegister = 0xF; // 0xF means no option is registered
			}
			// Any other option will be registered
			else
			{
				// Stores the option where the cursor is to saveblock 2
				gSaveBlock2Ptr->startMenuRegister = sCurrentStartMenuActions[sStartMenuCursorPos + sStartMenuScroll];
			}
			PlaySE(SE_SELECT);
			// Clear start menu
			FillWindowPixelBuffer(GetStartMenuWindowId(), PIXEL_FILL(1));
			// Update text - this is needed to display the option as blue when it is registered instantly
			InitStartMenu(4);
		}
    }
	
	if (gMain.newKeys & (START_BUTTON | B_BUTTON))
    {
        RemoveExtraStartMenuWindows();
        HideStartMenu();
        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPokedexCallback(void)
{
    if (!gPaletteFade.active)
    {
        IncrementGameStat(GAME_STAT_CHECKED_POKEDEX);
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_Pokedex);

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPokemonCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_PartyMenuFromStartMenu); // Display party menu

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuBagCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_BagMenuFromStartMenu); // Display bag menu

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuPokeNavCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitPokeNav);  // Display PokeNav

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuWaitCallback(void)
{
    gMenuCallback = WaitStartCallback; // Display wait menu

    return FALSE;
}

static bool8 StartMenuPlayerNameCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();

        if (IsUpdateLinkStateCBActive() || InUnionRoom())
            ShowPlayerTrainerCard(CB2_ReturnToFieldWithOpenMenu); // Display trainer card
        else if (FlagGet(FLAG_SYS_FRONTIER_PASS))
            ShowFrontierPass(CB2_ReturnToFieldWithOpenMenu); // Display frontier pass
        else
            ShowPlayerTrainerCard(CB2_ReturnToFieldWithOpenMenu); // Display trainer card

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuSaveCallback(void)
{
    if (InBattlePyramid())
        RemoveExtraStartMenuWindows();

    gMenuCallback = SaveStartCallback; // Display save menu

    return FALSE;
}

static bool8 StartMenuOptionCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitOptionMenu); // Display option menu
        gMain.savedCallback = CB2_ReturnToFieldWithOpenMenu;

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuExitCallback(void)
{
    RemoveExtraStartMenuWindows();
    HideStartMenu(); // Hide start menu

    return TRUE;
}

static bool8 StartMenuSafariZoneRetireCallback(void)
{
    RemoveExtraStartMenuWindows();
    HideStartMenu();
    SafariZoneRetirePrompt();

    return TRUE;
}

static bool8 StartMenuLinkModePlayerNameCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        CleanupOverworldWindowsAndTilemaps();
        ShowTrainerCardInLink(gLocalLinkPlayerId, CB2_ReturnToFieldWithOpenMenu);

        return TRUE;
    }

    return FALSE;
}

static bool8 StartMenuBattlePyramidRetireCallback(void)
{
    gMenuCallback = BattlePyramidRetireStartCallback; // Confirm retire

    return FALSE;
}

void sub_809FDD4(void)
{
    ClearDialogWindowAndFrameToTransparent(0, FALSE);
    ScriptUnfreezeEventObjects();
    CreateStartMenuTask(sub_809FA34);
    ScriptContext2_Enable();
}

static bool8 StartMenuBattlePyramidBagCallback(void)
{
    if (!gPaletteFade.active)
    {
        PlayRainStoppingSoundEffect();
        RemoveExtraStartMenuWindows();
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_PyramidBagMenuFromStartMenu);

        return TRUE;
    }

    return FALSE;
}

// First called by wait option in start menu
static bool8 WaitStartCallback(void)
{
    sDialogCallback = WaitTryToWaitCallback;
    gMenuCallback = WaitCallback;

    return FALSE;
}

// Works out where player is in the wait process
static bool8 WaitCallback(void)
{
    switch (RunWaitCallback())
    {
	// Player is still navigating wait menu
    case WAIT_IN_PROGRESS:
        return FALSE;
	// Player has waited successfully
	case WAIT_DONE:
		gMain.stopClockUpdating = FALSE;
		//UnfreezeScreenPostSaveOrWait();
		EnableBothScriptContexts();
		break;
	// Player chose not to/can't wait
    case WAIT_RETURN:
        //Go back to start menu
        //Menu_EraseScreen();
		InitStartMenu(0);
		gMenuCallback = HandleStartMenuInput;
    }

    return FALSE;
}

static u8 WaitTryToWaitCallback(void)
{
	// Close start menu & extra windows
    RemoveExtraStartMenuWindows();
    ClearStdWindowAndFrame(GetStartMenuWindowId(), TRUE);
    RemoveStartMenuWindow();
	switch(gSaveBlock2Ptr->waitStatus)
	{
	case WAIT_ABLE:
		if (gSaveBlock2Ptr->waitTime == 0) //no wait time left
			ShowWaitMessage(gText_WaitingTooOften, WaitCantWaitCallback);
		else
			ShowWaitMessage(gText_HowLongToWait, WaitDoWaitMenuCallback);
		break;
	default:
	case WAIT_UNABLE:
		ShowWaitMessage(gText_YouCantWaitNow, WaitCantWaitCallback);
	}
	return WAIT_IN_PROGRESS;
}

// Player can't wait - return to start menu/overworld
static u8 WaitCantWaitCallback(void)
{
	if (gMain.newKeys & (A_BUTTON | B_BUTTON))
	{
		// Remove textbox
		sub_80A0014();
		return WAIT_RETURN;
	}
}

// Player can wait - display wait menu
static u8 WaitDoWaitMenuCallback(void)
{
	int i;
	
	// Draw hours left window
	sTimeLeftWindowId = AddWindow(&sTimeLeftWindowTemplate);
    PutWindowTilemap(sTimeLeftWindowId);
    DrawStdWindowFrame(sTimeLeftWindowId, FALSE);
	
	// Draw list window
	sWaitListWindowId = AddWindow(&sWaitListWindowTemplate);
    PutWindowTilemap(sWaitListWindowId);
    DrawStdWindowFrame(sWaitListWindowId, FALSE);
	
	// Copies number of hours available to wait to gStringVar1
	ConvertIntToDecimalStringN(gStringVar1, gSaveBlock2Ptr->waitTime, STR_CONV_MODE_RIGHT_ALIGN, 2);
	
	// Copy hour strings to gStringVar2
	CopyHourStrings();
	
	// Does there need to be a space added?
	if (gSaveBlock2Ptr->waitTime >= 10)
		StringAppend(gStringVar1, CHAR_SPACE);
		
	// Append "HOUR"
	StringAppend(gStringVar1, gText_Hour);
	
	// Does there need to be an S added?
	if (gSaveBlock2Ptr->waitTime != 1)
	{
		StringAppend(gStringVar1, gText_S);
	}
	
	//Menu_PrintText(gWaitText_HoursRemaining, 1, 1);
	
	//for (i = 0; i < 5; i++)
	//{
	//	Menu_PrintText(gWaitMenuText[i], 23, 1 + i * 2);
	//}
	
	//sWaitMenuCursorPos = InitMenu(0, 0x17, 1, 5, 0, 6);
	
	// Print
	CopyWindowToVram(sTimeLeftWindowId, 2);
	CopyWindowToVram(sWaitListWindowId, 2);
	
	//dialogCallback = WaitMenu_InputProcessCallback;
	return WAIT_IN_PROGRESS;
}

static bool8 SaveStartCallback(void)
{
    InitSave();
    gMenuCallback = SaveCallback;

    return FALSE;
}

static bool8 SaveCallback(void)
{
    switch (RunSaveCallback())
    {
    case SAVE_IN_PROGRESS:
        return FALSE;
    case SAVE_CANCELED: // Back to start menu
        ClearDialogWindowAndFrameToTransparent(0, FALSE);
        InitStartMenu(0);
        gMenuCallback = HandleStartMenuInput;
        return FALSE;
    case SAVE_SUCCESS:
    case SAVE_ERROR:    // Close start menu
        ClearDialogWindowAndFrameToTransparent(0, TRUE);
        ScriptUnfreezeEventObjects();
        ScriptContext2_Disable();
        SoftResetInBattlePyramid();
        return TRUE;
    }

    return FALSE;
}

static bool8 BattlePyramidRetireStartCallback(void)
{
    InitBattlePyramidRetire();
    gMenuCallback = BattlePyramidRetireCallback;

    return FALSE;
}

static bool8 BattlePyramidRetireReturnCallback(void)
{
    InitStartMenu(0);
    gMenuCallback = HandleStartMenuInput;

    return FALSE;
}

static bool8 BattlePyramidRetireCallback(void)
{
    switch (RunSaveCallback())
    {
    case SAVE_SUCCESS: // No (Stay in battle pyramid)
        RemoveExtraStartMenuWindows();
        gMenuCallback = BattlePyramidRetireReturnCallback;
        return FALSE;
    case SAVE_IN_PROGRESS:
        return FALSE;
    case SAVE_CANCELED: // Yes (Retire from battle pyramid)
        ClearDialogWindowAndFrameToTransparent(0, TRUE);
        ScriptUnfreezeEventObjects();
        ScriptContext2_Disable();
        ScriptContext1_SetupScript(BattleFrontier_BattlePyramidEmptySquare_EventScript_252C88);
        return TRUE;
    }

    return FALSE;
}

static void InitSave(void)
{
    save_serialize_map();
    sDialogCallback = SaveConfirmSaveCallback;
    sSavingComplete = FALSE;
}

static u8 RunWaitCallback(void)
{
    // True if text is still printing
    if (RunTextPrintersAndIsPrinter0Active() == TRUE)
    {
        return WAIT_IN_PROGRESS;
    }
	
    return sDialogCallback();
}

static u8 RunSaveCallback(void)
{
    // True if text is still printing
    if (RunTextPrintersAndIsPrinter0Active() == TRUE)
    {
        return SAVE_IN_PROGRESS;
    }

    sSavingComplete = FALSE;
    return sDialogCallback();
}

void SaveGame(void) // Called when save is used via select and from cable_club.s
{
	// Make sure window borders load correctly if used via select
	if (gMain.optionRegister)
		sub_81973A4();
    InitSave();
    CreateTask(SaveGameTask, 0x50);
}

static void ShowWaitMessage(const u8 *message, u8 (*waitCallback)(void))
{
    StringExpandPlaceholders(gStringVar4, message);
    sub_819786C(0, TRUE);
    AddTextPrinterForMessage_2(TRUE);
    sDialogCallback = waitCallback;
}

static void ShowSaveMessage(const u8 *message, u8 (*saveCallback)(void))
{
    StringExpandPlaceholders(gStringVar4, message);
    sub_819786C(0, TRUE);
    AddTextPrinterForMessage_2(TRUE);
    sSavingComplete = TRUE;
    sDialogCallback = saveCallback;
}

static void SaveGameTask(u8 taskId)
{
    u8 status = RunSaveCallback();

    switch (status)
    {
    case SAVE_CANCELED:
    case SAVE_ERROR:
        gSpecialVar_Result = 0;
        break;
    case SAVE_SUCCESS:
        gSpecialVar_Result = status;
        break;
    case SAVE_IN_PROGRESS:
        return;
    }

    DestroyTask(taskId);
    EnableBothScriptContexts();
	// Allow clock to update again
	gMain.stopClockUpdating = FALSE;
	// Clear option register
	gMain.optionRegister = FALSE;
}

static void sub_80A0014(void)
{
    ClearDialogWindowAndFrame(0, TRUE);
}

static void HideSaveInfoWindow(void)
{
    RemoveSaveInfoWindow();
}

static void SaveStartTimer(void)
{
    sSaveDialogTimer = 60;
}

static bool8 SaveSuccesTimer(void)
{
    sSaveDialogTimer--;

    if (gMain.heldKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        return TRUE;
    }
    else if (sSaveDialogTimer == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static bool8 SaveErrorTimer(void)
{
    if (sSaveDialogTimer != 0)
    {
        sSaveDialogTimer--;
    }
    else if (gMain.heldKeys & A_BUTTON)
    {
        return TRUE;
    }

    return FALSE;
}

static u8 SaveConfirmSaveCallback(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), FALSE);
    RemoveStartMenuWindow();
    ShowSaveInfoWindow();

    if (InBattlePyramid())
    {
        ShowSaveMessage(gText_BattlePyramidConfirmRest, SaveYesNoCallback);
    }
    else
    {
        ShowSaveMessage(gText_ConfirmSave, SaveYesNoCallback);
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveYesNoCallback(void)
{
    DisplayYesNoMenuDefaultYes(); // Show Yes/No menu
    sDialogCallback = SaveConfirmInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        switch (gSaveFileStatus)
        {
        case 0:
        case 2:
            if (gDifferentSaveFile == FALSE)
            {
                sDialogCallback = SaveFileExistsCallback;
                return SAVE_IN_PROGRESS;
            }

            sDialogCallback = SaveSavingMessageCallback;
            return SAVE_IN_PROGRESS;
        default:
            sDialogCallback = SaveFileExistsCallback;
            return SAVE_IN_PROGRESS;
        }
    case -1: // B Button
    case 1: // No
        HideSaveInfoWindow();
        sub_80A0014();
        return SAVE_CANCELED;
    }

    return SAVE_IN_PROGRESS;
}

// A different save file exists
static u8 SaveFileExistsCallback(void)
{
    if (gDifferentSaveFile == TRUE)
    {
        ShowSaveMessage(gText_DifferentSaveFile, SaveConfirmOverwriteDefaultNoCallback);
    }
    else
    {
        ShowSaveMessage(gText_AlreadySavedFile, SaveConfirmOverwriteCallback);
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmOverwriteDefaultNoCallback(void)
{
    DisplayYesNoMenuWithDefault(1); // Show Yes/No menu (No selected as default)
    sDialogCallback = SaveOverwriteInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveConfirmOverwriteCallback(void)
{
    DisplayYesNoMenuDefaultYes(); // Show Yes/No menu
    sDialogCallback = SaveOverwriteInputCallback;
    return SAVE_IN_PROGRESS;
}

static u8 SaveOverwriteInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        sDialogCallback = SaveSavingMessageCallback;
        return SAVE_IN_PROGRESS;
    case -1: // B Button
    case 1: // No
        HideSaveInfoWindow();
        sub_80A0014();
        return SAVE_CANCELED;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveSavingMessageCallback(void)
{
    ShowSaveMessage(gText_SavingDontTurnOff, SaveDoSaveCallback);
    return SAVE_IN_PROGRESS;
}

static u8 SaveDoSaveCallback(void)
{
    u8 saveStatus;

    IncrementGameStat(GAME_STAT_SAVED_GAME);
    sub_81A9E90();

    if (gDifferentSaveFile == TRUE)
    {
        saveStatus = TrySavingData(SAVE_OVERWRITE_DIFFERENT_FILE);
        gDifferentSaveFile = FALSE;
    }
    else
    {
        saveStatus = TrySavingData(SAVE_NORMAL);
    }

    if (saveStatus == 1) // Save succeded
    {
        ShowSaveMessage(gText_PlayerSavedGame, SaveSuccessCallback);
    }
    else                 // Save error
    {
        ShowSaveMessage(gText_SaveError, SaveErrorCallback);
    }

    SaveStartTimer();
    return SAVE_IN_PROGRESS;
}

static u8 SaveSuccessCallback(void)
{
    if (!IsTextPrinterActive(0))
    {
        PlaySE(SE_SAVE);
        sDialogCallback = SaveReturnSuccessCallback;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveReturnSuccessCallback(void)
{
    if (!IsSEPlaying() && SaveSuccesTimer())
    {
        HideSaveInfoWindow();
        return SAVE_SUCCESS;
    }
    else
    {
        return SAVE_IN_PROGRESS;
    }
}

static u8 SaveErrorCallback(void)
{
    if (!IsTextPrinterActive(0))
    {
        PlaySE(SE_BOO);
        sDialogCallback = SaveReturnErrorCallback;
    }

    return SAVE_IN_PROGRESS;
}

static u8 SaveReturnErrorCallback(void)
{
    if (!SaveErrorTimer())
    {
        return SAVE_IN_PROGRESS;
    }
    else
    {
        HideSaveInfoWindow();
        return SAVE_ERROR;
    }
}

static void InitBattlePyramidRetire(void)
{
    sDialogCallback = BattlePyramidConfirmRetireCallback;
    sSavingComplete = FALSE;
}

static u8 BattlePyramidConfirmRetireCallback(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), FALSE);
    RemoveStartMenuWindow();
    ShowSaveMessage(gText_BattlePyramidConfirmRetire, BattlePyramidRetireYesNoCallback);

    return SAVE_IN_PROGRESS;
}

static u8 BattlePyramidRetireYesNoCallback(void)
{
    DisplayYesNoMenuWithDefault(1); // Show Yes/No menu (No selected as default)
    sDialogCallback = BattlePyramidRetireInputCallback;

    return SAVE_IN_PROGRESS;
}

static u8 BattlePyramidRetireInputCallback(void)
{
    switch (Menu_ProcessInputNoWrapClearOnChoose())
    {
    case 0: // Yes
        return SAVE_CANCELED;
    case -1: // B Button
    case 1: // No
        sub_80A0014();
        return SAVE_SUCCESS;
    }

    return SAVE_IN_PROGRESS;
}

static void sub_80A03D8(void)
{
    TransferPlttBuffer();
}

static bool32 sub_80A03E4(u8 *par1)
{
    switch (*par1)
    {
    case 0:
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0);
        SetVBlankCallback(NULL);
        ScanlineEffect_Stop();
        DmaClear16(3, PLTT, PLTT_SIZE);
        DmaFillLarge16(3, 0, (void *)(VRAM + 0x0), 0x18000, 0x1000);
        break;
    case 1:
        ResetSpriteData();
        ResetTasks();
        ResetPaletteFade();
        ScanlineEffect_Clear();
        break;
    case 2:
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sUnknown_085105A8, ARRAY_COUNT(sUnknown_085105A8));
        InitWindows(sUnknown_085105AC);
        LoadUserWindowBorderGfx_(0, 8, 224);
        Menu_LoadStdPalAt(240);
        break;
    case 3:
        ShowBg(0);
        BlendPalettes(-1, 16, 0);
        SetVBlankCallback(sub_80A03D8);
        EnableInterrupts(1);
        break;
    case 4:
        return TRUE;
    }

    (*par1)++;
    return FALSE;
}

void sub_80A0514(void) // Called from cable_club.s
{
    if (sub_80A03E4(&gMain.state))
    {
        CreateTask(sub_80A0550, 0x50);
        SetMainCallback2(sub_80A0540);
    }
}

static void sub_80A0540(void)
{
    RunTasks();
    UpdatePaletteFade();
}

static void sub_80A0550(u8 taskId)
{
    s16 *step = gTasks[taskId].data;

    if (!gPaletteFade.active)
    {
        switch (*step)
        {
        case 0:
            FillWindowPixelBuffer(0, PIXEL_FILL(1));
            AddTextPrinterParameterized2(0,
                                        1,
                                        gText_SavingDontTurnOffPower,
                                        255,
                                        NULL,
                                        2,
                                        1,
                                        3);
            DrawTextBorderOuter(0, 8, 14);
            PutWindowTilemap(0);
            CopyWindowToVram(0, 3);
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);

            if (gWirelessCommType != 0 && InUnionRoom())
            {
                if (sub_800A07C())
                {
                    *step = 1;
                }
                else
                {
                    *step = 5;
                }
            }
            else
            {
                gSoftResetDisabled = 1;
                *step = 1;
            }
            break;
        case 1:
            SetContinueGameWarpStatusToDynamicWarp();
            FullSaveGame();
            *step = 2;
            break;
        case 2:
            if (CheckSaveFile())
            {
                ClearContinueGameWarpStatus2();
                *step = 3;
                gSoftResetDisabled = 0;
            }
            break;
        case 3:
            BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
            *step = 4;
            break;
        case 4:
            FreeAllWindowBuffers();
            SetMainCallback2(gMain.savedCallback);
            DestroyTask(taskId);
            break;
        case 5:
            CreateTask(sub_8153688, 0x5);
            *step = 6;
            break;
        case 6:
            if (!FuncIsActiveTask(sub_8153688))
            {
                *step = 3;
            }
            break;
        }
    }
}

static void ShowSaveInfoWindow(void)
{
    struct WindowTemplate saveInfoWindow = sSaveInfoWindowTemplate;
    u8 gender;
    u8 color;
    u32 xOffset;
    u32 yOffset;

    if (!FlagGet(FLAG_SYS_POKEDEX_GET))
    {
        saveInfoWindow.height -= 2;
    }

    sSaveInfoWindowId = AddWindow(&saveInfoWindow);
    DrawStdWindowFrame(sSaveInfoWindowId, FALSE);

    gender = gSaveBlock2Ptr->playerGender;
    color = TEXT_COLOR_RED;  // Red when female, blue when male.

    if (gender == MALE)
    {
        color = TEXT_COLOR_BLUE;
    }

    // Print region name
    yOffset = 1;
    sub_819A344(3, gStringVar4, TEXT_COLOR_GREEN);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, 0, yOffset, 0xFF, NULL);

    // Print player name
    yOffset = 0x11;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingPlayer, 0, yOffset, 0xFF, NULL);
    sub_819A344(0, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    PrintPlayerNameOnWindow(sSaveInfoWindowId, gStringVar4, xOffset, yOffset, FALSE);

    // Print badge count
    yOffset = 0x21;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingBadges, 0, yOffset, 0xFF, NULL);
    sub_819A344(4, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);

    if (FlagGet(FLAG_SYS_POKEDEX_GET) == TRUE)
    {
        // Print pokedex count
        yOffset = 0x31;
        AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingPokedex, 0, yOffset, 0xFF, NULL);
        sub_819A344(1, gStringVar4, color);
        xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
        AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);
    }

    // Print play time
    yOffset += 0x10;
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gText_SavingTime, 0, yOffset, 0xFF, NULL);
    sub_819A344(2, gStringVar4, color);
    xOffset = GetStringRightAlignXOffset(1, gStringVar4, 0x70);
    AddTextPrinterParameterized(sSaveInfoWindowId, 1, gStringVar4, xOffset, yOffset, 0xFF, NULL);

    CopyWindowToVram(sSaveInfoWindowId, 2);
}

static void RemoveSaveInfoWindow(void)
{
    ClearStdWindowAndFrame(sSaveInfoWindowId, FALSE);
    RemoveWindow(sSaveInfoWindowId);
}

static void sub_80A08A4(u8 taskId)
{
    if (!FuncIsActiveTask(sub_8153688))
    {
        DestroyTask(taskId);
        EnableBothScriptContexts();
    }
}

void sub_80A08CC(void) // Referenced in data/specials.inc and data/scripts/maps/BattleFrontier_BattleTowerLobby.inc
{
    u8 taskId = CreateTask(sub_8153688, 0x5);
    gTasks[taskId].data[2] = 1;
    gTasks[CreateTask(sub_80A08A4, 0x6)].data[1] = taskId;
}

static void HideStartMenuWindow(void)
{
    ClearStdWindowAndFrame(GetStartMenuWindowId(), TRUE);
    RemoveStartMenuWindow();
	// Clock can run again
	gMain.stopClockUpdating = FALSE;
    ScriptUnfreezeEventObjects();
    ScriptContext2_Disable();
}

void HideStartMenu(void) // Called from map_name_popup.s
{
    PlaySE(SE_SELECT);
    HideStartMenuWindow();
}

void AppendToList(u8 *list, u8 *pos, u8 newEntry)
{
    list[*pos] = newEntry;
    (*pos)++;
}

// Select is pressed in the overworld
bool32 UseRegisteredMenuOption(void)
{
	// Hide map name in case it's being shown
	HideMapNamePopUpWindow();
	
	// 0xF means no option is registered
	if (gSaveBlock2Ptr->startMenuRegister != 0xF)
	{
		gMain.optionRegister = TRUE;
		PlaySE(SE_SELECT);
		RunRegisteredStartOption();
		return;
	}
	// If nothing is registered, run script
	else
		ScriptContext1_SetupScript(EventScript_NoRegisteredMenuOption);
	return TRUE;
}

// Runs the start menu option that is bound to select
void RunRegisteredStartOption(void)
{
	// Stop clock updating as an option is run
	gMain.stopClockUpdating = TRUE;
	ScriptContext2_Enable();
	
	if (!gPaletteFade.active)
    {	
		// Save/wait are called differently
		if (sStartMenuItems[gSaveBlock2Ptr->startMenuRegister].func.u8_void == StartMenuSaveCallback)
			SaveGame();
		// Create task if not save/wait
		else
			CreateTask(RegisteredOptionTask, 0x50);	
	}

	// No need to fade screen for save/wait
	if (sStartMenuItems[gSaveBlock2Ptr->startMenuRegister].func.u8_void != StartMenuSaveCallback
	 && sStartMenuItems[gSaveBlock2Ptr->startMenuRegister].func.u8_void != StartMenuWaitCallback)
		FadeScreen(1, 0);
}

// Creates the task itself
void RegisteredOptionTask(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0:
        gMenuCallback = sStartMenuItems[gSaveBlock2Ptr->startMenuRegister].func.u8_void;
        task->data[0]++;
        break;
    case 1:
        if (gMenuCallback() == 1)
            DestroyTask(taskId);
        break;
    }
}
