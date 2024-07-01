#include <ldp-in/pr8210-interpreter.h>

// callbacks, must be assigned before calling any other function in this interpreter
void (*g_pr8210i_play)() = 0;
void (*g_pr8210i_pause)() = 0;
void (*g_pr8210i_step)(int8_t i8TracksToStep) = 0;
void (*g_pr8210i_begin_search)(uint32_t uFrameNumber) = 0;
void (*g_pr8210i_change_audio)(uint8_t uChannel, uint8_t uEnable) = 0;
void (*g_pr8210i_error)(PR8210ErrCode_t code, uint16_t u16Val) = 0;
void (*g_pr8210i_skip)(int8_t i8TracksToSkip) = 0;
void (*g_pr8210i_change_auto_track_jump)(PR8210_BOOL bAutoTrackJumpEnabled) = 0;
PR8210_BOOL (*g_pr8210i_is_player_busy)() = 0;
void (*g_pr8210i_change_standby)(PR8210_BOOL bRaised) = 0;
void (*g_pr8210i_change_video_squelch)(PR8210_BOOL bRaised) = 0;

/////////////////////////

// so that we know whether incoming message is okay
uint8_t g_pr8210i_u8OldMsg = ~0, g_pr8210i_u8CurMsg = ~0;
uint32_t g_pr8210i_u32Frame = 0;
uint8_t g_pr8210i_u8FrameIdx = 0;
uint8_t g_pr8210i_u8Audio[2] = { 1, 1 };	// audio starts out enabled
PR8210_BOOL g_pr8210i_bJumpTriggerRaised = PR8210_TRUE;
PR8210_BOOL g_pr8210i_bScanCRaised = PR8210_TRUE;
PR8210_BOOL g_pr8210i_bPlayerBusy = PR8210_FALSE;
PR8210_BOOL g_pr8210i_bStandByRaised = PR8210_FALSE;
uint8_t g_pr8210i_u8VsyncCounter = 0;

// whether jmp trig and scan c are internally defined or externally defined (true means internal)
PR8210_BOOL g_pr8210i_bInternalMode = PR8210_TRUE;

void pr8210i_reset()
{
	g_pr8210i_u8OldMsg = ~0;
	g_pr8210i_u8CurMsg = ~0;
	g_pr8210i_u32Frame = 0;
	g_pr8210i_u8FrameIdx = 0;
	g_pr8210i_u8Audio[0] = g_pr8210i_u8Audio[1] = 1;	// default to audio being enabled
	g_pr8210i_bJumpTriggerRaised = PR8210_TRUE;
	g_pr8210i_bScanCRaised = PR8210_TRUE;
	g_pr8210i_bInternalMode = PR8210_TRUE;
	g_pr8210i_bPlayerBusy = PR8210_FALSE;
	g_pr8210i_bStandByRaised = PR8210_FALSE;
	g_pr8210i_u8VsyncCounter = 0;
}

void pr8210i_add_digit(uint8_t u8Digit)
{
	if (g_pr8210i_u8FrameIdx < 5)
	{
		g_pr8210i_u32Frame *= 10;
		g_pr8210i_u32Frame += u8Digit;
		g_pr8210i_u8FrameIdx++;
	}

	// TODO : test this on a real player to see what it does
	else
	{
		g_pr8210i_error(PR8210_ERR_TOO_MANY_DIGITS, 0);
	}
}

