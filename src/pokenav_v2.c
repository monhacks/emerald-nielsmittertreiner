#include "global.h"
#include "malloc.h"
#include "bg.h"
#include "gpu_regs.h"
#include "task.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_effect_helpers.h"
#include "field_player_avatar.h"
#include "field_weather.h"
#include "graphics.h"
#include "list_menu.h"
#include "palette.h"
#include "pokenav_v2.h"
#include "quests.h"
#include "region_map.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "international_string_util.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"
#include "text_window.h"
#include "time.h"
#include "trig.h"
#include "util.h"
#include "window.h"
#include "constants/quests.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"
#include "constants/songs.h"
#include "constants/weather.h"
#include "data/quests.h"

#define OPTION_SLIDE_X      128
#define OPTION_SLIDE_Y      160
#define OPTION_SLIDE_SPEED  16
#define TIME_COLON_BLINK    20

#define QUEST_SCROLL_DELAY_FULL          120
#define QUEST_SCROLL_DELAY_THREE_QUARTER ((QUEST_SCROLL_DELAY_FULL / 4) * 3)
#define QUEST_SCROLL_DELAY_HALF          ((QUEST_SCROLL_DELAY_FULL / 2))
#define QUEST_SCROLL_DELAY_QUARTER       (QUEST_SCROLL_DELAY_FULL / 4)

#define NUM_MENU_SPRITES   9
#define NUM_AGENDA_SPRITES 1
#define NUM_AGENDA_TASKS   3

enum
{
    WIN_HEADER,
    WIN_DESC,
    WIN_OPTION_TOP_LEFT,
    WIN_OPTION_BOTTOM_LEFT,
    WIN_OPTION_TOP_RIGHT,
    WIN_OPTION_BOTTOM_RIGHT,
    WIN_REGION_MAP_TITLE,
    WIN_REGION_MAP_SECTION,
    WIN_AGENDA_QUESTS_TITLE,
    WIN_AGENDA_QUESTS_LIST,
    WIN_AGENDA_QUESTS_PROGRESS,
    WIN_AGENDA_DATE_TIME,
    WIN_AGENDA_WEATHER,
};

enum
{
    TEXT_COLORS_TRANSPARENT,
    TEXT_COLORS_GRAY,
    TEXT_COLORS_WHITE,
    TEXT_COLORS_RED,
    TEXT_COLORS_GREEN,
    TEXT_COLORS_BLUE,
};

enum
{
    TEXT_QUEST_GRAY,
    TEXT_QUEST_GREEN,
};

enum
{
    TAG_PAL,
    TAG_OPTIONS_LEFT,
    TAG_OPTIONS_RIGHT,
    TAG_AGENDA_ICONS,
    TAG_SCROLL_ARROW,
};

enum
{
    REGION_MAP,
    AGENDA,
    RADIO,
    MAIN_MENU
};

struct Pokenav2Struct
{
    struct ListMenuTemplate *listMenuTemplate;
    struct ListMenuItem *listMenuItems;
    struct RegionMap *regionMap;
    void *tilemapBuffers[NUM_BACKGROUNDS];
    u8 *spriteIds;
    u8 *taskIds;
    u8 bgOffset;
    u8 cursor;
    u8 timeColonBlinkTimer:6;
    u8 timeColonInvisible:1;
    u8 timeColonRedraw:1;
    const u8 *const *description;
    u32 windowScrollDistance:4;
    u32 scrollDelayCounter:7;
    u32 printDelayCounter:3;
    u32 numLinesPrinted:4;
    u32 numLinesMax:4;
    u16 numQuests;
    u16 currItem;
};

struct WeatherReportText
{
    const u8 *string;
    const u8 *color;
};

static void VBlankCB_Pokenav2(void);
static void CB2_Pokenav2(void);
static void LoadOptionBgs(u8 option);
static void LoadOptionData(u8 option);
static bool8 CreateQuestListMenuTemplate(void);
static void InitEventWindows(void);
static void LoadMainMenuSprites(void);

static void UnloadOptionData(u8 option);
static void UnloadMainMenuSprites(void);

static void SpriteCB_Icons(struct Sprite *sprite);
static void Task_Pokenav2(u8 taskId);
static void Task_MainMenu(u8 taskId);
static void Task_RegionMap(u8 taskId);
static void Task_Agenda(u8 taskId);
static void QuestListMenuCursorMoveFunc(s32 itemIndex, bool8 onInit, struct ListMenu *list);
static void QuestListMenuItemPrintFunc(u8 windowId, u32 itemId, u8 y);
static void Task_FadeQuestTypeBadgePalette(u8 taskId);
static void Task_Radio(u8 taskId);

static bool8 Task_SlideMainMenuIn(u8 taskId);
static bool8 Task_SlideMainMenuOut(u8 taskId);
static bool8 Task_SlideOptionIn(u8 taskId);
static bool8 Task_SlideOptionOut(u8 taskId);

static void Task_LoadOptionData(u8 taskId);
static void Task_ReturnToMainMenu(u8 taskId);
static void Task_ExitPokenav2(u8 taskId);

// ewram
static EWRAM_DATA struct Pokenav2Struct *sPokenav2Ptr = NULL;

// .rodata
static const u32 sPokenav2GridTiles[] = INCBIN_U32("graphics/pokenav_v2/grid.4bpp.lz");
static const u32 sPokenav2GridTilemap[] = INCBIN_U32("graphics/pokenav_v2/grid.bin.lz");
static const u16 sPokenav2GridPalette[] = INCBIN_U16("graphics/pokenav_v2/grid.gbapal");

static const u32 sPokenav2WindowFrameTiles[] = INCBIN_U32("graphics/pokenav_v2/window.4bpp.lz");
static const u16 sPokenav2WindowFramePalette[] = INCBIN_U16("graphics/pokenav_v2/window.gbapal");

static const u32 sPokenav2DescTilemap[] = INCBIN_U32("graphics/pokenav_v2/desc.bin.lz");
static const u32 sPokenav2RegionMapFrameTilemap[] = INCBIN_U32("graphics/pokenav_v2/map.bin.lz");
static const u32 sPokenav2AgendaTilemap[] = INCBIN_U32("graphics/pokenav_v2/agenda.bin.lz");
static const u32 sPokenav2RadioTilemap[] = INCBIN_U32("graphics/pokenav_v2/radio.bin.lz");

