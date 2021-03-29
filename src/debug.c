#if DEBUG

#include "global.h"
#include "debug.h"
#include "confetti_util.h"
#include "data.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "international_string_util.h"
#include "item.h"
#include "list_menu.h"
#include "main.h"
#include "map_name_popup.h"
#include "menu.h"
#include "naming_screen.h"
#include "overworld.h"
#include "party_menu.h"
#include "pokemon.h"
#include "region_map.h"
#include "save_location.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "constants/event_objects.h"
#include "constants/event_object_movement.h"
#include "constants/flags.h"
#include "constants/map_groups.h"
#include "constants/items.h"
#include "constants/songs.h"

// Functions
// Window Functions
void Debug_OpenDebugMenu(void);
static void Debug_ShowMainMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate);
static void Debug_ShowUtilitySubMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate);
static void Debug_ShowPartySubMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate);
static void Debug_DestroyMenu(u8);
static void Debug_DestroyExtraWindow(u8);

// Input Handlers
static void DebugTask_HandleMenuInput_Main(u8);
static void DebugTask_HandleMenuInput_Utility(u8);
static void DebugTask_HandleMenuInput_Party(u8);
static void DebugTask_HandleMenuInput_Flags(u8);
static void DebugTask_HandleMenuInput_Vars(u8);
static void DebugTask_HandleMenuInput_SelectMapGroup(u8);
static void DebugTask_HandleMenuInput_SelectMapNumber(u8);
static void DebugTask_HandleMenuInput_SelectWarpNumber(u8);
static void DebugTask_HandleMenuInput_CheckSaveBlockSize(u8);
static void DebugTask_HandleMenuInput_Credits(u8);
static void DebugTask_HandleMenuInput_GodMode(u8);
static void DebugTask_HandleMenuInput_GenderChange(u8);
static void DebugTask_HandleMenuInput_GiveMon(u8);

// Main Menu Functions
static void DebugAction_OpenCredits(u8);
static void DebugAction_OpenGodMode(u8);
static void DebugAction_OpenUtilityMenu(u8);
static void DebugAction_OpenPartyMenu(u8);

// Utility Functions
static void DebugAction_ManageFlags(u8);
static void DebugAction_ManageVars(u8);
static void DebugAction_SetVarValue(u8);
static void DebugAction_OpenWarpMenu(u8);
static void DebugAction_CheckSaveBlockSize(u8);
static void DebugAction_ChangePlayerName(u8);
static void DebugAction_OpenGenderChange(u8);
static void DebugAction_ChangePlayerGender(u8);

// Party Functions
static void DebugAction_GiveMons(u8);
static void DebugAction_HealParty(u8);

static void DebugAction_Cancel(u8);

#define DEBUG_MAIN_MENU_WIDTH 7
#define DEBUG_UTILITY_MENU_WIDTH 12
#define DEBUG_PARTY_MENU_WIDTH 10

#define DEBUG_CREDITS_DISPLAY_WIDTH 26
#define DEBUG_CREDITS_DISPLAY_HEIGHT 13

#define DEBUG_GODMODE_DISPLAY_WIDTH 26
#define DEBUG_GODMODE_DISPLAY_HEIGHT 8

#define DEBUG_NUMBER_DISPLAY_WIDTH 14
#define DEBUG_NUMBER_DISPLAY_HEIGHT 3

#define DEBUG_WARP_DISPLAY_WIDTH 14
#define DEBUG_WARP_DISPLAY_HEIGHT 4

#define DEBUG_SAVEBLOCK_DISPLAY_WIDTH 10
#define DEBUG_SAVEBLOCK_DISPLAY_HEIGHT 2

#define DEBUG_HELP_WINDOW_WIDTH 28
#define DEBUG_HELP_WINDOW_HEIGHT 2

#define DEBUG_NUMBER_DIGITS_FLAGS 4
#define DEBUG_NUMBER_DIGITS_VARIABLES 5

#define TAG_CONFETTI 1001

#define BOB_OTID 23501

// Copy from `include/map_groups.h`
// Keep this up to date with new maps for the EURUS REGION!
static const u8 MAP_GROUP_COUNT[] = {70, 10, 12, 13, 13, 14, 9, 7, 7, 14, 8, 17, 10, 23, 13, 15, 15, 2, 2, 2, 3, 1, 1, 1, 108, 61, 89, 2, 1, 13, 1, 1, 3, 1, 0};

// Debug EventScripts
extern const u8 Debug_EventScript_DoConfetti[];

// Main Menu Strings
static const u8 gDebugText_Credits[] = _("{COLOR}{GREEN}CREDITS");
static const u8 gDebugText_GodMode[] = _("{COLOR}{RED}GODMODE");
static const u8 gDebugText_Utility[] = _("{COLOR}{BLUE}UTILITY");
static const u8 gDebugText_Party[]   = _("{COLOR}{BLUE}PARTY");
static const u8 gDebugText_Cancel[]  = _("CANCEL");

// Credits Window Strings
static const u8 gDebugText_Credits_CreditsList[]        = _("{STR_VAR_1}\n{STR_VAR_2}\n{STR_VAR_3}");
static const u8 gDebugText_Credits_CreditsFriend[]      = _("{COLOR}{GREEN}JENA197, MOXIC");
static const u8 gDebugText_Credits_CreditsPatron[]      = _("{COLOR}{BLUE}CAANTHS");
static const u8 gDebugText_Credits_CreditsHelpingHand[] = _("{COLOR}{RED}ETHAN, N3RL, SPACESAUR, SENNA");

// GodMode Window Strings
static const u8 gDebugText_GodMode_GodModeExplaination[] = _("{COLOR}{DARK_GREY}YOU ARE ABOUT TO ENTER GODMODE!\nCOLLISION, ENCOUNTERS AND TRAINERS\nWILL BE DISABLED!");
static const u8 gDebugText_GodMode_EnableGodMode[]       = _("{A_BUTTON} {COLOR}{GREEN}ENABLE GODMODE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");
static const u8 gDebugText_GodMode_DisableGodMode[]      = _("{A_BUTTON} {COLOR}{RED}DISABLE GODMODE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");

// GiveMon Window Strings
static const u8 gDebugText_GiveMon_GiveMonExplaination[] = _("{COLOR}{DARK_GREY}YOU ARE ABOUT OVERWRITE YOUR\nCURRENT PARTY WITH A HANDCHOSEN ONE,\nMADE BY BSBOB!\n{COLOR}{RED}YOUR CURRENT PARTY WILL BE LOST!");
static const u8 gDebugText_Party_RevievedBobParty[]      = _("{COLOR}{GREEN}RECEIVED BSBOB'S PARTY   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");
static const u8 gDebugText_Party_AlreadyHasBobParty[]    = _("{COLOR}{GREEN}YOU ALREADY HAVE BSBOB'S PARTY!   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");

// Utility Menu Strings
static const u8 gDebugText_Utility_SaveBlocks[]   = _("{COLOR}{GREEN}SAVEBLOCKS");
static const u8 gDebugText_Utility_ManageFlag[]   = _("{COLOR}{RED}MANAGE FLAGS");
static const u8 gDebugText_Utility_ManageVars[]   = _("{COLOR}{RED}MANAGE VARS");
static const u8 gDebugText_Utility_Warp[]         = _("{COLOR}{RED}WARP");
static const u8 gDebugText_Utility_ChangeName[]   = _("{COLOR}{BLUE}CHANGE NAME");
static const u8 gDebugText_Utility_ChangeGender[] = _("{COLOR}{BLUE}CHANGE GENDER");

