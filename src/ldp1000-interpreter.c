#include <ldp-in/ldp1000-interpreter.h>
#include <string.h>
#include <assert.h>

//////////////////

// private methods:

void ldp1000i_repeat_play();

//////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

void (*g_ldp1000i_play)(uint8_t u8Numerator, uint8_t u8Denominator, LDP1000_BOOL bBackward, LDP1000_BOOL bAudioSquelched) = 0;
void (*g_ldp1000i_pause)() = 0;
void (*g_ldp1000i_begin_search)(uint32_t u32FrameNum) = 0;
void (*g_ldp1000i_step_forward)() = 0;
void (*g_ldp1000i_step_reverse)() = 0;
void (*g_ldp1000i_skip)(int16_t i16TracksToSkip) = 0;
void (*g_ldp1000i_change_audio)(uint8_t uChannel, uint8_t uEnable) = 0;
void (*g_ldp1000i_change_video)(LDP1000_BOOL bEnable) = 0;
LDP1000Status_t (*g_ldp1000i_get_status)() = 0;
uint32_t (*g_ldp1000i_get_cur_frame_num)() = 0;

void (*g_ldp1000i_text_enable_changed)(LDP1000_BOOL bEnabled) = 0;
void (*g_ldp1000i_text_buffer_contents_changed)(const uint8_t *p8Buf32Bytes) = 0;
void (*g_ldp1000i_text_buffer_start_index_changed)(uint8_t u8StartIdx) = 0;
void (*g_ldp1000i_text_modes_changed)(uint8_t u8Mode, uint8_t u8X, uint8_t u8Y) = 0;

void (*g_ldp1000i_add_latency)(LDP1000Latency_t latency) = 0;
void (*g_ldp1000i_error)(LDP1000ErrCode_t code, uint8_t u8Val) = 0;

/////////////////////////////////

// so we know what to do when we get an ENTER command
typedef enum
{
	LDP1000I_STATE_NORMAL,
	LDP1000I_STATE_WAIT_SEARCH,	// in the middle of a search command
	LDP1000I_STATE_REPEAT0_WAIT_END_FRAME,		// repeat command received, waiting for end frame number
	LDP1000I_STATE_REPEAT1_WAIT_COUNT,		// repeat end of frame number received, waiting for iteration count
    LDP1000I_STATE_WAIT_VARIABLE_SPEED, // in the middle of a variable speed play command
	LDP1000I_STATE_SKIP_FORWARD,	// in the middle of skip forward command
	LDP1000I_STATE_SKIP_BACKWARD,	// in the middle of skip backward command
} LDP1000State_t;

LDP1000_EmulationType_t g_ldp1000i_type = (LDP1000_EmulationType_t) 0;

LDP1000State_t g_ldp1000i_state = LDP1000I_STATE_NORMAL;

// these are 16-bit values so that one byte can hold latency information
uint16_t g_ldp1000i_tx_buf[12];	// this should be small enough so as to not waste space but big enough to hold things like current frame number
uint16_t *g_ldp1000i_pTxBufStart = 0;
uint16_t *g_ldp1000i_pTxBufEnd = 0;
const uint16_t *g_ldp1000i_pTxBufLastGoodPtr = (g_ldp1000i_tx_buf + (sizeof(g_ldp1000i_tx_buf)/sizeof(uint16_t)) - 1);
uint8_t g_ldp1000i_u8TxBufCount = 0;	// how many bytes are in the tx buffer

#define LDP1000I_TX_WRAP(var) 	if (var > g_ldp1000i_pTxBufLastGoodPtr) var = g_ldp1000i_tx_buf;

// current frame entered in for stuff like searching, repeating, etc
uint32_t g_ldp1000i_u32Frame = 0;

// which digit we are entering (imagine we are entering into an array)
uint8_t g_ldp1000i_u8FrameIdx = 0;

// general purpose index (u8FrameIdx may be merged into this)
uint8_t g_ldp1000i_u8Idx = 0;

// which direction to go for variable speed play, repeat, etc
LDP1000_BOOL g_ldp1000i_directionIsReversed = LDP1000_FALSE;

// whether disc is in the middle of a search
LDP1000_BOOL g_ldp1000i_bSearchActive = LDP1000_FALSE;

// repeat state
LDP1000_BOOL g_ldp1000i_bRepeatActive = LDP1000_FALSE;
uint32_t g_ldp1000i_u32RepeatStartFrame = 0;
uint32_t g_ldp1000i_u32RepeatEndFrame = 0;
uint8_t g_ldp1000i_u8RepeatIterations = 0;

