#include "global.h"
#include "main.h"
#include "menu.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "palette.h"
#include "quests.h"
#include "international_string_util.h"
#include "item.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "string_util.h"
#include "text.h"
#include "trig.h"
#include "util.h"
#include "window.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/quests.h"
#include "constants/rgb.h"
#include "data/text/quest_descriptions.h"

#define TAG_QUEST_ICON 0x100

static u8 *GetQuestPointer(u16 id);
static void Task_QuestPopUpWindow(u8 taskId);
static void ShowQuestPopUpWindow(void);
static void LoadQuestPopUpWindowBgs(void);
static void SpriteCB_QuestIcons(struct Sprite *sprite);

// .rodata
static const struct SpritePalette sSpritePalette_QuestIcons =
{
    .data = gQuestPopUpIconPalette,
    .tag = TAG_QUEST_ICON
};

static const struct SpriteSheet sSpriteSheet_QuestIcons =
{
    .data = gQuestPopUpIconTiles,
    .size = 0x100,
    .tag = TAG_QUEST_ICON,
};

static const union AnimCmd sSpriteAnim_StoryIcon[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_LoggersIcon[] =
{
    ANIMCMD_FRAME(1, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_FossilsIcon[] =
{
    ANIMCMD_FRAME(2, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_OceanicIcon[] =
{
    ANIMCMD_FRAME(3, 8),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_SideIcon[] =
{
    ANIMCMD_FRAME(4, 8),
    ANIMCMD_END
};

static const union AnimCmd *const sSpriteAnimTable_QuestIcons[] =
{
    sSpriteAnim_StoryIcon,
    sSpriteAnim_LoggersIcon,
    sSpriteAnim_FossilsIcon,
    sSpriteAnim_OceanicIcon,
    sSpriteAnim_SideIcon,
};


static const struct OamData sOamData_QuestIcons =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

const struct SpriteTemplate sSpriteTemplate_QuestIcons =
{
    .tileTag = TAG_QUEST_ICON,
    .paletteTag = TAG_QUEST_ICON,
    .oam = &sOamData_QuestIcons,
    .anims = sSpriteAnimTable_QuestIcons,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCB_QuestIcons,
};

static const u8 sText_NewQuestUnlocked[] = _("NEW QUEST UNLOCKED!");

// .text
static u8 *GetQuestPointer(u16 id)
{
    if (id > NUM_QUESTS - 1)
        return NULL;
        
    return &gSaveBlock1Ptr->quests[id / 8];
}

u8 EnableQuest(u16 id)
{
    u8 *ptr = GetQuestPointer(id);
    if (ptr)
        *ptr |= 1 << (id & 7);
    return 0;
}

u8 DisableQuest(u16 id)
{
    u8 *ptr = GetQuestPointer(id);
    if (ptr)
        *ptr &= ~(1 << (id & 7));
    return 0;
}

bool8 QuestGet(u16 id)
{
    u8 *ptr = GetQuestPointer(id);

    if (!ptr)
        return FALSE;

    if (!(((*ptr) >> (id & 7)) & 1))
        return FALSE;

    return TRUE;
}

const u8 *QuestGetName(u16 id)
{
    return gQuests[id].name;
}

u8 QuestGetProgress(u16 id)
{
    return gQuests[id].progress();
}

u16 QuestGetGraphicsId(u16 id)
{
    u8 progress = QuestGetProgress(id);

    return gQuests[id].graphics(progress);
}

const u8 *const *QuestGetDescription(u16 id)
{
    u8 progress = QuestGetProgress(id);

    return gQuests[id].description(progress);
}

u8 QuestGetLines(u16 id)
{
    u8 progress = QuestGetProgress(id);

    return gQuests[id].lines(progress);
}

u8 QuestGetType(u16 id)
{
    return gQuests[id].type;
}

void ShowQuestPopup(u16 id)
{
    if (!FuncIsActiveTask(Task_QuestPopUpWindow))
    {
        gPopupTaskId = CreateTask(Task_QuestPopUpWindow, 100);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_TGT2_ALL | BLDCNT_EFFECT_BLEND);
        SetHBlankCallback(HBlankCB_DoublePopupWindow);
        EnableInterrupts(INTR_FLAG_HBLANK);
        gTasks[gPopupTaskId].data[0] = 6;
        gTasks[gPopupTaskId].data[2] = 24;
        gTasks[gPopupTaskId].data[6] = id;
    }
    else
    {
        if (gTasks[gPopupTaskId].data[0] != 2)
            gTasks[gPopupTaskId].data[0] = 2;
        gTasks[gPopupTaskId].data[3] = 1;
    }
}

void HideQuestPopUpWindow(void)
{
    if (FuncIsActiveTask(Task_QuestPopUpWindow))
    {
        //ClearStdWindowAndFrame(GetPrimaryPopUpWindowId(), TRUE);
        //ClearStdWindowAndFrame(GetSecondaryPopUpWindowId(), TRUE);
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

static void Task_QuestPopUpWindow(u8 taskId)
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
            ShowQuestPopUpWindow();
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
        HideQuestPopUpWindow();
        return;
    }

    SetDoublePopUpWindowScanlineBuffers(task->data[2]);
}

static void ShowQuestPopUpWindow(void)
{
    s16 *data = gTasks[gPopupTaskId].data;
    u8 string[QUEST_NAME_LENGTH];
    u8 *txtPtr;
    
    SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR);
    LoadSpritePalette(&sSpritePalette_QuestIcons);
    LoadSpriteSheet(&sSpriteSheet_QuestIcons);
    AddMapNamePopUpWindow();
    AddWeatherPopUpWindow();
    LoadQuestPopUpWindowBgs();
    LoadPalette(gPopUpWindowBorder_Palette, 0xE0, 32);

    data[5] = CreateSprite(&sSpriteTemplate_QuestIcons, 10, 12, 0);

    string[0] = EXT_CTRL_CODE_BEGIN;
    string[1] = EXT_CTRL_CODE_HIGHLIGHT;
    string[2] = TEXT_COLOR_TRANSPARENT;
    txtPtr = &(string[3]);

    StringCopy(txtPtr, QuestGetName(data[6]));
    AddTextPrinterParameterized(GetPrimaryPopUpWindowId(), FONT_SHORT, string, 20, 4, TEXT_SKIP_DRAW, NULL);
    AddTextPrinterParameterized(GetSecondaryPopUpWindowId(), FONT_SMALL, sText_NewQuestUnlocked, 144, 8, TEXT_SKIP_DRAW, NULL);

    CopyWindowToVram(GetPrimaryPopUpWindowId(), COPYWIN_FULL);
    CopyWindowToVram(GetSecondaryPopUpWindowId(), COPYWIN_FULL);
}

static void LoadQuestPopUpWindowBgs(void)
{
    u8 primaryWindowId = GetPrimaryPopUpWindowId();
    u8 secondaryWindowId = GetSecondaryPopUpWindowId();

    BlitBitmapRectToWindow(primaryWindowId, gPopUpWindowBorder_Tiles, 0, 0, DISPLAY_WIDTH, 24, 0, 0, DISPLAY_WIDTH, 24);
    BlitBitmapRectToWindow(secondaryWindowId, gPopUpWindowBorder_Tiles, 0, 24, DISPLAY_WIDTH, 24, 0, 0, DISPLAY_WIDTH, 24);
    PutWindowTilemap(primaryWindowId);
    PutWindowTilemap(secondaryWindowId);
}

static void SpriteCB_QuestIcons(struct Sprite *sprite)
{
    struct Task *task = &gTasks[gPopupTaskId];

    sprite->animNum = QuestGetType(task->data[6]);
    sprite->y2 = -task->data[2];
    sprite->data[0] += 4;
    sprite->data[0] &= 0x7F;
    sprite->data[1] = gSineTable[sprite->data[0]] >> 5;

    BlendPalette(256 + (sprite->oam.paletteNum * 16) + 4, 12, sprite->data[1], RGB_WHITEALPHA);

    if (!FuncIsActiveTask(Task_QuestPopUpWindow))
        DestroySpriteAndFreeResources(sprite);
}



u8 QuestProgressFunc1(void)
{
    u8 count = CountTotalItemQuantityInBag(ITEM_POTION);

    if (count >= 5)
        return QUEST_READY;
    else
        return count;
}

u16 QuestGraphicsFunc1(u8 progress)
{
    return OBJ_EVENT_GFX_MAN_2;
}

const u8 *const *QuestDescriptionFunc1(u8 progress)
{
    if (progress == QUEST_READY)
    {
        return sQuestDescriptionList2;
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar2, progress, STR_CONV_MODE_LEFT_ALIGN, 1);
        return sQuestDescriptionList1;
    }
}

u8 QuestDescriptionLinesFunc1(u8 progress)
{
    if (progress == QUEST_READY)
        return ARRAY_COUNT(sQuestDescriptionList2);
    else
        return ARRAY_COUNT(sQuestDescriptionList1);
}