// Gender Change Window Strings
static const u8 gDebugText_GenderChange_GenderChangeExplaination[] = _("{COLOR}{DARK_GREY}YOU ARE ABOUT TO CHANGE YOUR GENDER!\n{COLOR}{GREEN}WALK THROUGH A DOOR{COLOR}{DARK_GREY} TO UPDATE YOUR\nPLAYER SPRITE.");
static const u8 gDebugText_GenderChange_ChangeToFemale[]           = _("{A_BUTTON} {COLOR}{GREEN}CHANGE TO FEMALE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");
static const u8 gDebugText_GenderChange_ChangeToMale[]             = _("{A_BUTTON} {COLOR}{GREEN}CHANGE TO MALE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");
static const u8 gDebugText_GenderChange_ChangedToFemale[]          = _("{A_BUTTON} {COLOR}{GREEN}CHANGED TO FEMALE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");
static const u8 gDebugText_GenderChange_ChangedToMale[]            = _("{A_BUTTON} {COLOR}{GREEN}CHANGED TO MALE   {B_BUTTON} {COLOR}{DARK_GREY}CANCEL");

// Party Menu Strings
static const u8 gDebugText_Party_GiveMon[]   = _("{COLOR}{RED}GIVE MON");
static const u8 gDebugText_Party_HealParty[] = _("{COLOR}{GREEN}HEAL PARTY");

// Flags Menu Strings
static const u8 gDebugText_Flag_FlagDef[]   = _("FLAG: {STR_VAR_1}\n{STR_VAR_2}\n{STR_VAR_3}");
static const u8 gDebugText_Flag_FlagHex[]   = _("{STR_VAR_1}         0x{STR_VAR_2}");
static const u8 gDebugText_Flag_FlagSet[]   = _("{COLOR}{GREEN}TRUE{COLOR}{DARK_GREY}");
static const u8 gDebugText_Flag_FlagUnset[] = _("{COLOR}{RED}FALSE{COLOR}{DARK_GREY}");

// Var Menu Strings
static const u8 gDebugText_Var_VariableDef[] = _("VAR: {COLOR}{GREEN}{STR_VAR_1}\n{COLOR}{DARK_GREY}VAL: {STR_VAR_3}\n{STR_VAR_2}");
static const u8 gDebugText_Var_VariableHex[] = _("{STR_VAR_1}       0x{STR_VAR_2}");
static const u8 gDebugText_Var_VariableVal[] = _("VAR: {STR_VAR_1}\nVAL: {COLOR}{GREEN}{STR_VAR_3}{COLOR}{DARK_GREY}\n{STR_VAR_2}{A_BUTTON}");

// Warp Menu Strings
static const u8 gDebugText_Util_WarpToMap[]                = _("WARP TO MAP WARP");
static const u8 gDebugText_Util_WarpToMap_SelectMapGroup[] = _("{COLOR}{GREEN}GROUP: {STR_VAR_1}{COLOR}{DARK_GREY}          \n                 \n\n{STR_VAR_3}     ");
static const u8 gDebugText_Util_WarpToMap_SelectMap[]      = _("{COLOR}{GREEN}MAP: {STR_VAR_1}{COLOR}{DARK_GREY}            \nMAPSEC:          \n{STR_VAR_2}                       \n{STR_VAR_3}     ");
static const u8 gDebugText_Util_WarpToMap_SelectWarp[]     = _("WARP:             \n{COLOR}{GREEN}{STR_VAR_1}{COLOR}{DARK_GREY}                \n                                  \n{STR_VAR_3}     ");
static const u8 gDebugText_Util_WarpToMap_SelectMax[]      = _("{STR_VAR_1} / {STR_VAR_2}");

// Check SaveBlock Strings
static const u8 gDebugText_SaveBlocks_SaveBlockSize[] = _("{COLOR}{GREEN}{STR_VAR_1}{COLOR}{DARK_GREY} IS\n{STR_VAR_2} BYTES");
static const u8 gDebugText_SaveBlocks_SaveBlock1[]    = _("SAVEBLOCK1");
static const u8 gDebugText_SaveBlocks_SaveBlock2[]    = _("SAVEBLOCK2");

// Help Strings
static const u8 gDebugText_Help_Warning[]    = _("{COLOR}{RED}WARNING!");
static const u8 gDebugText_Help_General[]    = _("{DPAD_UPDOWN} SELECT   {A_BUTTON} CONFIRM   {B_BUTTON} CANCEL");
static const u8 gDebugText_Help_Credits[]    = _("{A_BUTTON} SUPRISE   {COLOR}{GREEN}{EMOJI_CIRCLE} FRIEND   {COLOR}{BLUE}{EMOJI_CIRCLE} PATRON   {COLOR}{RED}{EMOJI_CIRCLE} HELPING HAND");
static const u8 gDebugText_Help_GiveMon[]    = _("{A_BUTTON} OVERWRITE PARTY   {B_BUTTON} CANCEL");
static const u8 gDebugText_Help_Flags[]      = _("{DPAD_UPDOWN} FLAG   {DPAD_LEFTRIGHT} DIGIT   {A_BUTTON} TOGGLE   {B_BUTTON} CANCEL");
static const u8 gDebugText_Help_Vars[]       = _("{DPAD_UPDOWN} VAR/VAL   {DPAD_LEFTRIGHT} DIGIT   {A_BUTTON} SELECT   {B_BUTTON} CANCEL");
static const u8 gDebugText_Help_Warp[]       = _("{DPAD_UPDOWN} WARP DATA   {DPAD_LEFTRIGHT} DIGIT   {A_BUTTON} SELECT   {B_BUTTON} CANCEL");
static const u8 gDebugText_Help_SaveBlocks[] = _("{DPAD_UPDOWN} SEEK   {B_BUTTON} BACK");

// Digit Indicator Strings
static const u8 digitInidicator_1[]        = _("{LEFT_ARROW}    +1                        {RIGHT_ARROW}");
static const u8 digitInidicator_10[]       = _("{LEFT_ARROW}    +10                      {RIGHT_ARROW}");
static const u8 digitInidicator_100[]      = _("{LEFT_ARROW}    +100                    {RIGHT_ARROW}");
static const u8 digitInidicator_1000[]     = _("{LEFT_ARROW}    +1000                  {RIGHT_ARROW}");
static const u8 digitInidicator_10000[]    = _("{LEFT_ARROW}    +10000                {RIGHT_ARROW}");
static const u8 digitInidicator_100000[]   = _("{LEFT_ARROW}    +100000              {RIGHT_ARROW}");
static const u8 digitInidicator_1000000[]  = _("{LEFT_ARROW}    +1000000            {RIGHT_ARROW}");
static const u8 digitInidicator_10000000[] = _("{LEFT_ARROW}    +10000000          {RIGHT_ARROW}");

const u8 * const gText_DigitIndicator[] =
{
    digitInidicator_1,
    digitInidicator_10,
    digitInidicator_100,
    digitInidicator_1000,
    digitInidicator_10000,
    digitInidicator_100000,
    digitInidicator_1000000,
    digitInidicator_10000000
};