static const u8 sPokenav2OptionsLeft[] = INCBIN_U8("graphics/pokenav_v2/options_left.4bpp");
static const u8 sPokenav2OptionsRight[] = INCBIN_U8("graphics/pokenav_v2/options_right.4bpp");
static const u8 sPokenav2AgendaClockIcons[] = INCBIN_U8("graphics/pokenav_v2/agenda.4bpp");
static const u16 sPokenav2SpritePalette[] = INCBIN_U16("graphics/pokenav_v2/icons.gbapal");

static const u8 sPokenav2OptionLeftPositions[][2] =
{
    {48, 48},
    {48, 88},
    {192, 48},
    {192, 88},
};

static const u8 sPokenav2OptionRightPositions[][2] =
{
    {88, 48},
    {88, 88},
    {152, 48},
    {152, 88},
};

static const u8 sPokenav2TextColors[][3] =
{
    [TEXT_COLORS_TRANSPARENT] = {0, 0, 0},
    [TEXT_COLORS_GRAY]        = {0, 2, 3},
    [TEXT_COLORS_WHITE]       = {0, 1, 2},
    [TEXT_COLORS_RED]         = {0, 4, 3},
    [TEXT_COLORS_GREEN]       = {0, 6, 3},
    [TEXT_COLORS_BLUE]        = {0, 8, 3},
};