LDP1000_BOOL g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;
uint8_t g_ldp1000i_u8UICFunction = 0;	// which UIC function is active (u8Idx must be >0 for this value to mean something)
uint8_t g_ldp1000i_u8UIC_X = 0, g_ldp1000i_u8UIC_Y = 0, g_ldp1000i_u8UIC_Mode = 0;
uint8_t g_ldp1000i_u8UIC_Window = 0;
LDP1000_BOOL g_ldp1000i_bUI_Enabled = LDP1000_FALSE;
uint8_t g_ldp1000i_u8UIC_StartIdx = 0;
uint8_t g_ldp1000i_UIC_TextBuf[32] = { 0 };

uint8_t g_ldp1000i_u8UIC_PendingNotifications = 0;
#define LDP1000I_UIC_NOTIFY_MODES (1 << 0)
#define LDP1000I_UIC_NOTIFY_WINDOW (1 << 1)
#define LDP1000I_UIC_NOTIFY_BUFFER (1 << 2)

#define LDP1000I_RESET_FRAME()	g_ldp1000i_u32Frame = 0; g_ldp1000i_u8FrameIdx = 0

// since we will be making these calculations a lot
#define LATVAL_CLEAR	(LDP1000_LATENCY_CLEAR << 8)
#define	LATVAL_NUMBER	(LDP1000_LATENCY_NUMBER << 8)
#define LATVAL_ENTER	(LDP1000_LATENCY_ENTER << 8)
#define	LATVAL_PLAY		(LDP1000_LATENCY_PLAY << 8)
#define LATVAL_STILL	(LDP1000_LATENCY_STILL << 8)
#define LATVAL_INQUIRY	(LDP1000_LATENCY_INQUIRY << 8)
#define LATVAL_GENERIC	(LDP1000_LATENCY_GENERIC << 8)

#define LATACK_CLEAR	(LATVAL_CLEAR | 0x0A)
#define LATACK_NUMBER	(LATVAL_NUMBER | 0x0A)
#define LATACK_ENTER	(LATVAL_ENTER | 0x0A)
#define LATACK_PLAY		(LATVAL_PLAY | 0x0A)
#define LATACK_STILL	(LATVAL_STILL | 0x0A)
#define LATACK_INQUIRY	(LATVAL_INQUIRY | 0x0A)
#define LATACK_GENERIC	(LATVAL_GENERIC | 0x0A)
#define LATNAK_GENERIC	(LATVAL_GENERIC | 0x0B)

//////////////////////////////////

void ldp1000i_reset(LDP1000_EmulationType_t type)
{
	g_ldp1000i_state = LDP1000I_STATE_NORMAL;
	g_ldp1000i_pTxBufStart = g_ldp1000i_pTxBufEnd = g_ldp1000i_tx_buf;
	g_ldp1000i_type = type;
	g_ldp1000i_u32Frame = 0;
	g_ldp1000i_u8FrameIdx = 0;
	g_ldp1000i_u8Idx = 0;
	g_ldp1000i_bSearchActive = LDP1000_FALSE;
	g_ldp1000i_bRepeatActive = LDP1000_FALSE;
	g_ldp1000i_u32RepeatStartFrame = 0;
	g_ldp1000i_u32RepeatEndFrame = 0;
	g_ldp1000i_u8RepeatIterations = 0;
	g_ldp1000i_directionIsReversed = LDP1000_FALSE;
	g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;
	g_ldp1000i_u8UICFunction = 0;
	g_ldp1000i_u8UIC_X = 0xFF;	// so that we will send out a notification if value is set to 0
	g_ldp1000i_u8UIC_Y = 0xFF;	// " " "
	g_ldp1000i_u8UIC_Mode = 0xFF;	// " " "
	g_ldp1000i_u8UIC_Window = 0xFF;	// so that we will send out a notification if the window is set to 0
	g_ldp1000i_bUI_Enabled = LDP1000_FALSE;
	g_ldp1000i_u8UIC_StartIdx = 0;
	memset(g_ldp1000i_UIC_TextBuf, 0, sizeof(g_ldp1000i_UIC_TextBuf));
	g_ldp1000i_u8UIC_PendingNotifications = 0;
}

void ldp1000i_push_queue(uint16_t u16Val)
{
	*g_ldp1000i_pTxBufEnd = u16Val;
	g_ldp1000i_pTxBufEnd++;
	LDP1000I_TX_WRAP(g_ldp1000i_pTxBufEnd);
	g_ldp1000i_u8TxBufCount++;

	// we should never get close to this
//	assert(g_ldp1000i_u8TxBufCount < sizeof(g_ldp1000i_tx_buf));
}

