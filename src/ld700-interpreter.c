#include <ldp-in/ld700-interpreter.h>

// callbacks, must be assigned before calling any other function in this interpreter
void (*g_ld700i_play)() = 0;
void (*g_ld700i_pause)() = 0;
void (*g_ld700i_stop)() = 0;
void (*g_ld700i_eject)() = 0;
void (*g_ld700i_step)(LD700_BOOL bBackward) = 0;
void (*g_ld700i_begin_search)(uint32_t uFrameNumber) = 0;
void (*g_ld700i_change_audio)(LD700_BOOL bEnableLeft, LD700_BOOL bEnableRight) = 0;
void (*g_ld700i_error)(LD700ErrCode_t code, uint8_t u8Val) = 0;
void (*g_ld700i_on_ext_ack_changed)(LD700_BOOL bActive) = 0;

/////////////////////////

// so we can decode incoming commands properly
typedef enum
{
	LD700I_CMD_PREFIX,
	LD700I_CMD_PREFIX_XOR,
	LD700I_CMD,
	LD700I_CMD_XOR,
} LD700CmdState_t;

LD700CmdState_t g_ld700i_cmd_state = LD700I_CMD_PREFIX;

typedef enum
{
	LD700I_STATE_NORMAL,
	LD700I_STATE_ESCAPED,	// in the middle of an escape command
	LD700I_STATE_FRAME,	// in the middle of receiving a frame
	LD700I_STATE_SEARCHING
} LD700State_t;

LD700State_t g_ld700i_state;
uint32_t g_ld700i_u32Frame;
uint8_t g_ld700i_u8FrameIdx;
uint8_t g_ld700i_u8Audio[2];
uint8_t g_ld700i_u8DupeDetectorVsyncCounter;	// to detect duplicate commands to be dropped
uint8_t g_ld700i_u8ExtAckVsyncCounter;	// to manage the EXT_ACK' signal
LD700_BOOL g_ld700i_bExtAckActive;
uint8_t g_ld700i_u8QueuedCmd;
uint8_t g_ld700i_u8LastCmd;	// to drop rapidly repeated commands

// call every time you want EXT_ACK' to be a certain value.  The method will track if it's changed and trigger the callback if needed.
void ld700i_change_ext_ack(LD700_BOOL bActive)
{
	// callback gets called every time this value changes
	if (g_ld700i_bExtAckActive != bActive)
	{
		g_ld700i_on_ext_ack_changed(bActive);
		g_ld700i_bExtAckActive = bActive;
	}
}

void ld700i_reset()
{
	g_ld700i_u32Frame = 0;
	g_ld700i_u8FrameIdx = 0;
	g_ld700i_u8Audio[0] = g_ld700i_u8Audio[1] = 1;	// default to audio being enabled
	g_ld700i_u8DupeDetectorVsyncCounter = 0;
	g_ld700i_u8ExtAckVsyncCounter = 0;

	// force callback to be called so our implementor will be in the proper initial state
	g_ld700i_bExtAckActive = LD700_TRUE;
	ld700i_change_ext_ack(LD700_FALSE);

	g_ld700i_cmd_state = LD700I_CMD_PREFIX;
	g_ld700i_u8QueuedCmd = 0;	// apparently is not needed
	g_ld700i_u8LastCmd = 0xFF;
	g_ld700i_state = LD700I_STATE_NORMAL;
}

void ld700i_add_digit(uint8_t u8Digit)
{
	if (g_ld700i_u8FrameIdx < 5)
	{
		g_ld700i_u32Frame *= 10;
		g_ld700i_u32Frame += u8Digit;
		g_ld700i_u8FrameIdx++;
	}

	// TODO : test this on a real player to see what it does
	else
	{
		g_ld700i_error(LD700_ERR_TOO_MANY_DIGITS, 0);
	}
}

