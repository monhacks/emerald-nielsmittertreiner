#include "global.h"
#include "battle_pyramid.h"
#include "bg.h"
#include "main.h"
#include "event_data.h"
#include "field_weather.h"
#include "gpu_regs.h"
#include "international_string_util.h"
#include "menu.h"
#include "map_name_popup.h"
#include "palette.h"
#include "region_map.h"
#include "start_menu.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "constants/layouts.h"
#include "constants/region_map_sections.h"
#include "constants/weather.h"

// static functions
static void Task_MapNamePopUpWindow(u8 taskId);
static void ShowMapNamePopUpWindow(void);
static void LoadMapNamePopUpWindowBg(void);
static void HBlankCB_MapNamePopupWindow(void);

static EWRAM_DATA u8 sPopupTaskId = 0;

// .rodata
static const u16 sMapPopUp_Palette[16] = INCBIN_U16("graphics/interface/map_popup_palette.gbapal");

// .text
void ShowMapNamePopup(void)
{
    if (FlagGet(FLAG_HIDE_MAP_NAME_POPUP) != TRUE)
    {
        if (!FuncIsActiveTask(Task_MapNamePopUpWindow))
        {
            EnableInterrupts(INTR_FLAG_HBLANK);
            SetHBlankCallback(HBlankCB_MapNamePopupWindow);
            SetGpuReg(REG_OFFSET_BG0VOFS, 24);
            sPopupTaskId = CreateTask(Task_MapNamePopUpWindow, 90);
            gTasks[sPopupTaskId].data[0] = 6;
            gTasks[sPopupTaskId].data[2] = 24;
        }
        else
        {
            if (gTasks[sPopupTaskId].data[0] != 2)
                gTasks[sPopupTaskId].data[0] = 2;
            gTasks[sPopupTaskId].data[3] = 1;
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
        }
        break;
    case 0:
        task->data[2] -= 2;
        if (task->data[2] <= 0 )
        {
            task->data[2] = 0;
            task->data[0] = 1;
            gTasks[sPopupTaskId].data[1] = 0;
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
        ClearStdWindowAndFrame(GetMapNamePopUpWindowId(), TRUE);
        task->data[0] = 5;
        break;
    case 5:
        HideMapNamePopUpWindow();
        return;
    }
    SetGpuReg(REG_OFFSET_BG0VOFS, task->data[2]);
}

void HideMapNamePopUpWindow(void)
{
    if (FuncIsActiveTask(Task_MapNamePopUpWindow))
    {
        ClearStdWindowAndFrame(GetMapNamePopUpWindowId(), TRUE);
        RemoveMapNamePopUpWindow();
        SetGpuReg_ForcedBlank(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ);
        SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ);
        DisableInterrupts(INTR_FLAG_HBLANK);
        SetHBlankCallback(NULL);
        DestroyTask(sPopupTaskId);
    }
}

static void ShowMapNamePopUpWindow(void)
{
    u8 mapDisplayHeader[24];
    u8 *withoutPrefixPtr;
    
    SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR);

    mapDisplayHeader[0] = EXT_CTRL_CODE_BEGIN;
    mapDisplayHeader[1] = EXT_CTRL_CODE_HIGHLIGHT;
    mapDisplayHeader[2] = TEXT_COLOR_TRANSPARENT;
    withoutPrefixPtr = &(mapDisplayHeader[3]);
    GetMapName(withoutPrefixPtr, gMapHeader.regionMapSectionId, 0);
    AddMapNamePopUpWindow();
    LoadMapNamePopUpWindowBg();
    AddTextPrinterParameterized(GetMapNamePopUpWindowId(), FONT_NARROW, mapDisplayHeader, GetStringCenterAlignXOffset(FONT_NARROW, withoutPrefixPtr, 240), 4, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(GetMapNamePopUpWindowId(), COPYWIN_FULL);
}

static void LoadMapNamePopUpWindowBg(void)
{
    u8 popupWindowId = GetMapNamePopUpWindowId();
    u16 regionMapSectionId = gMapHeader.regionMapSectionId;

    if (regionMapSectionId >= KANTO_MAPSEC_START)
    {
        if (regionMapSectionId > KANTO_MAPSEC_END)
            regionMapSectionId -= KANTO_MAPSEC_COUNT;
        else
            regionMapSectionId = 0; // Discard kanto region sections;
    }

    PutWindowTilemap(popupWindowId);
    LoadPalette(sMapPopUp_Palette, 0xE0, sizeof(sMapPopUp_Palette));
    FillWindowPixelBuffer(popupWindowId, PIXEL_FILL(1));
}

static void HBlankCB_MapNamePopupWindow(void)
{
    struct Task *task = &gTasks[sPopupTaskId];
    s16 currentOffset = 24 - task->data[2];

    if (REG_VCOUNT < currentOffset || REG_VCOUNT > 160)
    {
        REG_BLDCNT = BLDCNT_TGT1_BG0 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND;
        REG_BLDALPHA = BLDALPHA_BLEND(15, 5);
    }
    else
    {
        gWeatherPtr->currBlendEVA = 8;
        gWeatherPtr->currBlendEVB = 10;
        gWeatherPtr->targetBlendEVA = 8;
        gWeatherPtr->targetBlendEVB = 10;
        REG_BLDALPHA = BLDALPHA_BLEND(8, 10);
    }
}
