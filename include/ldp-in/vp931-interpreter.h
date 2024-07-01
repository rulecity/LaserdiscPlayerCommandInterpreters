#ifndef VP931_INTERPRETER_H
#define VP931_INTERPRETER_H

#ifdef __cplusplus
extern "C"
{
#endif // C++

#include "datatypes.h"

typedef enum
{
	VP931_FALSE = 0,
	VP931_TRUE = 1
} VP931_BOOL;

typedef enum
{  
   VP931_ERROR, VP931_SEARCHING, VP931_PLAYING, VP931_PAUSED, VP931_SPINNING_UP
}
VP931Status_t;

// must be called every vsync (this is when pending commands are executed)
void vp931i_on_vsync(const uint8_t *p8CmdBuf, uint8_t u8CmdBytesRecvd, VP931Status_t status);

void vp931i_reset();

// returns the 6 status bytes to send at the beginning of a field
void vp931i_get_status_bytes(uint32_t u32VBILine18, VP931Status_t status, uint8_t *pDstBuffer);

/////////////////////////////////////////

// CALLBACKS

// plays the laserdisc
extern void (*g_vp931i_play)();

// pauses the laserdisc
extern void (*g_vp931i_pause)();

// begins searching to a frame (non-blocking, should return immediately).
// 'bAudioSquelchedOnComplete' true means normal behavior, false means that the audio should keep playing even when the disc is paused
extern void (*g_vp931i_begin_search)(uint32_t uFrameNumber, VP931_BOOL bAudioSquelchedOnComplete);

// skips tracks forward or backward
extern void (*g_vp931i_skip_tracks)(int16_t i16TracksToSkip);

// skips to a target picture number
extern void (*g_vp931i_skip_to_framenum)(uint32_t uFrameNumber);

typedef enum
{
	VP931_ERR_TOO_MANY_WRITES,
	VP931_ERR_UNKNOWN_CMD_BYTE,
	VP931_ERR_UNSUPPORTED_CMD_BYTE
}
VP931ErrCode_t;

// gets called by interpreter on error.  the code is an enum while the value relates to the error code (for example the value could be an unknown command byte)
extern void (*g_vp931i_error)(VP931ErrCode_t code, uint8_t u8Val);

#ifdef __cplusplus
}
#endif // C++

#endif // VP931_INTERPRETER_H
