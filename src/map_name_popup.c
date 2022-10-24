#include "global.h"
#include "battle_pyramid.h"
#include "bg.h"
#include "main.h"
#include "event_data.h"
#include "field_weather.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "international_string_util.h"
#include "menu.h"
#include "map_name_popup.h"
#include "palette.h"
#include "region_map.h"
#include "scanline_effect.h"
#include "start_menu.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "time.h"
#include "constants/layouts.h"
#include "constants/region_map_sections.h"
#include "constants/weather.h"

// static functions
static void Task_MapNamePopUpWindow(u8 taskId);
static void ShowMapNamePopUpWindow(void);
static void LoadMapNamePopUpWindowBgs(void);

// .text
void ShowMapNamePopup(void)
{
    if (FlagGet(FLAG_HIDE_MAP_NAME_POPUP) != TRUE)
    {
        if (!FuncIsActiveTask(Task_MapNamePopUpWindow))
        {
            gPopupTaskId = CreateTask(Task_MapNamePopUpWindow, 100);
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
            SetHBlankCallback(HBlankCB_DoublePopupWindow);
            EnableInterrupts(INTR_FLAG_HBLANK);
            gTasks[gPopupTaskId].data[0] = 6;
            gTasks[gPopupTaskId].data[2] = 24;
        }
        else
        {
            if (gTasks[gPopupTaskId].data[0] != 2)
                gTasks[gPopupTaskId].data[0] = 2;
            gTasks[gPopupTaskId].data[3] = 1;
        }
    }
}

static void Task_MapNamePopUpWindow(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 6:
        task->data[4]++;
        if (task->data[4] > 30)
        {
            task->data[0] = 0;
            task->data[4] = 0;
            ShowMapNamePopUpWindow();
            ScanlineEffect_SetParams(gPopUpScanlineEffectParams);
        }
        break;
    case 0:
        task->data[2] -= 2;
        if (task->data[2] <= 0 )
        {
            task->data[2] = 0;
            task->data[0] = 1;
            gTasks[gPopupTaskId].data[1] = 0;
        }
        break;
    case 1:
        task->data[1]++;
        if (task->data[1] > 120 )
        {
            task->data[1] = 0;
            task->data[0] = 2;
        }
        break;
    case 2:
        task->data[2] += 2;
        if (task->data[2] > 23)
        {
            task->data[2] = 24;
            if (task->data[3])
            {
                task->data[0] = 6;
                task->data[4] = 0;
                task->data[3] = 0;
            }
            else
            {
                task->data[0] = 4;
                return;
            }
        }
        break;
    case 4:
        ClearStdWindowAndFrame(GetPrimaryPopUpWindowId(), TRUE);
        ClearStdWindowAndFrame(GetSecondaryPopUpWindowId(), TRUE);
        task->data[0] = 5;
        break;
    case 5:
        HideMapNamePopUpWindow();
        return;
    }

    SetDoublePopUpWindowScanlineBuffers(task->data[2]);
}

void HideMapNamePopUpWindow(void)
{
    if (FuncIsActiveTask(Task_MapNamePopUpWindow))
    {
        ClearStdWindowAndFrame(GetPrimaryPopUpWindowId(), TRUE);
        ClearStdWindowAndFrame(GetSecondaryPopUpWindowId(), TRUE);
        RemovePrimaryPopUpWindow();
        RemoveSecondaryPopUpWindow();
        ScanlineEffect_Stop();
        SetGpuReg_ForcedBlank(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ | BLDCNT_EFFECT_BLEND);
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(8, 10));
        DisableInterrupts(INTR_FLAG_HBLANK);
        SetHBlankCallback(NULL);
        DestroyTask(gPopupTaskId);
    }
}

static void ShowMapNamePopUpWindow(void)
{
    u8 mapDisplayHeader[24];
    u8 *withoutPrefixPtr;
    
    SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR);
    AddMapNamePopUpWindow();
    AddWeatherPopUpWindow();
    LoadMapNamePopUpWindowBgs();
    LoadPalette(gPopUpWindowBorder_Palette, 0xE0, 32);

    mapDisplayHeader[0] = EXT_CTRL_CODE_BEGIN;
    mapDisplayHeader[1] = EXT_CTRL_CODE_HIGHLIGHT;
    mapDisplayHeader[2] = TEXT_COLOR_TRANSPARENT;
    withoutPrefixPtr = &(mapDisplayHeader[3]);
    GetMapName(withoutPrefixPtr, gMapHeader.regionMapSectionId, 0);
    AddTextPrinterParameterized(GetPrimaryPopUpWindowId(), FONT_SHORT, mapDisplayHeader, 8, 4, TEXT_SKIP_DRAW, NULL);

    withoutPrefixPtr = &(mapDisplayHeader[3]);
    FormatDecimalTimeWithoutSeconds(withoutPrefixPtr, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes, gSaveBlock2Ptr->is24HClockMode);

    AddTextPrinterParameterized(GetSecondaryPopUpWindowId(), FONT_SMALL, mapDisplayHeader, 200, 6, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(GetPrimaryPopUpWindowId(), COPYWIN_FULL);
    CopyWindowToVram(GetSecondaryPopUpWindowId(), COPYWIN_FULL);
}

static void LoadMapNamePopUpWindowBgs(void)
{
    u8 mapNamePopUpWindowId = GetPrimaryPopUpWindowId();
    u8 weatherPopUpWindowId = GetSecondaryPopUpWindowId();
    u16 regionMapSectionId = gMapHeader.regionMapSectionId;

    if (regionMapSectionId >= KANTO_MAPSEC_START)
    {
        if (regionMapSectionId > KANTO_MAPSEC_END)
            regionMapSectionId -= KANTO_MAPSEC_COUNT;
        else
            regionMapSectionId = 0; // Discard kanto region sections;
    }

    PutWindowTilemap(mapNamePopUpWindowId);
    PutWindowTilemap(weatherPopUpWindowId);
    BlitBitmapRectToWindow(mapNamePopUpWindowId, gPopUpWindowBorder_Tiles, 0, 0, DISPLAY_WIDTH, 24, 0, 0, DISPLAY_WIDTH, 24);
    BlitBitmapRectToWindow(weatherPopUpWindowId, gPopUpWindowBorder_Tiles, 0, 24, DISPLAY_WIDTH, 24, 0, 0, DISPLAY_WIDTH, 24);
}
