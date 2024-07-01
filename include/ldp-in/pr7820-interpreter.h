/*
 * pr7820-interpreter.h
 *
 * Copyright (C) 2015 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PR7820_INTERPRETER_H
#define PR7820_INTERPRETER_H

typedef enum
{  
   PR7820_ERROR, PR7820_SEARCHING, PR7820_STOPPED, PR7820_PLAYING, PR7820_PAUSED, PR7820_SPINNING_UP
}
PR7820Status_t;

typedef enum
{
	PR7820_FALSE = 0,
	PR7820_TRUE = 1
}
PR7820_BOOL;

#ifdef __cplusplus
extern "C"
{
#endif // C++

// resets all globals to default state
void pr7820i_reset();

PR7820_BOOL pr7820i_is_busy();
void pr7820i_write(unsigned char value);

// CALLBACKS

// returns current status of laserdisc player (playing, paused, etc..)
extern PR7820Status_t (*g_pr7820i_get_status)();

// plays the laserdisc
extern void (*g_pr7820i_play)();

// pauses the laserdisc
extern void (*g_pr7820i_pause)();

// begins searching to a frame (non-blocking, should return immediately).  Use g_pr7820i_get_status to report search errors and completion.
extern void (*g_pr7820i_begin_search)(unsigned int uFrameNumber);

// enables/disables left or right audio channels
extern void (*g_pr7820i_change_audio)(unsigned char uChannel, unsigned char uEnable);

// extended command: enable super (universal) mode
extern void (*g_pr7820i_enable_super_mode)();

typedef enum
{
	PR7820_ERR_TOO_MANY_DIGITS,
	PR7820_ERR_UNKNOWN_CMD_BYTE,
	PR7820_ERR_UNSUPPORTED_CMD_BYTE
}
PR7820ErrCode_t;

// This gets called if our interpreter gets an error.  The u8Val will be have meaning depending on the error code.
extern void (*g_pr7820i_on_error)(PR7820ErrCode_t code, unsigned char u8Val);

// end callbacks

#ifdef __cplusplus
}
#endif // c++

#endif
