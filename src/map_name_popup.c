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

// States and data defines for Task_MapNamePopUpWindow
enum {
    STATE_SLIDE_IN,
    STATE_WAIT,
    STATE_SLIDE_OUT,
    STATE_UNUSED,
    STATE_ERASE,
    STATE_END,
    STATE_PRINT, // For some reason the first state is numerically last.
};

#define POPUP_OFFSCREEN_Y  24
#define POPUP_SLIDE_SPEED  2

#define tState         data[0]
#define tOnscreenTimer data[1]
#define tYOffset       data[2]
#define tIncomingPopUp data[3]
#define tPrintTimer    data[4]

// .text
void ShowMapNamePopup(void)
{
    if (FlagGet(FLAG_HIDE_MAP_NAME_POPUP) != TRUE)
    {
        if (!FuncIsActiveTask(Task_MapNamePopUpWindow))
        {
            gPopupTaskId = CreateTask(Task_MapNamePopUpWindow, 100);
            gTasks[gPopupTaskId].tState = STATE_PRINT;
            gTasks[gPopupTaskId].data[2] = POPUP_OFFSCREEN_Y;
        }
        else
        {
            if (gTasks[gPopupTaskId].tState != STATE_SLIDE_OUT)
                gTasks[gPopupTaskId].tState = STATE_SLIDE_OUT;
            gTasks[gPopupTaskId].tIncomingPopUp = TRUE;
        }
    }
}

static void Task_MapNamePopUpWindow(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->tState)
    {
    case STATE_PRINT:
        // Wait, then create and print the pop up window
        if (++task->tPrintTimer > 30)
        {
            task->tState = STATE_SLIDE_IN;
            task->tPrintTimer = 0;
            ShowMapNamePopUpWindow();
            EnableInterrupts(INTR_FLAG_HBLANK);
            SetHBlankCallback(HBlankCB_DoublePopupWindow);
        }
        break;
    case STATE_SLIDE_IN:
        // Slide the window onscreen.
        task->tYOffset -= POPUP_SLIDE_SPEED;
        if (task->tYOffset <= 0 )
        {
            task->tYOffset = 0;
            task->tState = STATE_WAIT;
            gTasks[gPopupTaskId].tOnscreenTimer = 0;
        }
        break;
    case STATE_WAIT:
        // Wait while the window is fully onscreen.
        if (++task->tOnscreenTimer > 120)
        {
            task->tOnscreenTimer = 0;
            task->tState = STATE_SLIDE_OUT;
        }
        break;
    case STATE_SLIDE_OUT:
        task->tYOffset += POPUP_SLIDE_SPEED;
        if (task->tYOffset >= POPUP_OFFSCREEN_Y)
        {
            task->tYOffset = POPUP_OFFSCREEN_Y;
            if (task->tIncomingPopUp)
            {
                // A new pop up window is incoming,
                // return to the first state to show it.
                task->tState = STATE_PRINT;
                task->tPrintTimer = 0;
                task->tIncomingPopUp = FALSE;
            }
            else
            {
                task->tState = STATE_ERASE;
                return;
            }
        }
        break;
    case STATE_ERASE:
        ClearStdWindowAndFrame(GetPrimaryPopUpWindowId(), TRUE);
        ClearStdWindowAndFrame(GetSecondaryPopUpWindowId(), TRUE);
        task->tState = STATE_END;
        break;
    case STATE_END:
        HideMapNamePopUpWindow();
        return;
    }
}

void HideMapNamePopUpWindow(void)
{
    if (FuncIsActiveTask(Task_MapNamePopUpWindow))
    {
        ClearStdWindowAndFrame(GetPrimaryPopUpWindowId(), TRUE);
        ClearStdWindowAndFrame(GetSecondaryPopUpWindowId(), TRUE);
        RemovePrimaryPopUpWindow();
        RemoveSecondaryPopUpWindow();
        DisableInterrupts(INTR_FLAG_HBLANK);
        SetHBlankCallback(NULL);
        SetGpuReg_ForcedBlank(REG_OFFSET_BG0VOFS, 0);
        DestroyTask(gPopupTaskId);
    }
}

static void ShowMapNamePopUpWindow(void)
{
    u8 mapDisplayHeader[24];
    u8 *withoutPrefixPtr;
    u8 mapNamePopUpWindowId = AddMapNamePopUpWindow();
    u8 weatherPopUpWindowId = AddWeatherPopUpWindow();

    LoadMapNamePopUpWindowBgs();
    LoadPalette(gPopUpWindowBorder_Palette, 0xE0, 32);

    mapDisplayHeader[0] = EXT_CTRL_CODE_BEGIN;
    mapDisplayHeader[1] = EXT_CTRL_CODE_HIGHLIGHT;
    mapDisplayHeader[2] = TEXT_COLOR_TRANSPARENT;
    withoutPrefixPtr = &(mapDisplayHeader[3]);
    GetMapName(withoutPrefixPtr, gMapHeader.regionMapSectionId, 0);
    AddTextPrinterParameterized(mapNamePopUpWindowId, FONT_SHORT, mapDisplayHeader, 8, 4, TEXT_SKIP_DRAW, NULL);

    withoutPrefixPtr = &(mapDisplayHeader[3]);
    FormatDecimalTimeWithoutSeconds(withoutPrefixPtr, gSaveBlock2Ptr->inGameClock.hours, gSaveBlock2Ptr->inGameClock.minutes, gSaveBlock2Ptr->optionsClockMode);

    //GetStringRightAlignXOffset(FONT_SMALL, mapDisplayHeader, DISPLAY_WIDTH);
    AddTextPrinterParameterized(weatherPopUpWindowId, FONT_SMALL, mapDisplayHeader, GetStringRightAlignXOffset(FONT_SMALL, mapDisplayHeader, DISPLAY_WIDTH) - 8, 6, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(mapNamePopUpWindowId, COPYWIN_FULL);
    CopyWindowToVram(weatherPopUpWindowId, COPYWIN_FULL);
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
    CopyToWindowPixelBuffer(mapNamePopUpWindowId, gPopUpWindowBorderTop_Tiles, 2880, 0);
    CopyToWindowPixelBuffer(weatherPopUpWindowId, gPopUpWindowBorderBottom_Tiles, 2880, 0);
}