static const u8 sQuestListColors[][3] = 
{
    [TEXT_QUEST_GRAY]  = {0, 2, 3},
    [TEXT_QUEST_GREEN] = {0, 1, 3},
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

static const struct BgTemplate sPokenav2BgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
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
    [WIN_HEADER] = 
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x10,
    },
    [WIN_DESC] = 
    {
        .bg = 0,
        .tilemapLeft = 4,
        .tilemapTop = 15,
        .width = 22,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    [WIN_OPTION_TOP_LEFT] = 
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xC4,
    },
    [WIN_OPTION_BOTTOM_LEFT] = 
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xD4,
    },
    [WIN_OPTION_TOP_RIGHT] = 
    {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xE4,
    },
    [WIN_OPTION_BOTTOM_RIGHT] = 
    {
        .bg = 0,
        .tilemapLeft = 21,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0xF4,
    },
    [WIN_REGION_MAP_TITLE] = 
    {
        .bg = 0,
        .tilemapLeft = 25,
        .tilemapTop = 1,
        .width = 4,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    [WIN_REGION_MAP_SECTION] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 18,
        .width = 28,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x88,
    },
    [WIN_AGENDA_QUESTS_TITLE] = 
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 3,
        .width = 13,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x4C,
    },
    [WIN_AGENDA_QUESTS_LIST] = 
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 13,
        .height = 8,
        .paletteNum = 14,
        .baseBlock = 0x66,
    },
    [WIN_AGENDA_QUESTS_PROGRESS] = 
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 14,
        .width = 13,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0xCE,
    },
    [WIN_AGENDA_DATE_TIME] = 
    {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 14,
        .width = 8,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x10F,
    },
    [WIN_AGENDA_WEATHER] = 
    {
        .bg = 0,
        .tilemapLeft = 17,
        .tilemapTop = 3,
        .width = 11,
        .height = 4,
        .paletteNum = 15,
        .baseBlock = 0x12F,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WeatherReportText sWeatherReportText[] =
{
    [WEATHER_NONE] =
    {
        .string = gText_Pokenav2_NoReport,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_SUNNY_CLOUDS] =
    {
        .string = gText_Pokenav2_SunnyClouds,
        .color = sPokenav2TextColors[TEXT_COLORS_GREEN],
    },
    [WEATHER_SUNNY] =
    {
        .string = gText_Pokenav2_Clear,
        .color = sPokenav2TextColors[TEXT_COLORS_GREEN],
    },
    [WEATHER_RAIN] =
    {
        .string = gText_Pokenav2_Rain,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_SNOW] =
    {
        .string = gText_Pokenav2_Snow,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_RAIN_THUNDERSTORM] =
    {
        .string = gText_Pokenav2_Thunderstorm,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
    [WEATHER_FOG_HORIZONTAL] =
    {
        .string = gText_Pokenav2_Fog,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_VOLCANIC_ASH] =
    {
        .string = gText_Pokenav2_Ash,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
    [WEATHER_SANDSTORM] =
    {
        .string = gText_Pokenav2_Sandstorm,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
    [WEATHER_FOG_DIAGONAL] =
    {
        .string = gText_Pokenav2_Fog,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_UNDERWATER] =
    {
        .string = gText_Pokenav2_NoReport,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_SHADE] =
    {
        .string = gText_Pokenav2_Shade,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_DROUGHT] =
    {
        .string = gText_Pokenav2_Drought,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
    [WEATHER_DOWNPOUR] =
    {
        .string = gText_Pokenav2_Downpour,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
    [WEATHER_UNDERWATER_BUBBLES] =
    {
        .string = gText_Pokenav2_NoReport,
        .color = sPokenav2TextColors[TEXT_COLORS_BLUE],
    },
    [WEATHER_ABNORMAL] =
    {
        .string = gText_Pokenav2_Abnormal,
        .color = sPokenav2TextColors[TEXT_COLORS_RED],
    },
};

const struct SpritePalette sSpritePalette_OptionSprites =
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
    u32 bench;

    SetVBlankCallback(NULL);
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
    SetGpuReg(REG_OFFSET_BLDY, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
    DmaClear32(3, (void *)OAM, OAM_SIZE);
    FillPalette(RGB_BLACK, 0, PLTT_SIZE);
    ResetBgsAndClearDma3BusyFlags(0);
    ClearScheduledBgCopiesToVram();
    DeactivateAllTextPrinters();
    FreeAllSpritePalettes();
    ScanlineEffect_Stop();
    ResetPaletteFade();
    ResetSpriteData();
    ResetTasks();

    InitBgsFromTemplates(0, sPokenav2BgTemplates, ARRAY_COUNT(sPokenav2BgTemplates));
    InitWindows(sPokenav2WindowTemplates);
    sPokenav2Ptr = AllocZeroed(sizeof(struct Pokenav2Struct));
    sPokenav2Ptr->tilemapBuffers[0] = AllocZeroed(BG_SCREEN_SIZE);
    sPokenav2Ptr->tilemapBuffers[1] = AllocZeroed(BG_SCREEN_SIZE * 2);
    sPokenav2Ptr->tilemapBuffers[2] = AllocZeroed(BG_SCREEN_SIZE * 2);
    sPokenav2Ptr->tilemapBuffers[3] = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(0, sPokenav2Ptr->tilemapBuffers[0]);
    SetBgTilemapBuffer(1, sPokenav2Ptr->tilemapBuffers[1]);
    SetBgTilemapBuffer(2, sPokenav2Ptr->tilemapBuffers[2]);
    SetBgTilemapBuffer(3, sPokenav2Ptr->tilemapBuffers[3]);

    LZ77UnCompVram(sPokenav2GridTiles, (u16 *)BG_CHAR_ADDR(3));
    LZ77UnCompWram(sPokenav2GridTilemap, sPokenav2Ptr->tilemapBuffers[3]);
    LoadPalette(sPokenav2GridPalette, 0, sizeof(sPokenav2GridPalette));
    LoadPalette(GetOverworldTextboxPalettePtr(), 240, 32);
    CopyBgTilemapBufferToVram(3);
    
    LoadOptionBgs(MAIN_MENU);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_Pokenav2);
    SetMainCallback2(CB2_Pokenav2);
    CreateTask(Task_Pokenav2, 1);
    sPokenav2Ptr->bgOffset = OPTION_SLIDE_Y;
    
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
}

static void LoadOptionBgs(u8 option)
{
    DmaClear16(3, BG_CHAR_ADDR(1), BG_CHAR_SIZE);
    DmaClear16(3, BG_CHAR_ADDR(2), BG_CHAR_SIZE);
    DmaClear16(3, BG_SCREEN_ADDR(26), BG_SCREEN_SIZE);
    DmaClear16(3, BG_SCREEN_ADDR(28), BG_SCREEN_SIZE);
    DmaClear16(3, sPokenav2Ptr->tilemapBuffers[1], BG_SCREEN_SIZE * 2);
    DmaClear16(3, sPokenav2Ptr->tilemapBuffers[2], BG_SCREEN_SIZE * 2);

    switch (option)
    {
    case REGION_MAP:
        sPokenav2Ptr->regionMap = AllocZeroed(sizeof(struct RegionMap));
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompWram(sPokenav2RegionMapFrameTilemap, sPokenav2Ptr->tilemapBuffers[1]);
        InitRegionMap(sPokenav2Ptr->regionMap, FALSE);
        ClearWindowTilemap(WIN_HEADER);
        LoadPalette(sPokenav2WindowFramePalette, 16, 32);
        CopyBgTilemapBufferToVram(1);
        break;
    case AGENDA:
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompWram(sPokenav2AgendaTilemap, sPokenav2Ptr->tilemapBuffers[1]);
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        StringExpandPlaceholders(gStringVar1, gText_Pokenav2_PlayersAgenda);
        AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gStringVar1, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, gStringVar1);
        AddTextPrinterParameterized3(WIN_HEADER, FONT_SMALL, GetStringRightAlignXOffset(FONT_SMALL, gText_Pokenav2_ClockMode, DISPLAY_WIDTH) - 8, 0, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, gText_Pokenav2_ClockMode);
        LoadPalette(sPokenav2WindowFramePalette, 16, 32);
        LoadPalette(gQuestIconPalette, 224, 32);
        CopyBgTilemapBufferToVram(1);
        break;
    case RADIO:
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(1));
        LZ77UnCompWram(sPokenav2RadioTilemap, sPokenav2Ptr->tilemapBuffers[1]);
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_Pokenav2_EurusRadio, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, gText_Pokenav2_EurusRadio);
        CopyBgTilemapBufferToVram(1);
        break;
    case MAIN_MENU:
        LZ77UnCompVram(sPokenav2WindowFrameTiles, (u16 *)BG_CHAR_ADDR(2));
        LZ77UnCompWram(sPokenav2DescTilemap, sPokenav2Ptr->tilemapBuffers[2]);
        PutWindowTilemap(WIN_HEADER);
        FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(8));
        AddTextPrinterParameterized3(WIN_HEADER, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_Pokenav2, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, gText_Pokenav2);
        LoadPalette(sPokenav2WindowFramePalette, 16, 32);
        LoadMainMenuSprites();
        CopyBgTilemapBufferToVram(2);
        break;
    }

    CopyBgTilemapBufferToVram(0);
}

static void LoadOptionData(u8 option)
{
    switch (option)
    {
    case REGION_MAP:
        CreateRegionMapCursor(0, 0);
        CreateRegionMapPlayerIcon(1, 1);
        PutWindowTilemap(WIN_REGION_MAP_TITLE);
        PutWindowTilemap(WIN_REGION_MAP_SECTION);
        AddTextPrinterParameterized3(WIN_REGION_MAP_TITLE, FONT_SMALL, GetStringRightAlignXOffset(FONT_SMALL, gText_Pokenav2_Eurus, 24), 1, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, gText_Pokenav2_Eurus);
        AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, sPokenav2Ptr->regionMap->mapSecName, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, sPokenav2Ptr->regionMap->mapSecName);
        break;
    case AGENDA:
        sPokenav2Ptr->currItem = 0;

        PutWindowTilemap(WIN_AGENDA_QUESTS_TITLE);
        PutWindowTilemap(WIN_AGENDA_QUESTS_LIST);
        PutWindowTilemap(WIN_AGENDA_QUESTS_PROGRESS);
        PutWindowTilemap(WIN_AGENDA_DATE_TIME);
        PutWindowTilemap(WIN_AGENDA_WEATHER);

        InitEventWindows();

        AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek], 64), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek]);
        FormatDecimalTimeWithoutSeconds(gStringVar1, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes, gSaveBlock2Ptr->optionsClockMode);
        AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, FONT_SMALL, GetStringCenterAlignXOffset(FONT_SMALL, gStringVar1, 64), 16, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gStringVar1);
        if (sPokenav2Ptr->timeColonInvisible)
        {
            if (gSaveBlock2Ptr->optionsClockMode)
                FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 28, 16, 6, 12);
            else
                FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(1), 21, 16, 6, 12);
        }

        AddTextPrinterParameterized3(WIN_AGENDA_WEATHER, FONT_SMALL, GetStringCenterAlignXOffset(FONT_SMALL, gText_Pokenav2_WeatherForecast, 88), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gText_Pokenav2_WeatherForecast);
        AddTextPrinterParameterized3(WIN_AGENDA_WEATHER, FONT_SMALL, GetStringCenterAlignXOffset(FONT_SMALL, sWeatherReportText[GetSavedWeather()].string, 88), 16, sWeatherReportText[GetSavedWeather()].color, 0, sWeatherReportText[GetSavedWeather()].string);
        break;
    case RADIO:
        break;
    case MAIN_MENU:
        PutWindowTilemap(WIN_DESC);
        PutWindowTilemap(WIN_OPTION_TOP_LEFT);
        PutWindowTilemap(WIN_OPTION_BOTTOM_LEFT);
        PutWindowTilemap(WIN_OPTION_TOP_RIGHT);
        PutWindowTilemap(WIN_OPTION_BOTTOM_RIGHT);
        AddTextPrinterParameterized(WIN_OPTION_TOP_LEFT, FONT_NORMAL, gText_Pokenav2_Map, 4, 0, 0, NULL);
        AddTextPrinterParameterized(WIN_OPTION_BOTTOM_LEFT, FONT_NORMAL, gText_Pokenav2_Agenda, 4, 0, 0, NULL);
        AddTextPrinterParameterized(WIN_OPTION_TOP_RIGHT, FONT_NORMAL, gText_Pokenav2_Radio, 0, 0, 0, NULL);
        AddTextPrinterParameterized(WIN_OPTION_BOTTOM_RIGHT, FONT_NORMAL, gText_Pokenav2_TurnOff, 0, 0, 0, NULL);
        AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, sMenuDescriptions[sPokenav2Ptr->cursor], 176), 4, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, sMenuDescriptions[sPokenav2Ptr->cursor]);
        break;
    }

    CopyBgTilemapBufferToVram(0);
}

