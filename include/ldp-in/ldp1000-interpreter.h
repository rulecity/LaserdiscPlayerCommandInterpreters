#ifndef LDP1000_INTERPRETER_H
#define LDP1000_INTERPRETER_H

/*
 * ldp1000-interpreter.h
 *
 * Copyright (C) 2013 Matt Ownby
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
	LDP1000_FALSE = 0,
	LDP1000_TRUE = 1
} LDP1000_BOOL;

// Which type of LDP we are emulating.
// (1450 has text overlay which the 1000A does not)
typedef enum
{
	LDP1000_EMU_LDP1000A = 0,
	LDP1000_EMU_LDP1450
} LDP1000_EmulationType_t;

void ldp1000i_reset(LDP1000_EmulationType_t type);

// writes a byte to LDP
void ldp1000i_write(uint8_t u8Byte);

// returns non-zero if LDP has a byte to be read
LDP1000_BOOL ldp1000i_can_read();

typedef enum
{
	LDP1000_LATENCY_CLEAR = 0,
	LDP1000_LATENCY_NUMBER,
	LDP1000_LATENCY_ENTER,
	LDP1000_LATENCY_PLAY,
	LDP1000_LATENCY_STILL,
	LDP1000_LATENCY_INQUIRY,
	LDP1000_LATENCY_GENERIC
} LDP1000Latency_t;

// reads a byte from LDP; must call ldp1000i_can_read first to determine if byte is available to be read
// NOTE: High byte is LDP1000Latency_t associated with this byte, low byte is the actual value
uint16_t ldp1000i_read();

// Should be called after vblank has started and the new VBI data has been read, but before vblank has ended.
// This is to handle things like the "REPEAT" command
void ldp1000i_think_during_vblank();

// Returns a pointer to the beginning of the text overlay buffer.  The text overlay buffer is always 32-bytes big.
const uint8_t *ldp1000i_get_text_buffer();

// diagnostic methods used mainly by unit tests to quickly verify proper behavior
LDP1000_BOOL ldp1000i_isRepeatActive();

////////////////////////////////////////////////////////////////////////////

typedef enum
{  
   LDP1000_ERROR, LDP1000_SEARCHING, LDP1000_STOPPED, LDP1000_PLAYING, LDP1000_PAUSED, LDP1000_SPINNING_UP
}
LDP1000Status_t;

// play the disc (includes options for multispeed and reverse playback; arguments for standard 1X forward playback is 1, 1, FALSE, FALSE)
extern void (*g_ldp1000i_play)(uint8_t u8Numerator, uint8_t u8Denominator, LDP1000_BOOL bBackward, LDP1000_BOOL bAudioSquelched);

extern void (*g_ldp1000i_pause)();

extern void (*g_ldp1000i_begin_search)(uint32_t u32FrameNum);

extern void (*g_ldp1000i_step_forward)();

extern void (*g_ldp1000i_step_reverse)();

// performs skip (aka track jump)
extern void (*g_ldp1000i_skip)(int16_t i16TracksToSkip);

// enables/disables left or right audio channels
extern void (*g_ldp1000i_change_audio)(uint8_t u8Channel, uint8_t uEnable);

// enables/disables video output
extern void (*g_ldp1000i_change_video)(LDP1000_BOOL bEnable);

// returns current status of laserdisc player (playing, paused, etc..)
extern LDP1000Status_t (*g_ldp1000i_get_status)();

// returns current frame number (from VBI)
extern uint32_t (*g_ldp1000i_get_cur_frame_num)();

////////////////////////////////////////
// BEGIN TEXT OVERLAY SECTION (supported by LDP-1450 and used by Dragon's Lair 2 and Space Ace '91)

// Indicates that the text overlay has either changed to enabled or disabled state.
extern void (*g_ldp1000i_text_enable_changed)(LDP1000_BOOL bEnabled);

// Indicates that the text buffer's contents have changed. (text buffer is always 32 bytes)
// The buffer's pointer can be obtained by calling ldp1000i_get_text_buffer.
// Depending on the mode, either 20 characters or 30 characters must be displayed (unless they go off screen).
extern void (*g_ldp1000i_text_buffer_contents_changed)(const uint8_t *p8Buf32Bytes);

// Indicates that the index pointing to the where the text overlay buffer starts has changed.
// 'u8StartIdx' is the index to start displaying from.
// If the index is toward the end of the buffer (ie the renderer will run out of characters), wraparound must always occur.
extern void (*g_ldp1000i_text_buffer_start_index_changed)(uint8_t u8StartIdx);

// Indicates that the position to start drawing overlay text or the text mode has changed.

// MODE:
// Bits
// 0:1	- horizontal scaling
//				0: no scaling
//				1: twice as wide
//				2: three times as wide
//				3: four times as wide
// 2:3	- vertical scaling
//				0: no scaling
//				1: twice as tall
//				2: three times as tall
//				3: four times as tall
// 4	- 20x1 or 10x3
//				0: display a single line of 20 characters
//				1: display a 10x3 box of characters
// 5:6	- enhancements to appearence
//				0: no change
//				1: grey border around all characters
//				2: no change
//				3: grey background behind all characters (grey window)
// 7	- blue background?
//				0: characters superimposed on laserdisc video background
//				1: characters superimposed on a light blue background

extern void (*g_ldp1000i_text_modes_changed)(uint8_t u8Mode, uint8_t u8X, uint8_t u8Y);

// END TEXT OVERLAY SECTION
/////////////////////////////////////////////////////////

typedef enum
{
	LDP1000_ERR_UNKNOWN_CMD_BYTE,
	LDP1000_ERR_UNSUPPORTED_CMD_BYTE,
	LDP1000_ERR_TOO_MANY_DIGITS,
	LDP1000_ERR_UNHANDLED_SITUATION
} LDP1000ErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_ldp1000i_error)(LDP1000ErrCode_t code, uint8_t u8Val);

#ifdef __cplusplus
}
#endif // C++

#endif // LDP1000_INTERPRETER_H
