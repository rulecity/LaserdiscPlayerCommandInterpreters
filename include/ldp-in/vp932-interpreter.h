#ifndef VP932_INTERPRETER
#define VP932_INTERPRETER

#ifdef __cplusplus
extern "C"
{
#endif // C++

#include "datatypes.h"

typedef enum
{
	VP932_FALSE = 0,
	VP932_TRUE = 1
} VP932_BOOL;

void vp932i_reset();

// writes a byte to LDP
void vp932i_write(uint8_t u8Byte);

// returns non-zero if LDP has a byte to be read
VP932_BOOL vp932i_can_read();

// reads a byte from LDP; must call vp932i_can_read first to determine if byte is available to be read
uint8_t vp932i_read();

typedef enum
{  
   VP932_ERROR, VP932_SEARCHING, VP932_STOPPED, VP932_PLAYING, VP932_PAUSED, VP932_SPINNING_UP
}
VP932Status_t;

// Should be called after vblank has started and the new VBI data has been read, but before vblank has ended.
// This is to handle things like seeks completing.
// Status is the current laserdisc player status which only changes at most every vblank, so it's efficient to pass it in here.
void vp932i_think_during_vblank(VP932Status_t status);

////////////////////////////////////////////////////////////////////////////

extern void (*g_vp932i_play)(uint8_t u8Numerator, uint8_t u8Denominator, VP932_BOOL bBackward, VP932_BOOL bAudioSquelched);

extern void (*g_vp932i_step)(VP932_BOOL bBackward);

extern void (*g_vp932i_pause)();

extern void (*g_vp932i_begin_search)(uint32_t u32FrameNum);

// enables/disables left or right audio channels
extern void (*g_vp932i_change_audio)(uint8_t u8Channel, uint8_t uEnable);

// returns current frame number
extern uint32_t (*g_vp932i_get_cur_frame_num)();

typedef enum
{
	VP932_ERR_UNKNOWN_CMD_BYTE,
	VP932_ERR_UNSUPPORTED_CMD_BYTE,
	VP932_ERR_RX_BUF_OVERFLOW,
	VP932_ERR_UNHANDLED_SITUATION
} VP932ErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_vp932i_error)(VP932ErrCode_t code, uint8_t u8Val);

#ifdef __cplusplus
}
#endif // C++

#endif // VP932_INTERPRETER