static const s32 sPowersOfTen[] =
{
             1,
            10,
           100,
          1000,
         10000,
        100000,
       1000000,
      10000000,
     100000000,
    1000000000,
};

static const u16 gBobMonHeldItems[] =
{
    ITEM_LEFTOVERS
};

// Main Menu
enum {
    DEBUG_MENU_ITEM_CREDITS,
    DEBUG_MENU_ITEM_GODMODE,
    DEBUG_MENU_ITEM_UTILITY,
    DEBUG_MENU_ITEM_PARTY,
    DEBUG_MENU_ITEM_CANCEL,
};

// Utility Menu
enum {
    DEBUG_MENU_ITEM_SAVEBLOCKS,
    DEBUG_MENU_ITEM_MANAGE_FLAGS,
    DEBUG_MENU_ITEM_MANAGE_VARS,
    DEBUG_MENU_ITEM_WARP,
    DEBUG_MENU_ITEM_CHANGE_NAME,
    DEBUG_MENU_ITEM_CHANGE_GENDER,
};

enum {
    DEBUG_MENU_ITEM_GIVE_MONS,
    DEBUG_MENU_ITEM_HEAL_PARTY,
};

// List Menu Items
static const struct ListMenuItem sDebugMenuItems_Main[] =
{
    [DEBUG_MENU_ITEM_CREDITS] = {gDebugText_Credits, DEBUG_MENU_ITEM_CREDITS},
    [DEBUG_MENU_ITEM_GODMODE] = {gDebugText_GodMode, DEBUG_MENU_ITEM_GODMODE},
    [DEBUG_MENU_ITEM_UTILITY] = {gDebugText_Utility, DEBUG_MENU_ITEM_UTILITY},
    [DEBUG_MENU_ITEM_PARTY] = {gDebugText_Party, DEBUG_MENU_ITEM_PARTY},
    [DEBUG_MENU_ITEM_CANCEL] = {gDebugText_Cancel, DEBUG_MENU_ITEM_CANCEL},
};

static const struct ListMenuItem sDebugMenuItems_Utility[] =
{
    [DEBUG_MENU_ITEM_SAVEBLOCKS] = {gDebugText_Utility_SaveBlocks, DEBUG_MENU_ITEM_SAVEBLOCKS},
    [DEBUG_MENU_ITEM_MANAGE_FLAGS] = {gDebugText_Utility_ManageFlag, DEBUG_MENU_ITEM_MANAGE_FLAGS},
    [DEBUG_MENU_ITEM_MANAGE_VARS] = {gDebugText_Utility_ManageVars, DEBUG_MENU_ITEM_MANAGE_VARS},
    [DEBUG_MENU_ITEM_WARP] = {gDebugText_Utility_Warp, DEBUG_MENU_ITEM_WARP},
    [DEBUG_MENU_ITEM_CHANGE_NAME] = {gDebugText_Utility_ChangeName, DEBUG_MENU_ITEM_CHANGE_NAME},
    [DEBUG_MENU_ITEM_CHANGE_GENDER] = {gDebugText_Utility_ChangeGender, DEBUG_MENU_ITEM_CHANGE_GENDER},
};

static const struct ListMenuItem sDebugMenuItems_Party[] =
{
    [DEBUG_MENU_ITEM_GIVE_MONS] = {gDebugText_Party_GiveMon, DEBUG_MENU_ITEM_GIVE_MONS},
    [DEBUG_MENU_ITEM_HEAL_PARTY] = {gDebugText_Party_HealParty, DEBUG_MENU_ITEM_HEAL_PARTY},
};

// Menu Actions
static void (*const sDebugMenuActions_Main[])(u8) =
{
    [DEBUG_MENU_ITEM_CREDITS] = DebugAction_OpenCredits,
    [DEBUG_MENU_ITEM_GODMODE] = DebugAction_OpenGodMode,
    [DEBUG_MENU_ITEM_UTILITY] = DebugAction_OpenUtilityMenu,
    [DEBUG_MENU_ITEM_PARTY] = DebugAction_OpenPartyMenu,
    [DEBUG_MENU_ITEM_CANCEL] = DebugAction_Cancel,
};

static void (*const sDebugMenuActions_Utility[])(u8) =
{
    [DEBUG_MENU_ITEM_SAVEBLOCKS] = DebugAction_CheckSaveBlockSize,
    [DEBUG_MENU_ITEM_MANAGE_FLAGS] = DebugAction_ManageFlags,
    [DEBUG_MENU_ITEM_MANAGE_VARS] = DebugAction_ManageVars,
    [DEBUG_MENU_ITEM_WARP] = DebugAction_OpenWarpMenu,
    [DEBUG_MENU_ITEM_CHANGE_NAME] = DebugAction_ChangePlayerName,
    [DEBUG_MENU_ITEM_CHANGE_GENDER] = DebugAction_OpenGenderChange,
};

static void (*const sDebugMenuActions_Party[])(u8) =
{
    [DEBUG_MENU_ITEM_GIVE_MONS] = DebugAction_GiveMons,
    [DEBUG_MENU_ITEM_HEAL_PARTY] = DebugAction_HealParty,
};

// Window Templates
static const struct WindowTemplate sDebugMainMenuWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_MAIN_MENU_WIDTH,
    .height = 2 * ARRAY_COUNT(sDebugMenuItems_Main),
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct WindowTemplate sDebugUtilityMenuWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_UTILITY_MENU_WIDTH,
    .height = 2 * ARRAY_COUNT(sDebugMenuItems_Utility),
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct WindowTemplate sDebugPartyMenuWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_PARTY_MENU_WIDTH,
    .height = 2 * ARRAY_COUNT(sDebugMenuItems_Party),
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct WindowTemplate sDebugCreditsWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 2,
    .width = DEBUG_CREDITS_DISPLAY_WIDTH,
    .height = DEBUG_CREDITS_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct WindowTemplate sDebugWarningWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 2,
    .tilemapTop = 4,
    .width = DEBUG_GODMODE_DISPLAY_WIDTH,
    .height = DEBUG_GODMODE_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct WindowTemplate sDebugNumberDisplayWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_NUMBER_DISPLAY_WIDTH,
    .height = 2 * DEBUG_NUMBER_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 128,
};

static const struct WindowTemplate sDebugWarpDisplayWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_WARP_DISPLAY_WIDTH,
    .height = 2 * DEBUG_WARP_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 128,
};

static const struct WindowTemplate sDebugSaveBlockDisplayWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_SAVEBLOCK_DISPLAY_WIDTH,
    .height = 2 * DEBUG_SAVEBLOCK_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 128,
};

static const struct WindowTemplate sDebugHelpWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 18,
    .width = DEBUG_HELP_WINDOW_WIDTH,
    .height = DEBUG_HELP_WINDOW_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 360,
};

// List Menu Templates
static const struct ListMenuTemplate sDebugMenu_ListTemplate_Main =
{
    .items = sDebugMenuItems_Main,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .totalItems = ARRAY_COUNT(sDebugMenuItems_Main),
};


static const struct ListMenuTemplate sDebugMenu_ListTemplate_Utility = 
{
    .items = sDebugMenuItems_Utility,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .totalItems = ARRAY_COUNT(sDebugMenuItems_Utility),
};

