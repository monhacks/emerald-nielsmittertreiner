#include "global.h"
#include "bg.h"
#include "decompress.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "malloc.h"
#include "task.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokegear.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "start_menu.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define tPosition  data[0]
#define tState data[1]

#define STATE_INIT -1

#define OPTION_SCROLL_X       212
#define OPTION_SCROLL_Y       48
#define OPTION_SCROLL_SPEED   4

enum
{
    WIN_CLOCK,
    WIN_DESC,
};

enum
{
    TAG_POKEBALL,
    TAG_DIGITS,
    TAG_EXIT,
    TAG_EXIT_BG,
    TAG_CLOCK_BAR,
};

enum
{
    OPTION_AGENDA,
    OPTION_REGION_MAP,
    OPTION_PHONE,
    OPTION_RADIO,
    OPTION_MON_STATUS,
    OPTION_SETTINGS,
    OPTION_EXIT,
};

enum
{
    POKEGEAR_STYLE_1,
};

struct PokeGearStyle
{
    const void *tileset;
    const void *backgroundTilemap;
    const void *foregroundTilemap;
    const void *palette;
    void (*gfxInitFunc)(void);
    void (*gfxMainFunc)(void);
    void (*gfxExitFunc)(void);
};

struct PokeGearResources
{
    void *tilemapBuffers[NUM_BACKGROUNDS];
    struct UCoords8 bgOffset;
    u8 numOptions;
    u8 actions[5];
    u8 cursor:3;
    u8 style:5;
    u8 spriteId;
    u8 digitSpriteIds[6];
    u8 exitSpriteId;
    u8 exitBgSpriteId;
    u8 clockSpriteId;
    u8 fakeSeconds;
};

static void Task_PokeGear_1(u8 taskId);
static void Task_PokeGear_2(u8 taskId);
static void Task_ExitPokeGear_1(u8 taskId);
static void Task_ExitPokeGear_2(u8 taskId);
static void Task_ExitPokeGear_3(u8 taskId);
static void LoadPokeGearStyle(u8 style);
static void PokeGearStyle1_Init(void);
static void PokeGearStyle1_Main(void);
static void PokeGearStyle1_Exit(void);
static void SpriteCB_PokeBall(struct Sprite *sprite);
static void SpriteCB_ClockDigits(struct Sprite *sprite);
static void SpriteCB_Exit(struct Sprite *sprite);
static void SpriteCB_ExitBg(struct Sprite *sprite);
static void SpriteCB_ClockBar(struct Sprite *sprite);
static bool32 SlideMainMenuIn(void);
static bool32 SlideMainMenuOut(void);

// ewram
EWRAM_DATA static struct PokeGearResources *sPokeGearResources = NULL;

// .rodata
static const u8 sPokeGearClockX[] = 
{
    16, 30, 38, 47, 61, 73,
};

static const struct BgTemplate sPokeGearBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 25,
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
        .screenSize = 1,
        .paletteMode = 0,
        .priority = 2
    },
    {
        .bg = 3,
        .charBaseIndex = 1,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
    },
};

static const struct WindowTemplate sPokeGearWindowTemplates[] =
{
    [WIN_CLOCK] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 9,
        .width = 11,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x10,
    },
    [WIN_DESC] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 16,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 0x26,
    },
};

static const struct OamData sOamData_PokeGearPokeBall =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_NORMAL,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearDigits =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearExit =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearExitBg =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_BLEND,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};

static const struct OamData sOamData_PokeGearClockBar =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_BLEND,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x32),
    .x = 0,
    .size = SPRITE_SIZE(32x32),
    .tileNum = 0,
    .priority = 1,
    .paletteNum = 0,
};