static void InitEventWindows(void)
{
    FillWindowPixelBuffer(WIN_AGENDA_QUESTS_TITLE, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_AGENDA_QUESTS_LIST, PIXEL_FILL(0));
    FillWindowPixelBuffer(WIN_AGENDA_QUESTS_PROGRESS, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_AGENDA_QUESTS_TITLE, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_Pokenav2_ActiveQuests, 104), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gText_Pokenav2_ActiveQuests);

    if (!CreateQuestListMenuTemplate())
        AddTextPrinterParameterized3(WIN_AGENDA_QUESTS_LIST, FONT_SMALL, GetStringCenterAlignXOffset(FONT_SMALL, gText_Pokenav2_NoActiveQuests, 104), 32, sQuestListColors[TEXT_QUEST_GRAY], 0, gText_Pokenav2_NoActiveQuests);

    CopyWindowToVram(WIN_AGENDA_QUESTS_PROGRESS, COPYWIN_GFX);
}

static bool8 CreateQuestListMenuTemplate(void)
{
    u8 questIndex = 0;

    for (u32 i = 0; i < NUM_QUESTS; i++)
    {
        if (QuestGet(i) && QuestGetProgress(i) != QUEST_COMPLETED)
        {
            sPokenav2Ptr->numQuests++;
        }
    }

    if (sPokenav2Ptr->numQuests)
    {
        sPokenav2Ptr->listMenuTemplate = Alloc(sizeof(struct ListMenuTemplate));
        sPokenav2Ptr->listMenuItems = Alloc(sPokenav2Ptr->numQuests * sizeof(struct ListMenuItem));
        sPokenav2Ptr->spriteIds = AllocZeroed(NUM_AGENDA_SPRITES * sizeof(u8));
        sPokenav2Ptr->taskIds = AllocZeroed(NUM_AGENDA_TASKS * sizeof(u8));

        for (u32 i = 0; i < NUM_QUESTS; i++)
        {
            if (QuestGet(i) && QuestGetProgress(i) != QUEST_COMPLETED)
            {
                sPokenav2Ptr->listMenuItems[questIndex].name = QuestGetName(i);
                sPokenav2Ptr->listMenuItems[questIndex].id = i;

                if (QuestGetProgress(i) == QUEST_READY)
                    sPokenav2Ptr->listMenuItems[questIndex].colors = sQuestListColors[TEXT_QUEST_GREEN];
                else
                    sPokenav2Ptr->listMenuItems[questIndex].colors = sQuestListColors[TEXT_QUEST_GRAY];

                questIndex++;
            }
        }

        sPokenav2Ptr->listMenuTemplate->items = sPokenav2Ptr->listMenuItems;
        sPokenav2Ptr->listMenuTemplate->moveCursorFunc = QuestListMenuCursorMoveFunc;
        sPokenav2Ptr->listMenuTemplate->itemPrintFunc = QuestListMenuItemPrintFunc;
        sPokenav2Ptr->listMenuTemplate->totalItems = sPokenav2Ptr->numQuests;

        if (sPokenav2Ptr->numQuests > 5)
            sPokenav2Ptr->listMenuTemplate->maxShowed = 5;
        else
            sPokenav2Ptr->listMenuTemplate->maxShowed = sPokenav2Ptr->numQuests;

        sPokenav2Ptr->listMenuTemplate->windowId = WIN_AGENDA_QUESTS_LIST;
        sPokenav2Ptr->listMenuTemplate->header_X = 0;
        sPokenav2Ptr->listMenuTemplate->item_X = 8;
        sPokenav2Ptr->listMenuTemplate->cursor_X = 0;
        sPokenav2Ptr->listMenuTemplate->upText_Y = 0;
        sPokenav2Ptr->listMenuTemplate->cursorPal = 2;
        sPokenav2Ptr->listMenuTemplate->fillValue = 0;
        sPokenav2Ptr->listMenuTemplate->cursorShadowPal = 3;
        sPokenav2Ptr->listMenuTemplate->lettersSpacing = 1;
        sPokenav2Ptr->listMenuTemplate->itemVerticalPadding = 1;
        sPokenav2Ptr->listMenuTemplate->scrollMultiple = LIST_NO_MULTIPLE_SCROLL;
        sPokenav2Ptr->listMenuTemplate->fontId = FONT_SMALL;
        sPokenav2Ptr->listMenuTemplate->cursorKind = 0;

        sPokenav2Ptr->taskIds[0] = ListMenuInit(sPokenav2Ptr->listMenuTemplate, sPokenav2Ptr->currItem, 0);
        sPokenav2Ptr->taskIds[1] = AddScrollIndicatorArrowPairParameterized(SCROLL_ARROW_UP, 68, 20, 148 - 40, sPokenav2Ptr->listMenuTemplate->totalItems - 1, TAG_SCROLL_ARROW, TAG_SCROLL_ARROW, &sPokenav2Ptr->currItem);
        sPokenav2Ptr->taskIds[2] = CreateTask(Task_FadeQuestTypeBadgePalette, 2);
    }

    return sPokenav2Ptr->numQuests;
}

