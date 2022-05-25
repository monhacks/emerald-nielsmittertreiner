#include "global.h"
#include "malloc.h"
#include "bg.h"
#include "gpu_regs.h"
#include "task.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "event_object_movement.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "list_menu.h"
#include "palette.h"
#include "pokenav_v2.h"
#include "region_map.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "sound.h"
#include "time.h"
#include "international_string_util.h"
#include "string_util.h"
#include "strings.h"
#include "window.h"
#include "text.h"
#include "text_window.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/weather.h"
#include "debug/mgba.h"
#include "debug/printf.h"

#define TASK_MAIN         0
#define TASK_CLOCK        1
#define TASK_LIST_MENU    2
#define TASK_EVENT_TIME   3
#define TASK_SCROLL_ARROW 4

#define WIN_HEADER                0
#define WIN_DESC                  1
#define WIN_OPTION_TOP_LEFT       2
#define WIN_OPTION_BOTTOM_LEFT    3
#define WIN_OPTION_TOP_RIGHT      4
#define WIN_OPTION_BOTTOM_RIGHT   5
#define WIN_REGION_MAP_TITLE      6
#define WIN_REGION_MAP_SECTION    7
#define WIN_AGENDA_EVENTS_TITLE   8
#define WIN_AGENDA_EVENTS_CONTENT 9
#define WIN_AGENDA_DATE_TIME      10
#define WIN_AGENDA_WEATHER        11

#define EVENT_LOTTERY          0
#define EVENT_SALE             1
#define EVENT_WORLD_TOURNAMENT 2
#define EVENT_CATCHING_CONTEST 3
#define EVENT_TREASHURE_HUNT   4
#define EVENT_SAFARI_ZONE      5
#define EVENT_CEL_NAUTILUS     6
#define EVENT_MARKET           7

#define OPTION_SLIDE_X      128
#define OPTION_SLIDE_Y      160
#define OPTION_SLIDE_SPEED  16

#define TAG_PAL           100
#define TAG_OPTIONS_LEFT  101
#define TAG_OPTIONS_RIGHT 102
#define TAG_AGENDA_ICONS  103
#define TAG_SCROLL_ARROW  104

struct Pokenav2Struct
{
    struct RegionMap *regionMap;
    const struct EventSchedule *eventDataPtr;
    const struct EventTextData *eventTextPtr;
    struct ListMenuItem *listMenuItems;
    u8 spriteIds[9];
    u8 taskIds[6];
    u16 scrollOffset;
    u16 selectedRow;
    u16 currItem;
    s16 scrollPosition;
    u8 cursorPosition;
    u8 is24HClockMode:1;
};

struct WeatherReportText
{
    const u8 *string;
    const u8 *color;
};

struct EventSchedule
{
    u8 eventId;
    u8 location;
    u16 time[2];
};

struct EventTextData
{
    const u8 *title;
    const u8 *description;
};

struct EventScheduleTable
{
    const struct EventSchedule *ptr;
    u8 size;
};

enum Option
{
    RegionMap,
    Agenda,
    Radio,
    MainMenu
};

static void VBlankCB_Pokenav2(void);
static void CB2_Pokenav2(void);
static void LoadOptionBgs(enum Option option);
static void LoadOption(enum Option option);
static void LoadMainMenu(void);
static void LoadRegionMap(void);
static void LoadAgenda(void);
static void InitEventWindows(void);
static void CreateEventListMenuTemplate(void);
static void LoadRadio(void);
static void LoadOptionAndIconSprites(void);
static void UnloadOption(enum Option option);
static void UnloadMainMenu(void);
static void UnloadRegionMap(void);
static void UnloadAgenda(void);
static void UnloadRadio(void);
static void UnloadOptionAndIconSprites(void);
static void SpriteCB_Icons(struct Sprite *sprite);
static void UpdateOptionDescription(u8 option);
static void Task_Pokenav2_1(u8 taskId);
static void Task_Pokenav2_2(u8 taskId);
static void Task_Pokenav2_3(u8 taskId);
static void Task_RegionMap(u8 taskId);
static void Task_Agenda(u8 taskId);
static void Task_EventDetailsPage(u8 taskId);
static void Task_FormatClock(u8 taskId);
static void Task_FormatEventTime(u8 taskId);
static void FormatTimeString(u8 *dest, s8 hours, s8 minutes);
static void CreateEventDetailsPage(u8 input);
static void FormatEventTimeDisplay(u8 *dest, u8 input);
static void Task_Radio(u8 taskId);
static bool8 Task_SlideMainMenuIn(u8 taskId);
static bool8 Task_SlideMainMenuOut(u8 taskId);
static bool8 Task_SlideOptionIn(u8 taskId);
static bool8 Task_SlideOptionOut(u8 taskId);
static void Task_LoadOption_1(u8 taskId);
static void Task_LoadOption_2(u8 taskId);
static void Task_LoadOption_3(u8 taskId);
static void Task_LoadOption_4(u8 taskId);
static void Task_LoadOption_5(u8 taskId);
static void Task_LoadOption_6(u8 taskId);
static void Task_LoadOption_7(u8 taskId);
static void Task_ReturnToMainMenu_1(u8 taskId);
static void Task_ReturnToMainMenu_2(u8 taskId);
static void Task_ReturnToMainMenu_3(u8 taskId);
static void Task_ReturnToMainMenu_4(u8 taskId);
static void Task_ReturnToMainMenu_5(u8 taskId);
static void Task_ReturnToMainMenu_6(u8 taskId);
static void Task_ExitPokenav2_1(u8 taskId);
static void Task_ExitPokenav2_2(u8 taskId);
static void Task_ExitPokenav2_3(u8 taskId);
static void Task_ExitPokenav2_4(u8 taskId);

// ewram
EWRAM_DATA static struct Pokenav2Struct *sPokenav2StructPtr = NULL;

// .rodata
static const u32 sPokenav2GridTiles[] = INCBIN_U32("graphics/pokenav_v2/grid.4bpp.lz");
static const u32 sPokenav2GridTilemap[] = INCBIN_U32("graphics/pokenav_v2/grid.bin.lz");
static const u16 sPokenav2GridPalette[] = INCBIN_U16("graphics/pokenav_v2/grid.gbapal");

static const u32 sPokenav2WindowFrameTiles[] = INCBIN_U32("graphics/pokenav_v2/window.4bpp.lz");
static const u16 sPokenav2WindowFramePalette[] = INCBIN_U16("graphics/pokenav_v2/window.gbapal");

static const u32 sPokenav2HeaderTiles[] = INCBIN_U32("graphics/pokenav_v2/header.4bpp.lz");
static const u32 sPokenav2HeaderTilemap[] = INCBIN_U32("graphics/pokenav_v2/header.bin.lz");
static const u16 sPokenav2HeaderPalette[] = INCBIN_U16("graphics/pokenav_v2/header.gbapal");