static const struct ListMenuTemplate sDebugMenu_ListTemplate_Party = 
{
    .items = sDebugMenuItems_Party,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .totalItems = ARRAY_COUNT(sDebugMenuItems_Party),
};

// Window Functions
void Debug_OpenDebugMenu(void)
{
    Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
}

static void Debug_ShowMainMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate)
{
    struct ListMenuTemplate menuTemplate;
    u8 windowId1;
    u8 windowId2;
    u8 menuTaskId;
    u8 inputTaskId;

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugMainMenuWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);
    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    menuTemplate = ListMenuTemplate;
    menuTemplate.maxShowed = ARRAY_COUNT(sDebugMenuItems_Main);
    menuTemplate.windowId = windowId1;
    menuTemplate.header_X = 0;
    menuTemplate.item_X = 8;
    menuTemplate.cursor_X = 0;
    menuTemplate.upText_Y = 1;
    menuTemplate.cursorPal = 2;
    menuTemplate.fillValue = 1;
    menuTemplate.cursorShadowPal = 3;
    menuTemplate.lettersSpacing = 1;
    menuTemplate.itemVerticalPadding = 0;
    menuTemplate.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    menuTemplate.fontId = 1;
    menuTemplate.cursorKind = 0;
    menuTaskId = ListMenuInit(&menuTemplate, 0, 0);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_General);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    inputTaskId = CreateTask(HandleInput, 3);
    gTasks[inputTaskId].data[0] = menuTaskId;
    gTasks[inputTaskId].data[1] = windowId1;
    gTasks[inputTaskId].data[2] = windowId2;
}

static void Debug_ShowUtilitySubMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate)
{
    struct ListMenuTemplate menuTemplate;
    u8 windowId1;
    u8 windowId2;
    u8 menuTaskId;
    u8 inputTaskId;

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugUtilityMenuWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);
    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    menuTemplate = ListMenuTemplate;
    menuTemplate.maxShowed = ARRAY_COUNT(sDebugMenuItems_Utility);
    menuTemplate.windowId = windowId1;
    menuTemplate.header_X = 0;
    menuTemplate.item_X = 8;
    menuTemplate.cursor_X = 0;
    menuTemplate.upText_Y = 1;
    menuTemplate.cursorPal = 2;
    menuTemplate.fillValue = 1;
    menuTemplate.cursorShadowPal = 3;
    menuTemplate.lettersSpacing = 1;
    menuTemplate.itemVerticalPadding = 0;
    menuTemplate.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    menuTemplate.fontId = 1;
    menuTemplate.cursorKind = 0;
    menuTaskId = ListMenuInit(&menuTemplate, 0, 0);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_General);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    inputTaskId = CreateTask(HandleInput, 3);
    gTasks[inputTaskId].data[0] = menuTaskId;
    gTasks[inputTaskId].data[1] = windowId1;
    gTasks[inputTaskId].data[2] = windowId2;
}

static void Debug_ShowPartySubMenu(void (*HandleInput)(u8), struct ListMenuTemplate ListMenuTemplate)
{
    struct ListMenuTemplate menuTemplate;
    u8 windowId1;
    u8 windowId2;
    u8 menuTaskId;
    u8 inputTaskId;

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugPartyMenuWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);
    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    menuTemplate = ListMenuTemplate;
    menuTemplate.maxShowed = ARRAY_COUNT(sDebugMenuItems_Party);
    menuTemplate.windowId = windowId1;
    menuTemplate.header_X = 0;
    menuTemplate.item_X = 8;
    menuTemplate.cursor_X = 0;
    menuTemplate.upText_Y = 1;
    menuTemplate.cursorPal = 2;
    menuTemplate.fillValue = 1;
    menuTemplate.cursorShadowPal = 3;
    menuTemplate.lettersSpacing = 1;
    menuTemplate.itemVerticalPadding = 0;
    menuTemplate.scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
    menuTemplate.fontId = 1;
    menuTemplate.cursorKind = 0;
    menuTaskId = ListMenuInit(&menuTemplate, 0, 0);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_General);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    inputTaskId = CreateTask(HandleInput, 3);
    gTasks[inputTaskId].data[0] = menuTaskId;
    gTasks[inputTaskId].data[1] = windowId1;
    gTasks[inputTaskId].data[2] = windowId2;
}

static void Debug_DestroyMenu(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].data[0], NULL, NULL);
    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);
    DestroyTask(taskId);
}

static void Debug_DestroyExtraWindow(u8 taskId)
{
    ClearStdWindowAndFrame(gTasks[taskId].data[2], TRUE);
    RemoveWindow(gTasks[taskId].data[2]);
    DestroyTask(taskId);
}

// Input Handlers
static void DebugTask_HandleMenuInput_Main(u8 taskId)
{
    void (*func)(u8);
    u32 input = ListMenu_ProcessInput(gTasks[taskId].data[0]);

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if ((func = sDebugMenuActions_Main[input]) != NULL)
            func(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        Debug_DestroyMenu(taskId);
        EnableBothScriptContexts();
    }
}

static void DebugTask_HandleMenuInput_Utility(u8 taskId)
{
    void (*func)(u8);
    u32 input = ListMenu_ProcessInput(gTasks[taskId].data[0]);

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if ((func = sDebugMenuActions_Utility[input]) != NULL)
            func(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        Debug_DestroyMenu(taskId);
        Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
    }
}

static void DebugTask_HandleMenuInput_Party(u8 taskId)
{
    void (*func)(u8);
    u32 input = ListMenu_ProcessInput(gTasks[taskId].data[0]);

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if ((func = sDebugMenuActions_Party[input]) != NULL)
            func(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        Debug_DestroyMenu(taskId);
        Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
    }
}

static void DebugTask_HandleMenuInput_Flags(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
    {
        FlagToggle(gTasks[taskId].data[3]);
        if (FlagGet(gTasks[taskId].data[3]) == TRUE)
            PlaySE(SE_PC_LOGIN);
        else
            PlaySE(SE_PC_OFF);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }

    if (gMain.newKeys & DPAD_UP)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[3] += sPowersOfTen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] >= FLAGS_COUNT){
            gTasks[taskId].data[3] = FLAGS_COUNT - 1;
        }
    }

    if (gMain.newKeys & DPAD_DOWN)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[3] -= sPowersOfTen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] < 0){
            gTasks[taskId].data[3] = 0;
        }
    }

    if (gMain.newKeys & DPAD_LEFT)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[4] -= 1;
        if(gTasks[taskId].data[4] < 0)
        {
            gTasks[taskId].data[4] = 0;
        }
    }

    if (gMain.newKeys & DPAD_RIGHT)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[4] += 1;
        if(gTasks[taskId].data[4] > DEBUG_NUMBER_DIGITS_FLAGS-1)
        {
            gTasks[taskId].data[4] = DEBUG_NUMBER_DIGITS_FLAGS-1;
        }
    }

    if (gMain.newKeys & DPAD_ANY || gMain.newKeys & A_BUTTON)
    {
        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_FLAGS);
        ConvertIntToHexStringN(gStringVar2, gTasks[taskId].data[3], STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar1, gDebugText_Flag_FlagHex);
        if(FlagGet(gTasks[taskId].data[3]) == TRUE)
            StringCopyPadded(gStringVar2, gDebugText_Flag_FlagSet, CHAR_SPACE, 15);
        else
            StringCopyPadded(gStringVar2, gDebugText_Flag_FlagUnset, CHAR_SPACE, 15);
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        StringExpandPlaceholders(gStringVar4, gDebugText_Flag_FlagDef);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }
}

