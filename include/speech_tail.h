#ifndef GUARD_SPEECH_TAIL_H
#define GUARD_SPEECH_TAIL_H

void LoadTail(bool8 top, s16 x, s16 y);
void LoadTailFromObjectEventId(bool8 top, u32 id);
void LoadTailFromScript(void);
void LoadTailAutoFromScript(void);
u8 GetTailSpriteId(void);
void DestroyTail(void);

#endif // GUARD_SPEECH_TAIL_H