static const u32 sPokenav2DescTilemap[] = INCBIN_U32("graphics/pokenav_v2/desc.bin.lz");
static const u32 sPokenav2RegionMapFrameTilemap[] = INCBIN_U32("graphics/pokenav_v2/map.bin.lz");
static const u32 sPokenav2AgendaTilemap[] = INCBIN_U32("graphics/pokenav_v2/agenda.bin.lz");
static const u32 sPokenav2RadioTilemap[] = INCBIN_U32("graphics/pokenav_v2/radio.bin.lz");

static const u8 sPokenav2OptionsLeft[] = INCBIN_U8("graphics/pokenav_v2/options_left.4bpp");
static const u8 sPokenav2OptionsRight[] = INCBIN_U8("graphics/pokenav_v2/options_right.4bpp");
static const u8 sPokenav2AgendaClockIcons[] = INCBIN_U8("graphics/pokenav_v2/agenda.4bpp");
static const u16 sPokenav2SpritePalette[] = INCBIN_U16("graphics/pokenav_v2/icons.gbapal");

static const s16 sPokenav2OptionLeftPositions[][2] =
{
    {48, 48},
    {48, 88},
    {192, 48},
    {192, 88},
};

static const s16 sPokenav2OptionRightPositions[][2] =
{
    {88, 48},
    {88, 88},
    {152, 48},
    {152, 88},
};

static const u8 sTextColorGray[] =
{
    0, 2, 3
};

static const u8 sTextColorWhite[] =
{
    0, 1, 2
};

static const u8 sTextColorRed[] =
{
    0, 4, 3
};

static const u8 sTextColorGreen[] =
{
    0, 6, 3
};

static const u8 sTextColorBlue[] =
{
    0, 8, 3
};

static const u8 *const sMenuDescriptions[] =
{
    gText_Pokenav2_MapDesc,
    gText_Pokenav2_AgendaDesc,
    gText_Pokenav2_RadioDesc,
    gText_Pokenav2_TurnOffDesc
};

static const void (*const sPokenav2Funcs[])(u8) =
{
    Task_RegionMap,
    Task_Agenda,
    Task_Radio,
};

static const struct ListMenuTemplate sTodaysEventsListMenuTemplate =
{
    .items = NULL,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .itemPrintFunc = NULL,
    .totalItems = 0,
    .maxShowed = 0,
    .windowId = WIN_AGENDA_EVENTS_CONTENT,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 1,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = 1,
    .cursorKind = 0
};

static const struct EventSchedule sSundayEventSchedule[] =
{
    
};

static const struct EventSchedule sMondayEventSchedule[] =
{
    {EVENT_LOTTERY, MAPSEC_FIRWEALD_CITY, {10, 18}},
    {EVENT_SALE,  MAPSEC_MURENA_CITY, {12, 17}},
    {EVENT_WORLD_TOURNAMENT, MAPSEC_FIRWEALD_CITY, {10, 18}},
    {EVENT_CATCHING_CONTEST,  MAPSEC_MURENA_CITY, {12, 17}},
    {EVENT_TREASHURE_HUNT, MAPSEC_OUREA_CAVES, {10, 18}},
    {EVENT_SAFARI_ZONE, MAPSEC_FIRWEALD_CITY, {10, 18}},
    {EVENT_CEL_NAUTILUS,  MAPSEC_MURENA_CITY, {12, 17}},
    {EVENT_MARKET,  MAPSEC_MURENA_CITY, {12, 17}},
};

static const struct EventSchedule sTuesdayEventSchedule[] =
{
    
};

static const struct EventSchedule sWednesdayEventSchedule[] =
{
    {EVENT_MARKET, MAPSEC_MURENA_CITY, {12, 17}}
};

static const struct EventSchedule sThursdayEventSchedule[] =
{
    
};

static const struct EventSchedule sFridayEventSchedule[] =
{
    
};

static const struct EventSchedule sSaturdayEventSchedule[] =
{
    
};

static const struct EventTextData sEventTextData[] =
{
    [EVENT_LOTTERY] = {gText_Pokenav2_Lottery, gText_Pokenav2_LotteryDesc},
    [EVENT_SALE] = {gText_Pokenav2_Sale, gText_Pokenav2_SaleDesc},
    [EVENT_WORLD_TOURNAMENT] = {gText_Pokenav2_WorldTournament, gText_Pokenav2_WorldTournamentDesc},
    [EVENT_CATCHING_CONTEST] = {gText_Pokenav2_CatchingContest, gText_Pokenav2_CatchingContestDesc},
    [EVENT_TREASHURE_HUNT] = {gText_Pokenav2_TreasureHunt, gText_Pokenav2_TreasureHuntDesc},
    [EVENT_SAFARI_ZONE] = {gText_Pokenav2_SafariZone, gText_Pokenav2_SafariZoneDesc},
    [EVENT_CEL_NAUTILUS] = {gText_Pokenav2_CELNautilus, gText_Pokenav2_CELNautilusDesc},
    [EVENT_MARKET] = {gText_Pokenav2_Market, gText_Pokenav2_MarketDesc}
};

static const struct EventScheduleTable sEventScheduleTable[] =
{
    {sSundayEventSchedule, ARRAY_COUNT(sSundayEventSchedule)},
    {sMondayEventSchedule, ARRAY_COUNT(sMondayEventSchedule)},
    {sTuesdayEventSchedule, ARRAY_COUNT(sTuesdayEventSchedule)},
    {sWednesdayEventSchedule, ARRAY_COUNT(sWednesdayEventSchedule)},
    {sThursdayEventSchedule, ARRAY_COUNT(sThursdayEventSchedule)},
    {sFridayEventSchedule, ARRAY_COUNT(sFridayEventSchedule)},
    {sSaturdayEventSchedule, ARRAY_COUNT(sSaturdayEventSchedule)}
};

static const struct BgTemplate sPokenav2BgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 0,
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 26,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 1,
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 2,
        .paletteMode = 0,
        .priority = 2
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 25,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
    },
};

