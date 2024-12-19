#ifndef LDP_IN_LD700_INTERPRETER_H
#define LDP_IN_LD700_INTERPRETER_H

#include "datatypes.h"

#ifdef __cplusplus
extern "C"
{
#endif // C++

typedef enum
{
	LD700_FALSE = 0,
	LD700_TRUE = 1
} LD700_BOOL;

// resets all globals to default state
void ld700i_reset();

// sends control byte (for example, 0xA8 0x17 would be PLAY)
void ld700i_write(uint8_t u8Cmd);

// call for every vblank (helps us with timing)
void ld700i_on_vblank();

// call every time you want EXT_ACK' to be a certain value.  The method will track if it's changed and trigger the callback if needed.
void ld700i_change_ext_ack(LD700_BOOL bActive);

///////////////////////////////////////////////////////////////////////////////

typedef enum
{
   LD700_ERROR, LD700_SEARCHING, LD700_STOPPED, LD700_PLAYING, LD700_PAUSED, LD700_SPINNING_UP, LD700_TRAY_EJECTED
}
LD700Status_t;

// CALLBACKS

// plays the laserdisc (if tray is ejected, this loads the disc and spins up.  If disc is stopped, this spins up the disc)
extern void (*g_ld700i_play)();

// pauses the laserdisc
extern void (*g_ld700i_pause)();

// stops the laserdisc
extern void (*g_ld700i_stop)();

// ejects the laserdisc
extern void (*g_ld700i_eject)();

// steps forward or backward (1 for forward, -1 for backward)
extern void (*g_ld700i_step)(int8_t i8TracksToStep);

// begins searching to a frame (non-blocking, should return immediately).
extern void (*g_ld700i_begin_search)(uint32_t uFrameNumber);

// enables/disables left or right audio channels
extern void (*g_ld700i_change_audio)(uint8_t uChannel, LD700_BOOL bEnable);

// returns current status of laserdisc player (playing, paused, etc..)
extern LD700Status_t (*g_ld700i_get_status)();

// will get called every time the EXT ACK line changes. This line is active low, so bActive=true means the line has gone low.
extern void (*g_ld700i_on_ext_ack_changed)(LD700_BOOL bActive);

typedef enum
{
	LD700_ERR_UNKNOWN_CMD_BYTE,
	LD700_ERR_UNSUPPORTED_CMD_BYTE,
	LD700_ERR_TOO_MANY_DIGITS,
	LD700_ERR_UNHANDLED_SITUATION,
	LD700_ERR_CORRUPT_INPUT
} LD700ErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_ld700i_error)(LD700ErrCode_t code, uint8_t u8Val);

// end callbacks

#ifdef __cplusplus
}
#endif // c++

#endif //LDP_IN_LD700_INTERPRETER_H
