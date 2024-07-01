#ifndef PR8210_INTERPRETER_H
#define PR8210_INTERPRETER_H

#include "datatypes.h"

#ifdef __cplusplus
extern "C"
{
#endif // C++

typedef enum
{
	PR8210_FALSE = 0,
	PR8210_TRUE = 1
} PR8210_BOOL;

// resets all globals to default state
void pr8210i_reset();

// Send bits to the PR8210 (based on pulse intervals).
// Intervals are not handled within this interpreter because they are based off of specific input (emulated cpu cycle count or elapsed time) and would caused confusion and hurt performance to implement here.
// Timeout means the interval was so long that it is treated as the beginning of a new sequence of bits.
// 0 means a 0 bit shifted in, 1 means a 1 bit shifted in.
// UPDATE: commented out for now because this would involve fixing the Mach3 driver to use these which I don't want to do right now
//void pr8210i_bit_timeout();
//void pr8210i_bit_0();
//void pr8210i_bit_1();

// u16Msg is a 10-bit value
// value was shifted down as each bit came in, newest bit was added as the most significant position (bit 9)
void pr8210i_write(uint16_t u16Msg);

// PR-8210A only, used to skip tracks
void pr8210i_on_jmp_trigger_changed(PR8210_BOOL bJmpTrigRaised, PR8210_BOOL bScanCRaised);

// PR-8210A only, used to enable/disable auto track jump
void pr8210i_on_jmptrig_and_scanc_intext_changed(PR8210_BOOL bInternal);

// PR-8210A only, used as a time reference for the "stand by" line pulse
void pr8210i_on_vblank();

// CALLBACKS

// plays the laserdisc
extern void (*g_pr8210i_play)();

// pauses the laserdisc
extern void (*g_pr8210i_pause)();

// steps forward or backward (1 for forward, -1 for backward)
extern void (*g_pr8210i_step)(int8_t i8TracksToStep);

// begins searching to a frame (non-blocking, should return immediately).
extern void (*g_pr8210i_begin_search)(uint32_t uFrameNumber);

// enables/disables left or right audio channels
extern void (*g_pr8210i_change_audio)(uint8_t uChannel, uint8_t uEnable);

// skips forward or backward (PR-8210A only)
extern void (*g_pr8210i_skip)(int8_t i8TracksToSkip);

// enables/disables auto track jump (PR-8210A only)
extern void (*g_pr8210i_change_auto_track_jump)(PR8210_BOOL bAutoTrackJumpEnabled);

// returns true if player is spinning up or in the middle of a seek
extern PR8210_BOOL (*g_pr8210i_is_player_busy)();

// enables/disables stand-by pin/LED on PR-8210/A
extern void (*g_pr8210i_change_standby)(PR8210_BOOL bRaised);

// enables/disables VIDEO SQUELCH' on PR-8210/A
extern void (*g_pr8210i_change_video_squelch)(PR8210_BOOL bRaised);

typedef enum
{
	PR8210_ERR_UNKNOWN_CMD_BYTE,
	PR8210_ERR_UNSUPPORTED_CMD_BYTE,
	PR8210_ERR_TOO_MANY_DIGITS,
	PR8210_ERR_UNHANDLED_SITUATION,
	PR8210_ERR_CORRUPT_INPUT
} PR8210ErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_pr8210i_error)(PR8210ErrCode_t code, uint16_t u16Val);

// end callbacks

#ifdef __cplusplus
}
#endif // c++

#endif // PR8210_INTERPRETER_H