static const struct WindowTemplate sPokenav2WindowTemplates[] =
{
    { // WIN_HEADER
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x10,
    },
    { // WIN_DESC
        .bg = 0,
        .tilemapLeft = 4,
        .tilemapTop = 15,
        .width = 22,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    { // WIN_OPTION_TOP_LEFT
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xC4,
    },
    { // WIN_OPTION_BOTTOM_LEFT
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xD4,
    },
    { // WIN_OPTION_TOP_RIGHT
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xE4,
    },
    { // WIN_OPTION_BOTTOM_RIGHT
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xF4,
    },
    { // WIN_REGION_MAP_TITLE
        .bg = 0,
        .tilemapLeft = 25,
        .tilemapTop = 1,
        .width = 4,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    { // WIN_REGION_MAP_SECTION
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 18,
        .width = 28,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x88,
    },
    { // WIN_AGENDA_EVENTS_TITLE
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 13,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    { // WIN_AGENDA_EVENTS_CONTENT
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 13,
        .height = 13,
        .paletteNum = 15,
        .baseBlock = 0x66,
    },
    { // WIN_AGENDA_DATE_TIME
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 14,
        .width = 11,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x10F,
    },
    { // WIN_AGENDA_WEATHER
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 3,
        .width = 11,
        .height = 9,
        .paletteNum = 15,
        .baseBlock = 0x13B,
    },
    DUMMY_WIN_TEMPLATE
};

const struct SpritePalette gSpritePalette_OptionSprites =
{
    .data = sPokenav2SpritePalette,
    .tag = TAG_PAL
};

static const struct SpriteSheet sSpriteSheet_OptionLeftTiles =
{
    .data = sPokenav2OptionsLeft,
    .size = 0xE00,
    .tag = TAG_OPTIONS_LEFT,
};

static const struct SpriteSheet sSpriteSheet_OptionRightTiles =
{
    .data = sPokenav2OptionsRight,
    .size = 0xE00,
    .tag = TAG_OPTIONS_RIGHT,
};

static const struct SpriteSheet sSpriteSheet_AgendaClockTiles =
{
    .data = sPokenav2AgendaClockIcons,
    .size = 0x1C00,
    .tag = TAG_AGENDA_ICONS,
};

static const union AnimCmd sSpriteAnim_OptionLeft_0[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OptionLeft_1[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_OptionsLeft[] =
{
    sSpriteAnim_OptionLeft_0,
    sSpriteAnim_OptionLeft_1,
};

static const union AnimCmd sSpriteAnim_OptionRightMap[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OptionRightAgenda[] =
{
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OptionRightRadio[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OptionRightExit[] =
{
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_OptionsRight[] =
{
    sSpriteAnim_OptionRightMap,
    sSpriteAnim_OptionRightAgenda,
    sSpriteAnim_OptionRightRadio,
    sSpriteAnim_OptionRightExit,
};

static const union AnimCmd sSpriteAnim_Agenda0[] =
{
    ANIMCMD_FRAME(0, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda1[] =
{
    ANIMCMD_FRAME(4, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda2[] =
{
    ANIMCMD_FRAME(8, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda3[] =
{
    ANIMCMD_FRAME(12, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda4[] =
{
    ANIMCMD_FRAME(16, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda5[] =
{
    ANIMCMD_FRAME(20, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda6[] =
{
    ANIMCMD_FRAME(24, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda7[] =
{
    ANIMCMD_FRAME(28, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda8[] =
{
    ANIMCMD_FRAME(32, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda9[] =
{
    ANIMCMD_FRAME(36, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda10[] =
{
    ANIMCMD_FRAME(40, 2),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Agenda11[] =
{
    ANIMCMD_FRAME(44, 2),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_AgendaClockIcons[] =
{
    sSpriteAnim_Agenda0,
    sSpriteAnim_Agenda1,
    sSpriteAnim_Agenda2,
    sSpriteAnim_Agenda3,
    sSpriteAnim_Agenda4,
    sSpriteAnim_Agenda5,
    sSpriteAnim_Agenda6,
    sSpriteAnim_Agenda7,
    sSpriteAnim_Agenda8,
    sSpriteAnim_Agenda9,
    sSpriteAnim_Agenda10,
    sSpriteAnim_Agenda11,
};

static const struct OamData sOamData_OptionsLeft =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct OamData sOamData_OptionsRight =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct OamData sOamData_AgendaClockIcons =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
    .affineParam = 0
};

const struct SpriteTemplate sSpriteTemplate_OptionsLeft =
{
    .tileTag = TAG_OPTIONS_LEFT,
    .paletteTag = TAG_PAL,
    .oam = &sOamData_OptionsLeft,
    .anims = sSpriteAnimTable_OptionsLeft,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

const struct SpriteTemplate sSpriteTemplate_OptionsRight =
{
    .tileTag = TAG_OPTIONS_RIGHT,
    .paletteTag = TAG_PAL,
    .oam = &sOamData_OptionsRight,
    .anims = sSpriteAnimTable_OptionsRight,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

const struct SpriteTemplate sSpriteTemplate_AgendaClockIcons =
{
    .tileTag = TAG_AGENDA_ICONS,
    .paletteTag = TAG_PAL,
    .oam = &sOamData_AgendaClockIcons,
    .anims = sSpriteAnimTable_AgendaClockIcons,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

void CB2_InitPokenav2(void)
{
    ResetTasks();
    SetVBlankCallback(NULL);

    sPokenav2StructPtr = Alloc(sizeof(struct Pokenav2Struct));

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG3CNT, 0);
    SetGpuReg(REG_OFFSET_BG2CNT, 0);
    SetGpuReg(REG_OFFSET_BG1CNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);
    DmaFillLarge16(3, 0, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    DmaClear16(3, (void *)PLTT, PLTT_SIZE);
    LZ77UnCompVram(sPokenav2GridTiles, (u16 *)BG_CHAR_ADDR(3));
    LZ77UnCompVram(sPokenav2GridTilemap, (u16 *)BG_SCREEN_ADDR(25));
    LoadPalette(sPokenav2GridPalette, 0, sizeof(sPokenav2GridPalette));
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sPokenav2BgTemplates, ARRAY_COUNT(sPokenav2BgTemplates));
    InitWindows(sPokenav2WindowTemplates);
    DeactivateAllTextPrinters();
    ClearScheduledBgCopiesToVram();
    ScanlineEffect_Stop();
    ResetSpriteData();
    ResetPaletteFade();
    FreeAllSpritePalettes();
    LoadOptionBgs(MainMenu);
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_Pokenav2);
    SetMainCallback2(CB2_Pokenav2);

    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);

    sPokenav2StructPtr->scrollPosition = OPTION_SLIDE_Y;
    sPokenav2StructPtr->taskIds[TASK_MAIN] = CreateTask(Task_Pokenav2_1, TASK_MAIN);
    //sPokenav2StructPtr->taskIds[TASK_BG3SCROLL] = CreateTask(Task_ScrollBg3Squares, TASK_BG3SCROLL);

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);

    PlaySE(SE_POKENAV_ON);
}

static void VBlankCB_Pokenav2(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ChangeBgY(3, 96, BG_COORD_SUB);
    ChangeBgX(3, 96, BG_COORD_SUB);
}

static void CB2_Pokenav2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    DoScheduledBgTilemapCopiesToVram();
}

static void LoadOptionBgs(enum Option option)
{
    DmaClear16(3, BG_SCREEN_ADDR(26), BG_SCREEN_SIZE);
    DmaClear16(3, BG_SCREEN_ADDR(28), BG_SCREEN_SIZE);

    switch (option)
    {
    case RegionMap:
        sPokenav2StructPtr->regionMap = AllocZeroed(sizeof(struct RegionMap));
        InitRegionMap(sPokenav2StructPtr->regionMap, FALSE);
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompVram(sPokenav2RegionMapFrameTilemap, (u16 *)BG_SCREEN_ADDR(26));
        LoadPalette(sPokenav2WindowFramePalette, 32, sizeof(sPokenav2WindowFramePalette));
        ClearWindowTilemap(WIN_HEADER);
        break;
    case Agenda:
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        StringExpandPlaceholders(gStringVar4, gText_Pokenav2_PlayersAgenda);
        AddTextPrinterParameterized3(WIN_HEADER, 1, GetStringCenterAlignXOffset(1, gStringVar4, DISPLAY_WIDTH), 0, sTextColorWhite, 0, gStringVar4);
        AddTextPrinterParameterized3(WIN_HEADER, 1, GetStringRightAlignXOffset(1, gText_Pokenav2_ClockMode, DISPLAY_WIDTH) - 8, 0, sTextColorWhite, 0, gText_Pokenav2_ClockMode);
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompVram(sPokenav2AgendaTilemap, (u16 *)BG_SCREEN_ADDR(26));
        LoadPalette(sPokenav2WindowFramePalette, 32, sizeof(sPokenav2WindowFramePalette));
        break;
    case Radio:
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompVram(sPokenav2RadioTilemap, (u16 *)BG_SCREEN_ADDR(26));
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        AddTextPrinterParameterized3(WIN_HEADER, 1, GetStringCenterAlignXOffset(1, gText_Pokenav2_EurusRadio, DISPLAY_WIDTH), 0, sTextColorWhite, 0, gText_Pokenav2_EurusRadio);
        break;
    case MainMenu:
        PutWindowTilemap(WIN_HEADER);
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        AddTextPrinterParameterized3(WIN_HEADER, 1, GetStringCenterAlignXOffset(1, gText_Pokenav2, DISPLAY_WIDTH), 0, sTextColorWhite, 0, gText_Pokenav2);
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(2));
        LZ77UnCompVram(sPokenav2DescTilemap, (u16 *)BG_SCREEN_ADDR(28));
        LoadPalette(sPokenav2HeaderPalette, 16, sizeof(sPokenav2HeaderPalette));
        LoadPalette(sPokenav2WindowFramePalette, 32, sizeof(sPokenav2WindowFramePalette));
        LoadOptionAndIconSprites();
        break;
    }

    ScheduleBgCopyTilemapToVram(0);
}

static void LoadOption(enum Option option)
{
    switch (option)
    {
    case RegionMap:
        LoadRegionMap();
        break;
    case Agenda:
        LoadAgenda();
        break;
    case Radio:
        LoadRadio();
        break;
    case MainMenu:
        LoadMainMenu();
        break;
    }
}

static void LoadRegionMap(void)
{
    CreateRegionMapCursor(0, 0);
    CreateRegionMapPlayerIcon(1, 1);

    PutWindowTilemap(WIN_REGION_MAP_TITLE);
    PutWindowTilemap(WIN_REGION_MAP_SECTION);
    AddTextPrinterParameterized3(WIN_REGION_MAP_TITLE, 0, GetStringRightAlignXOffset(0, gText_Pokenav2_Eurus, 24), 1, sTextColorWhite, 0, gText_Pokenav2_Eurus);
    AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, 1, GetStringCenterAlignXOffset(1, sPokenav2StructPtr->regionMap->mapSecName, DISPLAY_WIDTH), 0, sTextColorGray, 0, sPokenav2StructPtr->regionMap->mapSecName);

    ScheduleBgCopyTilemapToVram(0);
}

static void LoadAgenda(void)
{
    const u8 *string, *color;
    u8 i;

    sPokenav2StructPtr->is24HClockMode = gSaveBlock2Ptr->is24HClockMode;
    sPokenav2StructPtr->eventDataPtr = sEventScheduleTable[gSaveBlock2Ptr->inGameClock.dayOfWeek].ptr;
    sPokenav2StructPtr->eventTextPtr = sEventTextData;
    sPokenav2StructPtr->scrollOffset = 0;
    sPokenav2StructPtr->selectedRow = 0;

    sPokenav2StructPtr->taskIds[TASK_CLOCK] = CreateTask(Task_FormatClock, TASK_CLOCK);

    PutWindowTilemap(WIN_AGENDA_EVENTS_TITLE);
    PutWindowTilemap(WIN_AGENDA_EVENTS_CONTENT);
    PutWindowTilemap(WIN_AGENDA_DATE_TIME);
    PutWindowTilemap(WIN_AGENDA_WEATHER);

    InitEventWindows();

    StringCopy(gStringVar1, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek]);
    AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, 1, GetStringCenterAlignXOffset(1, gStringVar1, 88), 0, sTextColorGray, 0, gStringVar1);
    FormatTimeString(gStringVar4, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes);
    AddTextPrinterParameterized(WIN_AGENDA_DATE_TIME, 0, gStringVar4, GetStringCenterAlignXOffset(1, gStringVar4, 88), 16, 0, NULL);

    AddTextPrinterParameterized3(WIN_AGENDA_WEATHER, 0, GetStringCenterAlignXOffset(0, gText_Pokenav2_WeatherForecast, 88), 0, sTextColorGray, 0, gText_Pokenav2_WeatherForecast);

    switch (GetSavedWeather())
    {
    case WEATHER_NONE:
        string = gText_Pokenav2_NoReport;
        color = sTextColorBlue;
        break;
    case WEATHER_SUNNY_CLOUDS:
        string = gText_Pokenav2_SunnyClouds;
        color = sTextColorGreen;
        break;
    case WEATHER_SUNNY:
        string = gText_Pokenav2_Clear;
        color = sTextColorGreen;
        break;
    case WEATHER_RAIN:
        string = gText_Pokenav2_Rain;
        color = sTextColorBlue;
        break;
    case WEATHER_SNOW:
        string = gText_Pokenav2_Snow;
        color = sTextColorBlue;
        break;
    case WEATHER_RAIN_THUNDERSTORM:
        string = gText_Pokenav2_Thunderstorm;
        color = sTextColorRed;
        break;
    case WEATHER_FOG_HORIZONTAL:
        string = gText_Pokenav2_Fog;
        color = sTextColorBlue;
        break;
    case WEATHER_VOLCANIC_ASH:
        string = gText_Pokenav2_Ash;
        color = sTextColorRed;
        break;
    case WEATHER_SANDSTORM:
        string = gText_Pokenav2_Sandstorm;
        color = sTextColorRed;
        break;
    case WEATHER_FOG_DIAGONAL:
        string = gText_Pokenav2_Fog;
        color = sTextColorBlue;
        break;
    case WEATHER_UNDERWATER:
        string = gText_Pokenav2_NoReport;
        color = sTextColorBlue;
        break;
    case WEATHER_SHADE:
        string = gText_Pokenav2_Shade;
        color = sTextColorBlue;
        break;
    case WEATHER_DROUGHT:
        string = gText_Pokenav2_Drought;
        color = sTextColorRed;
        break;
    case WEATHER_DOWNPOUR:
        string = gText_Pokenav2_Downpour;
        color = sTextColorRed;
        break;
    case WEATHER_UNDERWATER_BUBBLES:
        string = gText_Pokenav2_NoReport;
        color = sTextColorBlue;
        break;
    case WEATHER_ABNORMAL:
        string = gText_Pokenav2_Abnormal;
        color = sTextColorRed;
        break;
    }

    AddTextPrinterParameterized3(WIN_AGENDA_WEATHER, 0, GetStringCenterAlignXOffset(0, string, 88), 16, color, 0, string);

    ScheduleBgCopyTilemapToVram(0);
}

static void InitEventWindows(void)
{
    FillWindowPixelBuffer(WIN_AGENDA_EVENTS_TITLE, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_AGENDA_EVENTS_CONTENT, PIXEL_FILL(0));

    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_TITLE, 1, GetStringCenterAlignXOffset(1, gText_Pokenav2_TodaysEvents, 104), 0, sTextColorGray, 0, gText_Pokenav2_TodaysEvents);
    CreateEventListMenuTemplate();
    sPokenav2StructPtr->taskIds[TASK_LIST_MENU] = ListMenuInit(&gMultiuseListMenuTemplate, sPokenav2StructPtr->scrollOffset, sPokenav2StructPtr->selectedRow);
    sPokenav2StructPtr->taskIds[TASK_SCROLL_ARROW] = AddScrollIndicatorArrowPairParameterized(
        SCROLL_ARROW_UP,
        68,
        20,
        148,
        gMultiuseListMenuTemplate.totalItems - 1,
        TAG_SCROLL_ARROW,
        TAG_SCROLL_ARROW,
        &sPokenav2StructPtr->currItem);
}

static void CreateEventListMenuTemplate(void)
{
    u8 size;

    size = sEventScheduleTable[gSaveBlock2Ptr->inGameClock.dayOfWeek].size;
    sPokenav2StructPtr->listMenuItems = Alloc(size * sizeof(struct ListMenuItem));

    gMultiuseListMenuTemplate = sTodaysEventsListMenuTemplate;
    gMultiuseListMenuTemplate.totalItems = size;

    if (size < 6)
        gMultiuseListMenuTemplate.maxShowed = gMultiuseListMenuTemplate.totalItems;
    else
        gMultiuseListMenuTemplate.maxShowed = 6;

    for (u32 i = 0; i < size; i++)
    {
        sPokenav2StructPtr->listMenuItems[i].name = sPokenav2StructPtr->eventTextPtr[sPokenav2StructPtr->eventDataPtr[i].eventId].title;
        sPokenav2StructPtr->listMenuItems[i].id = i;
    }
    gMultiuseListMenuTemplate.items = sPokenav2StructPtr->listMenuItems;
}

static void LoadRadio(void)
{


    ScheduleBgCopyTilemapToVram(0);
}

static void LoadMainMenu(void)
{
    PutWindowTilemap(WIN_DESC);
    PutWindowTilemap(WIN_OPTION_TOP_LEFT);
    PutWindowTilemap(WIN_OPTION_BOTTOM_LEFT);
    PutWindowTilemap(WIN_OPTION_TOP_RIGHT);
    PutWindowTilemap(WIN_OPTION_BOTTOM_RIGHT);

    AddTextPrinterParameterized(WIN_OPTION_TOP_LEFT, 1, gText_Pokenav2_Map, 4, 0, 0, NULL);
    AddTextPrinterParameterized(WIN_OPTION_BOTTOM_LEFT, 1, gText_Pokenav2_Agenda, 4, 0, 0, NULL);
    AddTextPrinterParameterized(WIN_OPTION_TOP_RIGHT, 1, gText_Pokenav2_Radio, 0, 0, 0, NULL);
    AddTextPrinterParameterized(WIN_OPTION_BOTTOM_RIGHT, 1, gText_Pokenav2_TurnOff, 0, 0, 0, NULL);
    AddTextPrinterParameterized3(WIN_DESC, 1, GetStringCenterAlignXOffset(1, sMenuDescriptions[sPokenav2StructPtr->cursorPosition], 176), 4, sTextColorWhite, 0, sMenuDescriptions[sPokenav2StructPtr->cursorPosition]);

    ScheduleBgCopyTilemapToVram(0);
}

static void LoadOptionAndIconSprites(void)
{
    struct Sprite *sprite;

    LoadSpritePalette(&gSpritePalette_OptionSprites);
    LoadSpriteSheet(&sSpriteSheet_OptionLeftTiles);
    LoadSpriteSheet(&sSpriteSheet_OptionRightTiles);
    LoadSpriteSheet(&sSpriteSheet_AgendaClockTiles);

    for (u32 i = 0; i < 4; i++)
    {
        sPokenav2StructPtr->spriteIds[i] = CreateSprite(&sSpriteTemplate_OptionsLeft, sPokenav2OptionLeftPositions[i][0], sPokenav2OptionLeftPositions[i][1], 2);
        sPokenav2StructPtr->spriteIds[i + 4] = CreateSprite(&sSpriteTemplate_OptionsRight, sPokenav2OptionRightPositions[i][0], sPokenav2OptionRightPositions[i][1], 1);
        if (i < 2)
        {
            StartSpriteAnim(&gSprites[sPokenav2StructPtr->spriteIds[i]], 0);
        }
        else
        {
            StartSpriteAnim(&gSprites[sPokenav2StructPtr->spriteIds[i]], 1);
        }
        StartSpriteAnim(&gSprites[sPokenav2StructPtr->spriteIds[i + 4]], i);
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i]];
        if (sprite->animNum == 0)
        {
            sprite->x2 = -OPTION_SLIDE_X;
        }
        else
        {
            sprite->x2 = OPTION_SLIDE_X;
        }
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            sprite->x2 = -OPTION_SLIDE_X;
        }
        else
        {
            sprite->x2 = OPTION_SLIDE_X;
        }
    }

    sPokenav2StructPtr->spriteIds[8] = CreateSprite(&sSpriteTemplate_AgendaClockIcons, 88, 87, 0);
    StartSpriteAnim(&gSprites[sPokenav2StructPtr->spriteIds[8]], gSaveBlock2Ptr->inGameClock.hours < 12 ? gSaveBlock2Ptr->inGameClock.hours : gSaveBlock2Ptr->inGameClock.hours - 12);
    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    sprite->x2 = -OPTION_SLIDE_X;
}

static void UnloadOption(enum Option option)
{
    switch (option)
    {
    case RegionMap:
        UnloadRegionMap();
        break;
    case Agenda:
        UnloadAgenda();
        break;
    case Radio:
        UnloadRadio();
        break;
    case MainMenu:
        UnloadMainMenu();
        break;
    }
}

static void UnloadRegionMap(void)
{
    FreeRegionMapIconResources();

    ClearWindowTilemap(WIN_REGION_MAP_TITLE);
    ClearWindowTilemap(WIN_REGION_MAP_SECTION);

    Free(sPokenav2StructPtr->regionMap);
    sPokenav2StructPtr->regionMap = NULL;

    ScheduleBgCopyTilemapToVram(0);
}

static void UnloadAgenda(void)
{
    u8 i;

    gSaveBlock2Ptr->is24HClockMode = sPokenav2StructPtr->is24HClockMode;

    DestroyTask(sPokenav2StructPtr->taskIds[TASK_CLOCK]);
    DestroyListMenuTask(sPokenav2StructPtr->taskIds[TASK_LIST_MENU], &sPokenav2StructPtr->scrollOffset, &sPokenav2StructPtr->selectedRow);

    ClearWindowTilemap(WIN_AGENDA_EVENTS_TITLE);
    ClearWindowTilemap(WIN_AGENDA_EVENTS_CONTENT);
    ClearWindowTilemap(WIN_AGENDA_DATE_TIME);
    ClearWindowTilemap(WIN_AGENDA_WEATHER);

    Free(sPokenav2StructPtr->listMenuItems);
    sPokenav2StructPtr->listMenuItems = NULL;

    ScheduleBgCopyTilemapToVram(0);
}

static void UnloadRadio(void)
{
    
}

static void UnloadMainMenu(void)
{
    ClearWindowTilemap(WIN_DESC);
    ClearWindowTilemap(WIN_OPTION_TOP_LEFT);
    ClearWindowTilemap(WIN_OPTION_BOTTOM_LEFT);
    ClearWindowTilemap(WIN_OPTION_TOP_RIGHT);
    ClearWindowTilemap(WIN_OPTION_BOTTOM_RIGHT);

    ScheduleBgCopyTilemapToVram(0);
}

static void UnloadOptionAndIconSprites(void)
{
    FreeSpritePaletteByTag(TAG_PAL);
    FreeSpriteTilesByTag(TAG_OPTIONS_LEFT);
    FreeSpriteTilesByTag(TAG_OPTIONS_RIGHT);

    for (u32 i = 0; i < ARRAY_COUNT(sPokenav2StructPtr->spriteIds); i++)
    {
        FreeSpriteOamMatrix(&gSprites[sPokenav2StructPtr->spriteIds[i]]);
        DestroySprite(&gSprites[sPokenav2StructPtr->spriteIds[i]]);
    }
}

static void SpriteCB_Icons(struct Sprite *sprite)
{
    if (sprite->animNum == sPokenav2StructPtr->cursorPosition)
    {
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 < 8)
            {
                sprite->x2 += 2;
            }
            else
            {
                sprite->x2 = 8;
            }
        }
        else
        {
            if (sprite->x2 > -8)
            {
                sprite->x2 -= 2;
            }
            else
            {
                sprite->x2 = -8;
            }
        }
    }
    else
    {
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 > 0)
            {
                sprite->x2 -= 2;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
        else
        {
            if (sprite->x2 > 0)
            {
                sprite->x2 += 2;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
    }
}

static void SpriteCB_Agenda(struct Sprite *sprite)
{
    if (sPokenav2StructPtr->cursorPosition == 1)
    {
        if (sprite->x2 < 8)
        {
            sprite->x2 += 2;
        }
        else
        {
            sprite->x2 = 8;
        }
    }
    else
    {
        if (sprite->x2 > 0)
        {
            sprite->x2 -= 2;
        }
        else
        {
            sprite->x2 = 0;
        }
    }
}

static void UpdateOptionDescription(u8 option)
{
    FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_DESC, 1, GetStringCenterAlignXOffset(1, sMenuDescriptions[option], 176), 4, sTextColorWhite, 0, sMenuDescriptions[option]);
}

static void Task_Pokenav2_1(u8 taskId)
{
    if (Task_SlideMainMenuIn(taskId))
    {
        gTasks[taskId].func = Task_Pokenav2_2;
    }
}

static void Task_Pokenav2_2(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        sprite->callback = SpriteCB_Icons;
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    sprite->callback = SpriteCB_Agenda;

    LoadOption(MainMenu);
    gTasks[taskId].func = Task_Pokenav2_3;
}

static void Task_Pokenav2_3(u8 taskId)
{
    if (JOY_NEW(DPAD_LEFT))
    {
        if (sPokenav2StructPtr->cursorPosition & 2)
        {
            sPokenav2StructPtr->cursorPosition ^= 2;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2StructPtr->cursorPosition);
        }
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (!(sPokenav2StructPtr->cursorPosition & 2)
         && (sPokenav2StructPtr->cursorPosition ^ 2) < 4)
        {
            sPokenav2StructPtr->cursorPosition ^= 2;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2StructPtr->cursorPosition);
        }
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sPokenav2StructPtr->cursorPosition & 1)
        {
            sPokenav2StructPtr->cursorPosition ^= 1;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2StructPtr->cursorPosition);
        }
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (!(sPokenav2StructPtr->cursorPosition & 1)
         && (sPokenav2StructPtr->cursorPosition ^ 1) < 4)
        {
            sPokenav2StructPtr->cursorPosition ^= 1;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2StructPtr->cursorPosition);
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        gTasks[taskId].func = Task_LoadOption_1;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ExitPokenav2_1;
        PlaySE(SE_POKENAV_OFF);
    }
}

static void Task_RegionMap(u8 taskId)
{
    switch (DoRegionMapInputCallback())
    {
        case MAP_INPUT_MOVE_END:
            FillWindowPixelBuffer(WIN_REGION_MAP_SECTION, PIXEL_FILL(0));
            if (sPokenav2StructPtr->regionMap->mapSecType != MAPSECTYPE_NONE)
            {
                AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, 1, GetStringCenterAlignXOffset(1, sPokenav2StructPtr->regionMap->mapSecName, DISPLAY_WIDTH), 0, sTextColorGray, 0, sPokenav2StructPtr->regionMap->mapSecName);
            }
            else
            {
                AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, 1, GetStringCenterAlignXOffset(1, gText_ThreeDashes, DISPLAY_WIDTH), 0, sTextColorGray, 0, gText_ThreeDashes);
            }
            ScheduleBgCopyTilemapToVram(0);
            break;
    }

    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static void Task_Agenda(u8 taskId)
{
    u8 input = ListMenu_ProcessInput(sPokenav2StructPtr->taskIds[TASK_LIST_MENU]);

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        RemoveScrollIndicatorArrowPair(sPokenav2StructPtr->taskIds[TASK_SCROLL_ARROW]);
        DestroyListMenuTask(sPokenav2StructPtr->taskIds[TASK_LIST_MENU], &sPokenav2StructPtr->scrollOffset, &sPokenav2StructPtr->selectedRow);
        CreateEventDetailsPage(input);
        gTasks[taskId].func = Task_EventDetailsPage;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        RemoveScrollIndicatorArrowPair(sPokenav2StructPtr->taskIds[TASK_SCROLL_ARROW]);
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_SELECT);
        sPokenav2StructPtr->is24HClockMode ^= 1;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sPokenav2StructPtr->currItem > 0)
            sPokenav2StructPtr->currItem--;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (sPokenav2StructPtr->currItem < gMultiuseListMenuTemplate.totalItems - 1)
            sPokenav2StructPtr->currItem++;
    }
}

static void Task_EventDetailsPage(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        InitEventWindows();
        DestroyTask(sPokenav2StructPtr->taskIds[TASK_EVENT_TIME]);
        gTasks[taskId].func = Task_Agenda;
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_SELECT);
        sPokenav2StructPtr->is24HClockMode ^= 1;
    }
}

static void Task_FormatClock(u8 taskId)
{
    if (JOY_NEW(START_BUTTON))
    {
        FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 0, 16, 88, 12);
    }

    if (gSaveBlock2Ptr->inGameClock.hours && gSaveBlock2Ptr->inGameClock.minutes && gSaveBlock2Ptr->inGameClock.vblanks == 0)
    {
        FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 0, 0, 88, 16);
        StringCopy(gStringVar1, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek]);
        AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, 1, GetStringCenterAlignXOffset(1, gStringVar1, 88), 0, sTextColorGray, 0, gStringVar1);
    }
    FormatTimeString(gStringVar4, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes);
    AddTextPrinterParameterized(WIN_AGENDA_DATE_TIME, 0, gStringVar4, GetStringCenterAlignXOffset(1, gStringVar4, 88), 16, 0, NULL);

    if (gSaveBlock2Ptr->inGameClock.seconds % 6 == 0)
    {
        if (sPokenav2StructPtr->is24HClockMode)
        {
            FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 40, 16, 6, 12);
        }
        else
        {
            FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 33, 16, 6, 12);
        }
    }
}

static void Task_FormatEventTime(u8 taskId)
{
    if (JOY_NEW(START_BUTTON))
    {
        FillWindowPixelRect(WIN_AGENDA_EVENTS_CONTENT, PIXEL_FILL(1), 32, 88, 52, 12);
    }

    FormatEventTimeDisplay(gStringVar3, gTasks[TASK_EVENT_TIME].data[0]);
    AddTextPrinterParameterized(WIN_AGENDA_EVENTS_CONTENT, 0, gStringVar3, 32, 88, 0, NULL);
}

static void FormatTimeString(u8 *dest, s8 hours, s8 minutes)
{
    u8 *txtPtr;

    if (!sPokenav2StructPtr->is24HClockMode && gSaveBlock2Ptr->inGameClock.hours > 12)
    {
        txtPtr = ConvertIntToDecimalStringN(dest, gSaveBlock2Ptr->inGameClock.hours - 12, STR_CONV_MODE_LEADING_ZEROS, 2);
    }
    else
    {
        txtPtr = ConvertIntToDecimalStringN(dest, gSaveBlock2Ptr->inGameClock.hours, STR_CONV_MODE_LEADING_ZEROS, 2);
    }

    txtPtr = StringAppend(txtPtr, gText_Colon2);
    txtPtr = ConvertIntToDecimalStringN(txtPtr, gSaveBlock2Ptr->inGameClock.minutes, STR_CONV_MODE_LEADING_ZEROS, 2);

    if (!sPokenav2StructPtr->is24HClockMode)
    {
        txtPtr = StringAppend(txtPtr, gText_Space);
        if (gSaveBlock2Ptr->inGameClock.hours < 12)
        {
            txtPtr = StringAppend(txtPtr, gText_AM);
        }
        else
        {
            txtPtr = StringAppend(txtPtr, gText_PM);
        }
    }
}

static void CreateEventDetailsPage(u8 input)
{
    FillWindowPixelBuffer(WIN_AGENDA_EVENTS_TITLE, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_AGENDA_EVENTS_CONTENT, PIXEL_FILL(0));

    StringCopy(gStringVar1, sPokenav2StructPtr->eventTextPtr[sPokenav2StructPtr->eventDataPtr[input].eventId].title);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_TITLE, 1, 4, 0, sTextColorGray, 0, gText_Pokenav2_LeftArrow);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_TITLE, 7, GetStringCenterAlignXOffset(7, gStringVar1, 104), 0, sTextColorGray, 0, gStringVar1);

    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 0, 0, sTextColorBlue, 0, gText_Pokenav2_EventDescription);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 0, 56, sTextColorBlue, 0, gText_Pokenav2_EventLocation);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 0, 88, sTextColorBlue, 0, gText_Pokenav2_EventTimes);

    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 0, 12, sTextColorGray, 0, sPokenav2StructPtr->eventTextPtr[sPokenav2StructPtr->eventDataPtr[input].eventId].description);

    GetMapName(gStringVar2, sPokenav2StructPtr->eventDataPtr[input].location, 0);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 0, 68, sTextColorGray, 0, gStringVar2);

    FormatEventTimeDisplay(gStringVar3, input);
    AddTextPrinterParameterized3(WIN_AGENDA_EVENTS_CONTENT, 0, 32, 88, sTextColorGray, 0, gStringVar3);

    sPokenav2StructPtr->taskIds[TASK_EVENT_TIME] = CreateTask(Task_FormatEventTime, TASK_EVENT_TIME);
    gTasks[TASK_EVENT_TIME].data[0] = input;

    ScheduleBgCopyTilemapToVram(0);
}

