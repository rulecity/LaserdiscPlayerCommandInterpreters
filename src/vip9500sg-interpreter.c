#include <ldp-in/vip9500sg-interpreter.h>
#include <string.h>
#include <assert.h>

//////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

void (*g_vip9500sgi_play)() = 0;
void (*g_vip9500sgi_pause)() = 0;
void (*g_vip9500sgi_stop)() = 0;
void (*g_vip9500sgi_step_reverse)() = 0;
void (*g_vip9500sgi_begin_search)(uint32_t u32FrameNum) = 0;
void (*g_vip9500sgi_skip)(int32_t i32TracksToSkip) = 0;
void (*g_vip9500sgi_change_audio)(uint8_t uChannel, uint8_t uEnable) = 0;
VIP9500SGStatus_t (*g_vip9500sgi_get_status)() = 0;
uint32_t (*g_vip9500sgi_get_cur_frame_num)() = 0;
uint32_t (*g_vip9500sgi_get_cur_vbi_line18)() = 0;

void (*g_vip9500sgi_error)(VIP9500SGErrCode_t code, uint8_t u8Val) = 0;

/////////////////////////////////

// so we know what to do when we get an ENTER command
typedef enum
{
	VIP9500SGI_STATE_NORMAL,
	VIP9500SGI_STATE_WAIT_SEARCH,	// in the middle of a search command
	VIP9500SGI_STATE_SEARCHING,	// in the middle of a search
	VIP9500SGI_STATE_WAIT_SKIP_FORWARD,	// in the middle of a skip command
	VIP9500SGI_STATE_WAIT_SKIP_BACKWARD,	// in the middle of a skip command
	VIP9500SGI_STATE_WAITING_FOR_PLAYING,	// waiting for disc to be playing
	VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED,	// waiting for disc to be playing/paused

} VIP9500SGState_t;

VIP9500SGState_t g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;

// if true, we'll return picture number next time we see one in VBI
VIP9500SG_BOOL g_vip9500sgi_waitingForPicNum = VIP9500SG_FALSE;

///////////////////////////////////////////////////////////////////////////////

uint8_t g_vip9500sgi_tx_buf[12];	// this should be small enough so as to not waste space but big enough to hold things like current frame number
uint8_t *g_vip9500sgi_pTxBufStart = 0;
uint8_t *g_vip9500sgi_pTxBufEnd = 0;
const uint8_t *g_vip9500sgi_pTxBufLastGoodPtr = (g_vip9500sgi_tx_buf + sizeof(g_vip9500sgi_tx_buf) - 1);
uint8_t g_vip9500sgi_u8TxBufCount = 0;	// how many bytes are in the tx buffer

#define VIP9500SGI_TX_WRAP(var) 	if (var > g_vip9500sgi_pTxBufLastGoodPtr) var = g_vip9500sgi_tx_buf;

///////////////////////////////////////////////////////////////////////////////

uint8_t g_vip9500sgi_num_buf[5];	// holds currently entered in number (extra digits are discarded)
uint8_t *g_vip9500sgi_pNumBufStart = 0;
uint8_t *g_vip9500sgi_pNumBufEnd = 0;
const uint8_t *g_vip9500sgi_pNumBufLastGoodPtr = (g_vip9500sgi_num_buf + sizeof(g_vip9500sgi_num_buf) - 1);
uint8_t g_vip9500sgi_u8NumBufCount = 0;	// how many bytes are in the number buffer

#define VIP9500SGI_NUM_WRAP(var) 	if (var > g_vip9500sgi_pNumBufLastGoodPtr) var = g_vip9500sgi_num_buf;

///////////////////////////////////////////////////////////////////////////////

// current frame entered in for stuff like searching, repeating, etc
uint32_t g_vip9500sgi_u32Frame = 0;

// general purpose index
uint8_t g_vip9500sgi_u8Idx = 0;

// so our post-vblank handler knows what success byte to return
uint8_t g_vip9500sgi_u8LastCmdByte = 0;

