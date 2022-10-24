#ifndef GUARD_TIME_H
#define GUARD_TIME_H

extern struct InGameClock gInGameClock;
extern const u8 *const gText_DaysOfWeek[];

void InGameClock_Run(void);
void InGameClock_SetTime(s8 dayOfWeek, s8 hour, s8 minute);
void FormatDecimalTimeWithoutSeconds(u8 *dest, s8 hour, s8 minute, bool8 is24Hour);

#endif // GUARD_TIME_H