static void FormatEventTimeDisplay(u8 *dest, u8 input)
{
    u8 *txtPtr;

    if (!sPokenav2StructPtr->is24HClockMode)
    {
        if (sPokenav2StructPtr->eventDataPtr[input].time[0] > 12)
        {
            txtPtr = ConvertIntToDecimalStringN(dest, sPokenav2StructPtr->eventDataPtr[input].time[0] - 12, STR_CONV_MODE_LEADING_ZEROS, 2);
            txtPtr = StringAppend(txtPtr, gText_PM);
        }
        else
        {
            txtPtr = ConvertIntToDecimalStringN(dest, sPokenav2StructPtr->eventDataPtr[input].time[0], STR_CONV_MODE_LEADING_ZEROS, 2);
            txtPtr = StringAppend(txtPtr, gText_AM);
        }
    }
    else
        txtPtr = ConvertIntToDecimalStringN(dest, sPokenav2StructPtr->eventDataPtr[input].time[0], STR_CONV_MODE_LEADING_ZEROS, 2);

    txtPtr = StringAppend(txtPtr, gText_Space);
    txtPtr = StringAppend(txtPtr, gText_Dash);
    txtPtr = StringAppend(txtPtr, gText_Space);

    if (!sPokenav2StructPtr->is24HClockMode)
    {
        if (sPokenav2StructPtr->eventDataPtr[input].time[1] > 12)
        {
            txtPtr = ConvertIntToDecimalStringN(txtPtr, sPokenav2StructPtr->eventDataPtr[input].time[1] - 12, STR_CONV_MODE_LEADING_ZEROS, 2);
            txtPtr = StringAppend(txtPtr, gText_PM);
        }
        else
        {
            txtPtr = ConvertIntToDecimalStringN(txtPtr, sPokenav2StructPtr->eventDataPtr[input].time[1], STR_CONV_MODE_LEADING_ZEROS, 2);
            txtPtr = StringAppend(txtPtr, gText_AM);
        }
    }
    else
        txtPtr = ConvertIntToDecimalStringN(txtPtr, sPokenav2StructPtr->eventDataPtr[input].time[1], STR_CONV_MODE_LEADING_ZEROS, 2);
}

