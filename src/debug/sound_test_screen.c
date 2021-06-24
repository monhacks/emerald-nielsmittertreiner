#if DEBUG
#include "global.h"
#include "debug/sound_test_screen.h"
#include "bg.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "m4a.h"
#include "main.h"
#include "menu.h"
#include "palette.h"
#include "text.h"
#include "text_window.h"
#include "title_screen.h"
#include "window.h"
#include "scanline_effect.h"
#include "sound.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define NUM_EMERALD_BGM 133
#define NUM_EMERALD_SE 269

#define NUM_VANADIUM_BGM 4

#define tSelectedWindow data[0]

#define tBgmIndex       data[1]
#define tSeIndex        data[2]

#define tBgmScroll      data[3]
#define tSeScroll       data[4]
#define tScrollExcess   data[5]

#define tSelectedBgm    data[6]
#define tSelectedSe     data[7]

#define tMode           data[15]

static void MainCB2_SoundTestScreen(void);
static void VBlankCB_SoundTestScreen(void);
static bool8 CB2_SetupSoundTestScreen(void);
static void InitSoundTestScreenWindows(void);
static void Task_DrawSoundTestScreenWindows(u8 taskId);
static void Task_HandleSoundTestScreenInput(u8 taskId);
static bool8 Task_CheckSoundTestScreenInput(u8 taskId);
static void Task_HandleDrawSoundTestScreenInfo(u8 taskId);
static void HighlightSelectedWindow(s16 selectedWindow);
static bool8 IsBGMWindow(s16 selectedWindow);
static bool8 IsEmeraldMode(s16 mode);
static void PrintSongName(s16 selectedWindow, const u8 *const string);

static EWRAM_DATA u8 sSoundTestHeaderWindowId = 0;

static const u8 gText_SoundTest[] = _("SOUND TEST:");
static const u8 gText_SelectMode[] = _("SELECT: MODE");
static const u8 gText_ABPlayStop[] = _("A/B: PLAY/STOP");
static const u8 gText_BGM[] = _("BGM");
static const u8 gText_SE[] = _("SE");
static const u8 gText_Emerald[] = _("EMERALD");
static const u8 gText_Vanadium[] = _("VANADIUM");

static u8 gText_BGMScroll[2];
static u8 gText_SEScroll[2];

static const struct BgTemplate sSoundTestBgTemplates[2] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sSoundTestScreenWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 9,
        .width = 24,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0x6B,
    },
    {
        .bg = 0,
        .tilemapLeft = 3,
        .tilemapTop = 15,
        .width = 24,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 0xB3,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct WindowTemplate sSoundTestScreenHeaderWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 3,
    .tilemapTop = 2,
    .width = 24,
    .height = 4,
    .paletteNum = 15,
    .baseBlock = 0x0B,
};

static const u16 sSoundTestScreenBg_Pal[] = {RGB(5, 10, 14)};
static const u8 *const gBGMEmeraldNames[];
static const u8 *const gBGMVanadiumNames[];
static const u8 *const gSENames[];

static void MainCB2_SoundTestScreen(void)
{
    RunTasks();
    UpdatePaletteFade();
}

static void VBlankCB_SoundTestScreen(void)
{
    TransferPlttBuffer();
}

void CB2_InitSoundTestScreen(void)
{
    if (CB2_SetupSoundTestScreen())
    {
        CreateTask(Task_DrawSoundTestScreenWindows, 0);
    }
}

static bool8 CB2_SetupSoundTestScreen(void)
{
    switch(gMain.state)
    {
    case 0:
    default:
        SetVBlankCallback(NULL);
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        SetGpuReg(REG_OFFSET_BG2CNT, 0);
        SetGpuReg(REG_OFFSET_BG1CNT, 0);
        SetGpuReg(REG_OFFSET_BG0CNT, 0);
        SetGpuReg(REG_OFFSET_BG2HOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        SetGpuReg(REG_OFFSET_BG1HOFS, 0);
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
        DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
        DmaFill16(3, 0, (void *)(PLTT + 2), PLTT_SIZE - 2);
        ResetPaletteFade();
        ResetTasks();
        ResetSpriteData();
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sSoundTestBgTemplates, ARRAY_COUNT(sSoundTestBgTemplates));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        ShowBg(0);
        ShowBg(3);
        LoadPalette(sSoundTestScreenBg_Pal, 0, sizeof(sSoundTestScreenBg_Pal));
        InitSoundTestScreenWindows();
        BeginNormalPaletteFade(PALETTES_BG, 0, 0x10, 0, RGB_WHITEALPHA);
        EnableInterrupts(INTR_FLAG_VBLANK);
        SetVBlankCallback(VBlankCB_SoundTestScreen);
        gMain.state = 1;
        break;
    case 1:
        UpdatePaletteFade();
        if(!gPaletteFade.active)
        {
            SetMainCallback2(MainCB2_SoundTestScreen);
            return TRUE;
        }
    }
    return FALSE;
}

static void InitSoundTestScreenWindows(void)
{
    InitWindows(sSoundTestScreenWindowTemplates);
    sSoundTestHeaderWindowId = AddWindow(&sSoundTestScreenHeaderWindowTemplate);
    DrawStdWindowFrame(sSoundTestHeaderWindowId, FALSE);
    FillWindowPixelBuffer(sSoundTestHeaderWindowId, PIXEL_FILL(1));
    PutWindowTilemap(sSoundTestHeaderWindowId);
    CopyWindowToVram(sSoundTestHeaderWindowId, 3);
    DeactivateAllTextPrinters();
    LoadWindowGfx(0, 0, 2, 224);
    LoadPalette(gUnknown_0860F074, 0xF0, 0x20);
}

