/*
 * ldv1000-interpreter.h
 *
 * Copyright (C) 2009 Matt Ownby
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

// LDV1000-interpreter.H
// Stripped down version to be used with real-time linux

#ifndef LDV1000_INTERPRETER_H
#define LDV1000_INTERPRETER_H

#include "datatypes.h"

typedef enum
{
   LDV1000_ERROR, LDV1000_SEARCHING, LDV1000_STOPPED, LDV1000_PLAYING, LDV1000_PAUSED, LDV1000_SPINNING_UP, LDV1000_DISC_SWITCHING
}
LDV1000Status_t;

typedef enum
{
	LDV1000_FALSE = 0,
	LDV1000_TRUE = 1
}
LDV1000_BOOL;

typedef enum
{
	LDV1000_EMU_STANDARD = 0,
	LDV1000_EMU_BADLANDS,

	// 'super' mode, a new mode that Shaun Wood and I came up with for Dexter to make I/O super fast
	LDV1000_EMU_SUPER
} LDV1000_EmulationType_t;

typedef enum
{
	LDV1000_DISCSWITCH_NONE = 0,
	LDV1000_DISCSWITCH_WAITING_FOR_DISC_ID
} LDV1000_DiscSwitchState_t;

#ifdef __cplusplus
extern "C"
{
#endif // C++

// resets all globals to default state
	// (also indicates whether to emulate a regular LD-V1000 or a Badlands LD-V1000)
void reset_ldv1000i(LDV1000_EmulationType_t type);

unsigned char read_ldv1000i();
void write_ldv1000i (unsigned char value);

// CALLBACKS

// returns current status of laserdisc player (playing, paused, etc..)
extern LDV1000Status_t (*g_ldv1000i_get_status)();

// returns the last frame number reported by the laserdisc's VBI
extern uint32_t (*g_ldv1000i_get_cur_frame_num)();

// plays the laserdisc
extern void (*g_ldv1000i_play)();

// pauses the laserdisc
extern void (*g_ldv1000i_pause)();

// begins searching to a frame (non-blocking, should return immediately).  Use g_ldv1000i_get_status to report search errors and completion.
extern void (*g_ldv1000i_begin_search)(uint32_t uFrameNumber);

// steps backward 1 frame (optionally, you can point this to g_ldv1000i_pause as a hack which will be good enough for most games)
extern void (*g_ldv1000i_step_reverse)();

// changes the playback speed
extern void (*g_ldv1000i_change_speed)(uint8_t uNumerator, uint8_t uDenominator);

extern void (*g_ldv1000i_skip_forward)(uint8_t uTracks);
extern void (*g_ldv1000i_skip_backward)(uint8_t uTracks);

// enables/disables left or right audio channels
extern void (*g_ldv1000i_change_audio)(uint8_t uChannel, uint8_t uEnable);

// This gets called if our interpreter gets an error.
// It will contain a pointer to a string describing the error.
extern void (*g_ldv1000i_on_error)(const char *pszErrMsg);

// extended command: returns an null-terminated array of disc IDs that could be switched to
extern const uint8_t *(*g_ldv1000i_query_available_discs)();

// extended command: reteurns the currently active disc ID (or 00 if no disc is active)
extern uint8_t (*g_ldv1000i_query_active_disc)();

// extended command: begins switching to the newly requested disc (non-blocking, should return immediately).
// Behaves like a search.
// Use g_ldv1000i_get_status to report error/completion.
extern void (*g_ldv1000i_begin_changing_to_disc)(uint8_t idDisc);

// extended command: enable/disable authentic seek delay
extern void (*g_ldv1000i_change_seek_delay)(LDV1000_BOOL bEnabled);

// extended command: enable/disable authentic spin-up delay
extern void (*g_ldv1000i_change_spinup_delay)(LDV1000_BOOL bEnabled);

// extended command: enable/disable super (universal) mode
extern void (*g_ldv1000i_change_super_mode)(LDV1000_BOOL bEnabled);

// end callbacks

#ifdef __cplusplus
}
#endif // c++

#endif