static void DebugTask_HandleMenuInput_Vars(u8 taskId)
{
    if (gMain.newKeys & DPAD_UP)
    {
        gTasks[taskId].data[3] += sPowersOfTen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] > VARS_END){
            gTasks[taskId].data[3] = VARS_END;
        }
    }
    if (gMain.newKeys & DPAD_DOWN)
    {
        gTasks[taskId].data[3] -= sPowersOfTen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] < VARS_START){
            gTasks[taskId].data[3] = VARS_START;
        }
    }
    if (gMain.newKeys & DPAD_LEFT)
    {
        gTasks[taskId].data[4] -= 1;
        if(gTasks[taskId].data[4] < 0)
        {
            gTasks[taskId].data[4] = 0;
        }
    }
    if (gMain.newKeys & DPAD_RIGHT)
    {
        gTasks[taskId].data[4] += 1;
        if(gTasks[taskId].data[4] > DEBUG_NUMBER_DIGITS_VARIABLES-1)
        {
            gTasks[taskId].data[4] = DEBUG_NUMBER_DIGITS_VARIABLES-1;
        }
    }

    if (gMain.newKeys & DPAD_ANY)
    {
        PlaySE(SE_SELECT);

        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        ConvertIntToHexStringN(gStringVar2, gTasks[taskId].data[3], STR_CONV_MODE_LEFT_ALIGN, 4);
        StringExpandPlaceholders(gStringVar1, gDebugText_Var_VariableHex);
        if (VarGetIfExist(gTasks[taskId].data[3]) == 65535) // Current value, if 65535 the value hasnt been set
            gTasks[taskId].data[5] = 0;
        else
            gTasks[taskId].data[5] = VarGet(gTasks[taskId].data[3]);
        ConvertIntToDecimalStringN(gStringVar3, gTasks[taskId].data[5], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        StringCopy(gStringVar2, gText_DigitIndicator[gTasks[taskId].data[4]]); // Current digit

        //Combine str's to full window string
        StringExpandPlaceholders(gStringVar4, gDebugText_Var_VariableDef);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }

    if (gMain.newKeys & A_BUTTON)
    {
        gTasks[taskId].data[4] = 0;

        PlaySE(SE_SELECT);

        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        ConvertIntToHexStringN(gStringVar2, gTasks[taskId].data[3], STR_CONV_MODE_LEFT_ALIGN, 4);
        StringExpandPlaceholders(gStringVar1, gDebugText_Var_VariableHex);
        if (VarGetIfExist(gTasks[taskId].data[3]) == 65535) // Current value if 65535 the value hasnt been set
            gTasks[taskId].data[5] = 0;
        else
            gTasks[taskId].data[5] = VarGet(gTasks[taskId].data[3]);
        ConvertIntToDecimalStringN(gStringVar3, gTasks[taskId].data[5], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
        StringCopy(gStringVar2, gText_DigitIndicator[gTasks[taskId].data[4]]); // Current digit
        StringExpandPlaceholders(gStringVar4, gDebugText_Var_VariableVal);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);

        gTasks[taskId].func = DebugAction_SetVarValue;
        gTasks[taskId].data[2] = gTasks[taskId].data[2];
        gTasks[taskId].data[6] = gTasks[taskId].data[5]; // New value selector
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_SelectMapGroup(u8 taskId)
{
    if (gMain.newKeys & DPAD_ANY)
    {
        PlaySE(SE_SELECT);
        if(gMain.newKeys & DPAD_UP)
        {
            gTasks[taskId].data[3] += sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] > MAP_GROUPS_COUNT-1)
                gTasks[taskId].data[3] = MAP_GROUPS_COUNT-1;
        }
        if(gMain.newKeys & DPAD_DOWN)
        {
            gTasks[taskId].data[3] -= sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] < 0)
                gTasks[taskId].data[3] = 0;
        }
        if(gMain.newKeys & DPAD_LEFT)
        {
            if(gTasks[taskId].data[4] > 0)
                gTasks[taskId].data[4] -= 1;
        }
        if(gMain.newKeys & DPAD_RIGHT)
        {
            if(gTasks[taskId].data[4] < 2)
                gTasks[taskId].data[4] += 1;
        }

        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
        ConvertIntToDecimalStringN(gStringVar2, 33, STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(gStringVar1, gDebugText_Util_WarpToMap_SelectMax);
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectMapGroup);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }

    if (gMain.newKeys & A_BUTTON)
    {
        gTasks[taskId].data[5] = gTasks[taskId].data[3];
        gTasks[taskId].data[3] = 0;
        gTasks[taskId].data[4] = 0;

        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
        ConvertIntToDecimalStringN(gStringVar2, gTasks[taskId].data[5]-1, STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(gStringVar1, gDebugText_Util_WarpToMap_SelectMax);
        GetMapName(gStringVar2, Overworld_GetMapHeaderByGroupAndId(gTasks[taskId].data[5], gTasks[taskId].data[3])->regionMapSectionId, 0);
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectMap);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);

        gTasks[taskId].func = DebugTask_HandleMenuInput_SelectMapNumber;
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_SelectMapNumber(u8 taskId)
{
    u8 max_value = MAP_GROUP_COUNT[gTasks[taskId].data[5]]; //maps in the selected map group

    if (gMain.newKeys & DPAD_ANY)
    {
        PlaySE(SE_SELECT);
        if(gMain.newKeys & DPAD_UP)
        {
            gTasks[taskId].data[3] += sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] > max_value-1)
                gTasks[taskId].data[3] = max_value-1;
        }
        if(gMain.newKeys & DPAD_DOWN)
        {
            gTasks[taskId].data[3] -= sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] < 0)
                gTasks[taskId].data[3] = 0;
        }
        if(gMain.newKeys & DPAD_LEFT)
        {
            if(gTasks[taskId].data[4] > 0)
                gTasks[taskId].data[4] -= 1;
        }
        if(gMain.newKeys & DPAD_RIGHT)
        {
            if(gTasks[taskId].data[4] < 2)
                gTasks[taskId].data[4] += 1;
        }

        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
        ConvertIntToDecimalStringN(gStringVar2, MAP_GROUP_COUNT[gTasks[taskId].data[5]]-1, STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(gStringVar1, gDebugText_Util_WarpToMap_SelectMax);
        GetMapName(gStringVar2, Overworld_GetMapHeaderByGroupAndId(gTasks[taskId].data[5], gTasks[taskId].data[3])->regionMapSectionId, 0);
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectMap);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }

    if (gMain.newKeys & A_BUTTON)
    {
        gTasks[taskId].data[6] = gTasks[taskId].data[3];
        gTasks[taskId].data[3] = 0;
        gTasks[taskId].data[4] = 0;

        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectWarp);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
        gTasks[taskId].func = DebugTask_HandleMenuInput_SelectWarpNumber;
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_SelectWarpNumber(u8 taskId)
{
    if (gMain.newKeys & DPAD_ANY)
    {
        PlaySE(SE_SELECT);
        if(gMain.newKeys & DPAD_UP)
        {
            gTasks[taskId].data[3] += sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] > 10)
                gTasks[taskId].data[3] = 10;
        }
        if(gMain.newKeys & DPAD_DOWN)
        {
            gTasks[taskId].data[3] -= sPowersOfTen[gTasks[taskId].data[4]];
            if(gTasks[taskId].data[3] < 0)
                gTasks[taskId].data[3] = 0;
        }

        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
        StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectWarp);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }

    if (gMain.newKeys & A_BUTTON)
    {
        gTasks[taskId].data[7] = gTasks[taskId].data[3];
        //WARP
        SetWarpDestinationToMapWarp(gTasks[taskId].data[5], gTasks[taskId].data[6], gTasks[taskId].data[7]); //If not warp with the number available -> center of map
        DoWarp();
        ResetInitialPlayerAvatarState();
        Debug_DestroyExtraWindow(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_CheckSaveBlockSize(u8 taskId)
{
    if (gMain.newKeys & DPAD_UP)
    {
        StringCopy(gStringVar1, gDebugText_SaveBlocks_SaveBlock1);
        ConvertIntToDecimalStringN(gStringVar2, sizeof(struct SaveBlock1), STR_CONV_MODE_LEFT_ALIGN, 6);
    }
    
    if (gMain.newKeys & DPAD_DOWN)
    {
        StringCopy(gStringVar1, gDebugText_SaveBlocks_SaveBlock2);
        ConvertIntToDecimalStringN(gStringVar2, sizeof(struct SaveBlock2), STR_CONV_MODE_LEFT_ALIGN, 6);
    }
    
    if (gMain.newKeys & DPAD_ANY)
    {
        PlaySE(SE_SELECT);
        FillWindowPixelBuffer(gTasks[taskId].data[1], PIXEL_FILL(1));
        StringExpandPlaceholders(gStringVar3, gDebugText_SaveBlocks_SaveBlockSize);
        AddTextPrinterParameterized(gTasks[taskId].data[1], 1, gStringVar3, 1, 1, 0, NULL);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        RemoveScrollIndicatorArrowPair(gTasks[taskId].data[3]);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_Credits(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_M_ENCORE2);
        ScriptContext1_SetupScript(Debug_EventScript_DoConfetti);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        ConfettiUtil_Free();
        FreeSpriteTilesByTag(TAG_CONFETTI);
        FreeSpritePaletteByTag(TAG_CONFETTI);
        Debug_DestroyMenu(taskId);
        Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
    }
}

static void DebugTask_HandleMenuInput_GodMode(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
    {
        if (FlagGet(FLAG_DISABLE_ENCOUNTERS) && FlagGet(FLAG_DISABLE_TRAINERS) && FlagGet(FLAG_DISABLE_OBJECT_COLLISION))
        {
            PlaySE(SE_PC_OFF);
            StringExpandPlaceholders(gStringVar4, gDebugText_GodMode_EnableGodMode);
        }
        else
        {
            PlaySE(SE_PC_LOGIN);
            StringExpandPlaceholders(gStringVar4, gDebugText_GodMode_DisableGodMode);
        }
        FillWindowPixelBuffer(gTasks[taskId].data[8], PIXEL_FILL(1));
        AddTextPrinterParameterized(gTasks[taskId].data[8], 0, gStringVar4, 1, 1, 0, NULL);
        FlagToggle(FLAG_DISABLE_ENCOUNTERS);
        FlagToggle(FLAG_DISABLE_TRAINERS);
        FlagToggle(FLAG_DISABLE_OBJECT_COLLISION);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyMenu(taskId);
        Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
    }
}

static void DebugTask_HandleMenuInput_GenderChange(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
    {
        if (gSaveBlock2Ptr->playerGender == MALE)
            gSaveBlock2Ptr->playerGender = FEMALE;
        else
            gSaveBlock2Ptr->playerGender = MALE;

        if (gSaveBlock2Ptr->playerGender == MALE)
            StringExpandPlaceholders(gStringVar4, gDebugText_GenderChange_ChangedToMale);
        else
            StringExpandPlaceholders(gStringVar4, gDebugText_GenderChange_ChangedToFemale);
        
        PlaySE(SE_PC_LOGIN);
        FillWindowPixelBuffer(gTasks[taskId].data[8], PIXEL_FILL(1));
        AddTextPrinterParameterized(gTasks[taskId].data[8], 0, gStringVar4, 1, 1, 0, NULL);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyExtraWindow(taskId);
        DebugAction_OpenUtilityMenu(taskId);
    }
}

static void DebugTask_HandleMenuInput_GiveMon(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
    {
        PlayFanfare(MUS_SLOTS_WIN);
        FillWindowPixelBuffer(gTasks[taskId].data[8], PIXEL_FILL(1));
        StringExpandPlaceholders(gStringVar4, gDebugText_Party_RevievedBobParty);
        AddTextPrinterParameterized(gTasks[taskId].data[8], 0, gStringVar4, 1, 1, 0, NULL);

        ZeroPlayerPartyMons();
        FlagSet(FLAG_HAS_BOB_PARTY);

        // Give Mons
        CreateMon(&gPlayerParty[0], SPECIES_BLAZIKEN, 80, MAX_PER_STAT_IVS, 1, NATURE_BRAVE, OT_ID_PRESET, BOB_OTID);
        CreateMon(&gPlayerParty[1], SPECIES_GARDEVOIR, 80, MAX_PER_STAT_IVS, 1, NATURE_TIMID, OT_ID_PRESET, BOB_OTID);
        CreateMon(&gPlayerParty[2], SPECIES_NINJASK, 80, MAX_PER_STAT_IVS, 1, NATURE_ADAMANT, OT_ID_PRESET, BOB_OTID);
        CreateMon(&gPlayerParty[3], SPECIES_MILOTIC, 80, MAX_PER_STAT_IVS, 1, NATURE_MILD, OT_ID_PRESET, BOB_OTID);
        CreateMon(&gPlayerParty[4], SPECIES_AGGRON, 80, MAX_PER_STAT_IVS, 1, NATURE_IMPISH, OT_ID_PRESET, BOB_OTID);
        CreateMon(&gPlayerParty[5], SPECIES_FLYGON, 80, MAX_PER_STAT_IVS, 1, NATURE_BRAVE, OT_ID_PRESET, BOB_OTID);

        // Give Held Items
        SetMonData(&gPlayerParty[0], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);
        SetMonData(&gPlayerParty[1], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);
        SetMonData(&gPlayerParty[2], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);
        SetMonData(&gPlayerParty[3], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);
        SetMonData(&gPlayerParty[4], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);
        SetMonData(&gPlayerParty[5], MON_DATA_HELD_ITEM, &gBobMonHeldItems[0]);

        // Set Blaziken Move Data
        SetMonMoveSlot(&gPlayerParty[0], MOVE_BLAZE_KICK, 0);
        SetMonMoveSlot(&gPlayerParty[0], MOVE_SKY_UPPERCUT, 1);
        SetMonMoveSlot(&gPlayerParty[0], MOVE_SLASH, 2);
        SetMonMoveSlot(&gPlayerParty[0], MOVE_LOW_KICK, 3);

        // Set Gardevoir Move Data
        SetMonMoveSlot(&gPlayerParty[1], MOVE_PSYCHIC, 0);
        SetMonMoveSlot(&gPlayerParty[1], MOVE_THUNDERBOLT, 1);
        SetMonMoveSlot(&gPlayerParty[1], MOVE_SHADOW_BALL, 2);
        SetMonMoveSlot(&gPlayerParty[1], MOVE_PROTECT, 3);

        // Set Ninjask Move Data
        SetMonMoveSlot(&gPlayerParty[2], MOVE_SWORDS_DANCE, 0);
        SetMonMoveSlot(&gPlayerParty[2], MOVE_AERIAL_ACE, 1);
        SetMonMoveSlot(&gPlayerParty[2], MOVE_SLASH, 2);
        SetMonMoveSlot(&gPlayerParty[2], MOVE_SUBSTITUTE, 3);

        // Set Milotic Move Data
        SetMonMoveSlot(&gPlayerParty[3], MOVE_ICE_BEAM, 0);
        SetMonMoveSlot(&gPlayerParty[3], MOVE_SURF, 1);
        SetMonMoveSlot(&gPlayerParty[3], MOVE_DIVE, 2);
        SetMonMoveSlot(&gPlayerParty[3], MOVE_WATERFALL, 3);

        // Set Aggron Move Data
        SetMonMoveSlot(&gPlayerParty[4], MOVE_EARTHQUAKE, 0);
        SetMonMoveSlot(&gPlayerParty[4], MOVE_METAL_CLAW, 1);
        SetMonMoveSlot(&gPlayerParty[4], MOVE_IRON_DEFENSE, 2);
        SetMonMoveSlot(&gPlayerParty[4], MOVE_IRON_TAIL, 3);

        // Set Flygon Move Data
        SetMonMoveSlot(&gPlayerParty[5], MOVE_DRAGON_BREATH, 0);
        SetMonMoveSlot(&gPlayerParty[5], MOVE_DRAGON_CLAW, 1);
        SetMonMoveSlot(&gPlayerParty[5], MOVE_FLY, 2);
        SetMonMoveSlot(&gPlayerParty[5], MOVE_THUNDERBOLT, 23);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyMenu(taskId);
        Debug_ShowMainMenu(DebugTask_HandleMenuInput_Main, sDebugMenu_ListTemplate_Main);
    }
}

// Main Menu Functions
static void DebugAction_OpenCredits(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;
    u8 tConfettiTaskId;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugCreditsWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Credits
    StringCopy(gStringVar1, gDebugText_Credits_CreditsFriend);
    StringCopy(gStringVar2, gDebugText_Credits_CreditsPatron);
    StringCopy(gStringVar3, gDebugText_Credits_CreditsHelpingHand);
    StringExpandPlaceholders(gStringVar4, gDebugText_Credits_CreditsList);
    AddTextPrinterParameterized(windowId1, 0, gStringVar4, 1, 1, 0, NULL);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_Credits);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_Credits;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[3] = tConfettiTaskId;
    gTasks[taskId].data[8] = windowId2;
}

static void DebugAction_OpenGodMode(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;
    u8 x;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugWarningWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display GodMode Warning
    StringExpandPlaceholders(gStringVar1, gDebugText_Help_Warning);
    x = GetStringCenterAlignXOffset(1, gStringVar1, 210);
    AddTextPrinterParameterized(windowId1, 1, gStringVar1, x, 1, 0, NULL);

    // Display GodMode Explaination
    StringExpandPlaceholders(gStringVar2, gDebugText_GodMode_GodModeExplaination);
    AddTextPrinterParameterized(windowId1, 1, gStringVar2, 1, 16, 0, NULL);

    // Display Help
    if (FlagGet(FLAG_DISABLE_ENCOUNTERS) && FlagGet(FLAG_DISABLE_TRAINERS) && FlagGet(FLAG_DISABLE_OBJECT_COLLISION))
        StringExpandPlaceholders(gStringVar4, gDebugText_GodMode_DisableGodMode);
    else
        StringExpandPlaceholders(gStringVar4, gDebugText_GodMode_EnableGodMode);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_GodMode;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
}

static void DebugAction_OpenUtilityMenu(u8 taskId)
{
    Debug_DestroyExtraWindow(taskId);
    Debug_DestroyMenu(taskId);
    Debug_ShowUtilitySubMenu(DebugTask_HandleMenuInput_Utility, sDebugMenu_ListTemplate_Utility);
}

static void DebugAction_OpenPartyMenu(u8 taskId)
{
    Debug_DestroyExtraWindow(taskId);
    Debug_DestroyMenu(taskId);
    Debug_ShowPartySubMenu(DebugTask_HandleMenuInput_Party, sDebugMenu_ListTemplate_Party);
}

// Utility Functions
static void DebugAction_ManageFlags(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugNumberDisplayWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display initial Flag
    ConvertIntToDecimalStringN(gStringVar1, 0, STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_FLAGS);
    ConvertIntToHexStringN(gStringVar2, 0, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar1, gDebugText_Flag_FlagHex);
    if(FlagGet(0) == TRUE)
        StringCopyPadded(gStringVar2, gDebugText_Flag_FlagSet, CHAR_SPACE, 15);
    else
        StringCopyPadded(gStringVar2, gDebugText_Flag_FlagUnset, CHAR_SPACE, 15);
    StringCopy(gStringVar3, gText_DigitIndicator[0]);
    StringExpandPlaceholders(gStringVar4, gDebugText_Flag_FlagDef);
    AddTextPrinterParameterized(windowId1, 1, gStringVar4, 1, 1, 0, NULL);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_Flags);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_Flags;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
    gTasks[taskId].data[3] = 0;            // Current Flag
    gTasks[taskId].data[4] = 0;            // Digit Selected
}

