#include <ldp-in/ld700-interpreter.h>

// callbacks, must be assigned before calling any other function in this interpreter
void (*g_ld700i_play)() = 0;
void (*g_ld700i_pause)() = 0;
void (*g_ld700i_stop)() = 0;
void (*g_ld700i_eject)() = 0;
void (*g_ld700i_step)(int8_t i8TracksToStep) = 0;
void (*g_ld700i_begin_search)(uint32_t uFrameNumber) = 0;
void (*g_ld700i_change_audio)(uint8_t uChannel, uint8_t uEnable) = 0;
void (*g_ld700i_error)(LD700ErrCode_t code, uint8_t u8Val) = 0;
LD700Status_t (*g_ld700i_get_status)() = 0;
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

// so we know what to do when we get an ENTER command
typedef enum
{
	LD700I_STATE_NORMAL,
	LD700I_STATE_FRAME,	// in the middle of receiving a frame
} LD700State_t;

LD700State_t g_ld700i_state = LD700I_STATE_NORMAL;

uint32_t g_ld700i_u32Frame = 0;
uint8_t g_ld700i_u8FrameIdx = 0;
uint8_t g_ld700i_u8Audio[2] = { 1, 1 };	// audio starts out enabled
uint8_t g_ld700i_u8VsyncCounter = 0;
LD700_BOOL g_ld700i_bExtAckActive = LD700_FALSE;
uint8_t g_ld700i_u8QueuedCmd = 0;

void ld700i_reset()
{
	g_ld700i_u32Frame = 0;
	g_ld700i_u8FrameIdx = 0;
//	g_ld700i_u8Audio[0] = g_ld700i_u8Audio[1] = 1;	// default to audio being enabled
//	g_ld700i_u8VsyncCounter = 0;
	g_ld700i_bExtAckActive = LD700_FALSE;
	g_ld700i_cmd_state = LD700I_CMD_PREFIX;
	g_ld700i_on_ext_ack_changed(g_ld700i_bExtAckActive);
//	g_ld700i_u8QueuedCmd = 0;	// apparently is not needed
//	g_ld700i_state = LD700I_STATE_NORMAL;
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

void ld700i_write(uint8_t u8Cmd)
{
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
		break;
	case 0x16:	// reject
		{
			LD700Status_t status = g_ld700i_get_status();
			if ((status == LD700_PLAYING) || (status == LD700_PAUSED))
			{
				g_ld700i_stop();
			}
			else if (status == LD700_STOPPED)
			{
				g_ld700i_eject();
			}
			else
			{
				g_ld700i_error(LD700_ERR_UNHANDLED_SITUATION, 0);
			}
		}
		break;
	case 0x17:	// play
		g_ld700i_play();
		break;
	case 0x18:	// pause
		g_ld700i_pause();
		break;
	case 0x41:	// prepare to enter frame number
		g_ld700i_state = LD700I_STATE_FRAME;
		break;
	case 0x42:	// begin search
		g_ld700i_state = LD700I_STATE_NORMAL;
		g_ld700i_begin_search(g_ld700i_u32Frame);
		g_ld700i_u8VsyncCounter = 0;
		g_ld700i_u32Frame = 0;
		g_ld700i_u8FrameIdx = 0;
		break;
		/*
	case 0xD:	// toggle right audio
		g_ld700i_u8Audio[1] ^= 1;
		g_ld700i_change_audio(1, g_ld700i_u8Audio[1]);
		break;
	case 0xE:	// toggle left audio
		g_ld700i_u8Audio[0] ^= 1;
		g_ld700i_change_audio(0, g_ld700i_u8Audio[0]);
		break;
		 */
	}
}

void ld700i_on_vblank()
{
	// do we need to do anything here?
}
