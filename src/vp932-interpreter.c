#include <ldp-in/vp932-interpreter.h>
#include <string.h>
#include <assert.h>

//////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

void (*g_vp932i_play)(uint8_t u8Numerator, uint8_t u8Denominator, VP932_BOOL bBackward, VP932_BOOL bAudioSquelched) = 0;
void (*g_vp932i_step)(VP932_BOOL bBackward) = 0;
void (*g_vp932i_pause)() = 0;
void (*g_vp932i_begin_search)(uint32_t u32FrameNum) = 0;
void (*g_vp932i_skip)(int32_t i32TracksToSkip) = 0;
void (*g_vp932i_change_audio)(uint8_t uChannel, uint8_t uEnable) = 0;
uint32_t (*g_vp932i_get_cur_frame_num)() = 0;
VP932_BOOL (*g_vp932i_get_dsr)() = 0;
void (*g_vp932i_set_rts)(VP932_BOOL) = 0;

void (*g_vp932i_error)(VP932ErrCode_t code, uint8_t u8Val) = 0;

/////////////////////////////////

typedef enum
{
	VP932_STATE_NORMAL = 0,
	VP932_STATE_SEARCHING,	// in the middle of a disc search
} VP932State_t;

VP932State_t g_vp932i_state = VP932_STATE_NORMAL;

// whether to play the disc after a search is complete
VP932_BOOL g_vp932i_play_after_search = VP932_FALSE;

uint8_t g_vp932i_tx_buf[12];	// this should be small enough so as to not waste space but big enough to hold things like current frame number
uint8_t *g_vp932i_pTxBufStart = 0;
uint8_t *g_vp932i_pTxBufEnd = 0;
const uint8_t *g_vp932i_pTxBufLastGoodPtr = (g_vp932i_tx_buf + sizeof(g_vp932i_tx_buf) - 1);
uint8_t g_vp932i_u8TxBufCount = 0;	// how many bytes are in the tx buffer
uint16_t g_vp932i_u16LastFrameNumberSearched = 0;	// last frame number we searched for

#define VP932I_TX_WRAP(var) 	if (var > g_vp932i_pTxBufLastGoodPtr) var = g_vp932i_tx_buf;

uint8_t g_vp932i_rx_buf[12];	// should be as small as possible to save space
uint8_t g_vp932i_rx_buf_idx = 0;	// current position of rx buf (0 means buffer is empty)

//////////////////////////////////

void vp932i_reset()
{
	g_vp932i_state = VP932_STATE_NORMAL;
	g_vp932i_play_after_search = VP932_FALSE;
	g_vp932i_pTxBufStart = g_vp932i_pTxBufEnd = g_vp932i_tx_buf;
	g_vp932i_u8TxBufCount = 0;
	g_vp932i_u16LastFrameNumberSearched = 0;
}

void vp932i_push_tx_queue(uint8_t u8Val)
{
	*g_vp932i_pTxBufEnd = u8Val;
	g_vp932i_pTxBufEnd++;
	VP932I_TX_WRAP(g_vp932i_pTxBufEnd);
	g_vp932i_u8TxBufCount++;

	// we should never get close to this
//	assert(g_vp932i_u8TxBufCount < sizeof(g_vp932i_tx_buf));
}

uint8_t vp932i_pop_tx_queue()
{
	uint8_t u8Res = *g_vp932i_pTxBufStart;
	g_vp932i_pTxBufStart++;
	VP932I_TX_WRAP(g_vp932i_pTxBufStart);
	g_vp932i_u8TxBufCount--;

	// sanity check
//	assert(g_vp932i_u8TxBufCount != -1);

	return u8Res;
}