static void DebugAction_ManageVars(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugNumberDisplayWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);
    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    //Display initial Variable
    ConvertIntToDecimalStringN(gStringVar1, VARS_START, STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
    ConvertIntToHexStringN(gStringVar2, VARS_START, STR_CONV_MODE_LEFT_ALIGN, 4);
    StringExpandPlaceholders(gStringVar1, gDebugText_Var_VariableHex);
    ConvertIntToDecimalStringN(gStringVar3, 0, STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
    StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
    StringCopy(gStringVar2, gText_DigitIndicator[0]);
    StringExpandPlaceholders(gStringVar4, gDebugText_Var_VariableDef);
    AddTextPrinterParameterized(windowId1, 1, gStringVar4, 1, 1, 0, NULL);

    //Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_Vars);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_Vars;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
    gTasks[taskId].data[3] = VARS_START;   // Current Variable
    gTasks[taskId].data[4] = 0;            // Digit Selected
    gTasks[taskId].data[5] = 0;            // Current Variable VALUE
}

static void DebugAction_SetVarValue(u8 taskId)
{
    if (gMain.newKeys & DPAD_UP)
    {
        if (gTasks[taskId].data[6] + sPowersOfTen[gTasks[taskId].data[4]] <= 32000)
            gTasks[taskId].data[6] += sPowersOfTen[gTasks[taskId].data[4]];
        else
            gTasks[taskId].data[6] = 32000-1;
        if (gTasks[taskId].data[6] >= 32000)
            gTasks[taskId].data[6] = 32000-1;
    }

    if (gMain.newKeys & DPAD_DOWN)
    {
        gTasks[taskId].data[6] -= sPowersOfTen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[6] < 0){
            gTasks[taskId].data[6] = 0;
        }
    }

    if (gMain.newKeys & DPAD_LEFT)
    {
        gTasks[taskId].data[4] -= 1;
        if(gTasks[taskId].data[4] < 0)
        {
            gTasks[taskId].data[4] = 0;
        }
    }

    if (gMain.newKeys & DPAD_RIGHT)
    {
        gTasks[taskId].data[4] += 1;
        if(gTasks[taskId].data[4] > 4)
        {
            gTasks[taskId].data[4] = 4;
        }
    }

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        VarSet(gTasks[taskId].data[3], gTasks[taskId].data[6]);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        DebugAction_ManageVars(taskId);
    }

    if (gMain.newKeys & DPAD_ANY || gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);

        FillWindowPixelBuffer(gTasks[taskId].data[2], PIXEL_FILL(1));
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        ConvertIntToHexStringN(gStringVar2, gTasks[taskId].data[3], STR_CONV_MODE_LEFT_ALIGN, 4);
        StringExpandPlaceholders(gStringVar1, gDebugText_Var_VariableHex);
        StringCopyPadded(gStringVar1, gStringVar1, CHAR_SPACE, 15);
        ConvertIntToDecimalStringN(gStringVar3, gTasks[taskId].data[6], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_VARIABLES);
        StringCopyPadded(gStringVar3, gStringVar3, CHAR_SPACE, 15);
        StringCopy(gStringVar2, gText_DigitIndicator[gTasks[taskId].data[4]]); // Current digit
        StringExpandPlaceholders(gStringVar4, gDebugText_Var_VariableVal);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }
}