uint16_t ldp1000i_pop_queue()
{
	uint16_t u16Res = *g_ldp1000i_pTxBufStart;
	g_ldp1000i_pTxBufStart++;
	LDP1000I_TX_WRAP(g_ldp1000i_pTxBufStart);
	g_ldp1000i_u8TxBufCount--;

	// sanity check
//	assert(g_ldp1000i_u8TxBufCount != -1);

	return u16Res;
}

void ldp1000i_add_digit(uint8_t u8Digit)
{
	if (g_ldp1000i_u8FrameIdx < 5)
	{
		uint8_t u8Tmp = u8Digit & 0xF;
		g_ldp1000i_u32Frame *= 10;
		g_ldp1000i_u32Frame += u8Tmp;
		g_ldp1000i_u8FrameIdx++;
		ldp1000i_push_queue(LATACK_NUMBER);
	}

	// TODO : test this on a real player to see what it does
	// (WDO 7/16: a real 1450 uses the last 5 digits entered)
	else
	{
		g_ldp1000i_error(LDP1000_ERR_TOO_MANY_DIGITS, 0);
		ldp1000i_push_queue(LATVAL_NUMBER | 0xB);
	}
}

void ldp1000i_write(uint8_t u8Byte)
{
	// if we not in UIC mode, then process incoming bytes normally
	if (!g_ldp1000i_UIC_Input_Active)
	{
		switch (u8Byte)
		{
		case 0x26:	// video off (mute video output)
			g_ldp1000i_change_video(LDP1000_FALSE);
			ldp1000i_push_queue(LATACK_GENERIC);
			break;
		case 0x27:	// video on
			g_ldp1000i_change_video(LDP1000_TRUE);
			ldp1000i_push_queue(LATACK_GENERIC);
			break;
		case 0x2D:	// skip forward
			g_ldp1000i_state = LDP1000I_STATE_SKIP_FORWARD;
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_ENTER);	// this is a guess

			// disc becomes paused as soon as this command is received (this is a guess, the disc arrives at its destination paused, so this is a decent place to put the pause)
			g_ldp1000i_pause();

			break;
		case 0x2E:	// skip backward
			g_ldp1000i_state = LDP1000I_STATE_SKIP_BACKWARD;
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_ENTER);	// this is a guess

			// disc becomes paused as soon as this command is received (this is a guess, the disc arrives at its destination paused, so this is a decent place to put the pause)
			g_ldp1000i_pause();

			break;

		// TO-DO: step/forward reverse is only honored once per vsync (or per frame?)
		//        additional steps are ignored (but ACK is still returned)
		case 0x2B:	// step forward
			g_ldp1000i_step_forward();
			ldp1000i_push_queue(LATACK_STILL);
			break;
		case 0x2C:	// step rev
			g_ldp1000i_step_reverse();
			ldp1000i_push_queue(LATACK_STILL);
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
			ldp1000i_add_digit(u8Byte);
			break;
		case 0x3A:	// play forward at 1X
			g_ldp1000i_play(1, 1, LDP1000_FALSE, LDP1000_FALSE);
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x3B:	// play forward at 3X
			g_ldp1000i_play(3, 1, LDP1000_FALSE, LDP1000_TRUE);
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x3C:	// play forward at 1/5X
			g_ldp1000i_play(1, 5, LDP1000_FALSE, LDP1000_TRUE);
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x3D:	// variable speed forward play
			g_ldp1000i_state = LDP1000I_STATE_WAIT_VARIABLE_SPEED;
			g_ldp1000i_directionIsReversed = LDP1000_FALSE;
			LDP1000I_RESET_FRAME(); // use the frame # buffer for the speed
			ldp1000i_push_queue(LATACK_GENERIC); // not documented

			// for some reason, this command will cause the disc to play forward at 1/7X (confirmed on real hardware, but not documented)
			g_ldp1000i_play(1, 7, LDP1000_FALSE, LDP1000_TRUE);
			break;
		case 0x3F:	// stop
			ldp1000i_push_queue(LATACK_PLAY);	// same as play for stop
			g_ldp1000i_error(LDP1000_ERR_UNSUPPORTED_CMD_BYTE, u8Byte);	// no point in implementing stop command because no game is going to use it
			break;
		case 0x40:	// enter
			switch (g_ldp1000i_state)
			{
			case LDP1000I_STATE_WAIT_SEARCH:
				g_ldp1000i_begin_search(g_ldp1000i_u32Frame);
				g_ldp1000i_bSearchActive = LDP1000_TRUE;
				g_ldp1000i_state = LDP1000I_STATE_NORMAL;	// done with search command
				ldp1000i_push_queue(LATACK_ENTER);
				break;
				// if we have just received the end frame to loop to
			case LDP1000I_STATE_REPEAT0_WAIT_END_FRAME:
				g_ldp1000i_u32RepeatEndFrame = g_ldp1000i_u32Frame;

				// Set playback direction.
				// If dest frame is ahead of current frame, we will play forward.
				if (g_ldp1000i_u32RepeatEndFrame >= g_ldp1000i_u32RepeatStartFrame)
				{
					g_ldp1000i_directionIsReversed = LDP1000_FALSE;
				}
				// Else we will play in reverse.
				else
				{
					g_ldp1000i_directionIsReversed = LDP1000_TRUE;
				}

				LDP1000I_RESET_FRAME();
				g_ldp1000i_state = LDP1000I_STATE_REPEAT1_WAIT_COUNT;
				ldp1000i_push_queue(LATACK_ENTER);

				break;
				// if we have received the number of loop iterations to perform
			case LDP1000I_STATE_REPEAT1_WAIT_COUNT:

				// if they specify repeat iterations, then make use of what they specified
				if (g_ldp1000i_u8FrameIdx != 0)
				{
					g_ldp1000i_u8RepeatIterations = g_ldp1000i_u32Frame;
				}
				// else 1 is implied if they don't provide an iteration count
				else
				{
					g_ldp1000i_u8RepeatIterations = 1;
				}

				ldp1000i_push_queue(LATACK_ENTER);

				ldp1000i_repeat_play();

				g_ldp1000i_bRepeatActive = LDP1000_TRUE;

				break;
            case LDP1000I_STATE_WAIT_VARIABLE_SPEED:
			    // TO-DO: use only the last three digits entered
				if (g_ldp1000i_u32Frame == 0)
				{
			    	g_ldp1000i_pause();
		        	ldp1000i_push_queue(LATACK_ENTER);
                }
			    else if (g_ldp1000i_u32Frame <= 255)
				{
					// audio always squelched for multispeed playbacvk
				    g_ldp1000i_play(1, g_ldp1000i_u32Frame, g_ldp1000i_directionIsReversed, LDP1000_TRUE);
		    	    ldp1000i_push_queue(LATACK_ENTER);
				}
				else
				{
				    ldp1000i_push_queue(LATNAK_GENERIC);
				}
				g_ldp1000i_state = LDP1000I_STATE_NORMAL;
				break;
			case LDP1000I_STATE_SKIP_FORWARD:
				g_ldp1000i_skip(((int16_t) g_ldp1000i_u32Frame));
				ldp1000i_push_queue(LATACK_ENTER);
				ldp1000i_push_queue(LATVAL_GENERIC | 1);	// skip complete result code
				break;
			case LDP1000I_STATE_SKIP_BACKWARD:
				g_ldp1000i_skip(-((int16_t) g_ldp1000i_u32Frame));
				ldp1000i_push_queue(LATACK_ENTER);
				ldp1000i_push_queue(LATVAL_GENERIC | 1);	// skip complete result code
				break;
			default:
				g_ldp1000i_error(LDP1000_ERR_UNKNOWN_CMD_BYTE, u8Byte);
				break;
			}
			break;
		case 0x41:	// clear entry
			g_ldp1000i_state = LDP1000I_STATE_NORMAL;
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_GENERIC);	// latency is undocumented
			break;
		case 0x43:	// begin search
			g_ldp1000i_state = LDP1000I_STATE_WAIT_SEARCH;
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_ENTER);	// search latency the same as enter
			g_ldp1000i_bRepeatActive = LDP1000_FALSE;	// search command cancels repeat (confirmed on real hardware)

			// disc becomes paused as soon as search command is received
			g_ldp1000i_pause();

			break;
		case 0x44:	// begin repeat
			g_ldp1000i_state = LDP1000I_STATE_REPEAT0_WAIT_END_FRAME;
			g_ldp1000i_u32RepeatStartFrame = g_ldp1000i_get_cur_frame_num();
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_GENERIC);

			// disc becomes paused as soon as repeat command is received
			g_ldp1000i_pause();

			break;
		case 0x46:	// enable left audio
			g_ldp1000i_change_audio(0, 1);
			ldp1000i_push_queue(LATACK_STILL);	// same as still
			break;
		case 0x47:	// disable left audio
			g_ldp1000i_change_audio(0, 0);
			ldp1000i_push_queue(LATACK_STILL);	// same as still
			break;
		case 0x48:	// enable right audio
			g_ldp1000i_change_audio(1, 1);
			ldp1000i_push_queue(LATACK_STILL);	// same as still
			break;
		case 0x49:	// disable right audio
			g_ldp1000i_change_audio(1, 0);
			ldp1000i_push_queue(LATACK_STILL);	// same as still
			break;
		case 0x4A:	// play reverse at 1X
			g_ldp1000i_play(1, 1, LDP1000_TRUE, LDP1000_TRUE);	// audio squelched for reverse
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x4B:	// play reverse at 3X
			g_ldp1000i_play(3, 1, LDP1000_TRUE, LDP1000_TRUE);
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x4C:	// play reverse at 1/5X
			g_ldp1000i_play(1, 5, LDP1000_TRUE, LDP1000_TRUE);
			ldp1000i_push_queue(LATACK_PLAY);
			break;
		case 0x4D:	// variable speed reverse play
			g_ldp1000i_state = LDP1000I_STATE_WAIT_VARIABLE_SPEED;
			g_ldp1000i_directionIsReversed = LDP1000_TRUE;
			LDP1000I_RESET_FRAME(); // use the frame # buffer for the speed
			ldp1000i_push_queue(LATACK_GENERIC); // not documented

			// for some reason, this command will cause the disc to play at 1/7X (confirmed on real hardware, but not documented)
			g_ldp1000i_play(1, 7, LDP1000_TRUE, LDP1000_TRUE);
			break;
		case 0x4F:	// pause
			g_ldp1000i_pause();
			ldp1000i_push_queue(LATACK_STILL);
			break;
		case 0x56:	// clear all
			g_ldp1000i_state = LDP1000I_STATE_NORMAL;
			LDP1000I_RESET_FRAME();
			ldp1000i_push_queue(LATACK_CLEAR);
			break;
		case 0x60:	// ADDR INQ (get current frame number)
			{
				// According to LDP-1000A, the 5 bytes returned are set when the frame number is read from VBI.
				// If the VBI contains no frame number or is corrupt, the last good frame number is retained.

				// This is unfortunately a very expensive operation.  I've optimized it as best as I can.
				uint8_t arr[4];
				uint32_t u32FrameNum = g_ldp1000i_get_cur_frame_num();
				arr[3] = (u32FrameNum % 10) | 0x30;
				u32FrameNum /= 10;
				arr[2] = (u32FrameNum % 10) | 0x30;
				u32FrameNum /= 10;
				arr[1] = (u32FrameNum % 10) | 0x30;
				u32FrameNum /= 10;
				arr[0] = (u32FrameNum % 10) | 0x30;
				u32FrameNum /= 10;
				ldp1000i_push_queue(LATVAL_GENERIC | (u32FrameNum | 0x30));	// u32FrameNum will hold the last digit so no need to store it to an array
				ldp1000i_push_queue(LATVAL_INQUIRY | arr[0]);
				ldp1000i_push_queue(LATVAL_INQUIRY | arr[1]);
				ldp1000i_push_queue(LATVAL_INQUIRY | arr[2]);
				ldp1000i_push_queue(LATVAL_INQUIRY | arr[3]);
			}
			break;
		case 0x62:	// motor on
			// On the ldp-1450, I have personally verified that if the motor is already on, it will return a NAK (0xB).
			// As of right now, we do not support turning off the motor, so we assume the motor is always on.
			ldp1000i_push_queue(LATNAK_GENERIC);
			break;
		case 0x67:	// status inquiry

			// if it's a 1450
			if (g_ldp1000i_type == LDP1000_EMU_LDP1450)
			{
				// Bits in status, 1 condition is described
				//
				// R1 = D7:1, D6:search/repeat mode, D5:motor off, D4:initialization state,
				//		D3:Disc is supported by tray with spindle motor off and with clamp for the disc off (has nothing to do with emulation so ignore),
				//		D2:no disc in the closed compartment, D1:optical pick up circuit is out of focus, D0:an erroneous command was received
				// R2 = 0 always
				// R3 = D7:0, D6:extended code found, D5:CLV disc (0 = CAV), D4:12" (0 = 8"),D3:0, D2:0, D1&D0:0=No judgement,1=NTSC,2=PAL,3=undefined
				// R4 = D7:wait for # defining various step speed,
				//		D6:0, D5:player is waiting for a number input of repeats,
				//		D4:0, D3:PSC stop (set when player is in STILL mode as a result of a picture stop code),
				//		D2:repeat mode, D1:search mode, D0:number input flag
				// R5 = D7:REV (0 means FWD), D6:stop, D5:still, D4:scan, D3:step, D2:slow, D1:fast, D0:play

				// Search/repeat mode is set when program is physically executing a SEARCH or a REPEAT.
				// Repeat mode: set when REPEAT HEX (0x44) is received and remains set until playback begins.
				// Search mode: set when SEARCH HEX (0x43) is received and remains set until searching begins (confirmed on real LDP1450)
				// Number input flag: set when waiting for numerical input in SEARCH, REPEAT, and MARK-SET modes

				uint8_t u8 = 0x80;
				LDP1000Status_t stat = g_ldp1000i_get_status();
				if (stat == LDP1000_SEARCHING)
				{
					u8 |= 0x40;
				}
				ldp1000i_push_queue(LATVAL_GENERIC | u8);
				ldp1000i_push_queue(LATVAL_INQUIRY | 0);
				ldp1000i_push_queue(LATVAL_INQUIRY | 0x10);	// 0x10 means a 12" disc is inserted (apparently)

				u8 = 0;
				if (g_ldp1000i_state == LDP1000I_STATE_WAIT_SEARCH)
				{
					u8 |= 3;	// bit 0 means we are accepting digits as input, bit 1 means we are in the middle of a search command
				}
				ldp1000i_push_queue(LATVAL_INQUIRY | u8);

				// if playing, the returned status is 1
				if (stat == LDP1000_PLAYING)
				{
					u8 = 1;
				}
				else if (stat == LDP1000_SEARCHING)
				{
					// if in the middle of a search, the value is 0 here (verified on a real LDP-1450)
					u8 = 0;
				}

				// anything else to avoid freaking out the arcade ROM programs
				else
				{
					// 0x20 means disc is paused (still frame)
					u8 = 0x20;
				}
				ldp1000i_push_queue(LATVAL_INQUIRY | u8);

				// "ANY" also provided this tip, apparently it was once in the MAME source code and I don't know where it came from.
				// I am putting it here to refer to later but I don't know how accurate it is.  It seems at least somewhat consistent with what I got from DL2 source code.
				/*
				From Mame 0.111

				Byte 0: LDP ready status:
				0x80 Ready
				0x82 Disc Spinning down
				0x86 Tray closed & no disc loaded
				0x8A Disc ejecting, tray opening
				0x92 Disc loading, tray closing
				0x90 Disc spinning up
				0xC0 Busy Searching

				Byte 1: Error status:
				0x00 No error
				0x01 Focus unlock
				0x02 Communication Error

				Byte 2: Disc/motor status:
				0x01 Motor Off (disc spinning down or tray opening/ed)
				0x02 Tray closed & motor Off (no disc loaded)
				0x11 Motor On

				Byte 3: Command/argument status:
				0x00 Normal
				0x01 Relative search or Mark Set pending (Enter not sent)
				0x03 Search pending (Enter not sent)
				0x05 Repeat pending (Enter not sent) or Memory Search in progress
				0x80 FWD or REV Step

				Byte 4: Playback mode:
				0x00 Searching, Motor On (spinning up), motor Off (spinning down), tray opened/ing (no motor phase lock)
				0x01 FWD Play
				0x02 FWD Fast
				0x04 FWD Slow
				0x08 FWD Step
				0x10 FWD Scan
				0x20 Still
				0x81 REV Play
				0x82 REV Fast
				0x84 REV Slow
				0x88 REV Step
				0x90 REV Scan
				*/
			}

			// else it's a 1000A and the results are totally different
			else
			{
				g_ldp1000i_error(LDP1000_ERR_UNSUPPORTED_CMD_BYTE, u8Byte);
			}
			break;

		case 0x80:	// User Index Control (sets the user index)
			g_ldp1000i_UIC_Input_Active = LDP1000_TRUE;
			g_ldp1000i_u8Idx = 0;	// prepare to receive UIC function code
			g_ldp1000i_u8UIC_PendingNotifications = 0;	// clear out any notifications
			ldp1000i_push_queue(LATACK_GENERIC);
			break;
			
		case 0x81:	// User Index on
			if (g_ldp1000i_bUI_Enabled == LDP1000_FALSE)
			{
				g_ldp1000i_bUI_Enabled = LDP1000_TRUE;
				g_ldp1000i_text_enable_changed(LDP1000_TRUE);
			}
			ldp1000i_push_queue(LATACK_GENERIC);
			break;
			
		case 0x82:	// User Index off
			if (g_ldp1000i_bUI_Enabled == LDP1000_TRUE)
			{
				g_ldp1000i_bUI_Enabled = LDP1000_FALSE;
				g_ldp1000i_text_enable_changed(LDP1000_FALSE);
			}
			ldp1000i_push_queue(LATACK_GENERIC);
			break;
			
			// STUBS which we should implement if something is using them (return ACK but don't do anything)
		case 0x24:	// audio off (mute)
		case 0x25:	// audio on (unmute)
			g_ldp1000i_error(LDP1000_ERR_UNSUPPORTED_CMD_BYTE, u8Byte);
			ldp1000i_push_queue(LATACK_GENERIC);
			break;

			// STUBS (return ACK but don't do anything)
		case 0x28:	// Picture Stop Code enable
		case 0x29:	// Picture Stop Code disable
		case 0x50:	// index on (frame number display)
		case 0x51:	// index off
		case 0x55:	// frame mode
		case 0x6E:	// CX on
		case 0x6F:	// CX off
			ldp1000i_push_queue(LATACK_GENERIC);
			break;

		default:
			g_ldp1000i_error(LDP1000_ERR_UNKNOWN_CMD_BYTE, u8Byte);
			ldp1000i_push_queue(LATNAK_GENERIC);
			break;
		}
	} // end if we are not in UIC state

	// else we are in UIC state
	else
	{
		// if we are receiving the UIC function
		if (g_ldp1000i_u8Idx == 0)
		{
			// range check
			if (u8Byte <= 2)
			{
				g_ldp1000i_u8UICFunction = u8Byte;
				ldp1000i_push_queue(LATACK_GENERIC);
			}
			// TODO : see what a real player would return here
			else
			{
				ldp1000i_push_queue(LATNAK_GENERIC);
				g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;
			}
		}
		// else we have the function established
		else
		{
			switch (g_ldp1000i_u8UICFunction)
			{
			default:
			case 0:	// set display mode and coordinate
				switch (g_ldp1000i_u8Idx)
				{
				default:
				case 1:	// X coordinate
					// if coordinate will change, notify
					if (g_ldp1000i_u8UIC_X != u8Byte)
					{
						g_ldp1000i_u8UIC_X = u8Byte;
						g_ldp1000i_u8UIC_PendingNotifications |= LDP1000I_UIC_NOTIFY_MODES;
					}
					ldp1000i_push_queue(LATACK_GENERIC);
					break;
				case 2:	// Y coordinate
					// if coordinate will change, notify
					if (g_ldp1000i_u8UIC_Y != u8Byte)
					{
						g_ldp1000i_u8UIC_Y = u8Byte;
						g_ldp1000i_u8UIC_PendingNotifications |= LDP1000I_UIC_NOTIFY_MODES;
					}
					ldp1000i_push_queue(LATACK_GENERIC);
					break;
				case 3:	// mode
					if (g_ldp1000i_u8UIC_Mode != u8Byte)
					{
						g_ldp1000i_u8UIC_Mode = u8Byte;
						g_ldp1000i_u8UIC_PendingNotifications |= LDP1000I_UIC_NOTIFY_MODES;
					}
					ldp1000i_push_queue(LATACK_GENERIC);
					g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;	// we're done
					break;
				}
				break;
			case 1:	// set buffer contents

				// if we are receiving the starting index within the buffer
				if (g_ldp1000i_u8Idx == 1)
				{
					g_ldp1000i_u8UIC_StartIdx = u8Byte & 31;	// range is 0-31 so just be safe
					ldp1000i_push_queue(LATACK_GENERIC);
				}
				// else if this is the end-of-line character
				else if (u8Byte == 0x1A)
				{
					g_ldp1000i_text_buffer_contents_changed(g_ldp1000i_UIC_TextBuf);
					ldp1000i_push_queue(LATACK_GENERIC);
					g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;	// we're done
				}
				// else if we are receiving the actual bytes
				else if (u8Byte <= 0x5F)
				{
					g_ldp1000i_UIC_TextBuf[g_ldp1000i_u8UIC_StartIdx] = u8Byte;
					g_ldp1000i_u8UIC_StartIdx++;
					g_ldp1000i_u8UIC_StartIdx &= 31;	// range is 0-31 so just be safe
					ldp1000i_push_queue(LATACK_GENERIC);
				}
				// else out of range, so return an error (this is a way for us to get out of UIC mode if we are in it wrongly)
				else
				{
					ldp1000i_push_queue(LATNAK_GENERIC);
					g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;	// we're done
				}
				break;
			case 2:		// set window function
				if (g_ldp1000i_u8UIC_Window != u8Byte)
				{
					g_ldp1000i_u8UIC_Window = u8Byte;
					g_ldp1000i_u8UIC_PendingNotifications |= LDP1000I_UIC_NOTIFY_WINDOW;
				}
				ldp1000i_push_queue(LATACK_GENERIC);
				g_ldp1000i_UIC_Input_Active = LDP1000_FALSE;	// we're done
				break;
			}
		}

		// if we are leaving this UIC command, send out any notifications needed
		if (!g_ldp1000i_UIC_Input_Active)
		{
			if (g_ldp1000i_u8UIC_PendingNotifications & LDP1000I_UIC_NOTIFY_MODES)
			{
				g_ldp1000i_text_modes_changed(g_ldp1000i_u8UIC_Mode, g_ldp1000i_u8UIC_X, g_ldp1000i_u8UIC_Y);
			}
			
			if (g_ldp1000i_u8UIC_PendingNotifications & LDP1000I_UIC_NOTIFY_WINDOW)
			{
				g_ldp1000i_text_buffer_start_index_changed(g_ldp1000i_u8UIC_Window);
			}			
		}

		g_ldp1000i_u8Idx++;
	}
}

