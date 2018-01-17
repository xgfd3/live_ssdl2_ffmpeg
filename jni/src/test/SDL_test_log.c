/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*

 Used by the test framework and test cases.

*/

/* quiet windows compiler warnings */
#define _CRT_SECURE_NO_WARNINGS

#include "SDL_config.h"

#include <stdarg.h> /* va_list */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "SDL.h"

#include "SDL_test.h"

/* !
 * Converts unix timestamp to its ascii representation in localtime
 *
 * Note: Uses a static data internally, so the return value
 * isn't valid after the next call of this function. If you
 * want to retain the return value, make a copy of it.
 *
 * \param timestamp A Timestamp, i.e. time(0)
 *
 * \return Ascii representation of the timestamp in localtime in the format '08/23/01 14:55:02'
 */
char *SDLTest_TimestampToString(const time_t timestamp)
{
    time_t copy;
    static char buffer[64];
    struct tm *local;
    const char *fmt = "%x %X";

    SDL_memset(buffer, 0, sizeof(buffer));
    copy = timestamp;
    local = localtime(&copy);
    strftime(buffer, sizeof(buffer), fmt, local);

    return buffer;
}

/*
 * Prints given message with a timestamp in the TEST category and INFO priority.
 */
void SDLTest_Log(const char *fmt, ...)
{
    va_list list;
    char logMessage[SDLTEST_MAX_LOGMESSAGE_LENGTH];

    /* Print log message into a data */
    SDL_memset(logMessage, 0, SDLTEST_MAX_LOGMESSAGE_LENGTH);
    va_start(list, fmt);
    SDL_vsnprintf(logMessage, SDLTEST_MAX_LOGMESSAGE_LENGTH - 1, fmt, list);
    va_end(list);

    /* Log with timestamp and newline */
    SDL_LogMessage(SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_INFO, " %s: %s", SDLTest_TimestampToString(time(0)), logMessage);
}

/*
 * Prints given message with a timestamp in the TEST category and the ERROR priority.
 */
void SDLTest_LogError(const char *fmt, ...)
{
    va_list list;
    char logMessage[SDLTEST_MAX_LOGMESSAGE_LENGTH];

    /* Print log message into a data */
    SDL_memset(logMessage, 0, SDLTEST_MAX_LOGMESSAGE_LENGTH);
    va_start(list, fmt);
    SDL_vsnprintf(logMessage, SDLTEST_MAX_LOGMESSAGE_LENGTH - 1, fmt, list);
    va_end(list);

    /* Log with timestamp and newline */
    SDL_LogMessage(SDL_LOG_CATEGORY_TEST, SDL_LOG_PRIORITY_ERROR, "%s: %s", SDLTest_TimestampToString(time(0)), logMessage);
}
