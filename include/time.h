#ifndef GUARD_TIME_H
#define GUARD_TIME_H

extern struct InGameClock gInGameClock;

extern const u8 *const gDaysOfWeek[];

void InGameClock_Start(void);
void InGameClock_Stop(void);
void InGameClock_Update(void);
void InGameClock_SetTime(s8 dayOfWeek, s8 hour, s8 minute);
void FormatDecimalTimeWithoutSeconds(u8 *dest, s8 hour, s8 minute, bool8 isAMPM);

#endif // GUARD_TIME_H