static void Task_Radio(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_1;
    }
}

static bool8 Task_SlideMainMenuIn(u8 taskId)
{
    struct Sprite *sprite;

    if (sPokenav2StructPtr->scrollPosition > 0)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        sPokenav2StructPtr->scrollPosition -= OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        return TRUE;
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i]];
        if (sprite->animNum == 0)
        {
            if (sprite->x2 < 0)
            {
                sprite->x2 += OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
        else
        {
            if (sprite->x2 > 0)
            {
                sprite->x2 -= OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 < 0)
            {
                sprite->x2 += OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
        else
        {
            if (sprite->x2 > 0)
            {
                sprite->x2 -= OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = 0;
            }
        }
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    if (sprite->x2 < 0)
    {
        sprite->x2 += OPTION_SLIDE_SPEED;
    }
    else
    {
        sprite->x2 = 0;
    }

    return FALSE;
}

static bool8 Task_SlideMainMenuOut(u8 taskId)
{
    struct Sprite *sprite;

    if (sPokenav2StructPtr->scrollPosition < OPTION_SLIDE_Y)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        sPokenav2StructPtr->scrollPosition += OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - OPTION_SLIDE_Y);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - OPTION_SLIDE_Y);
        return TRUE;
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i]];
        if (sprite->animNum == 0)
        {
            if (sprite->x2 > -OPTION_SLIDE_X)
            {
                sprite->x2 -= OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = -OPTION_SLIDE_X;
            }
        }
        else
        {
            if (sprite->x2 < OPTION_SLIDE_X)
            {
                sprite->x2 += OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = OPTION_SLIDE_X;
            }
        }
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 > -OPTION_SLIDE_X)
            {
                sprite->x2 -= OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = -OPTION_SLIDE_X;
            }
        }
        else
        {
            if (sprite->x2 < OPTION_SLIDE_X)
            {
                sprite->x2 += OPTION_SLIDE_SPEED;
            }
            else
            {
                sprite->x2 = OPTION_SLIDE_X;
            }
        }
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    if (sprite->x2 > -OPTION_SLIDE_X)
    {
        sprite->x2 -= OPTION_SLIDE_SPEED;
    }
    else
    {
        sprite->x2 = -OPTION_SLIDE_X;
    }

    return FALSE;
}

