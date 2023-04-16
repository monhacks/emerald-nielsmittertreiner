#include "global.h"
#include "time.h"
#include "string_util.h"
#include "strings.h"
#include "text.h"

const u8 *const gText_DaysOfWeek[] =
{
    gText_Sunday,
    gText_Monday,
    gText_Tuesday,
    gText_Wednesday,
    gText_Thursday,
    gText_Friday,
    gText_Saturday
};

void InGameClock_Run(void)
{
    gSaveBlock2Ptr->inGameClock.vblanks++;

    // 20 vblanks for a 72 hour cycle
    if (gSaveBlock2Ptr->inGameClock.vblanks < 10)
        return;

    gSaveBlock2Ptr->inGameClock.vblanks = 0;
    gSaveBlock2Ptr->inGameClock.seconds++;

    if (gSaveBlock2Ptr->inGameClock.seconds < 60)
        return;

    gSaveBlock2Ptr->inGameClock.seconds = 0;
    gSaveBlock2Ptr->inGameClock.minutes++;

    if (gSaveBlock2Ptr->inGameClock.minutes < 60)
        return;

    gSaveBlock2Ptr->inGameClock.minutes = 0;
    gSaveBlock2Ptr->inGameClock.hours++;

    if (gSaveBlock2Ptr->inGameClock.hours < 24)
        return;

    gSaveBlock2Ptr->inGameClock.hours = 0;
    gSaveBlock2Ptr->inGameClock.dayOfWeek++;

    if (gSaveBlock2Ptr->inGameClock.dayOfWeek < 7)
        return;

    gSaveBlock2Ptr->inGameClock.dayOfWeek = 0;
}

void InGameClock_SetTime(s8 dayOfWeek, s8 hour, s8 minute)
{
    gSaveBlock2Ptr->inGameClock.dayOfWeek = dayOfWeek;
    gSaveBlock2Ptr->inGameClock.hours = hour;
    gSaveBlock2Ptr->inGameClock.minutes = minute;
}

void FormatDecimalTimeWithoutSeconds(u8 *dest, s8 hour, s8 minute, bool8 is24Hour)
{
    switch (is24Hour)
    {
    case TRUE:
        dest = ConvertIntToDecimalStringN(dest, hour, STR_CONV_MODE_LEADING_ZEROS, 2);
        *dest++ = CHAR_COLON;
        dest = ConvertIntToDecimalStringN(dest, minute, STR_CONV_MODE_LEADING_ZEROS, 2);
        break;
    case FALSE:
        if (hour < 13)
            dest = ConvertIntToDecimalStringN(dest, hour, STR_CONV_MODE_LEADING_ZEROS, 2);
        else
            dest = ConvertIntToDecimalStringN(dest, hour - 12, STR_CONV_MODE_LEADING_ZEROS, 2);

        *dest++ = CHAR_COLON;
        dest = ConvertIntToDecimalStringN(dest, minute, STR_CONV_MODE_LEADING_ZEROS, 2);

        if (hour < 13)
            dest = StringAppend(dest, gText_AM);
        else
            dest = StringAppend(dest, gText_PM);
        break;
    }

    *dest = EOS;
}