static const union AffineAnimCmd sAffineAnim_PokeGearPokeBall[] =
{
    AFFINEANIMCMD_FRAME(0, 0, 2, 1),
    AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd *const sAffineAnims_PokeGearPokeBall[] =
{
    sAffineAnim_PokeGearPokeBall,
};

static const union AnimCmd sSpriteAnim_Digit0[] =
{
    ANIMCMD_FRAME(0, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit1[] =
{
    ANIMCMD_FRAME(4, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit2[] =
{
    ANIMCMD_FRAME(8, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit3[] =
{
    ANIMCMD_FRAME(12, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit4[] =
{
    ANIMCMD_FRAME(16, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit5[] =
{
    ANIMCMD_FRAME(20, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit6[] =
{
    ANIMCMD_FRAME(24, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit7[] =
{
    ANIMCMD_FRAME(28, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit8[] =
{
    ANIMCMD_FRAME(32, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Digit9[] =
{
    ANIMCMD_FRAME(36, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_DigitOff[] =
{
    ANIMCMD_FRAME(40, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ColonOff[] =
{
    ANIMCMD_FRAME(44, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_ColonOn[] =
{
    ANIMCMD_FRAME(48, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_AM[] =
{
    ANIMCMD_FRAME(52, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_PM[] =
{
    ANIMCMD_FRAME(56, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_AMPMOff[] =
{
    ANIMCMD_FRAME(60, 5),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Point[] =
{
    ANIMCMD_FRAME(64, 5),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearDigits[] =
{
    sSpriteAnim_Digit0,
    sSpriteAnim_Digit1,
    sSpriteAnim_Digit2,
    sSpriteAnim_Digit3,
    sSpriteAnim_Digit4,
    sSpriteAnim_Digit5,
    sSpriteAnim_Digit6,
    sSpriteAnim_Digit7,
    sSpriteAnim_Digit8,
    sSpriteAnim_Digit9,
    sSpriteAnim_DigitOff,
    sSpriteAnim_ColonOff,
    sSpriteAnim_ColonOn,
    sSpriteAnim_AM,
    sSpriteAnim_PM,
    sSpriteAnim_AMPMOff,
    sSpriteAnim_Point,
};

static const union AnimCmd sSpriteAnim_Unselected[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Selected[] =
{
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearExit[] =
{
    sSpriteAnim_Unselected,
    sSpriteAnim_Selected,
};

static const union AnimCmd sSpriteAnim_Flip[] =
{
    ANIMCMD_FRAME(0, 8, .hFlip = TRUE),
    ANIMCMD_END
};

static const union AnimCmd *const sAnims_PokeGearExitBg[] =
{
    sSpriteAnim_Flip,
};

static const struct SpriteTemplate sPokeGearPokeBallSpriteTemplate =
{
    .tileTag = TAG_POKEBALL,
    .paletteTag = TAG_POKEBALL,
    .oam = &sOamData_PokeGearPokeBall,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = sAffineAnims_PokeGearPokeBall,
    .callback = SpriteCB_PokeBall,
};

static const struct SpriteTemplate sPokeGearDigitsSpriteTemplate =
{
    .tileTag = TAG_DIGITS,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearDigits,
    .anims = sAnims_PokeGearDigits,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ClockDigits,
};

static const struct SpriteTemplate sPokeGearExitSpriteTemplate =
{
    .tileTag = TAG_EXIT,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearExit,
    .anims = sAnims_PokeGearExit,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_Exit,
};

static const struct SpriteTemplate sPokeGearExitBgSpriteTemplate =
{
    .tileTag = TAG_EXIT_BG,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearExitBg,
    .anims = sAnims_PokeGearExitBg,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ExitBg,
};

static const struct SpriteTemplate sPokeGearClockBarSpriteTemplate =
{
    .tileTag = TAG_CLOCK_BAR,
    .paletteTag = TAG_DIGITS,
    .oam = &sOamData_PokeGearClockBar,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_ClockBar,
};

static const struct Subsprite sPokeGearClockBarSubsprites[] =
{
    {
        .x = -64,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = -32,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = 0,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 0,
        .priority = 1,
    },
    {
        .x = 32,
        .y = -16,
        .shape = SPRITE_SHAPE(32x32),
        .size = SPRITE_SIZE(32x32),
        .tileOffset = 32,
        .priority = 1,
    },
};

static const struct SubspriteTable sPokeGearClockBarSubspriteTable[] =
{
    {ARRAY_COUNT(sPokeGearClockBarSubsprites), sPokeGearClockBarSubsprites},
};


static const struct CompressedSpriteSheet sPokeGearPokeBallSpriteSheet =
{
    .data = gPokeGearPokeBall_Gfx,
    .size = 512,
    .tag = TAG_POKEBALL,
};

static const struct CompressedSpriteSheet sPokeGearDigitSpriteSheet =
{
    .data = gPokeGearMainMenu_Digits_Gfx,
    .size = 2176,
    .tag = TAG_DIGITS,
};

static const struct CompressedSpriteSheet sPokeGearExitSpriteSheet =
{
    .data = gPokeGearMainMenu_Exit_Gfx,
    .size = 256,
    .tag = TAG_EXIT,
};

static const struct CompressedSpriteSheet sPokeGearExitBgSpriteSheet =
{
    .data = gPokeGearMainMenu_BarEnd_Gfx,
    .size = 512,
    .tag = TAG_EXIT_BG,
};

static const struct CompressedSpriteSheet sPokeGearClockBarSpriteSheet =
{
    .data = gPokeGearMainMenu_ClockBar_Gfx,
    .size = 1024,
    .tag = TAG_CLOCK_BAR,
};

static const struct SpritePalette sPokeGearDigitSpritePalette =
{
    .data = gPokeGearMainMenu_Digits_Pal,
    .tag = TAG_DIGITS,
};

static const struct PokeGearStyle sPokeGearStyleData[] =
{
    [POKEGEAR_STYLE_1] =
    {
        .tileset = gPokeGearStyle1_Gfx,
        .backgroundTilemap = gPokeGearStyle1_Background_Map,
        .foregroundTilemap = gPokeGearStyle1_Foreground_Map,
        .palette = gPokeGearStyle1_Pal,
        .gfxInitFunc = PokeGearStyle1_Init,
        .gfxMainFunc = PokeGearStyle1_Main,
        .gfxExitFunc = PokeGearStyle1_Exit,
    },
};

static void VBlankCB_PokeGear(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    if (sPokeGearStyleData->gfxMainFunc)
        sPokeGearStyleData->gfxMainFunc();
}

static void CB2_PokeGear(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
}

void CB2_InitPokeGear(void)
{
    u8 spriteId;

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

    InitBgsFromTemplates(0, sPokeGearBgTemplates, ARRAY_COUNT(sPokeGearBgTemplates));
    InitWindows(sPokeGearWindowTemplates);
    sPokeGearResources = AllocZeroed(sizeof(struct PokeGearResources));
    sPokeGearResources->tilemapBuffers[0] = AllocZeroed(BG_SCREEN_SIZE);
    sPokeGearResources->tilemapBuffers[1] = AllocZeroed(BG_SCREEN_SIZE * 2);
    sPokeGearResources->tilemapBuffers[2] = AllocZeroed(BG_SCREEN_SIZE * 2);
    sPokeGearResources->tilemapBuffers[3] = AllocZeroed(BG_SCREEN_SIZE);
    SetBgTilemapBuffer(0, sPokeGearResources->tilemapBuffers[0]);
    SetBgTilemapBuffer(1, sPokeGearResources->tilemapBuffers[1]);
    SetBgTilemapBuffer(2, sPokeGearResources->tilemapBuffers[2]);
    SetBgTilemapBuffer(3, sPokeGearResources->tilemapBuffers[3]);

    LoadPokeGearStyle(gSaveBlock2Ptr->pokeGearStyle);
    LZ77UnCompVram(gPokeGearHeader_Gfx, (u16 *)BG_CHAR_ADDR(0));
    LZ77UnCompWram(gPokeGearHeader_Map, sPokeGearResources->tilemapBuffers[0]);
    LZ77UnCompVram(gPokeGearMainMenu_Gfx, (u16 *)BG_CHAR_ADDR(2));
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_TGT1_OBJ | BLDCNT_TGT2_BG3 | BLDCNT_EFFECT_BLEND);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 8));

    sPokeGearResources->numOptions = 2;
    for (u32 i = OPTION_REGION_MAP; i < OPTION_EXIT; i++)
    {
        AppendToList(sPokeGearResources->actions, &sPokeGearResources->numOptions, i);
    }

    CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_OptionEnd_Map, 29 - (5 * (sPokeGearResources->numOptions - 2)) - 1, 11, 2, 2);
    for (u32 i = 0; i < sPokeGearResources->numOptions - 2; i++)
    {
        CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_OptionBar_Map, 25 - (5 * i) + 4, 11, 1, 2);
        CopyToBgTilemapBufferRect(2, gPokeGearMainMenu_Option_Map, 25 - (5 * i), 9, 4, 5);
    }

    LoadSpritePalette(&sPokeGearDigitSpritePalette);
    LoadCompressedSpriteSheet(&sPokeGearDigitSpriteSheet);
    LoadCompressedSpriteSheet(&sPokeGearClockBarSpriteSheet);
    LoadCompressedSpriteSheet(&sPokeGearExitBgSpriteSheet);
    LoadCompressedSpriteSheet(&sPokeGearExitSpriteSheet);

    sPokeGearResources->exitBgSpriteId = CreateSprite(&sPokeGearExitBgSpriteTemplate, 224, 48, 1);
    sPokeGearResources->exitSpriteId = CreateSprite(&sPokeGearExitSpriteTemplate, 228, 44, 0);
    sPokeGearResources->clockSpriteId = CreateSprite(&sPokeGearClockBarSpriteTemplate, 64, 48, 2);
    SetSubspriteTables(&gSprites[sPokeGearResources->clockSpriteId], sPokeGearClockBarSubspriteTable);

    for (u32 i = 0; i < 6; i++)
    {
        spriteId = CreateSprite(&sPokeGearDigitsSpriteTemplate, sPokeGearClockX[i], 44, 0);
        gSprites[spriteId].tPosition = i;
        gSprites[spriteId].tState = STATE_INIT;
        gSprites[spriteId].callback = SpriteCB_ClockDigits;
        sPokeGearResources->digitSpriteIds[i] = spriteId;
    }

    CopyBgTilemapBufferToVram(0);
    CopyBgTilemapBufferToVram(1);
    CopyBgTilemapBufferToVram(2);
    CopyBgTilemapBufferToVram(3);
    
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    EnableInterrupts(INTR_FLAG_VBLANK);
    SetVBlankCallback(VBlankCB_PokeGear);
    SetMainCallback2(CB2_PokeGear);
    CreateTask(Task_PokeGear_1, 1);
    sPokeGearResources->bgOffset.x = OPTION_SCROLL_X;
    sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    PlaySE(SE_POKENAV_ON);
}

static void Task_PokeGear_1(u8 taskId)
{
    if (SlideMainMenuIn())
        gTasks[taskId].func = Task_PokeGear_2;
}

static void Task_PokeGear_2(u8 taskId)
{
    struct Sprite *digit;
    RtcCalcLocalTime();

    if (JOY_NEW(B_BUTTON))
    {
        gTasks[taskId].func = Task_ExitPokeGear_1;
        PlaySE(SE_POKENAV_OFF);
    }
    else if (JOY_NEW(START_BUTTON))
    {
        PlaySE(SE_BALL_TRAY_EXIT);
        gSaveBlock2Ptr->twentyFourHourClock ^= 1;
        for (u32 i = 0; i < ARRAY_COUNT(sPokeGearResources->digitSpriteIds); i++)
        {
            digit = &gSprites[sPokeGearResources->digitSpriteIds[i]];
            digit->tState = STATE_INIT;
        }
    }

    if (++sPokeGearResources->fakeSeconds >= 60)
    {
        sPokeGearResources->fakeSeconds = 0;
    }
}

static void Task_ExitPokeGear_1(u8 taskId)
{
    BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
    gTasks[taskId].func = Task_ExitPokeGear_2;
}

static void Task_ExitPokeGear_2(u8 taskId)
{
    if (SlideMainMenuOut())
        gTasks[taskId].func = Task_ExitPokeGear_3;
}

static void Task_ExitPokeGear_3(u8 taskId)
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

// PokeGear Style Functions
static void LoadPokeGearStyle(u8 style)
{
    LZ77UnCompVram(sPokeGearStyleData[style].tileset, (u16 *)BG_CHAR_ADDR(1));
    LZ77UnCompWram(sPokeGearStyleData[style].foregroundTilemap, sPokeGearResources->tilemapBuffers[1]);
    LZ77UnCompWram(sPokeGearStyleData[style].backgroundTilemap, sPokeGearResources->tilemapBuffers[3]);
    LoadPalette(sPokeGearStyleData[style].palette, 0, 32);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);

    if (sPokeGearStyleData[style].gfxInitFunc)
        sPokeGearStyleData[style].gfxInitFunc();
}

static void PokeGearStyle1_Init(void)
{
    const struct SpritePalette palette = {gPokeGearStyle1_PokeBall_Pal, TAG_POKEBALL};

    LoadSpritePalette(&palette);
    LoadCompressedSpriteSheet(&sPokeGearPokeBallSpriteSheet);
    sPokeGearResources->spriteId = CreateSprite(&sPokeGearPokeBallSpriteTemplate, 224, 144, 0);
}

static void PokeGearStyle1_Main(void)
{
    ChangeBgX(3, Q_8_8(0.25), BG_COORD_SUB);
    ChangeBgY(3, Q_8_8(0.25), BG_COORD_SUB);
}

static void PokeGearStyle1_Exit(void)
{
    DestroySpriteAndFreeResources(&gSprites[sPokeGearResources->spriteId]);
}

static void SpriteCB_PokeBall(struct Sprite *sprite)
{
    sprite->y2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ClockDigits(struct Sprite *sprite)
{
    bool8 clock = gSaveBlock2Ptr->twentyFourHourClock;
    u8 value = sprite->tState;

    if (sprite->tState == STATE_INIT || sPokeGearResources->fakeSeconds == 0)
    {
        switch (sprite->tPosition)
        {
            case 0:
                value = gLocalTime.hours;
                if (!clock)
                {
                    if (value > 12)
                        value -= 12;
                    else if (value == 0)
                        value = 12;
                }
                value = value / 10;
                if (value != 0)
                    value += 1;
                break;
            case 1:
                value = gLocalTime.hours;
                if (!clock)
                {
                    if (value > 12)
                        value -= 12;
                    else if (value == 0)
                        value = 12;
                }
                value = value % 10 + 1;
                break;
            case 2:
                // handled outside of switch
                break;
            case 3:
                value = gLocalTime.minutes / 10 + 1;
                break;
            case 4:
                value = gLocalTime.minutes % 10 + 1;
                break;
            case 5:
                if (clock)
                    value = 13;
                else if (gLocalTime.hours < 12)
                    value = 14;
                else
                    value = 15;
                break;
            default:
                value = 0;
                break;
        }
    }

    if (sprite->tPosition == 2)
    {
        if (sPokeGearResources->fakeSeconds < 30)
            value = 12;
        else
            value = 11;
    }

    if (sprite->tState != value)
    {
        sprite->tState = value;
        StartSpriteAnim(sprite, value);
    }

    sprite->x2 = 512 - sPokeGearResources->bgOffset.x;
}

static void SpriteCB_Exit(struct Sprite *sprite)
{
    if (JOY_NEW(B_BUTTON))
        StartSpriteAnimIfDifferent(sprite, 1);

    if (sprite->animEnded)
        StartSpriteAnimIfDifferent(sprite, 0);

    sprite->x2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ExitBg(struct Sprite *sprite)
{
    sprite->x2 = sPokeGearResources->bgOffset.y;
}

static void SpriteCB_ClockBar(struct Sprite *sprite)
{
    sprite->x2 = 512 - sPokeGearResources->bgOffset.x;
}

// Animation
static bool32 SlideMainMenuIn(void)
{
    for (u32 i = 0; i < 4; i++)
    {
        if (sPokeGearResources->bgOffset.x > 1)
        {
            sPokeGearResources->bgOffset.x -= OPTION_SCROLL_SPEED;
            SetGpuReg(REG_OFFSET_BG2HOFS, 512 - sPokeGearResources->bgOffset.x);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BG2HOFS, 0);
            sPokeGearResources->bgOffset.x = 0;
            return TRUE;
        }
    }

    if (sPokeGearResources->bgOffset.y > 1)
    {
        SetGpuReg(REG_OFFSET_BG0VOFS, sPokeGearResources->bgOffset.y);
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokeGearResources->bgOffset.y);
        sPokeGearResources->bgOffset.y -= OPTION_SCROLL_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        sPokeGearResources->bgOffset.y = 0;
    }

    return FALSE;
}

static bool32 SlideMainMenuOut(void)
{
    for (u32 i = 0; i < 4; i++)
    {
        if (sPokeGearResources->bgOffset.x < OPTION_SCROLL_X)
        {
            sPokeGearResources->bgOffset.x += OPTION_SCROLL_SPEED;
            SetGpuReg(REG_OFFSET_BG2HOFS, 512 - sPokeGearResources->bgOffset.x);
        }
        else
        {
            SetGpuReg(REG_OFFSET_BG2HOFS, OPTION_SCROLL_X);
            sPokeGearResources->bgOffset.x = 0;
            return TRUE;
        }
    }

    if (sPokeGearResources->bgOffset.y < OPTION_SCROLL_Y)
    {
        SetGpuReg(REG_OFFSET_BG0VOFS, sPokeGearResources->bgOffset.y);
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - sPokeGearResources->bgOffset.y);
        sPokeGearResources->bgOffset.y += OPTION_SCROLL_SPEED;
    }
    else
    {
        SetGpuReg(REG_OFFSET_BG0VOFS, OPTION_SCROLL_Y);
        SetGpuReg(REG_OFFSET_BG1VOFS, 512 - OPTION_SCROLL_Y);
        sPokeGearResources->bgOffset.y = OPTION_SCROLL_Y;
        return TRUE;
    }

    return FALSE;
}