static bool8 Task_SlideOptionIn(u8 taskId)
{
    if (sPokenav2StructPtr->scrollPosition > 0)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        sPokenav2StructPtr->scrollPosition -= OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        return TRUE;
    }

    return FALSE;
}

static bool8 Task_SlideOptionOut(u8 taskId)
{
    if (sPokenav2StructPtr->scrollPosition < OPTION_SLIDE_Y)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2StructPtr->scrollPosition);
        sPokenav2StructPtr->scrollPosition += OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - OPTION_SLIDE_Y);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - OPTION_SLIDE_Y);
        return TRUE;
    }

    return FALSE;
}

static void Task_LoadOption_1(u8 taskId)
{
    if (sPokenav2StructPtr->cursorPosition == 3)
    {
        PlaySE(SE_POKENAV_OFF);
        gTasks[taskId].func = Task_ExitPokenav2_1;
    }
    else
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].func = Task_LoadOption_2;
    }
}

static void Task_LoadOption_2(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        sprite->callback = SpriteCallbackDummy;
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    sprite->callback = SpriteCallbackDummy;

    UnloadOption(MainMenu);
    PlaySE(SE_BALL_TRAY_ENTER);
    gTasks[taskId].func = Task_LoadOption_3;
}

static void Task_LoadOption_3(u8 taskId)
{
    if (Task_SlideMainMenuOut(taskId))
    {
        gTasks[taskId].func = Task_LoadOption_4;
    }
}