static void LoadMainMenuSprites(void)
{
    struct Sprite *sprite;

    LoadSpritePalette(&sSpritePalette_OptionSprites);
    LoadSpriteSheet(&sSpriteSheet_OptionLeftTiles);
    LoadSpriteSheet(&sSpriteSheet_OptionRightTiles);
    LoadSpriteSheet(&sSpriteSheet_AgendaClockTiles);

    sPokenav2Ptr->spriteIds = AllocZeroed(NUM_MENU_SPRITES * sizeof(u8));

    for (u32 i = 0; i < 4; i++)
    {
        sPokenav2Ptr->spriteIds[i] = CreateSprite(&sSpriteTemplate_OptionsLeft, (s16)sPokenav2OptionLeftPositions[i][0], (s16)sPokenav2OptionLeftPositions[i][1], 2);
        sPokenav2Ptr->spriteIds[i + 4] = CreateSprite(&sSpriteTemplate_OptionsRight, (s16)sPokenav2OptionRightPositions[i][0], (s16)sPokenav2OptionRightPositions[i][1], 1);
        StartSpriteAnim(&gSprites[sPokenav2Ptr->spriteIds[i]], (i < 2 ? 0 : 1));
        StartSpriteAnim(&gSprites[sPokenav2Ptr->spriteIds[i + 4]], i);
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i]];
        sprite->x2 = sprite->animNum == 0 ? -OPTION_SLIDE_X : OPTION_SLIDE_X;
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        sprite->x2 = sprite->animNum == 0 || sprite->animNum == 1 ? -OPTION_SLIDE_X : OPTION_SLIDE_X;
    }

    sPokenav2Ptr->spriteIds[8] = CreateSprite(&sSpriteTemplate_AgendaClockIcons, 88, 87, 0);
    StartSpriteAnim(&gSprites[sPokenav2Ptr->spriteIds[8]], gSaveBlock2Ptr->inGameClock.hours < 12 ? gSaveBlock2Ptr->inGameClock.hours : gSaveBlock2Ptr->inGameClock.hours - 12);
    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    sprite->x2 = -OPTION_SLIDE_X;
}

static void UnloadOptionData(u8 option)
{
    switch (option)
    {
    case REGION_MAP:
        ClearWindowTilemap(WIN_REGION_MAP_TITLE);
        ClearWindowTilemap(WIN_REGION_MAP_SECTION);
        FreeRegionMapIconResources();
        FREE_AND_SET_NULL(sPokenav2Ptr->regionMap);
        break;
    case AGENDA:
        ClearWindowTilemap(WIN_AGENDA_QUESTS_TITLE);
        ClearWindowTilemap(WIN_AGENDA_QUESTS_LIST);
        ClearWindowTilemap(WIN_AGENDA_QUESTS_PROGRESS);
        ClearWindowTilemap(WIN_AGENDA_DATE_TIME);
        ClearWindowTilemap(WIN_AGENDA_WEATHER);
        
        if (sPokenav2Ptr->numQuests)
        {
            DestroyListMenuTask(sPokenav2Ptr->taskIds[0], &sPokenav2Ptr->currItem, NULL);
            RemoveScrollIndicatorArrowPair(sPokenav2Ptr->taskIds[1]);
            DestroyTask(sPokenav2Ptr->taskIds[2]);
            DestroySprite(&gSprites[sPokenav2Ptr->spriteIds[0]]);
            FREE_AND_SET_NULL(sPokenav2Ptr->listMenuTemplate);
            FREE_AND_SET_NULL(sPokenav2Ptr->listMenuItems);
            FREE_AND_SET_NULL(sPokenav2Ptr->spriteIds);
            FREE_AND_SET_NULL(sPokenav2Ptr->taskIds);
            sPokenav2Ptr->numQuests = 0;
        }
        break;
    case RADIO:
        break;
    case MAIN_MENU:
        ClearWindowTilemap(WIN_DESC);
        ClearWindowTilemap(WIN_OPTION_TOP_LEFT);
        ClearWindowTilemap(WIN_OPTION_BOTTOM_LEFT);
        ClearWindowTilemap(WIN_OPTION_TOP_RIGHT);
        ClearWindowTilemap(WIN_OPTION_BOTTOM_RIGHT);
        break;
    }

    CopyBgTilemapBufferToVram(0);
}

static void UnloadMainMenuSprites(void)
{
    FreeSpritePaletteByTag(TAG_PAL);
    FreeSpriteTilesByTag(TAG_OPTIONS_LEFT);
    FreeSpriteTilesByTag(TAG_OPTIONS_RIGHT);

    for (u32 i = 0; i < NUM_MENU_SPRITES; i++)
        DestroySprite(&gSprites[sPokenav2Ptr->spriteIds[i]]);

    FREE_AND_SET_NULL(sPokenav2Ptr->spriteIds);
}

static void SpriteCB_Icons(struct Sprite *sprite)
{
    if (sprite->animNum == sPokenav2Ptr->cursor)
    {
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 < 8)
                sprite->x2 += 2;
            else
                sprite->x2 = 8;
        }
        else
        {
            if (sprite->x2 > -8)
                sprite->x2 -= 2;
            else
                sprite->x2 = -8;
        }
    }
    else
    {
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 > 0)
                sprite->x2 -= 2;
            else
                sprite->x2 = 0;
        }
        else
        {
            if (sprite->x2 > 0)
                sprite->x2 += 2;
            else
                sprite->x2 = 0;
        }
    }
}

static void SpriteCB_Agenda(struct Sprite *sprite)
{
    if (sPokenav2Ptr->cursor == 1)
    {
        if (sprite->x2 < 8)
            sprite->x2 += 2;
        else
            sprite->x2 = 8;
    }
    else
    {
        if (sprite->x2 > 0)
            sprite->x2 -= 2;
        else
            sprite->x2 = 0;
    }
}

static void UpdateOptionDescription(u8 option)
{
    FillWindowPixelBuffer(WIN_DESC, PIXEL_FILL(0));
    AddTextPrinterParameterized3(WIN_DESC, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, sMenuDescriptions[option], 176), 4, sPokenav2TextColors[TEXT_COLORS_WHITE], 0, sMenuDescriptions[option]);
}