#define VIP9500SGI_RESET_FRAME()	g_vip9500sgi_pNumBufStart = g_vip9500sgi_pNumBufEnd = g_vip9500sgi_num_buf; g_vip9500sgi_u8NumBufCount = 0

//////////////////////////////////

void vip9500sgi_reset()
{
	g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;

	g_vip9500sgi_pTxBufStart = g_vip9500sgi_pTxBufEnd = g_vip9500sgi_tx_buf;
	g_vip9500sgi_u8TxBufCount = 0;

	VIP9500SGI_RESET_FRAME();

	g_vip9500sgi_u32Frame = 0;
	g_vip9500sgi_u8Idx = 0;
}

void vip9500sgi_push_queue(uint8_t u8Val)
{
	*g_vip9500sgi_pTxBufEnd = u8Val;
	g_vip9500sgi_pTxBufEnd++;
	VIP9500SGI_TX_WRAP(g_vip9500sgi_pTxBufEnd);
	g_vip9500sgi_u8TxBufCount++;

	// we should never get close to this
	assert(g_vip9500sgi_u8TxBufCount < sizeof(g_vip9500sgi_tx_buf));
}

uint8_t vip9500sgi_pop_queue()
{
	uint8_t u8Res = *g_vip9500sgi_pTxBufStart;
	g_vip9500sgi_pTxBufStart++;
	VIP9500SGI_TX_WRAP(g_vip9500sgi_pTxBufStart);
	g_vip9500sgi_u8TxBufCount--;

	// sanity check
	assert(g_vip9500sgi_u8TxBufCount != -1);

	return u8Res;
}

void vip9500sgi_add_digit(uint8_t u8Digit)
{
	// make sure we don't overflow
	assert(g_vip9500sgi_pNumBufEnd <= g_vip9500sgi_pNumBufLastGoodPtr);

	*g_vip9500sgi_pNumBufEnd = u8Digit;
	g_vip9500sgi_pNumBufEnd++;
	VIP9500SGI_NUM_WRAP(g_vip9500sgi_pNumBufEnd);

	// buffer cannot have more than 5 digits.  oldest digits get discarded.  Tested on a real player.
	if (g_vip9500sgi_u8NumBufCount < 5)
	{
		g_vip9500sgi_u8NumBufCount++;
	}
	else
	{
		g_vip9500sgi_pNumBufStart++;
		VIP9500SGI_NUM_WRAP(g_vip9500sgi_pNumBufStart);
	}

	// we should never overflow
	assert(g_vip9500sgi_u8NumBufCount <= sizeof(g_vip9500sgi_num_buf));
}

// converts array into integer and stores it in g_vip9500sgi_u32Frame
void vip9500sgi_process_number()
{
	g_vip9500sgi_u32Frame = 0;

	while (g_vip9500sgi_u8NumBufCount > 0)
	{
		g_vip9500sgi_u32Frame *= 10;
		g_vip9500sgi_u32Frame += ((*g_vip9500sgi_pNumBufStart) & 0xF);
		g_vip9500sgi_pNumBufStart++;
		VIP9500SGI_NUM_WRAP(g_vip9500sgi_pNumBufStart);
		g_vip9500sgi_u8NumBufCount--;
	}
}

