#ifndef VIP9500SG_INTERPRETER_H
#define VIP9500SG_INTERPRETER_H

/*
 * vip9500sg-interpreter.h
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

#ifdef __cplusplus
extern "C"
{
#endif // C++

#include "datatypes.h"

/////////////////////////////////////////

typedef enum
{
	VIP9500SG_FALSE = 0,
	VIP9500SG_TRUE = 1
} VIP9500SG_BOOL;

void vip9500sgi_reset();

// writes a byte to LDP
void vip9500sgi_write(uint8_t u8Byte);

// returns non-zero if LDP has a byte to be read
VIP9500SG_BOOL vip9500sgi_can_read();

// reads a byte from LDP; must call vip9500sgi_can_read first to determine if byte is available to be read
uint8_t vip9500sgi_read();

// Should be called right after vblank has ended (ie VBI data is read, seeking/skipping is finished).
// This is to handle things like seeks/skips completing and 'get current picture number' requests.
void vip9500sgi_think_after_vblank();

////////////////////////////////////////////////////////////////////////////

typedef enum
{  
   VIP9500SG_ERROR, VIP9500SG_SEARCHING, VIP9500SG_STEPPING, VIP9500SG_STOPPED, VIP9500SG_PLAYING, VIP9500SG_PAUSED, VIP9500SG_SPINNING_UP
}
VIP9500SGStatus_t;

extern void (*g_vip9500sgi_play)();

extern void (*g_vip9500sgi_pause)();

extern void (*g_vip9500sgi_stop)();

extern void (*g_vip9500sgi_step_reverse)();

extern void (*g_vip9500sgi_begin_search)(uint32_t u32FrameNum);

extern void (*g_vip9500sgi_skip)(int32_t i32TracksToSkip);

// enables/disables left or right audio channels
extern void (*g_vip9500sgi_change_audio)(uint8_t u8Channel, uint8_t uEnable);

// returns current status of laserdisc player (playing, paused, etc..)
extern VIP9500SGStatus_t (*g_vip9500sgi_get_status)();

// returns current frame number in decimal format
extern uint32_t (*g_vip9500sgi_get_cur_frame_num)();

// returns current 24-bit VBI data from line 18
extern uint32_t (*g_vip9500sgi_get_cur_vbi_line18)();

typedef enum
{
	VIP9500SG_ERR_UNKNOWN_CMD_BYTE,
	VIP9500SG_ERR_UNSUPPORTED_CMD_BYTE,
	VIP9500SG_ERR_TOO_MANY_DIGITS,
	VIP9500SG_ERR_UNHANDLED_SITUATION
} VIP9500SGErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_vip9500sgi_error)(VIP9500SGErrCode_t code, uint8_t u8Val);

#ifdef __cplusplus
}
#endif // C++

#endif // VIP9500SG_INTERPRETER_H