static void Task_MainMenu(u8 taskId)
{
    if (JOY_NEW(DPAD_LEFT))
    {
        if (sPokenav2Ptr->cursor & 2)
        {
            sPokenav2Ptr->cursor ^= 2;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2Ptr->cursor);
        }
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (!(sPokenav2Ptr->cursor & 2) && (sPokenav2Ptr->cursor ^ 2) < 4)
        {
            sPokenav2Ptr->cursor ^= 2;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2Ptr->cursor);
        }
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sPokenav2Ptr->cursor & 1)
        {
            sPokenav2Ptr->cursor ^= 1;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2Ptr->cursor);
        }
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (!(sPokenav2Ptr->cursor & 1) && (sPokenav2Ptr->cursor ^ 1) < 4)
        {
            sPokenav2Ptr->cursor ^= 1;
            PlaySE(SE_DEX_SCROLL);
            UpdateOptionDescription(sPokenav2Ptr->cursor);
        }
    }
    else if (JOY_NEW(A_BUTTON))
    {
        gTasks[taskId].func = Task_LoadOptionData;
    }
    else if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ExitPokenav2;
        PlaySE(SE_POKENAV_OFF);
    }
}

static void Task_Pokenav2_2(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        sprite->callback = SpriteCB_Icons;
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    sprite->callback = SpriteCB_Agenda;

    LoadOptionData(MAIN_MENU);
    gTasks[taskId].func = Task_MainMenu;
}

static void Task_Pokenav2(u8 taskId)
{
    if (Task_SlideMainMenuIn(taskId))
        gTasks[taskId].func = Task_Pokenav2_2;
}

static void Task_RegionMap(u8 taskId)
{
    switch (DoRegionMapInputCallback())
    {
        case MAP_INPUT_MOVE_END:
            FillWindowPixelBuffer(WIN_REGION_MAP_SECTION, PIXEL_FILL(0));
            if (sPokenav2Ptr->regionMap->mapSecType != MAPSECTYPE_NONE)
                AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, sPokenav2Ptr->regionMap->mapSecName, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, sPokenav2Ptr->regionMap->mapSecName);
            else
                AddTextPrinterParameterized3(WIN_REGION_MAP_SECTION, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_ThreeDashes, DISPLAY_WIDTH), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gText_ThreeDashes);
            
            CopyBgTilemapBufferToVram(0);
            break;
    }

    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu;
    }
}

static void PrintQuestDescription(const u8 *string)
{
    u8 lineHeight = GetFontAttribute(FONT_SHORT, FONTATTR_MAX_LETTER_HEIGHT);
    u8 description[QUEST_DESC_LINE_LENGTH];
    u8 y;

    if (sPokenav2Ptr->numLinesPrinted)
        y = lineHeight;

    StringExpandPlaceholders(description, string);
    AddTextPrinterParameterized3(WIN_AGENDA_QUESTS_PROGRESS, FONT_SHORT, 4, y, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, description);
    sPokenav2Ptr->numLinesPrinted++;

    sPokenav2Ptr->scrollDelayCounter = QUEST_SCROLL_DELAY_FULL;
    sPokenav2Ptr->windowScrollDistance = lineHeight / 2;
}

static void Task_Agenda(u8 taskId)
{
    u8 input = ListMenu_ProcessInput(sPokenav2Ptr->taskIds[0]);
    u8 text[8];

    if (--sPokenav2Ptr->timeColonBlinkTimer == 0)
    {
        sPokenav2Ptr->timeColonBlinkTimer = TIME_COLON_BLINK;
        sPokenav2Ptr->timeColonInvisible ^= 1;
        sPokenav2Ptr->timeColonRedraw = TRUE;
    }

    if (sPokenav2Ptr->timeColonRedraw)
    {
        FillWindowPixelBuffer(WIN_AGENDA_DATE_TIME, PIXEL_FILL(0));
        AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, FONT_NORMAL, GetStringCenterAlignXOffset(FONT_NORMAL, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek], 64), 0, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, gText_DaysOfWeek[gSaveBlock2Ptr->inGameClock.dayOfWeek]);
        FormatDecimalTimeWithoutSeconds(text, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes, gSaveBlock2Ptr->optionsClockMode);
        AddTextPrinterParameterized3(WIN_AGENDA_DATE_TIME, FONT_SMALL, GetStringCenterAlignXOffset(FONT_SMALL, text, 64), 16, sPokenav2TextColors[TEXT_COLORS_GRAY], 0, text);

        if (sPokenav2Ptr->timeColonInvisible)
        {
            if (gSaveBlock2Ptr->optionsClockMode)
                FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(0), 28, 16, 6, 12);
            else
                FillWindowPixelRect(WIN_AGENDA_DATE_TIME, PIXEL_FILL(0), 23, 16, 6, 12);
        }
        sPokenav2Ptr->timeColonRedraw = FALSE;
    }

    if (sPokenav2Ptr->numQuests)
    {
        if (sPokenav2Ptr->numLinesMax > 2)
        {
            switch (sPokenav2Ptr->scrollDelayCounter)
            {
            case QUEST_SCROLL_DELAY_FULL:
                FillBgTilemapBufferRect(1, 17, 14, 17, 1, 1, 1);
                CopyBgTilemapBufferToVram(1);
                break;
            case QUEST_SCROLL_DELAY_THREE_QUARTER:
                FillBgTilemapBufferRect(1, 18, 14, 17, 1, 1, 1);
                CopyBgTilemapBufferToVram(1);
                break;
            case QUEST_SCROLL_DELAY_HALF:
                FillBgTilemapBufferRect(1, 19, 14, 17, 1, 1, 1);
                CopyBgTilemapBufferToVram(1);
                break;
            case QUEST_SCROLL_DELAY_QUARTER:
                FillBgTilemapBufferRect(1, 20, 14, 17, 1, 1, 1);
                CopyBgTilemapBufferToVram(1);
                break;
            case 0:
                FillBgTilemapBufferRect(1, 2, 14, 17, 1, 1, 1);
                CopyBgTilemapBufferToVram(1);
                break;
            default:
                break;
            }
        }    

        if (sPokenav2Ptr->scrollDelayCounter == 0)
        {
            if (sPokenav2Ptr->numLinesPrinted > 1 && sPokenav2Ptr->windowScrollDistance && sPokenav2Ptr->numLinesMax > 2 && sPokenav2Ptr->numLinesPrinted < sPokenav2Ptr->numLinesMax)
            {
                ScrollWindow(WIN_AGENDA_QUESTS_PROGRESS, 0, 2, PIXEL_FILL(0));
                CopyWindowToVram(WIN_AGENDA_QUESTS_PROGRESS, COPYWIN_GFX);
                sPokenav2Ptr->windowScrollDistance--;
            }
            else if (sPokenav2Ptr->printDelayCounter == 0)
            {
                if (sPokenav2Ptr->numLinesPrinted == 0)
                {
                    for (u32 i = 0; i < 2; i++)
                        PrintQuestDescription(sPokenav2Ptr->description[i]);
                }
                else if (sPokenav2Ptr->numLinesPrinted < sPokenav2Ptr->numLinesMax)
                {
                    PrintQuestDescription(sPokenav2Ptr->description[sPokenav2Ptr->numLinesPrinted]);
                }
                else if (sPokenav2Ptr->numLinesPrinted == sPokenav2Ptr->numLinesMax && sPokenav2Ptr->numLinesMax > 2)
                {
                    FillWindowPixelBuffer(WIN_AGENDA_QUESTS_PROGRESS, PIXEL_FILL(0));
                    CopyWindowToVram(WIN_AGENDA_QUESTS_PROGRESS, COPYWIN_GFX);
                    sPokenav2Ptr->scrollDelayCounter = 0;
                    sPokenav2Ptr->printDelayCounter = GetFontAttribute(FONT_SHORT, FONTATTR_MAX_LETTER_HEIGHT) / 2;
                    sPokenav2Ptr->numLinesPrinted = 0;
                }
            }
            else if (sPokenav2Ptr->numLinesMax > 2)
            {
                sPokenav2Ptr->printDelayCounter--;
            }
        }
        else if (sPokenav2Ptr->printDelayCounter == 0)
        {
            if (sPokenav2Ptr->numLinesPrinted == 0)
            {
                for (u32 i = 0; i < 2; i++)
                    PrintQuestDescription(sPokenav2Ptr->description[i]);
            }
            else if (sPokenav2Ptr->numLinesPrinted < sPokenav2Ptr->numLinesMax)
            {
                PrintQuestDescription(sPokenav2Ptr->description[sPokenav2Ptr->numLinesPrinted]);
            }
            else if (sPokenav2Ptr->numLinesPrinted == sPokenav2Ptr->numLinesMax && sPokenav2Ptr->numLinesMax > 2)
            {
                FillWindowPixelBuffer(WIN_AGENDA_QUESTS_PROGRESS, PIXEL_FILL(0));
                CopyWindowToVram(WIN_AGENDA_QUESTS_PROGRESS, COPYWIN_GFX);
                sPokenav2Ptr->scrollDelayCounter = 0;
                sPokenav2Ptr->printDelayCounter = GetFontAttribute(FONT_SHORT, FONTATTR_MAX_LETTER_HEIGHT) / 2;
                sPokenav2Ptr->numLinesPrinted = 0;
            }
        }
        else if (sPokenav2Ptr->numLinesMax > 2)
        {
            sPokenav2Ptr->scrollDelayCounter--;
        }
    }

    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu;
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_SELECT);
        gSaveBlock2Ptr->optionsClockMode ^= 1;
        sPokenav2Ptr->timeColonRedraw = TRUE;
    }
    else if (JOY_NEW(DPAD_UP))
    {
        if (sPokenav2Ptr->currItem > 0)
            sPokenav2Ptr->currItem--;
    }
    else if (JOY_NEW(DPAD_DOWN))
    {
        if (sPokenav2Ptr->currItem < sPokenav2Ptr->listMenuTemplate->totalItems - 1)
            sPokenav2Ptr->currItem++;
    }
}