static void Task_LoadOption_4(u8 taskId)
{
    UnloadOptionAndIconSprites();
    ScheduleBgCopyTilemapToVram(0);
    gTasks[taskId].func = Task_LoadOption_5;
}

static void Task_LoadOption_5(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    LoadOptionBgs(sPokenav2StructPtr->cursorPosition);
    sPokenav2StructPtr->scrollPosition = OPTION_SLIDE_Y;
    gTasks[taskId].func = Task_LoadOption_6;
}

static void Task_LoadOption_6(u8 taskId)
{
    if (Task_SlideOptionIn(taskId))
    {
        gTasks[taskId].func = Task_LoadOption_7;
    }
}

static void Task_LoadOption_7(u8 taskId)
{
    LoadOption(sPokenav2StructPtr->cursorPosition);
    gTasks[taskId].func = sPokenav2Funcs[sPokenav2StructPtr->cursorPosition];
}

static void Task_ReturnToMainMenu_1(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    UnloadOption(sPokenav2StructPtr->cursorPosition);
    sPokenav2StructPtr->scrollPosition = 0;
    gTasks[taskId].func = Task_ReturnToMainMenu_2;
}

static void Task_ReturnToMainMenu_2(u8 taskId)
{
    if (Task_SlideOptionOut(taskId))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_3;
    }
}