static void Task_DrawSoundTestScreenWindows(u8 taskId)
{
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG0 | WININ_WIN0_OBJ | WININ_WIN1_BG0 | WININ_WIN1_OBJ);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_DARKEN | BLDCNT_TGT1_BG0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 7);
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(16, 223));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(8, 55));
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(16, 223));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(56, 103));

    DrawStdFrameWithCustomTileAndPalette(sSoundTestHeaderWindowId, TRUE, 2, 14);
    AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_SoundTest, 1, 1, 0, 0);
    AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_Emerald, 64, 1, 0, 0);
    AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_SelectMode, 1, 16, 0, 0);
    AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_ABPlayStop, 108, 16, 0, 0);

    DrawStdFrameWithCustomTileAndPalette(0, TRUE, 2, 14);
    ConvertIntToDecimalStringN(gText_BGMScroll, 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(0, 0, gText_BGMScroll, 1, 12, 0, 0);
    AddTextPrinterParameterized(0, 1, gText_BGM, 1, 0, 0, 0);

    DrawStdFrameWithCustomTileAndPalette(1, TRUE, 2, 14);
    ConvertIntToDecimalStringN(gText_SEScroll, 1, STR_CONV_MODE_LEFT_ALIGN, 2);
    AddTextPrinterParameterized(1, 0, gText_SEScroll, 1, 12, 0, 0);
    AddTextPrinterParameterized(1, 1, gText_SE, 1, 0, 0, 0);

    PrintSongName(0, gBGMEmeraldNames[gTasks[taskId].tBgmIndex]);
    PrintSongName(1, gSENames[gTasks[taskId].tSeIndex]);

    m4aSoundInit();
    gTasks[taskId].func = Task_HandleSoundTestScreenInput;
    gTasks[taskId].tSelectedWindow = 0;
    gTasks[taskId].tBgmIndex = 0;
    gTasks[taskId].tSeIndex = 1;
    gTasks[taskId].tBgmScroll = 1;
    gTasks[taskId].tSeScroll = 1;
}

static void Task_HandleSoundTestScreenInput(u8 taskId)
{
    if (Task_CheckSoundTestScreenInput(taskId))
    {
        gTasks[taskId].func = Task_HandleDrawSoundTestScreenInfo;
    }
}

static void Task_HandleDrawSoundTestScreenInfo(u8 taskId)
{
    HighlightSelectedWindow(gTasks[taskId].tSelectedWindow);
    FillWindowPixelRect(sSoundTestHeaderWindowId, PIXEL_FILL(1), 64, 1, 120, 16);
    if (IsEmeraldMode(gTasks[taskId].tMode))
    {
        AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_Emerald, 64, 1, 0, 0);
        PrintSongName(0, gBGMEmeraldNames[gTasks[taskId].tBgmIndex]);
    }
    else
    {
        AddTextPrinterParameterized(sSoundTestHeaderWindowId, 1, gText_Vanadium, 64, 1, 0, 0);
        PrintSongName(0, gBGMVanadiumNames[gTasks[taskId].tBgmIndex]);
    }

    FillWindowPixelRect(gTasks[taskId].tSelectedWindow, PIXEL_FILL(1), 1, 12, 16, 12);
    if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
    {
        ConvertIntToDecimalStringN(gText_BGMScroll, gTasks[taskId].tBgmScroll, STR_CONV_MODE_LEFT_ALIGN, 2);
        AddTextPrinterParameterized(0, 0, gText_BGMScroll, 1, 12, 0, 0);
        AddTextPrinterParameterized(0, 1, gText_BGM, 1, 0, 0, 0);
    }
    else
    {
        ConvertIntToDecimalStringN(gText_SEScroll, gTasks[taskId].tSeScroll, STR_CONV_MODE_LEFT_ALIGN, 2);
        AddTextPrinterParameterized(1, 0, gText_SEScroll, 1, 12, 0, 0);
        AddTextPrinterParameterized(1, 1, gText_SE, 1, 0, 0, 0);
    }

    PrintSongName(1, gSENames[gTasks[taskId].tSeIndex - 1]);
    gTasks[taskId].func = Task_HandleSoundTestScreenInput;
}