static void QuestListMenuCursorMoveFunc(s32 itemIndex, bool8 onInit, struct ListMenu *list)
{
    u16 id = sPokenav2Ptr->listMenuItems[itemIndex].id;

    if (!onInit)
        PlaySE(SE_DEX_SCROLL);

    FillWindowPixelBuffer(WIN_AGENDA_QUESTS_PROGRESS, PIXEL_FILL(0));
    sPokenav2Ptr->numLinesPrinted = 0;
    sPokenav2Ptr->scrollDelayCounter = 0;
    sPokenav2Ptr->printDelayCounter = 0;
    sPokenav2Ptr->numLinesMax = QuestGetLines(id);
    sPokenav2Ptr->description = QuestGetDescription(id);

    FillBgTilemapBufferRect(1, 2, 14, 17, 1, 1, 1);
    CopyBgTilemapBufferToVram(1);

    DestroySprite(&gSprites[sPokenav2Ptr->spriteIds[0]]);
    sPokenav2Ptr->spriteIds[0] = CreateObjectGraphicsSprite(QuestGetGraphicsId(id), SpriteCallbackDummy, 134, 124, 0);
    gSprites[sPokenav2Ptr->spriteIds[0]].oam.priority = 0;
    StartSpriteAnim(&gSprites[sPokenav2Ptr->spriteIds[0]], GetMoveDirectionAnimNum(DIR_SOUTH));
}

static void QuestListMenuItemPrintFunc(u8 windowId, u32 itemId, u8 y)
{
    u16 src = QuestGetType(itemId) * 8;

    BlitBitmapRectToWindow(windowId, gQuestIconTiles, 0, src, 8, 8, 95, y + 4, 8, 8);
    CopyWindowToVram(windowId, COPYWIN_GFX);
}

static void Task_FadeQuestTypeBadgePalette(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    task->data[0] += 4;
    task->data[0] &= 0x7F;
    task->data[1] = gSineTable[task->data[0]] >> 5;

    BlendPalette(228, 12, task->data[1], RGB_WHITEALPHA);
}

static void Task_Radio(u8 taskId)
{
    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu;
    }
}

static bool8 Task_SlideMainMenuIn(u8 taskId)
{
    struct Sprite *sprite;

    if (sPokenav2Ptr->bgOffset > 0)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2Ptr->bgOffset);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2Ptr->bgOffset);
        sPokenav2Ptr->bgOffset -= OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        return TRUE;
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i]];
        if (sprite->animNum == 0)
        {
            if (sprite->x2 < 0)
                sprite->x2 += OPTION_SLIDE_SPEED;
            else
                sprite->x2 = 0;
        }
        else
        {
            if (sprite->x2 > 0)
                sprite->x2 -= OPTION_SLIDE_SPEED;
            else
                sprite->x2 = 0;
        }
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 < 0)
                sprite->x2 += OPTION_SLIDE_SPEED;
            else
                sprite->x2 = 0;
        }
        else
        {
            if (sprite->x2 > 0)
                sprite->x2 -= OPTION_SLIDE_SPEED;
            else
                sprite->x2 = 0;
        }
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    if (sprite->x2 < 0)
        sprite->x2 += OPTION_SLIDE_SPEED;
    else
        sprite->x2 = 0;

    return FALSE;
}