static void DebugAction_OpenWarpMenu(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugWarpDisplayWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Warp
    ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, 2);
    ConvertIntToDecimalStringN(gStringVar2, 33, STR_CONV_MODE_LEADING_ZEROS, 2);
    StringExpandPlaceholders(gStringVar1, gDebugText_Util_WarpToMap_SelectMax);
    StringCopy(gStringVar3, gText_DigitIndicator[0]);
    StringExpandPlaceholders(gStringVar4, gDebugText_Util_WarpToMap_SelectMapGroup);
    AddTextPrinterParameterized(windowId1, 1, gStringVar4, 1, 1, 0, NULL);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_Warp);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_SelectMapGroup;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
    gTasks[taskId].data[3] = 0;            // Current Warp
    gTasks[taskId].data[4] = 0;            // Digit Selected
    gTasks[taskId].data[5] = 0;            // Map Group
    gTasks[taskId].data[6] = 0;            // Map Number
    gTasks[taskId].data[7] = 0;            // Warp
}

static void DebugAction_CheckSaveBlockSize(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;
    u8 tArrowTaskId;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugSaveBlockDisplayWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display Scrolling Indicator
    tArrowTaskId = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 93, 6, 42, 0, 110, 110, 0);

    // Display SaveBlock1
    StringCopy(gStringVar1, gDebugText_SaveBlocks_SaveBlock1);
    ConvertIntToDecimalStringN(gStringVar2, sizeof(struct SaveBlock1), STR_CONV_MODE_LEFT_ALIGN, 6);
    StringExpandPlaceholders(gStringVar3, gDebugText_SaveBlocks_SaveBlockSize);
    AddTextPrinterParameterized(windowId1, 1, gStringVar3, 1, 1, 0, NULL);

    // Display Help
    StringExpandPlaceholders(gStringVar4, gDebugText_Help_SaveBlocks);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_CheckSaveBlockSize;
    gTasks[taskId].data[1] = windowId1;
    gTasks[taskId].data[3] = tArrowTaskId;
}