static bool8 Task_CheckSoundTestScreenInput(u8 taskId)
{
    if (JOY_NEW(START_BUTTON))
    {
        DoSoftReset();
    }
    else if ((JOY_NEW(SELECT_BUTTON)))
    {
        m4aMPlayAllStop();
        gTasks[taskId].tMode ^= 1;
        gTasks[taskId].tBgmIndex = 0;
        return TRUE;
    }
    else if ((JOY_REPEAT(L_BUTTON)))
    {
        if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
        {
            if (gTasks[taskId].tBgmScroll > 1)
            {
                gTasks[taskId].tBgmScroll--;
            }
            else
            {
                gTasks[taskId].tBgmScroll = 1;
            }
        }
        else
        {
            if (gTasks[taskId].tSeScroll > 1)
            {
                gTasks[taskId].tSeScroll--;
            }
            else
            {
                gTasks[taskId].tSeScroll = 1;
            }
        }
        return TRUE;
    }
    else if ((JOY_REPEAT(R_BUTTON)))
    {
        if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
        {
            if (gTasks[taskId].tBgmScroll < 16)
            {
                gTasks[taskId].tBgmScroll++;
            }
            else
            {
                gTasks[taskId].tBgmScroll = 16;
            }
        }
        else
        {
            if (gTasks[taskId].tSeScroll < 16)
            {
                gTasks[taskId].tSeScroll++;
            }
            else
            {
                gTasks[taskId].tSeScroll = 16;
            }
        }
        return TRUE;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
        {
            if (IsEmeraldMode(gTasks[taskId].tMode))
            {
                if (gTasks[taskId].tBgmIndex >= 0)
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedBgm + 350);
                    m4aSongNumStart(gTasks[taskId].tBgmIndex + 350);
                    gTasks[taskId].tSelectedBgm = gTasks[taskId].tBgmIndex;
                }
                else
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedBgm + 350);
                    gTasks[taskId].tSelectedBgm = 0;
                }
            }
            else
            {
                if (gTasks[taskId].tBgmIndex >= 0)
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedBgm + 610);
                    m4aSongNumStart(gTasks[taskId].tBgmIndex + 610);
                    gTasks[taskId].tSelectedBgm = gTasks[taskId].tBgmIndex;
                }
                else
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedBgm + 610);
                    gTasks[taskId].tSelectedBgm = 0;
                }
            }
        }
        else
        {
            if (gTasks[taskId].tSelectedSe != 0)
            {
                if (gTasks[taskId].tSeIndex != 0)
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedSe);
                    m4aSongNumStart(gTasks[taskId].tSeIndex);
                    gTasks[taskId].tSelectedSe = gTasks[taskId].tSeIndex;
                }
                else
                {
                    m4aSongNumStop(gTasks[taskId].tSelectedSe);
                    gTasks[taskId].tSelectedSe = 0;
                }
            }
            else if (gTasks[taskId].tSeIndex != 0)
            {
                m4aSongNumStart(gTasks[taskId].tSeIndex);
                gTasks[taskId].tSelectedSe = gTasks[taskId].tSeIndex;
            }
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        m4aMPlayAllStop();
    }
    else if ((JOY_NEW(DPAD_UP)))
    {
        gTasks[taskId].tSelectedWindow ^= 1;
        return TRUE;
    }
    else if ((JOY_NEW(DPAD_DOWN)))
    {
        gTasks[taskId].tSelectedWindow ^= 1;
        return TRUE;
    }
    else if ((JOY_REPEAT(DPAD_LEFT)))
    {
        if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
        {
            gTasks[taskId].tScrollExcess = gTasks[taskId].tBgmScroll - gTasks[taskId].tBgmIndex;

            if (IsEmeraldMode(gTasks[taskId].tMode))
            {
                if (gTasks[taskId].tBgmIndex == 0)
                {
                    gTasks[taskId].tBgmIndex = NUM_EMERALD_BGM - gTasks[taskId].tBgmScroll + 1;
                }
                else if (gTasks[taskId].tBgmScroll > gTasks[taskId].tBgmIndex)
                {
                    gTasks[taskId].tBgmIndex = NUM_EMERALD_BGM - gTasks[taskId].tScrollExcess + 1;
                }
                else
                {
                    gTasks[taskId].tBgmIndex -= gTasks[taskId].tBgmScroll;
                }
            }
            else
            {
                if (gTasks[taskId].tBgmIndex == 0)
                {
                    gTasks[taskId].tBgmIndex = NUM_VANADIUM_BGM - gTasks[taskId].tBgmScroll + 1;
                }
                else if (gTasks[taskId].tBgmScroll > gTasks[taskId].tBgmIndex)
                {
                    gTasks[taskId].tBgmIndex = NUM_VANADIUM_BGM - gTasks[taskId].tScrollExcess + 1;
                }
                else
                {
                    gTasks[taskId].tBgmIndex -= gTasks[taskId].tBgmScroll;
                }
            }
        }
        else
        {
            gTasks[taskId].tScrollExcess = gTasks[taskId].tSeScroll - gTasks[taskId].tSeIndex;

            if (gTasks[taskId].tSeIndex == 1)
            {
                gTasks[taskId].tSeIndex = NUM_EMERALD_SE - gTasks[taskId].tSeScroll + 1;
            }
            else if (gTasks[taskId].tSeScroll > gTasks[taskId].tSeIndex)
            {
                gTasks[taskId].tSeIndex = NUM_EMERALD_SE - gTasks[taskId].tScrollExcess + 1;
            }
            else
            {
                gTasks[taskId].tSeIndex -= gTasks[taskId].tSeScroll;
            }

        }
        return TRUE;
    }
    else if ((JOY_REPEAT(DPAD_RIGHT)))
    {
        if (IsBGMWindow(gTasks[taskId].tSelectedWindow))
        {
            if (IsEmeraldMode(gTasks[taskId].tMode))
            {
                if (gTasks[taskId].tBgmIndex + gTasks[taskId].tBgmScroll > NUM_EMERALD_BGM)
                {
                    gTasks[taskId].tScrollExcess = gTasks[taskId].tBgmIndex + gTasks[taskId].tBgmScroll - NUM_EMERALD_BGM - 1;
                    gTasks[taskId].tBgmIndex = gTasks[taskId].tScrollExcess;
                }
                else
                {
                    gTasks[taskId].tBgmIndex += gTasks[taskId].tBgmScroll;
                }
            }
            else
            {
                if (gTasks[taskId].tBgmIndex + gTasks[taskId].tBgmScroll > NUM_VANADIUM_BGM)
                {
                    gTasks[taskId].tScrollExcess = gTasks[taskId].tBgmIndex + gTasks[taskId].tBgmScroll - NUM_VANADIUM_BGM - 1;
                    gTasks[taskId].tBgmIndex = gTasks[taskId].tScrollExcess;
                }
                else
                {
                    gTasks[taskId].tBgmIndex += gTasks[taskId].tBgmScroll;
                }
            }
        }
        else
        {
            if (gTasks[taskId].tSeIndex + gTasks[taskId].tSeScroll > NUM_EMERALD_SE)
            {
                gTasks[taskId].tScrollExcess = gTasks[taskId].tSeIndex + gTasks[taskId].tSeScroll - NUM_EMERALD_SE;
                gTasks[taskId].tSeIndex = gTasks[taskId].tScrollExcess;
            }
            else
            {
                gTasks[taskId].tSeIndex += gTasks[taskId].tSeScroll;
            }

        }
        return TRUE;
    }
    return FALSE;
}

static void HighlightSelectedWindow(s16 selectedWindow)
{
    switch (selectedWindow)
    {
    case 0:
        SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(16, 223));
        SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(56, 103));
        break;
    case 1:
        SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(16, 223));
        SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(104, 151));
        break;
    }
}