void vp932i_process_rx_buf()
{
	uint8_t u8Idx = 0;
	uint8_t u8Val = 0;
	VP932_BOOL bSearchCmdActive = VP932_FALSE;
	uint16_t u16Number = 0;	// number, such as a frame number

	// go until we get to the end of the buffer
	while (u8Idx < g_vp932i_rx_buf_idx)
	{
		u8Val = g_vp932i_rx_buf[u8Idx++];

		switch (u8Val)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			u16Number *= 10;
			u16Number += (u8Val & 0x0F);
			break;
		case 'D':	// display disable/enable

			// Dexter doesn't support frame number display, so we just ignore this command
			break;

		case 'F':	// prepare to search
			u16Number = 0;	// number is expected to come next, so we reset it to prepare
			bSearchCmdActive = VP932_TRUE;
			break;

		case 'L':	// step forward
			g_vp932i_step(VP932_FALSE);
			break;

		case 'M':	// step reverse
			g_vp932i_step(VP932_TRUE);
			break;

		case 'N':	// complete search, then play
			if (bSearchCmdActive)
			{
				// if they try to search to the frame that we're already on, then just play
				// (DL Euro does this for every scene)
				if (g_vp932i_u16LastFrameNumberSearched != u16Number)
				{
					g_vp932i_u16LastFrameNumberSearched = u16Number;
					g_vp932i_begin_search(u16Number);
				}
				// else we don't initiate a new search, but we still want to return the expected status code so we pretend like we are searching

				g_vp932i_play_after_search = VP932_TRUE;
				g_vp932i_state = VP932_STATE_SEARCHING;

			}

			bSearchCmdActive = VP932_FALSE;
			break;
		case 'R':	// complete search, stay paused

			// to workaround weird DL Euro behavior upon boot-up
//			if (bSearchCmdActive)
			{
				// if they try to search to the frame that we're already on, then just ignore
				// (Sidam DL does this for diagnostics mode)
				if (g_vp932i_u16LastFrameNumberSearched != u16Number)
				{
					g_vp932i_u16LastFrameNumberSearched = u16Number;
					g_vp932i_begin_search(u16Number);
				}
				// else we don't initiate a new search, but we still want to return the expected status code so we pretend like we are searching

				g_vp932i_play_after_search = VP932_FALSE;
				g_vp932i_state = VP932_STATE_SEARCHING;
			}
			bSearchCmdActive = VP932_FALSE;
			break;
		case 'S':	// set multispeed playback speed (just a guess!)

			// DL Euro sends "S002" which we assume means 1X for multi-speed playback mode.
			// We can ignore this command since DL Euro does not require any true multi-speed playback.

			break;

		case 'U':	// initiate multi-speed playback with audio muted

			g_vp932i_u16LastFrameNumberSearched = 0;	// once we play, this check no longer applies

			// DL Euro does not change multi-speed playback speed (to our knowledge) so we hard-code 1/1
			g_vp932i_play(1, 1, VP932_FALSE, VP932_TRUE);

			break;

		case 'V':	// initiate multi-speed playback, reverse

			g_vp932i_u16LastFrameNumberSearched = 0;	// once we play, this check no longer applies

			// DL Euro does not change multi-speed playback speed (to our knowledge) so we hard-code 1/1
			g_vp932i_play(1, 1, VP932_TRUE, VP932_TRUE);

			break;

		case 'X':	// clear
			u16Number = 0;
			bSearchCmdActive = VP932_FALSE;
			break;
		case '*':	// pause
			g_vp932i_pause();
			break;
		default:
			g_vp932i_error(VP932_ERR_UNKNOWN_CMD_BYTE, u8Val);
			break;
		}
	}

	// now that buffer is processed, we need to empty it to prepare to receive the next buffer
	g_vp932i_rx_buf_idx = 0;
}

void vp932i_write(uint8_t u8Byte)
{
	switch (u8Byte)
	{
	case 0x00:	// behaves like clear (undocumented, inferred from observed behavior)
		g_vp932i_rx_buf_idx = 0;		
		break;
	case 0x0D:	// carriage return (end of command)
		vp932i_process_rx_buf();
		break;

	default:

		if (g_vp932i_rx_buf_idx < sizeof(g_vp932i_rx_buf))
		{
			g_vp932i_rx_buf[g_vp932i_rx_buf_idx++] = u8Byte;
		}
		// else we've overflowed our buffer; we either need to make it bigger or we probably have a bug
		else
		{
			g_vp932i_error(VP932_ERR_RX_BUF_OVERFLOW,

				// nothing meaningful to put for the value so just put 0
				0);
		}

		break;
	}
}

VP932_BOOL vp932i_can_read()
{
	return (VP932_BOOL) (g_vp932i_u8TxBufCount != 0);
}

uint8_t vp932i_read()
{
//	assert(g_vp932i_u8TxBufCount > 0);
	return vp932i_pop_tx_queue();
}

void vp932i_think_during_vblank(VP932Status_t status)
{
	// if we're in the middle of a search
	if (g_vp932i_state == VP932_STATE_SEARCHING)
	{
		switch (status)
		{
		case VP932_PAUSED:
			if (g_vp932i_play_after_search == VP932_TRUE)
			{
				// A1 to be returned after successful search+play
				vp932i_push_tx_queue('A');
				vp932i_push_tx_queue('1');
				vp932i_push_tx_queue('\r');

				g_vp932i_u16LastFrameNumberSearched = 0;	// once we play, this check no longer applies

				g_vp932i_play(1, 1, VP932_FALSE, VP932_FALSE);
			}
			else
			{
				// A0 to be returned after successful search
				vp932i_push_tx_queue('A');
				vp932i_push_tx_queue('0');
				vp932i_push_tx_queue('\r');
			}
			g_vp932i_state = VP932_STATE_NORMAL;	// search is done, we're back to normal
			break;
		case VP932_SEARCHING:
			// if we're still searching, nothing to do
			break;
		default:
			// we're in an unknown state, so send an error
			// TODO
			break;
		}
	}
}