void vip9500sgi_write(uint8_t u8Byte)
{
	uint8_t u8SuccessByte = u8Byte | 0x80;	// general purpose success

	// we don't want to overwrite our command with the 'enter' byte or digits
	if ((u8Byte != 0x41) && (u8Byte != 0x6B) && ((u8Byte & 0xF0) != 0x30))
	{
		g_vip9500sgi_u8LastCmdByte = u8Byte;
	}

	switch (u8Byte)
	{
	case 0x24:	// pause
		g_vip9500sgi_pause();

		// I've observed that most commands have a delay associated with them.  I'm _guessing_ that the pause command also does, but don't have proof.
		g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED;
		break;
	case 0x25:	// play
		g_vip9500sgi_play();
		g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING;	// spin-up, etc.
		break;
	case 0x29:	// step reverse
		// Astron, GR, and Cobra Command only seem to use this for pause
		g_vip9500sgi_step_reverse();
		g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED;	// we go into stepping state and are done once we reach the paused state
		break;
	case 0x2b:	// begin search
		g_vip9500sgi_state = VIP9500SGI_STATE_WAIT_SEARCH;
		VIP9500SGI_RESET_FRAME();
		break;
	case 0x2f:	// stop
		g_vip9500sgi_stop();
		vip9500sgi_push_queue(u8SuccessByte);
		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
		vip9500sgi_add_digit(u8Byte);
		break;
	case 0x41: // Enter
		vip9500sgi_process_number();
		switch (g_vip9500sgi_state)
		{
		case VIP9500SGI_STATE_WAIT_SEARCH:
			g_vip9500sgi_begin_search(g_vip9500sgi_u32Frame);
			g_vip9500sgi_state = VIP9500SGI_STATE_SEARCHING;
			vip9500sgi_push_queue(0x41); // acknowledge that we will search
			break;
		case VIP9500SGI_STATE_WAIT_SKIP_FORWARD:
			g_vip9500sgi_skip((int32_t) g_vip9500sgi_u32Frame +1);	// +1 due to quirk of the LDP
			g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED;
			vip9500sgi_push_queue(0x41); // acknowledge that we will skip
			break;
		case VIP9500SGI_STATE_WAIT_SKIP_BACKWARD:
			g_vip9500sgi_skip( (-((int32_t) g_vip9500sgi_u32Frame)) + 1);	// +1 due to quirk of the LDP
			g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED;
			vip9500sgi_push_queue(0x41); // acknowledge that we will skip
			break;
		default:
			g_vip9500sgi_error(VIP9500SG_ERR_UNKNOWN_CMD_BYTE, u8Byte);
			break;
		}
		break;
	case 0x46:	// prepare to skip forward
		g_vip9500sgi_state = VIP9500SGI_STATE_WAIT_SKIP_FORWARD;
		VIP9500SGI_RESET_FRAME();
		break;
	case 0x47:	// prepare to skip backward
		g_vip9500sgi_state = VIP9500SGI_STATE_WAIT_SKIP_BACKWARD;
		VIP9500SGI_RESET_FRAME();
		break;
	case 0x53:	// Play forward at 1X with sound enabled, note that if disc is stopped this will return an error 0x1D
		g_vip9500sgi_play();

		// real LDP has some delay when responding this command.
		g_vip9500sgi_state = VIP9500SGI_STATE_WAITING_FOR_PLAYING;
		break;

	case 0x68:	// reset
		vip9500sgi_reset();
		vip9500sgi_push_queue(u8SuccessByte);
		break;

	case 0x6b:	// get current frame
		// the real LDP has some delay when responding to this command.  I suspect it waits until the next picture number is decoded.
		g_vip9500sgi_waitingForPicNum = VIP9500SG_TRUE;
		break;

		// STUBS: return success but don't actually do anything
	case 0x4c:	// frame counter on
	case 0x4d:	// frame counter off
	case 0x6e:	// unknown
	case 0x71:	// turn on response
	case 0x75:	// some reset function, I found it in hitachi.cpp but don't know what else it does
		vip9500sgi_push_queue(u8SuccessByte);
		break;

		// STUBS: stuff we don't support but maybe need to if a game uses it
	case 0x3a:	// clear digits
	case 0x48:	// enable left audio
	case 0x49:	// disable left audio
	case 0x4a:	// enable right audio
	case 0x4b:	// disable right audio
		g_vip9500sgi_error(VIP9500SG_ERR_UNSUPPORTED_CMD_BYTE, u8Byte);
		vip9500sgi_push_queue(u8SuccessByte);
		break;


	default:
		g_vip9500sgi_error(VIP9500SG_ERR_UNKNOWN_CMD_BYTE, u8Byte);
		break;
	}
}

VIP9500SG_BOOL vip9500sgi_can_read()
{
	return (VIP9500SG_BOOL) (g_vip9500sgi_u8TxBufCount != 0);
}