static void DebugAction_ChangePlayerName(u8 taskId)
{
    DoNamingScreen(NAMING_SCREEN_PLAYER, gSaveBlock2Ptr->playerName, gSaveBlock2Ptr->playerGender, 0, 0, CB2_ReturnToField);
}

static void DebugAction_OpenGenderChange(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;
    u8 x;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugWarningWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display GenderChange Warning
    StringExpandPlaceholders(gStringVar1, gDebugText_Help_Warning);
    x = GetStringCenterAlignXOffset(1, gStringVar1, 210);
    AddTextPrinterParameterized(windowId1, 1, gStringVar1, x, 1, 0, NULL);

    // Display GodMode Explaination
    StringExpandPlaceholders(gStringVar2, gDebugText_GenderChange_GenderChangeExplaination);
    AddTextPrinterParameterized(windowId1, 1, gStringVar2, 1, 16, 0, NULL);

    // Display Help
    if (gSaveBlock2Ptr->playerGender == MALE)
        StringExpandPlaceholders(gStringVar4, gDebugText_GenderChange_ChangeToFemale);
    else
        StringExpandPlaceholders(gStringVar4, gDebugText_GenderChange_ChangeToMale);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_GenderChange;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
}

static void DebugAction_ChangePlayerGender(u8 taskId)
{
    struct ObjectEvent *objEvent = &gObjectEvents[gPlayerAvatar.objectEventId];

    if (gSaveBlock2Ptr->playerGender == MALE)
        gSaveBlock2Ptr->playerGender = FEMALE;
    else
        gSaveBlock2Ptr->playerGender = MALE;

    ResetInitialPlayerAvatarState();
}

// Party Functions
static void DebugAction_GiveMons(u8 taskId)
{
    u8 windowId1;
    u8 windowId2;
    u8 x;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    HideMapNamePopUpWindow();
    LoadMessageBoxAndBorderGfx();

    windowId1 = AddWindow(&sDebugWarningWindowTemplate);
    windowId2 = AddWindow(&sDebugHelpWindowTemplate);

    DrawStdWindowFrame(windowId1, FALSE);
    DrawStdWindowFrame(windowId2, FALSE);

    CopyWindowToVram(windowId1, 3);
    CopyWindowToVram(windowId2, 4);

    // Display GiveMon Warning
    StringExpandPlaceholders(gStringVar1, gDebugText_Help_Warning);
    x = GetStringCenterAlignXOffset(1, gStringVar1, 210);
    AddTextPrinterParameterized(windowId1, 1, gStringVar1, x, 1, 0, NULL);

    // Display GiveMon Explaination
    StringExpandPlaceholders(gStringVar2, gDebugText_GiveMon_GiveMonExplaination);
    AddTextPrinterParameterized(windowId1, 1, gStringVar2, 1, 16, 0, NULL);

    // Display Help
    if (FlagGet(FLAG_HAS_BOB_PARTY))
        StringExpandPlaceholders(gStringVar4, gDebugText_Party_AlreadyHasBobParty);
    else
        StringExpandPlaceholders(gStringVar4, gDebugText_Help_GiveMon);
    AddTextPrinterParameterized(windowId2, 0, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugTask_HandleMenuInput_GiveMon;
    gTasks[taskId].data[2] = windowId1;
    gTasks[taskId].data[8] = windowId2;
}

static void DebugAction_HealParty(u8 taskId)
{
    HealPlayerParty();
    PlaySE(SE_USE_ITEM);
}

// Cancel Function
static void DebugAction_Cancel(u8 taskId)
{
    Debug_DestroyExtraWindow(taskId);
    Debug_DestroyMenu(taskId);
    EnableBothScriptContexts();
}

#endif