void pr8210i_write(uint16_t u16Msg)
{
	uint8_t u8Cmd;

	// test header and footer bits to make sure it complies (MACH3 sends in all 0 bits and we don't want to flag this as an error)
	if (((u16Msg & 0x307) != 4) && (u16Msg != 0))
	{
		g_pr8210i_error(PR8210_ERR_CORRUPT_INPUT, u16Msg);
		return;
	}

	// isolate 5 bits that make up the message
	u8Cmd = u16Msg >> 3;

	// if this is the first time we've seen this command, ignore it since all commands must (apparently) come at least twice to be valid
	if (u8Cmd != g_pr8210i_u8OldMsg)
	{
		g_pr8210i_u8OldMsg = u8Cmd;
		g_pr8210i_u8CurMsg = ~0;	// TODO : is this necessary?
		return;
	}
	// if the command has been received 3 or more times, just ignore it
	else if (u8Cmd == g_pr8210i_u8CurMsg)
	{
		return;
	}
	// else process it

	switch (u8Cmd)
	{
	default:	// unknown
		g_pr8210i_error(PR8210_ERR_UNKNOWN_CMD_BYTE, u8Cmd);
		break;
	case 0:	// filler (aka End Of Command), used by cobra command and cliff hanger, but cobra command does not always send it, so it must remain optional
		//  nothing to do
		break;
	case 4: // step forward
		g_pr8210i_step(1);
		break;
	case 5:	// play
		g_pr8210i_play();
		break;
	case 9: // step backward
		g_pr8210i_step(-1);
		break;
	case 0x0A:	// pause
		g_pr8210i_pause();
		break;
	case 0xB:	// search
		// If at least one digit has been received, perform a search.
		// Regardless, always reset buffered frame number and digit count when we receive one of these.
		// (this may not be authentic behavior, but it seems to be compatible, at least)
		// This behavior is necessary to support Goal To Go's tendency to not switch to a separate command (ie a non-0xB) between two consecutive seeks.
		if (g_pr8210i_u8FrameIdx != 0)
		{
			g_pr8210i_begin_search(g_pr8210i_u32Frame);
			g_pr8210i_change_standby(PR8210_TRUE);	// star rider code apparently expects stand by to immediately go high when search starts
			g_pr8210i_bStandByRaised = PR8210_TRUE;
			g_pr8210i_bPlayerBusy = PR8210_TRUE;	
			g_pr8210i_u8VsyncCounter = 0;	// counter used to determine when to blink stand by
		}
		g_pr8210i_u32Frame = 0;
		g_pr8210i_u8FrameIdx = 0;
		break;
	case 0xD:	// toggle right audio
		g_pr8210i_u8Audio[1] ^= 1;
		g_pr8210i_change_audio(1, g_pr8210i_u8Audio[1]);
		break;
	case 0xE:	// toggle left audio
		g_pr8210i_u8Audio[0] ^= 1;
		g_pr8210i_change_audio(0, g_pr8210i_u8Audio[0]);
		break;
	case 0xF:	// reject
		// ignore
		break;
	case 0x10:	// 0
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:	// 9
		pr8210i_add_digit(u8Cmd & 0xF);
		break;
	case 1:	// 3X FWD
	case 2:	// SCAN FWD
	case 3:	// SLOW FWD
	case 6:	// 3X REV
	case 7:	// SCAN REV
	case 8:	// SLOW REV
	case 0xC:	// chapter
	case 0x1A:	// frame disp
		g_pr8210i_error(PR8210_ERR_UNSUPPORTED_CMD_BYTE, u8Cmd);
		break;
	}

	// allow repeated spamming of the same command (ie only process it once)
	g_pr8210i_u8CurMsg = u8Cmd;
}

// PR-8210A only
void pr8210i_on_jmp_trigger_changed(PR8210_BOOL bJmpTrigRaised, PR8210_BOOL bScanCRaised)
{
	// cache this for special case of going external while jump trigger is low
	g_pr8210i_bScanCRaised = bScanCRaised;

	// do nothing if this call has no effect
	if (g_pr8210i_bJumpTriggerRaised == bJmpTrigRaised)
	{
		return;
	}

	g_pr8210i_bJumpTriggerRaised = bJmpTrigRaised;

	// only act on this change if PR-8210A is in external mode
	if (g_pr8210i_bInternalMode)
	{
		return;
	}

	// if jump trigger has gone low (active)
	if (!bJmpTrigRaised)
	{
		int8_t i8TracksToSkip = bScanCRaised ? 1 : -1;	// high means forward, low means backward

		// ignore jump triggers that come in while we are searching
		if (!g_pr8210i_bPlayerBusy)
		{
			g_pr8210i_skip(i8TracksToSkip);
		}
	}
	// else jump trigger has gone high (inactive)
}

// PR-8210A only
void pr8210i_on_jmptrig_and_scanc_intext_changed(PR8210_BOOL bInternal)
{
	// do nothing if call has no effect
	if (g_pr8210i_bInternalMode == bInternal)
	{
		return;
	}

	g_pr8210i_bInternalMode = bInternal;
	g_pr8210i_change_auto_track_jump(bInternal);

	// edge case: if jump trigger was already low before we were external
	if (!g_pr8210i_bJumpTriggerRaised)
	{
		g_pr8210i_bJumpTriggerRaised = PR8210_TRUE;	// force jump trigger to be processed
		pr8210i_on_jmp_trigger_changed(PR8210_FALSE, g_pr8210i_bScanCRaised);
	}
}

void pr8210i_on_vblank()
{
	// if player has been busy up to this point
	if (g_pr8210i_bPlayerBusy)
	{
		// if player is still busy, check to see whether we need to blink the stand by line
		if (g_pr8210i_is_player_busy())
		{
			// if 7 vsyncs have passed (~117ms, close to goal of 112.5ms) blink the stand by
			if (g_pr8210i_u8VsyncCounter >= 6)
			{
				g_pr8210i_bStandByRaised ^= PR8210_TRUE;
				g_pr8210i_change_standby(g_pr8210i_bStandByRaised);
				g_pr8210i_u8VsyncCounter = 0;
			}
			// else we don't want to pulse stand by yet
			else
			{
				g_pr8210i_u8VsyncCounter++;
			}
		}
		// else player is no longer busy, stand by goes instantly false
		else
		{
			// don't change the stand by if it's already the way we want it
			if (g_pr8210i_bStandByRaised == PR8210_TRUE)
			{
				g_pr8210i_change_standby(PR8210_FALSE);
			}
			g_pr8210i_bPlayerBusy = PR8210_FALSE;
		}
	}
	// else player was not busy
}