uint8_t vip9500sgi_read()
{
	assert(g_vip9500sgi_u8TxBufCount > 0);
	return vip9500sgi_pop_queue();
}

void vip9500sgi_think_picnum_query(VIP9500SGStatus_t stat)
{
	// if picture number will be valid
	if ((stat == VIP9500SG_PAUSED) || (stat == VIP9500SG_PLAYING))
	{
		uint32_t line18 = g_vip9500sgi_get_cur_vbi_line18();

		// if this field contains a picture number, then we're done
		// The real player has some delay before returning a result for the current picture number query.
		// I am _guessing_ that it waits for the next picture number to be decoded in VBI.
		if (((line18 >> 16) & 0xF0) == 0xF0)
		{
			uint32_t curframe = g_vip9500sgi_get_cur_frame_num();
			vip9500sgi_push_queue(0x6b); // frame response
			vip9500sgi_push_queue((uint8_t) ((curframe >> 8) & 0xff)); // high byte of frame
			vip9500sgi_push_queue((uint8_t) (curframe & 0xff)); // low byte of frame

			g_vip9500sgi_waitingForPicNum = VIP9500SG_FALSE;
		}
		// else wait for the next field
	}
	else
	{
		// if picture number is requested during spin-up, a real player returns an error.
		// This is a good default for all other conditions for now.
		vip9500sgi_push_queue(0x1d); // error code from real player

		g_vip9500sgi_waitingForPicNum = VIP9500SG_FALSE;
	}

}

void vip9500sgi_think_after_vblank()
{
	VIP9500SGStatus_t stat = g_vip9500sgi_get_status();

	switch (g_vip9500sgi_state)
	{
		// nothing to do
	case VIP9500SGI_STATE_NORMAL:
	case VIP9500SGI_STATE_WAIT_SEARCH:
	case VIP9500SGI_STATE_WAIT_SKIP_FORWARD:
	case VIP9500SGI_STATE_WAIT_SKIP_BACKWARD:
		if (g_vip9500sgi_waitingForPicNum)
		{
			vip9500sgi_think_picnum_query(stat);
		}
		break;
	// if we are in the middle of a search
	case VIP9500SGI_STATE_SEARCHING:
		{
			switch (stat)
			{
				// if search is complete
			case VIP9500SG_PAUSED:
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				vip9500sgi_push_queue(0xb0);	// search complete
				break;
				// if we're still working, do nothing
			case VIP9500SG_SEARCHING:
				break;
			default:
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				vip9500sgi_push_queue(0x1d);	// error code confirmed on a real player
				break;
			}
		}
		break;
	case VIP9500SGI_STATE_WAITING_FOR_PLAYING:
		{
			switch (stat)
			{
			case VIP9500SG_PLAYING:
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				vip9500sgi_push_queue(g_vip9500sgi_u8LastCmdByte | 0x80);	// success!
				break;
				// if we're still working, keep waiting
			case VIP9500SG_SPINNING_UP:
				break;
			default:
				g_vip9500sgi_error(VIP9500SG_ERR_UNHANDLED_SITUATION, stat);
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				break;
			}

			if (g_vip9500sgi_waitingForPicNum)
			{
				vip9500sgi_think_picnum_query(stat);
			}
		}
		break;
	case VIP9500SGI_STATE_WAITING_FOR_PLAYING_OR_PAUSED:
		{
			switch (stat)
			{
			case VIP9500SG_PLAYING:
			case VIP9500SG_PAUSED:
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				vip9500sgi_push_queue(g_vip9500sgi_u8LastCmdByte | 0x80);	// success!
				break;
				// if we're still working, keep waiting
			case VIP9500SG_STEPPING:
				break;
			default:
				g_vip9500sgi_error(VIP9500SG_ERR_UNHANDLED_SITUATION, stat);
				g_vip9500sgi_state = VIP9500SGI_STATE_NORMAL;
				break;
			}
		}
		break;
	default:	// unhandled state which we need to handle
		g_vip9500sgi_error(VIP9500SG_ERR_UNHANDLED_SITUATION, 0);
		break;
	}
}
