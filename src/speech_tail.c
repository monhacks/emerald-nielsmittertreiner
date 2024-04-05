#include "global.h"
#include "decompress.h"
#include "event_data.h"
#include "field_player_avatar.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "overworld.h"
#include "speech_tail.h"
#include "sprite.h"
#include "constants/event_objects.h"
#include "event_object_movement.h"

#define TAG_SPEECH_TAIL 0x9000

#define TEXTBOX_LEFT_X 70
#define TEXTBOX_RIGHT_X 170 
#define TEXTBOX_Y_TOP 44
#define TEXTBOX_Y_BOTTOM 116

#define sSpriteId2 data[0]

static const struct OamData sSpeechTailOamData =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_DOUBLE,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = 0,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0
};

static const struct CompressedSpriteSheet sSpeechTailSpriteSheet =
{
    gSpeechTail_Gfx, 0x1000, TAG_SPEECH_TAIL
};

static const struct SpritePalette sSpeechTailPalette =
{
    gSpeechTail_Pal, TAG_SPEECH_TAIL
};

static const struct SpriteTemplate sSpeechTailSpriteTemplate =
{
    .tileTag = TAG_SPEECH_TAIL,
    .paletteTag = TAG_SPEECH_TAIL,
    .oam = &sSpeechTailOamData,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

EWRAM_DATA static u8 sSpeechTailSpriteId = 0;

void LoadTail(bool8 top, s16 x, s16 y)
{ 
    bool8 flash = FALSE;
    s16 textboxX = (x < 120) ? TEXTBOX_LEFT_X : TEXTBOX_RIGHT_X;
    s16 textboxY = (top == TRUE) ? TEXTBOX_Y_TOP : TEXTBOX_Y_BOTTOM;
    u8 spriteId;
    u8 spriteId2 = SPRITE_NONE;

    if (GetFlashLevel() > 1)
        flash = TRUE;

    if (GetSpriteTileStartByTag(TAG_SPEECH_TAIL) == 0xFFFF)
        LoadCompressedSpriteSheet(&sSpeechTailSpriteSheet);

    if (IndexOfSpritePaletteTag(TAG_SPEECH_TAIL) == 0xFF)
        LoadSpritePalette(&sSpeechTailPalette);

    if (flash)
    {
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
        SetGpuRegBits(REG_OFFSET_WINOUT, WINOUT_WINOBJ_OBJ);
    }

    spriteId = CreateSprite(&sSpeechTailSpriteTemplate, (textboxX + x) / 2, (textboxY + y) / 2, 0);
    if (flash)
        spriteId2 = CreateSprite(&sSpeechTailSpriteTemplate, (textboxX + x) / 2, (textboxY + y) / 2, 0);
    
    InitSpriteAffineAnim(&gSprites[spriteId]);
    SetOamMatrix(gSprites[spriteId].oam.matrixNum, 
                 Q_8_8(1), 
                 Q_8_8(1.0 / ((float)(y - textboxY) / -(x - textboxX))), // calculate x shear factor
                 Q_8_8(0), 
                 Q_8_8(64 / (double)(textboxY - y))); // calculate y scale factor
    
    if (spriteId2 != SPRITE_NONE)
    {
        gSprites[spriteId2].oam.matrixNum = gSprites[spriteId].oam.matrixNum;
        gSprites[spriteId2].oam.objMode = ST_OAM_OBJ_WINDOW;
        gSprites[spriteId].sSpriteId2 = spriteId2;
    }

    sSpeechTailSpriteId = spriteId;
}

void LoadTailFromObjectEventId(bool8 top, u32 id)
{
    struct ObjectEvent *objectEvent;
    struct Sprite *sprite;
    s16 x, y;

    objectEvent = &gObjectEvents[GetObjectEventIdByLocalIdAndMap(id, gSaveBlock1Ptr->location.mapNum, gSaveBlock1Ptr->location.mapGroup)];
    sprite = &gSprites[objectEvent->spriteId];
    x = sprite->oam.x - (sprite->x2 + sprite->centerToCornerVecX);
    y = sprite->oam.y - (sprite->y2 + sprite->centerToCornerVecY) + 8;

    LoadTail(top, x, y);
}

u8 GetTailSpriteId(void)
{
    return sSpeechTailSpriteId;
}

void DestroyTail(void)
{
    FreeSpriteOamMatrix(&gSprites[sSpeechTailSpriteId]);
    FreeSpritePaletteByTag(TAG_SPEECH_TAIL);
    FreeSpriteTilesByTag(TAG_SPEECH_TAIL);

    if (GetFlashLevel() > 1 && gSprites[sSpeechTailSpriteId].sSpriteId2 != SPRITE_NONE)
        DestroySprite(&gSprites[gSprites[sSpeechTailSpriteId].sSpriteId2]);

    DestroySprite(&gSprites[sSpeechTailSpriteId]);
}