static void Task_ReturnToMainMenu_3(u8 taskId)
{
    LoadOptionBgs(MainMenu);
    gTasks[taskId].func = Task_ReturnToMainMenu_4;
}

static void Task_ReturnToMainMenu_4(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    gTasks[taskId].func = Task_ReturnToMainMenu_5;
}

static void Task_ReturnToMainMenu_5(u8 taskId)
{
    if (Task_SlideMainMenuIn(taskId))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_6;
    }
}

static void Task_ReturnToMainMenu_6(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        sprite->callback = SpriteCB_Icons;
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    sprite->callback = SpriteCB_Agenda;

    LoadOption(MainMenu);
    gTasks[taskId].func = Task_Pokenav2_3;
}

static void Task_ExitPokenav2_1(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2StructPtr->spriteIds[i + 4]];
        sprite->callback = SpriteCallbackDummy;
    }

    sprite = &gSprites[sPokenav2StructPtr->spriteIds[8]];
    sprite->callback = SpriteCallbackDummy;

    UnloadOption(MainMenu);
    sPokenav2StructPtr->cursorPosition = 0;
    gTasks[taskId].func = Task_ExitPokenav2_2;
}

static void Task_ExitPokenav2_2(u8 taskId)
{
    if (Task_SlideMainMenuOut(taskId))
    {
        gTasks[taskId].func = Task_ExitPokenav2_3;
    }
}

static void Task_ExitPokenav2_3(u8 taskId)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ExitPokenav2_4;
}

static void Task_ExitPokenav2_4(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        FreeAllWindowBuffers();
        TRY_FREE_AND_SET_NULL(sPokenav2StructPtr);
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}