static bool8 IsBGMWindow(s16 selectedWindow)
{
    if (selectedWindow == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static bool8 IsEmeraldMode(s16 mode)
{
    if (mode == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static void PrintSongName(s16 selectedWindow, const u8 *const string)
{
    FillWindowPixelRect(selectedWindow, PIXEL_FILL(1), 40, 4, 152, 16);
    AddTextPrinterParameterized(selectedWindow, 1, string, 40, 4, 0, 0);
}

#define SOUND_LIST_VANADIUM_BGM \
    X(MUS_ACREN_FOREST, "MUS-ACREN-FOREST") \
    X(MUS_ROUTE403, "MUS-ROUTE403") \
    X(MUS_SUNSET, "MUS-SUNSET") \
    X(MUS_CELEBI, "MUS-CELEBI") \
    X(MUS_MURENA, "MUS-MURENA") \

#define SOUND_LIST_EMERALD_BGM \
    X(MUS_LITTLEROOT_TEST, "MUS-LITTLEROOT-TEST") \
    X(MUS_GSC_ROUTE38, "MUS-GSC-ROUTE38") \
    X(MUS_CAUGHT, "MUS-CAUGHT") \
    X(MUS_VICTORY_WILD, "MUS-VICTORY-WILD") \
    X(MUS_VICTORY_GYM_LEADER, "MUS-VICTORY-GYM-LEADER") \
    X(MUS_VICTORY_LEAGUE, "MUS-VICTORY-LEAGUE") \
    X(MUS_C_COMM_CENTER, "MUS-C-COMM-CENTER") \
    X(MUS_GSC_PEWTER, "MUS-GSC-PEWTER") \
    X(MUS_C_VS_LEGEND_BEAST, "MUS-C-VS-LEGEND-BEAST") \
    X(MUS_ROUTE101, "MUS-ROUTE101") \
    X(MUS_ROUTE110, "MUS-ROUTE110") \
    X(MUS_ROUTE120, "MUS-ROUTE120") \
    X(MUS_PETALBURG, "MUS-PETALBURG") \
    X(MUS_OLDALE, "MUS-OLDALE") \
    X(MUS_GYM, "MUS-GYM") \
    X(MUS_SURF, "MUS-SURF") \
    X(MUS_PETALBURG_WOODS, "MUS-PETALBURG-WOODS") \
    X(MUS_LEVEL_UP, "MUS-LEVEL-UP") \
    X(MUS_HEAL, "MUS-HEAL") \
    X(MUS_OBTAIN_BADGE, "MUS-OBTAIN-BADGE") \
    X(MUS_OBTAIN_ITEM, "MUS-OBTAIN-ITEM") \
    X(MUS_EVOLVED, "MUS-EVOLVED") \
    X(MUS_OBTAIN_TMHM, "MUS-OBTAIN-TMHM") \
    X(MUS_LILYCOVE_MUSEUM, "MUS-LILYCOVE-MUSEUM") \
    X(MUS_ROUTE122, "MUS-ROUTE122") \
    X(MUS_OCEANIC_MUSEUM, "MUS-OCEANIC-MUSEUM") \
    X(MUS_EVOLUTION_INTRO, "MUS-EVOLUTION-INTRO") \
    X(MUS_EVOLUTION, "MUS-EVOLUTION") \
    X(MUS_MOVE_DELETED, "MUS-MOVE-DELETED") \
    X(MUS_ENCOUNTER_GIRL, "MUS-ENCOUNTER-GIRL") \
    X(MUS_ENCOUNTER_MALE, "MUS-ENCOUNTER-MALE") \
    X(MUS_ABANDONED_SHIP, "MUS-ABANDONED-SHIP") \
    X(MUS_FORTREE, "MUS-FORTREE") \
    X(MUS_BIRCH_LAB, "MUS-BIRCH-LAB") \
    X(MUS_B_TOWER_RS, "MUS-B-TOWER-RS") \
    X(MUS_ENCOUNTER_SWIMMER, "MUS-ENCOUNTER-SWIMMER") \
    X(MUS_CAVE_OF_ORIGIN, "MUS-CAVE-OF-ORIGIN") \
    X(MUS_OBTAIN_BERRY, "MUS-OBTAIN-BERRY") \
    X(MUS_AWAKEN_LEGEND, "MUS-AWAKEN-LEGEND") \
    X(MUS_SLOTS_JACKPOT, "MUS-SLOTS-JACKPOT") \
    X(MUS_SLOTS_WIN, "MUS-SLOTS-WIN") \
    X(MUS_TOO_BAD, "MUS-TOO-BAD") \
    X(MUS_ROULETTE, "MUS-ROULETTE") \
    X(MUS_LINK_CONTEST_P1, "MUS-LINK-CONTEST-P1") \
    X(MUS_LINK_CONTEST_P2, "MUS-LINK-CONTEST-P2") \
    X(MUS_LINK_CONTEST_P3, "MUS-LINK-CONTEST-P3") \
    X(MUS_LINK_CONTEST_P4, "MUS-LINK-CONTEST-P4") \
    X(MUS_ENCOUNTER_RICH, "MUS-ENCOUNTER-RICH") \
    X(MUS_VERDANTURF, "MUS-VERDANTURF") \
    X(MUS_RUSTBORO, "MUS-RUSTBORO") \
    X(MUS_POKE_CENTER, "MUS-POKE-CENTER") \
    X(MUS_ROUTE104, "MUS-ROUTE104") \
    X(MUS_ROUTE119, "MUS-ROUTE119") \
    X(MUS_CYCLING, "MUS-CYCLING") \
    X(MUS_POKE_MART, "MUS-POKE-MART") \
    X(MUS_LITTLEROOT, "MUS-LITTLEROOT") \
    X(MUS_MT_CHIMNEY, "MUS-MT-CHIMNEY") \
    X(MUS_ENCOUNTER_FEMALE, "MUS-ENCOUNTER-FEMALE") \
    X(MUS_LILYCOVE, "MUS-LILYCOVE") \
    X(MUS_ROUTE111, "MUS-ROUTE111") \
    X(MUS_HELP, "MUS-HELP") \
    X(MUS_UNDERWATER, "MUS-UNDERWATER") \
    X(MUS_VICTORY_TRAINER, "MUS-VICTORY-TRAINER") \
    X(MUS_TITLE, "MUS-TITLE") \
    X(MUS_INTRO, "MUS-INTRO") \
    X(MUS_ENCOUNTER_MAY, "MUS-ENCOUNTER-MAY") \
    X(MUS_ENCOUNTER_INTENSE, "MUS-ENCOUNTER-INTENSE") \
    X(MUS_ENCOUNTER_COOL, "MUS-ENCOUNTER-COOL") \
    X(MUS_ROUTE113, "MUS-ROUTE113") \
    X(MUS_ENCOUNTER_AQUA, "MUS-ENCOUNTER-AQUA") \
    X(MUS_FOLLOW_ME, "MUS-FOLLOW-ME") \
    X(MUS_ENCOUNTER_BRENDAN, "MUS-ENCOUNTER-BRENDAN") \
    X(MUS_EVER_GRANDE, "MUS-EVER-GRANDE") \
    X(MUS_ENCOUNTER_SUSPICIOUS, "MUS-ENCOUNTER-SUSPICIOUS") \
    X(MUS_VICTORY_AQUA_MAGMA, "MUS-VICTORY-AQUA-MAGMA") \
    X(MUS_CABLE_CAR, "MUS-CABLE-CAR") \
    X(MUS_GAME_CORNER, "MUS-GAME-CORNER") \
    X(MUS_DEWFORD, "MUS-DEWFORD") \
    X(MUS_SAFARI_ZONE, "MUS-SAFARI-ZONE") \
    X(MUS_VICTORY_ROAD, "MUS-VICTORY-ROAD") \
    X(MUS_AQUA_MAGMA_HIDEOUT, "MUS-AQUA-MAGMA-HIDEOUT") \
    X(MUS_SAILING, "MUS-SAILING") \
    X(MUS_MT_PYRE, "MUS-MT-PYRE") \
    X(MUS_SLATEPORT, "MUS-SLATEPORT") \
    X(MUS_MT_PYRE_EXTERIOR, "MUS-MT-PYRE-EXTERIOR") \
    X(MUS_SCHOOL, "MUS-SCHOOL") \
    X(MUS_HALL_OF_FAME, "MUS-HALL-OF-FAME") \
    X(MUS_FALLARBOR, "MUS-FALLARBOR") \
    X(MUS_SEALED_CHAMBER, "MUS-SEALED-CHAMBER") \
    X(MUS_CONTEST_WINNER, "MUS-CONTEST-WINNER") \
    X(MUS_CONTEST, "MUS-CONTEST") \
    X(MUS_ENCOUNTER_MAGMA, "MUS-ENCOUNTER-MAGMA") \
    X(MUS_INTRO_BATTLE, "MUS-INTRO-BATTLE") \
    X(MUS_WEATHER_KYOGRE, "MUS-WEATHER-KYOGRE") \
    X(MUS_WEATHER_GROUDON, "MUS-WEATHER-GROUDON") \
    X(MUS_SOOTOPOLIS, "MUS-SOOTOPOLIS") \
    X(MUS_CONTEST_RESULTS, "MUS-CONTEST-RESULTS") \
    X(MUS_HALL_OF_FAME_ROOM, "MUS-HALL-OF-FAME-ROOM") \
    X(MUS_TRICK_HOUSE, "MUS-TRICK-HOUSE") \
    X(MUS_ENCOUNTER_TWINS, "MUS-ENCOUNTER-TWINS") \
    X(MUS_ENCOUNTER_ELITE_FOUR, "MUS-ENCOUNTER-ELITE-FOUR") \
    X(MUS_ENCOUNTER_HIKER, "MUS-ENCOUNTER-HIKER") \
    X(MUS_CONTEST_LOBBY, "MUS-CONTEST-LOBBY") \
    X(MUS_ENCOUNTER_INTERVIEWER, "MUS-ENCOUNTER-INTERVIEWER") \
    X(MUS_ENCOUNTER_CHAMPION, "MUS-ENCOUNTER-CHAMPION") \
    X(MUS_CREDITS, "MUS-CREDITS") \
    X(MUS_END, "MUS-END") \
    X(MUS_B_FRONTIER, "MUS-B-FRONTIER") \
    X(MUS_B_ARENA, "MUS-B-ARENA") \
    X(MUS_OBTAIN_B_POINTS, "MUS-OBTAIN-B-POINTS") \
    X(MUS_REGISTER_MATCH_CALL, "MUS-REGISTER-MATCH-CALL") \
    X(MUS_B_PYRAMID, "MUS-B-PYRAMID") \
    X(MUS_B_PYRAMID_TOP, "MUS-B-PYRAMID-TOP") \
    X(MUS_B_PALACE, "MUS-B-PALACE") \
    X(MUS_RAYQUAZA_APPEARS, "MUS-RAYQUAZA-APPEARS") \
    X(MUS_B_TOWER, "MUS-B-TOWER") \
    X(MUS_OBTAIN_SYMBOL, "MUS-OBTAIN-SYMBOL") \
    X(MUS_B_DOME, "MUS-B-DOME") \
    X(MUS_B_PIKE, "MUS-B-PIKE") \
    X(MUS_B_FACTORY, "MUS-B-FACTORY") \
    X(MUS_VS_RAYQUAZA, "MUS-VS-RAYQUAZA") \
    X(MUS_VS_FRONTIER_BRAIN, "MUS-VS-FRONTIER-BRAIN") \
    X(MUS_VS_MEW, "MUS-VS-MEW") \
    X(MUS_B_DOME_LOBBY, "MUS-B-DOME-LOBBY") \
    X(MUS_VS_WILD, "MUS-VS-WILD") \
    X(MUS_VS_AQUA_MAGMA, "MUS-VS-AQUA-MAGMA") \
    X(MUS_VS_TRAINER, "MUS-VS-TRAINER") \
    X(MUS_VS_GYM_LEADER, "MUS-VS-GYM-LEADER") \
    X(MUS_VS_CHAMPION, "MUS-VS-CHAMPION") \
    X(MUS_VS_REGI, "MUS-VS-REGI") \
    X(MUS_VS_KYOGRE_GROUDON, "MUS-VS-KYOGRE-GROUDON") \
    X(MUS_VS_RIVAL, "MUS-VS-RIVAL") \
    X(MUS_VS_ELITE_FOUR, "MUS-VS-ELITE-FOUR") \
    X(MUS_VS_AQUA_MAGMA_LEADER, "MUS-VS-AQUA-MAGMA-LEADER") \

#define SOUND_LIST_SE \
    X(SE_USE_ITEM, "SE-USE-ITEM") \
    X(SE_PC_LOGIN, "SE-PC-LOGIN") \
    X(SE_PC_OFF, "SE-PC-OFF") \
    X(SE_PC_ON, "SE-PC-ON") \
    X(SE_SELECT, "SE-SELECT") \
    X(SE_WIN_OPEN, "SE-WIN-OPEN") \
    X(SE_WALL_HIT, "SE-WALL-HIT") \
    X(SE_DOOR, "SE-DOOR") \
    X(SE_EXIT, "SE-EXIT") \
    X(SE_LEDGE, "SE-LEDGE") \
    X(SE_BIKE_BELL, "SE-BIKE-BELL") \
    X(SE_NOT_EFFECTIVE, "SE-NOT-EFFECTIVE") \
    X(SE_EFFECTIVE, "SE-EFFECTIVE") \
    X(SE_SUPER_EFFECTIVE, "SE-SUPER-EFFECTIVE") \
    X(SE_BALL_OPEN, "SE-BALL-OPEN") \
    X(SE_FAINT, "SE-FAINT") \
    X(SE_FLEE, "SE-FLEE") \
    X(SE_SLIDING_DOOR, "SE-SLIDING-DOOR") \
    X(SE_SHIP, "SE-SHIP") \
    X(SE_BANG, "SE-BANG") \
    X(SE_PIN, "SE-PIN") \
    X(SE_BOO, "SE-BOO") \
    X(SE_BALL, "SE-BALL") \
    X(SE_CONTEST_PLACE, "SE-CONTEST-PLACE") \
    X(SE_A, "SE-A") \
    X(SE_I, "SE-I") \
    X(SE_U, "SE-U") \
    X(SE_E, "SE-E") \
    X(SE_O, "SE-O") \
    X(SE_N, "SE-N") \
    X(SE_SUCCESS, "SE-SUCCESS") \
    X(SE_FAILURE, "SE-FAILURE") \
    X(SE_EXP, "SE-EXP") \
    X(SE_BIKE_HOP, "SE-BIKE-HOP") \
    X(SE_SWITCH, "SE-SWITCH") \
    X(SE_CLICK, "SE-CLICK") \
    X(SE_FU_ZAKU, "SE-FU-ZAKU") \
    X(SE_CONTEST_CONDITION_LOSE, "SE-CONTEST-CONDITION-LOSE") \
    X(SE_LAVARIDGE_FALL_WARP, "SE-LAVARIDGE-FALL-WARP") \
    X(SE_ICE_STAIRS, "SE-ICE-STAIRS") \
    X(SE_ICE_BREAK, "SE-ICE-BREAK") \
    X(SE_ICE_CRACK, "SE-ICE-CRACK") \
    X(SE_FALL, "SE-FALL") \
    X(SE_UNLOCK, "SE-UNLOCK") \
    X(SE_WARP_IN, "SE-WARP-IN") \
    X(SE_WARP_OUT, "SE-WARP-OUT") \
    X(SE_REPEL, "SE-REPEL") \
    X(SE_ROTATING_GATE, "SE-ROTATING-GATE") \
    X(SE_TRUCK_MOVE, "SE-TRUCK-MOVE") \
    X(SE_TRUCK_STOP, "SE-TRUCK-STOP") \
    X(SE_TRUCK_UNLOAD, "SE-TRUCK-UNLOAD") \
    X(SE_TRUCK_DOOR, "SE-TRUCK-DOOR") \
    X(SE_BERRY_BLENDER, "SE-BERRY-BLENDER") \
    X(SE_CARD, "SE-CARD") \
    X(SE_SAVE, "SE-SAVE") \
    X(SE_BALL_BOUNCE_1, "SE-BALL-BOUNCE-1") \
    X(SE_BALL_BOUNCE_2, "SE-BALL-BOUNCE-2") \
    X(SE_BALL_BOUNCE_3, "SE-BALL-BOUNCE-3") \
    X(SE_BALL_BOUNCE_4, "SE-BALL-BOUNCE-4") \
    X(SE_BALL_TRADE, "SE-BALL-TRADE") \
    X(SE_BALL_THROW, "SE-BALL-THROW") \
    X(SE_NOTE_C, "SE-NOTE-C") \
    X(SE_NOTE_D, "SE-NOTE-D") \
    X(SE_NOTE_E, "SE-NOTE-E") \
    X(SE_NOTE_F, "SE-NOTE-F") \
    X(SE_NOTE_G, "SE-NOTE-G") \
    X(SE_NOTE_A, "SE-NOTE-A") \
    X(SE_NOTE_B, "SE-NOTE-B") \
    X(SE_NOTE_C_HIGH, "SE-NOTE-C-HIGH") \
    X(SE_PUDDLE, "SE-PUDDLE") \
    X(SE_BRIDGE_WALK, "SE-BRIDGE-WALK") \
    X(SE_ITEMFINDER, "SE-ITEMFINDER") \
    X(SE_DING_DONG, "SE-DING-DONG") \
    X(SE_BALLOON_RED, "SE-BALLOON-RED") \
    X(SE_BALLOON_BLUE, "SE-BALLOON-BLUE") \
    X(SE_BALLOON_YELLOW, "SE-BALLOON-YELLOW") \
    X(SE_BREAKABLE_DOOR, "SE-BREAKABLE-DOOR") \
    X(SE_MUD_BALL, "SE-MUD-BALL") \
    X(SE_FIELD_POISON, "SE-FIELD-POISON") \
    X(SE_ESCALATOR, "SE-ESCALATOR") \
    X(SE_THUNDERSTORM, "SE-THUNDERSTORM") \
    X(SE_THUNDERSTORM_STOP, "SE-THUNDERSTORM-STOP") \
    X(SE_DOWNPOUR, "SE-DOWNPOUR") \
    X(SE_DOWNPOUR_STOP, "SE-DOWNPOUR-STOP") \
    X(SE_RAIN, "SE-RAIN") \
    X(SE_RAIN_STOP, "SE-RAIN-STOP") \
    X(SE_THUNDER, "SE-THUNDER") \
    X(SE_THUNDER2, "SE-THUNDER2") \
    X(SE_ELEVATOR, "SE-ELEVATOR") \
    X(SE_LOW_HEALTH, "SE-LOW-HEALTH") \
    X(SE_EXP_MAX, "SE-EXP-MAX") \
    X(SE_ROULETTE_BALL, "SE-ROULETTE-BALL") \
    X(SE_ROULETTE_BALL2, "SE-ROULETTE-BALL2") \
    X(SE_TAILLOW_WING_FLAP, "SE-TAILLOW-WING-FLAP") \
    X(SE_SHOP, "SE-SHOP") \
    X(SE_CONTEST_HEART, "SE-CONTEST-HEART") \
    X(SE_CONTEST_CURTAIN_RISE, "SE-CONTEST-CURTAIN-RISE") \
    X(SE_CONTEST_CURTAIN_FALL, "SE-CONTEST-CURTAIN-FALL") \
    X(SE_CONTEST_ICON_CHANGE, "SE-CONTEST-ICON-CHANGE") \
    X(SE_CONTEST_ICON_CLEAR, "SE-CONTEST-ICON-CLEAR") \
    X(SE_CONTEST_MONS_TURN, "SE-CONTEST-MONS-TURN") \
    X(SE_SHINY, "SE-SHINY") \
    X(SE_INTRO_BLAST, "SE-INTRO-BLAST") \
    X(SE_MUGSHOT, "SE-MUGSHOT") \
    X(SE_APPLAUSE, "SE-APPLAUSE") \
    X(SE_VEND, "SE-VEND") \
    X(SE_ORB, "SE-ORB") \
    X(SE_DEX_SCROLL, "SE-DEX-SCROLL") \
    X(SE_DEX_PAGE, "SE-DEX-PAGE") \
    X(SE_POKENAV_ON, "SE-POKENAV-ON") \
    X(SE_POKENAV_OFF, "SE-POKENAV-OFF") \
    X(SE_DEX_SEARCH, "SE-DEX-SEARCH") \
    X(SE_EGG_HATCH, "SE-EGG-HATCH") \
    X(SE_BALL_TRAY_ENTER, "SE-BALL-TRAY-ENTER") \
    X(SE_BALL_TRAY_BALL, "SE-BALL-TRAY-BALL") \
    X(SE_BALL_TRAY_EXIT, "SE-BALL-TRAY-EXIT") \
    X(SE_GLASS_FLUTE, "SE-GLASS-FLUTE") \
    X(SE_M_THUNDERBOLT, "SE-M-THUNDERBOLT") \
    X(SE_M_THUNDERBOLT2, "SE-M-THUNDERBOLT2") \
    X(SE_M_HARDEN, "SE-M-HARDEN") \
    X(SE_M_NIGHTMARE, "SE-M-NIGHTMARE") \
    X(SE_M_VITAL_THROW, "SE-M-VITAL-THROW") \
    X(SE_M_VITAL_THROW2, "SE-M-VITAL-THROW2") \
    X(SE_M_BUBBLE, "SE-M-BUBBLE") \
    X(SE_M_BUBBLE2, "SE-M-BUBBLE2") \
    X(SE_M_BUBBLE3, "SE-M-BUBBLE3") \
    X(SE_M_RAIN_DANCE, "SE-M-RAIN-DANCE") \
    X(SE_M_CUT, "SE-M-CUT") \
    X(SE_M_STRING_SHOT, "SE-M-STRING-SHOT") \
    X(SE_M_STRING_SHOT2, "SE-M-STRING-SHOT2") \
    X(SE_M_ROCK_THROW, "SE-M-ROCK-THROW") \
    X(SE_M_GUST, "SE-M-GUST") \
    X(SE_M_GUST2, "SE-M-GUST2") \
    X(SE_M_DOUBLE_SLAP, "SE-M-DOUBLE-SLAP") \
    X(SE_M_DOUBLE_TEAM, "SE-M-DOUBLE-TEAM") \
    X(SE_M_RAZOR_WIND, "SE-M-RAZOR-WIND") \
    X(SE_M_ICY_WIND, "SE-M-ICY-WIND") \
    X(SE_M_THUNDER_WAVE, "SE-M-THUNDER-WAVE") \
    X(SE_M_COMET_PUNCH, "SE-M-COMET-PUNCH") \
    X(SE_M_MEGA_KICK, "SE-M-MEGA-KICK") \
    X(SE_M_MEGA_KICK2, "SE-M-MEGA-KICK2") \
    X(SE_M_CRABHAMMER, "SE-M-CRABHAMMER") \
    X(SE_M_JUMP_KICK, "SE-M-JUMP-KICK") \
    X(SE_M_FLAME_WHEEL, "SE-M-FLAME-WHEEL") \
    X(SE_M_FLAME_WHEEL2, "SE-M-FLAME-WHEEL2") \
    X(SE_M_FLAMETHROWER, "SE-M-FLAMETHROWER") \
    X(SE_M_FIRE_PUNCH, "SE-M-FIRE-PUNCH") \
    X(SE_M_TOXIC, "SE-M-TOXIC") \
    X(SE_M_SACRED_FIRE, "SE-M-SACRED-FIRE") \
    X(SE_M_SACRED_FIRE2, "SE-M-SACRED-FIRE2") \
    X(SE_M_EMBER, "SE-M-EMBER") \
    X(SE_M_TAKE_DOWN, "SE-M-TAKE-DOWN") \
    X(SE_M_BLIZZARD, "SE-M-BLIZZARD") \
    X(SE_M_BLIZZARD2, "SE-M-BLIZZARD2") \
    X(SE_M_SCRATCH, "SE-M-SCRATCH") \
    X(SE_M_VICEGRIP, "SE-M-VICEGRIP") \
    X(SE_M_WING_ATTACK, "SE-M-WING-ATTACK") \
    X(SE_M_FLY, "SE-M-FLY") \
    X(SE_M_SAND_ATTACK, "SE-M-SAND-ATTACK") \
    X(SE_M_RAZOR_WIND2, "SE-M-RAZOR-WIND2") \
    X(SE_M_BITE, "SE-M-BITE") \
    X(SE_M_HEADBUTT, "SE-M-HEADBUTT") \
    X(SE_M_SURF, "SE-M-SURF") \
    X(SE_M_HYDRO_PUMP, "SE-M-HYDRO-PUMP") \
    X(SE_M_WHIRLPOOL, "SE-M-WHIRLPOOL") \
    X(SE_M_HORN_ATTACK, "SE-M-HORN-ATTACK") \
    X(SE_M_TAIL_WHIP, "SE-M-TAIL-WHIP") \
    X(SE_M_MIST, "SE-M-MIST") \
    X(SE_M_POISON_POWDER, "SE-M-POISON-POWDER") \
    X(SE_M_BIND, "SE-M-BIND") \
    X(SE_M_DRAGON_RAGE, "SE-M-DRAGON-RAGE") \
    X(SE_M_SING, "SE-M-SING") \
    X(SE_M_PERISH_SONG, "SE-M-PERISH-SONG") \
    X(SE_M_PAY_DAY, "SE-M-PAY-DAY") \
    X(SE_M_DIG, "SE-M-DIG") \
    X(SE_M_DIZZY_PUNCH, "SE-M-DIZZY-PUNCH") \
    X(SE_M_SELF_DESTRUCT, "SE-M-SELF-DESTRUCT") \
    X(SE_M_EXPLOSION, "SE-M-EXPLOSION") \
    X(SE_M_ABSORB_2, "SE-M-ABSORB-2") \
    X(SE_M_ABSORB, "SE-M-ABSORB") \
    X(SE_M_SCREECH, "SE-M-SCREECH") \
    X(SE_M_BUBBLE_BEAM, "SE-M-BUBBLE-BEAM") \
    X(SE_M_BUBBLE_BEAM2, "SE-M-BUBBLE-BEAM2") \
    X(SE_M_SUPERSONIC, "SE-M-SUPERSONIC") \
    X(SE_M_BELLY_DRUM, "SE-M-BELLY-DRUM") \
    X(SE_M_METRONOME, "SE-M-METRONOME") \
    X(SE_M_BONEMERANG, "SE-M-BONEMERANG") \
    X(SE_M_LICK, "SE-M-LICK") \
    X(SE_M_PSYBEAM, "SE-M-PSYBEAM") \
    X(SE_M_FAINT_ATTACK, "SE-M-FAINT-ATTACK") \
    X(SE_M_SWORDS_DANCE, "SE-M-SWORDS-DANCE") \
    X(SE_M_LEER, "SE-M-LEER") \
    X(SE_M_SWAGGER, "SE-M-SWAGGER") \
    X(SE_M_SWAGGER2, "SE-M-SWAGGER2") \
    X(SE_M_HEAL_BELL, "SE-M-HEAL-BELL") \
    X(SE_M_CONFUSE_RAY, "SE-M-CONFUSE-RAY") \
    X(SE_M_SNORE, "SE-M-SNORE") \
    X(SE_M_BRICK_BREAK, "SE-M-BRICK-BREAK") \
    X(SE_M_GIGA_DRAIN, "SE-M-GIGA-DRAIN") \
    X(SE_M_PSYBEAM2, "SE-M-PSYBEAM2") \
    X(SE_M_SOLAR_BEAM, "SE-M-SOLAR-BEAM") \
    X(SE_M_PETAL_DANCE, "SE-M-PETAL-DANCE") \
    X(SE_M_TELEPORT, "SE-M-TELEPORT") \
    X(SE_M_MINIMIZE, "SE-M-MINIMIZE") \
    X(SE_M_SKETCH, "SE-M-SKETCH") \
    X(SE_M_SWIFT, "SE-M-SWIFT") \
    X(SE_M_REFLECT, "SE-M-REFLECT") \
    X(SE_M_BARRIER, "SE-M-BARRIER") \
    X(SE_M_DETECT, "SE-M-DETECT") \
    X(SE_M_LOCK_ON, "SE-M-LOCK-ON") \
    X(SE_M_MOONLIGHT, "SE-M-MOONLIGHT") \
    X(SE_M_CHARM, "SE-M-CHARM") \
    X(SE_M_CHARGE, "SE-M-CHARGE") \
    X(SE_M_STRENGTH, "SE-M-STRENGTH") \
    X(SE_M_HYPER_BEAM, "SE-M-HYPER-BEAM") \
    X(SE_M_WATERFALL, "SE-M-WATERFALL") \
    X(SE_M_REVERSAL, "SE-M-REVERSAL") \
    X(SE_M_ACID_ARMOR, "SE-M-ACID-ARMOR") \
    X(SE_M_SANDSTORM, "SE-M-SANDSTORM") \
    X(SE_M_TRI_ATTACK, "SE-M-TRI-ATTACK") \
    X(SE_M_TRI_ATTACK2, "SE-M-TRI-ATTACK2") \
    X(SE_M_ENCORE, "SE-M-ENCORE") \
    X(SE_M_ENCORE2, "SE-M-ENCORE2") \
    X(SE_M_BATON_PASS, "SE-M-BATON-PASS") \
    X(SE_M_MILK_DRINK, "SE-M-MILK-DRINK") \
    X(SE_M_ATTRACT, "SE-M-ATTRACT") \
    X(SE_M_ATTRACT2, "SE-M-ATTRACT2") \
    X(SE_M_MORNING_SUN, "SE-M-MORNING-SUN") \
    X(SE_M_FLATTER, "SE-M-FLATTER") \
    X(SE_M_SAND_TOMB, "SE-M-SAND-TOMB") \
    X(SE_M_GRASSWHISTLE, "SE-M-GRASSWHISTLE") \
    X(SE_M_SPIT_UP, "SE-M-SPIT-UP") \
    X(SE_M_DIVE, "SE-M-DIVE") \
    X(SE_M_EARTHQUAKE, "SE-M-EARTHQUAKE") \
    X(SE_M_TWISTER, "SE-M-TWISTER") \
    X(SE_M_SWEET_SCENT, "SE-M-SWEET-SCENT") \
    X(SE_M_YAWN, "SE-M-YAWN") \
    X(SE_M_SKY_UPPERCUT, "SE-M-SKY-UPPERCUT") \
    X(SE_M_STAT_INCREASE, "SE-M-STAT-INCREASE") \
    X(SE_M_HEAT_WAVE, "SE-M-HEAT-WAVE") \
    X(SE_M_UPROAR, "SE-M-UPROAR") \
    X(SE_M_HAIL, "SE-M-HAIL") \
    X(SE_M_COSMIC_POWER, "SE-M-COSMIC-POWER") \
    X(SE_M_TEETER_DANCE, "SE-M-TEETER-DANCE") \
    X(SE_M_STAT_DECREASE, "SE-M-STAT-DECREASE") \
    X(SE_M_HAZE, "SE-M-HAZE") \
    X(SE_M_HYPER_BEAM2, "SE-M-HYPER-BEAM2") \
    X(SE_RG_DOOR, "SE-RG-DOOR") \
    X(SE_RG_CARD_FLIP, "SE-RG-CARD-FLIP") \
    X(SE_RG_CARD_FLIPPING, "SE-RG-CARD-FLIPPING") \
    X(SE_RG_CARD_OPEN, "SE-RG-CARD-OPEN") \
    X(SE_RG_BAG_CURSOR, "SE-RG-BAG-CURSOR") \
    X(SE_RG_BAG_POCKET, "SE-RG-BAG-POCKET") \
    X(SE_RG_BALL_CLICK, "SE-RG-BALL-CLICK") \
    X(SE_RG_SHOP, "SE-RG-SHOP") \
    X(SE_RG_SS_ANNE_HORN, "SE-RG-SS-ANNE-HORN") \
    X(SE_RG_HELP_OPEN, "SE-RG-HELP-OPEN") \
    X(SE_RG_HELP_CLOSE, "SE-RG-HELP-CLOSE") \
    X(SE_RG_HELP_ERROR, "SE-RG-HELP-ERROR") \
    X(SE_RG_DEOXYS_MOVE, "SE-RG-DEOXYS-MOVE") \
    X(SE_RG_POKE_JUMP_SUCCESS, "SE-RG-POKE-JUMP-SUCCESS") \
    X(SE_RG_POKE_JUMP_FAILURE, "SE-RG-POKE-JUMP-FAILURE") \
    X(SE_PHONE_CALL, "SE-PHONE-CALL") \
    X(SE_PHONE_CLICK, "SE-PHONE-CLICK") \
    X(SE_ARENA_TIMEUP1, "SE-ARENA-TIMEUP1") \
    X(SE_ARENA_TIMEUP2, "SE-ARENA-TIMEUP2") \
    X(SE_PIKE_CURTAIN_CLOSE, "SE-PIKE-CURTAIN-CLOSE") \
    X(SE_PIKE_CURTAIN_OPEN, "SE-PIKE-CURTAIN-OPEN") \
    X(SE_SUDOWOODO_SHAKE, "SE-SUDOWOODO-SHAKE") \

// Create Vanadium BGM list
#define X(songId, name) static const u8 sBGMName_Vanadium_##songId[] = _(name);
SOUND_LIST_VANADIUM_BGM
#undef X

#define X(songId, name) sBGMName_Vanadium_##songId,
static const u8 *const gBGMVanadiumNames[] =
{
SOUND_LIST_VANADIUM_BGM
};
#undef X

// Create Emerald BGM list
#define X(songId, name) static const u8 sBGMName_Emerald_##songId[] = _(name);
SOUND_LIST_EMERALD_BGM
#undef X

#define X(songId, name) sBGMName_Emerald_##songId,
static const u8 *const gBGMEmeraldNames[] =
{
SOUND_LIST_EMERALD_BGM
};
#undef X

// Create SE list
#define X(songId, name) static const u8 sSEName_##songId[] = _(name);
SOUND_LIST_SE
#undef X

#define X(songId, name) sSEName_##songId,
static const u8 *const gSENames[] =
{
SOUND_LIST_SE
};
#undef X


#endif