LDP1000_BOOL ldp1000i_can_read()
{
	return (LDP1000_BOOL) (g_ldp1000i_u8TxBufCount != 0);
}

uint16_t ldp1000i_read()
{
//	assert(g_ldp1000i_u8TxBufCount > 0);
	return ldp1000i_pop_queue();
}

void ldp1000i_think_during_vblank()
{
	if (g_ldp1000i_bSearchActive)
	{
		LDP1000Status_t stat = g_ldp1000i_get_status();
		switch (stat)
		{
			// if search is complete
		case LDP1000_PAUSED:

			g_ldp1000i_bSearchActive = LDP1000_FALSE;

			// if this was a regular search and not a repeat
			if (!g_ldp1000i_bRepeatActive)
			{
				ldp1000i_push_queue(LATVAL_GENERIC | 1);	// search complete result code
			}
			// else it's a repeat, doing a loop
			else
			{
				ldp1000i_repeat_play();
			}
			break;
			// if we're still searching, do nothing
		case LDP1000_SEARCHING:
			break;
		default:
			g_ldp1000i_error(LDP1000_ERR_UNHANDLED_SITUATION, 0);
			break;
		}
	} // end if a search was active
	// else if repeat is active, see if it's time to take action
	else if (g_ldp1000i_bRepeatActive)
	{
		uint32_t u32CurFrame = g_ldp1000i_get_cur_frame_num();

		// if we've reached our destination frame
		if (
			((!g_ldp1000i_directionIsReversed) && (u32CurFrame >= g_ldp1000i_u32RepeatEndFrame)) ||
			((g_ldp1000i_directionIsReversed) && (u32CurFrame <= g_ldp1000i_u32RepeatEndFrame))
			)
		{
			// if this is our last loop
			if (g_ldp1000i_u8RepeatIterations == 1)
			{
				// still on the end frame itself
				g_ldp1000i_pause();
				ldp1000i_push_queue(LATVAL_GENERIC | 1);	// send completion result code (same as for searches)
				g_ldp1000i_bRepeatActive = LDP1000_FALSE;
			}
			// else we have more work to do (or else we are in an endless loop)
			else
			{
				g_ldp1000i_begin_search(g_ldp1000i_u32RepeatStartFrame);
				g_ldp1000i_bSearchActive = LDP1000_TRUE;

				// if our iterations can be decremented (0 means endless loop)
				if (g_ldp1000i_u8RepeatIterations > 0)
				{
					g_ldp1000i_u8RepeatIterations--;
				}
			}
		} // end if we've reached destination frame
	} // end if repeat is active

}

const uint8_t *ldp1000i_get_text_buffer()
{
	return g_ldp1000i_UIC_TextBuf;
}

LDP1000_BOOL ldp1000i_isRepeatActive()
{
	return g_ldp1000i_bRepeatActive;
}

// this is a separate method in case we ever decide to support multi-speed repeat playback
void ldp1000i_repeat_play()
{
	// NOTE : multi-speed playback is optional for the REPEAT command but no game uses it so no point in supporting it
	g_ldp1000i_play(1, 1, g_ldp1000i_directionIsReversed,

		// if direction is reversed, we want to squelch audio to be consistent for normal ldp-1450 behavior when playing in reverse
		g_ldp1000i_directionIsReversed);
}