static bool8 Task_SlideMainMenuOut(u8 taskId)
{
    struct Sprite *sprite;

    if (sPokenav2Ptr->bgOffset < OPTION_SLIDE_Y)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2Ptr->bgOffset);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2Ptr->bgOffset);
        sPokenav2Ptr->bgOffset += OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - OPTION_SLIDE_Y);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - OPTION_SLIDE_Y);
        return TRUE;
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i]];
        if (sprite->animNum == 0)
        {
            if (sprite->x2 > -OPTION_SLIDE_X)
                sprite->x2 -= OPTION_SLIDE_SPEED;
            else
                sprite->x2 = -OPTION_SLIDE_X;
        }
        else
        {
            if (sprite->x2 < OPTION_SLIDE_X)
                sprite->x2 += OPTION_SLIDE_SPEED;
            else
                sprite->x2 = OPTION_SLIDE_X;
        }
    }

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        if (sprite->animNum == 0 || sprite->animNum == 1)
        {
            if (sprite->x2 > -OPTION_SLIDE_X)
                sprite->x2 -= OPTION_SLIDE_SPEED;
            else
                sprite->x2 = -OPTION_SLIDE_X;
        }
        else
        {
            if (sprite->x2 < OPTION_SLIDE_X)
                sprite->x2 += OPTION_SLIDE_SPEED;
            else
                sprite->x2 = OPTION_SLIDE_X;
        }
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    if (sprite->x2 > -OPTION_SLIDE_X)
        sprite->x2 -= OPTION_SLIDE_SPEED;
    else
        sprite->x2 = -OPTION_SLIDE_X;

    return FALSE;
}

static bool8 Task_SlideOptionIn(u8 taskId)
{
    if (sPokenav2Ptr->bgOffset > 0)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2Ptr->bgOffset);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2Ptr->bgOffset);
        sPokenav2Ptr->bgOffset -= OPTION_SLIDE_SPEED;
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
    if (sPokenav2Ptr->bgOffset < OPTION_SLIDE_Y)
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokenav2Ptr->bgOffset);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - sPokenav2Ptr->bgOffset);
        sPokenav2Ptr->bgOffset += OPTION_SLIDE_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - OPTION_SLIDE_Y);
        SetGpuReg(REG_OFFSET_BG2VOFS, 512 - OPTION_SLIDE_Y);
        return TRUE;
    }

    return FALSE;
}

static void Task_LoadOption_7(u8 taskId)
{
    LoadOptionData(sPokenav2Ptr->cursor);
    gTasks[taskId].func = sPokenav2Funcs[sPokenav2Ptr->cursor];
}

static void Task_LoadOption_6(u8 taskId)
{
    if (Task_SlideOptionIn(taskId))
    {
        gTasks[taskId].func = Task_LoadOption_7;
    }
}

static void Task_LoadOption_5(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    LoadOptionBgs(sPokenav2Ptr->cursor);
    sPokenav2Ptr->bgOffset = OPTION_SLIDE_Y;
    gTasks[taskId].func = Task_LoadOption_6;
}

static void Task_LoadOption_4(u8 taskId)
{
    UnloadMainMenuSprites();
    gTasks[taskId].func = Task_LoadOption_5;
}

static void Task_LoadOption_3(u8 taskId)
{
    if (Task_SlideMainMenuOut(taskId))
    {
        gTasks[taskId].func = Task_LoadOption_4;
    }
}

static void Task_LoadOption_2(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        sprite->callback = SpriteCallbackDummy;
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    sprite->callback = SpriteCallbackDummy;

    UnloadOptionData(MAIN_MENU);
    PlaySE(SE_BALL_TRAY_ENTER);
    gTasks[taskId].func = Task_LoadOption_3;
}

static void Task_LoadOptionData(u8 taskId)
{
    if (sPokenav2Ptr->cursor == 3)
    {
        PlaySE(SE_POKENAV_OFF);
        gTasks[taskId].func = Task_ExitPokenav2;
    }
    else
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].func = Task_LoadOption_2;
    }
}

static void Task_ReturnToMainMenu_6(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        sprite->callback = SpriteCB_Icons;
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    sprite->callback = SpriteCB_Agenda;

    LoadOptionData(MAIN_MENU);
    gTasks[taskId].func = Task_MainMenu;
}

static void Task_ReturnToMainMenu_5(u8 taskId)
{
    if (Task_SlideMainMenuIn(taskId))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_6;
    }
}

static void Task_ReturnToMainMenu_4(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    gTasks[taskId].func = Task_ReturnToMainMenu_5;
}

static void Task_ReturnToMainMenu_3(u8 taskId)
{
    LoadOptionBgs(MAIN_MENU);
    gTasks[taskId].func = Task_ReturnToMainMenu_4;
}

static void Task_ReturnToMainMenu_2(u8 taskId)
{
    if (Task_SlideOptionOut(taskId))
    {
        gTasks[taskId].func = Task_ReturnToMainMenu_3;
    }
}

static void Task_ReturnToMainMenu(u8 taskId)
{
    PlaySE(SE_BALL_TRAY_ENTER);
    UnloadOptionData(sPokenav2Ptr->cursor);
    sPokenav2Ptr->bgOffset = 0;
    gTasks[taskId].func = Task_ReturnToMainMenu_2;
}

static void Task_ExitPokenav2_4(u8 taskId)
{
    void *tilemapBuffers;

    if (!gPaletteFade.active)
    {
        for (u32 i = 0; i < NUM_BACKGROUNDS; i++)
        {
            UnsetBgTilemapBuffer(i);
            tilemapBuffers = GetBgTilemapBuffer(i);
            FREE_AND_SET_NULL(tilemapBuffers);
        }
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_ReturnToFieldWithOpenMenu);
    }
}

static void Task_ExitPokenav2_3(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ExitPokenav2_4;
}

static void Task_ExitPokenav2_2(u8 taskId)
{
    if (Task_SlideMainMenuOut(taskId))
    {
        gTasks[taskId].func = Task_ExitPokenav2_3;
    }
}

static void Task_ExitPokenav2(u8 taskId)
{
    struct Sprite *sprite;

    for (u32 i = 0; i < 4; i++)
    {
        sprite = &gSprites[sPokenav2Ptr->spriteIds[i + 4]];
        sprite->callback = SpriteCallbackDummy;
    }

    sprite = &gSprites[sPokenav2Ptr->spriteIds[8]];
    sprite->callback = SpriteCallbackDummy;

    UnloadOptionData(MAIN_MENU);
    sPokenav2Ptr->cursor = 0;
    gTasks[taskId].func = Task_ExitPokenav2_2;
}