void ld700i_cmd_error(uint8_t u8Cmd)
{
	g_ld700i_error(LD700_ERR_UNKNOWN_CMD_BYTE, u8Cmd);
	g_ld700i_cmd_state = LD700I_CMD_PREFIX;
}

// 0 means something else so we need another distinct value to indicate no change
#define NO_CHANGE 0xFF

void ld700i_write(uint8_t u8Cmd, const LD700Status_t status)
{
	LD700_BOOL bDupeDetected = LD700_FALSE;
	uint8_t u8NewExtAckVsyncCounter = 3;	// default value

	switch (g_ld700i_cmd_state)
	{
	case LD700I_CMD_PREFIX:
		if (u8Cmd == 0xA8) g_ld700i_cmd_state++;
		else ld700i_cmd_error(u8Cmd);
		return;
	case LD700I_CMD_PREFIX_XOR:
		if (u8Cmd == 0x57) g_ld700i_cmd_state++;
		else ld700i_cmd_error(u8Cmd);
		return;
	case LD700I_CMD:
		g_ld700i_u8QueuedCmd = u8Cmd;
		g_ld700i_cmd_state++;
		return;
	default:	// LD700I_CMD_XOR
		g_ld700i_cmd_state = LD700I_CMD_PREFIX;
		if (u8Cmd != (g_ld700i_u8QueuedCmd ^ 0xFF))
		{
			ld700i_cmd_error(u8Cmd);
			return;
		}
		break;
	}

	// rapidly repeated commands are dropped
	// (a human pressing keys on a remote control will cause commands to rapidly repeat)
	if ((g_ld700i_u8QueuedCmd == g_ld700i_u8LastCmd) && (g_ld700i_u8DupeDetectorVsyncCounter != 0))
	{
		bDupeDetected = LD700_TRUE;
	}

	// this is decremented every vsync.  After 3 vsyncs without receiving a command, a repeated command is interpreted as a new command, not a duplicate.
	g_ld700i_u8DupeDetectorVsyncCounter = 3;

	if (bDupeDetected)
	{
		goto done;
	}

	// if we're receiving a normal command
	if (g_ld700i_state != LD700I_STATE_ESCAPED)
	{
		switch (g_ld700i_u8QueuedCmd)
		{
		default:	// unknown
			g_ld700i_error(LD700_ERR_UNKNOWN_CMD_BYTE, g_ld700i_u8QueuedCmd);
			break;
		case 0x0:	// 0
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:	// 9
			ld700i_add_digit(g_ld700i_u8QueuedCmd);
			u8NewExtAckVsyncCounter = 4;	// observed on real hardware when disc was paused. I also saw 2, 3, and 5 but 4 seems to match Halcyon's cadence so I'll choose 4.
			break;
		case 0x16: // reject
			if ((status == LD700_PLAYING) || (status == LD700_PAUSED))
			{
				g_ld700i_stop();
			}
			else if (status == LD700_STOPPED)
			{
				g_ld700i_eject();
			}
			else if (status == LD700_TRAY_EJECTED)
			{
				// do nothing because it's redundant to send an eject command when we're alrady ejected
			}
			else
			{
				g_ld700i_error(LD700_ERR_UNHANDLED_SITUATION, status);
			}
			u8NewExtAckVsyncCounter = NO_CHANGE;	// I've never seen this command respond with an ACK
			break;
		case 0x17:	// play

			if (status == LD700_STOPPED)
			{
				u8NewExtAckVsyncCounter = NO_CHANGE; // observed on real hardware
			}

			g_ld700i_play();
			break;
		case 0x18:	// pause
			g_ld700i_pause();
			break;
		case 0x41:	// prepare to enter frame number
			g_ld700i_state = LD700I_STATE_FRAME;
			u8NewExtAckVsyncCounter = 4;	// observed on real hardware when disc was paused. I also saw 5, but I'll choose 4 for lower latency.
			break;
		case 0x42:	// begin search
			g_ld700i_state = LD700I_STATE_NORMAL;
			g_ld700i_begin_search(g_ld700i_u32Frame);
			g_ld700i_u32Frame = 0;
			g_ld700i_u8FrameIdx = 0;
			break;
		case 0x49:	// enable right
			g_ld700i_change_audio(LD700_FALSE, LD700_TRUE);
			break;
		case 0x4A:	// enable stereo
			if (status != LD700_TRAY_EJECTED)
			{
				if (status == LD700_STOPPED)
				{
					u8NewExtAckVsyncCounter = 5; // observed on real hardware
				}
				// TODO : test/add other states
			}
			else
			{
				u8NewExtAckVsyncCounter = NO_CHANGE;
			}

			g_ld700i_u8Audio[0] = g_ld700i_u8Audio[1] = 1;
			g_ld700i_change_audio(LD700_TRUE, LD700_TRUE);
			break;
		case 0x4B:	// enable left
			g_ld700i_change_audio(LD700_TRUE, LD700_FALSE);
			break;
		case 0x5F:	// escape
			g_ld700i_state = LD700I_STATE_ESCAPED;
			u8NewExtAckVsyncCounter = NO_CHANGE;
			break;
		}
	}
	// else we're receiving an escaped command
	else
	{
		switch (g_ld700i_u8QueuedCmd)
		{
		default:	// unknown
			g_ld700i_error(LD700_ERR_UNKNOWN_CMD_BYTE, g_ld700i_u8QueuedCmd);
			break;
		case 0x02:	// disable video
		case 0x04:	// disable audio
			// not supported, but we will control EXT_ACK'
			u8NewExtAckVsyncCounter = 5;	// observed on real hardware
			break;
		case 0x03:	// enable video
		case 0x05:	// enable audio
			// not supported, but we will control EXT_ACK'
			u8NewExtAckVsyncCounter = 3;	// observed on real hardware
			break;
		case 0x06:	// disable character generator display
			// not supported, but we will control EXT_ACK'
			u8NewExtAckVsyncCounter = 2;	// observed on real hardware
			break;
		case 0x07:	// enable character generator display
			// not supported, but we will control EXT_ACK'
			u8NewExtAckVsyncCounter = NO_CHANGE;	// observed on real hardware (when disc is stopped, so maybe it's different if disc is playing)
			break;
		}

		g_ld700i_state = LD700I_STATE_NORMAL;
	}

	// to detect duplicates
	g_ld700i_u8LastCmd = g_ld700i_u8QueuedCmd;

done:
	// if this value has been set, then we replace the global value
	if (u8NewExtAckVsyncCounter != NO_CHANGE)
	{
		g_ld700i_u8ExtAckVsyncCounter = u8NewExtAckVsyncCounter;
	}
}

void ld700i_on_vblank(const LD700Status_t stat)
{
	if (g_ld700i_u8DupeDetectorVsyncCounter != 0)
	{
		g_ld700i_u8DupeDetectorVsyncCounter--;
	}

	// EXT_ACK' is active after a command has been received or if the disc is searching/spinning-up
	ld700i_change_ext_ack((g_ld700i_u8ExtAckVsyncCounter != 0) || (stat == LD700_SEARCHING) || (stat == LD700_SPINNING_UP));

	if (g_ld700i_u8ExtAckVsyncCounter != 0)
	{
		g_ld700i_u8ExtAckVsyncCounter--;
	}

	switch (g_ld700i_state)
	{
		// nothing to do
	default:
		break;
	// if we are in the middle of a search
	case LD700I_STATE_SEARCHING:
		{
			switch (stat)
			{
				// if search is complete
			case LD700_PAUSED:
				g_ld700i_state = LD700I_STATE_NORMAL;
				break;
				// if we're still working, do nothing
			case LD700_SEARCHING:
				break;
			default:
				g_ld700i_state = LD700I_STATE_NORMAL;
				g_ld700i_error(LD700_ERR_UNHANDLED_SITUATION, 0);
				break;
			}
		}
		break;
	}
}
