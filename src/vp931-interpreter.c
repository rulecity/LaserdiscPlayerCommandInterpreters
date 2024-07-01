#include <ldp-in/vp931-interpreter.h>

//////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

void (*g_vp931i_play)() = 0;
void (*g_vp931i_pause)() = 0;
void (*g_vp931i_begin_search)(uint32_t uFrameNumber, VP931_BOOL bAudioSquelchedOnComplete) = 0;
void (*g_vp931i_skip_tracks)(int16_t i16Tracks) = 0;
void (*g_vp931i_skip_to_framenum)(uint32_t uFrameNumber) = 0;
void (*g_vp931i_error)(VP931ErrCode_t code, uint8_t u8Val) = 0;

//////////////////////////////////////////////////////////////

// private methods

uint16_t BCDtoHEX12(const uint8_t *BCD_String)				// Convert player BCD format to 12-bit integer
{
	uint16_t Hex_Value;

	Hex_Value = (uint16_t)(BCD_String[0] & 0x0F) * 100;
	Hex_Value += (BCD_String[1] >> 4) * 10;
	Hex_Value += BCD_String[1] & 0x0F;

	return(Hex_Value);
}

uint32_t BCDtoHEX20(const uint8_t *BCD_String)				// Convert player BCD format to 20-bit integer
{
	uint32_t Hex_Value;

	Hex_Value = (uint32_t)(BCD_String[0] & 0x07) * 10000;	// discard top 5 bits as they are reserved and cannot be part of the picture number
	Hex_Value += (uint32_t)(BCD_String[1] >> 4) * 1000;
	Hex_Value += (uint32_t)(BCD_String[1] & 0x0F) * 100;
	Hex_Value += (BCD_String[2] >> 4) * 10;
	Hex_Value += BCD_String[2] & 0x0F;

	return(Hex_Value);
}

void vp931i_process_cmd(const uint8_t *pCmdBuf, VP931Status_t status)
{
	uint8_t u8HighNibble = (pCmdBuf[0] & 0xF0);

	// more common
	if (pCmdBuf[0] == 0)
	{
		switch (pCmdBuf[1] & 0xF0)
		{
		default:	// unknown
			g_vp931i_error(VP931_ERR_UNKNOWN_CMD_BYTE, pCmdBuf[1]);
			break;
		case 0x00:		// Play
			// Firefox spams the play command.  We need to check the status to make sure we don't get overwhelmed by said spammage.
			switch (status)
			{
			// do nothing in these cases
			default:
			case VP931_SPINNING_UP:
			case VP931_PLAYING:
			case VP931_SEARCHING:
				break;

				// only if we are paused (or had a search error) should we actually send a play command
			case VP931_PAUSED:
			case VP931_ERROR:
				g_vp931i_play();
				break;
			}
			break;
		case 0x20:		// Halt/pause

			// only pause if we aren't already paused.
			// don't spam the pause command (FFR spams this if nothing is plugged into the PIF board)
			if (status != VP931_PAUSED)
			{
				g_vp931i_pause();
			}
			break;
		case 0xE0:		// Jump XXX tracks forward
			{
				uint16_t u16TracksToJumpForward = BCDtoHEX12(&pCmdBuf[1]);
				g_vp931i_skip_tracks(u16TracksToJumpForward);
			}
			break;
		case 0xF0:		// Jump XXX tracks backward
			{
				uint16_t u16TracksToJumpBackward = BCDtoHEX12(&pCmdBuf[1]);
				g_vp931i_skip_tracks(-u16TracksToJumpBackward);
			}
			break;
		case 0x10:		// Reverse play
		case 0x40:		// Slow forward
		case 0x50:		// Slow backward
		case 0xA0:		// Scan forward 75X
		case 0xB0:		// Scan backward 75X
			g_vp931i_error(VP931_ERR_UNSUPPORTED_CMD_BYTE, pCmdBuf[1]);
			break;
		}
	}
	// goto + halt
	else if (u8HighNibble == 0xD0)
	{
		uint32_t uTargetPicNum = BCDtoHEX20(&pCmdBuf[0]);
		g_vp931i_begin_search(uTargetPicNum, VP931_TRUE);
	}
	// goto + play
	else if (u8HighNibble == 0xF0)
	{
		uint32_t uTargetPicNum = BCDtoHEX20(&pCmdBuf[0]);

		// skip won't work unless disc is playing, so if disc is not playing, send a play command before performing the skip
		if (status != VP931_PLAYING)
		{
			g_vp931i_play();
		}

		g_vp931i_skip_to_framenum(uTargetPicNum);
	}
	// else video/audio options
	else if (pCmdBuf[0] == 0x02)
	{
		g_vp931i_error(VP931_ERR_UNSUPPORTED_CMD_BYTE, pCmdBuf[0]);
	}
	// else unknown
	else
	{
		g_vp931i_error(VP931_ERR_UNKNOWN_CMD_BYTE, pCmdBuf[0]);
	}
}

//////////////////////////////////////////////////////////////

void vp931i_on_vsync(const uint8_t *p8CmdBuf, uint8_t u8CmdBytesRecvd, VP931Status_t status)
{
	uint8_t idx = 0;

	// process all command sets of 3 (don't process partial command sets)
	while ((idx+3) <= u8CmdBytesRecvd)
	{
		vp931i_process_cmd(&p8CmdBuf[idx], status);
		idx += 3;	// 3 bytes per command
	}

	u8CmdBytesRecvd = 0;	// ready to receive new commands
}

void vp931i_reset()
{
	// nothing to do here for now
}

void vp931i_get_status_bytes(uint32_t u32VBILine18, VP931Status_t status, uint8_t *pDstBuffer)
{
	// no need to & 0xFF because we are assigning bytes
	pDstBuffer[0] = (u32VBILine18 >> 16);
	pDstBuffer[1] = (u32VBILine18 >> 8);
	pDstBuffer[2] = (u32VBILine18);

	switch (status)
	{
	case VP931_SPINNING_UP:
		pDstBuffer[0] = 0x88;	// lead-in for lack of better thing to put here
		pDstBuffer[1] = 0xFF;
		pDstBuffer[2] = 0xFF;
		pDstBuffer[3] = 2;	// "powering up", firefox will ignore VP931's vsync while this is here
		break;
	case VP931_PAUSED:
		pDstBuffer[3] = 5;	// decent guess, needs more research
		break;
	case VP931_PLAYING:
		pDstBuffer[3] = 4;
		break;
	case VP931_SEARCHING:
		// don't report any frame numbers while searching (to avoid inadvertant behavior)
		// This won't work properly on Firefox, but it never does a conventional search anyway, so we will leave it here for now.
		pDstBuffer[0] = 0;
		pDstBuffer[1] = 0;
		pDstBuffer[2] = 0;
		pDstBuffer[3] = 8;
		// TODO : the other byte here (0x60 or 0x70) is always ignored by Firefox, so we defer implementing it for now
		break;
	default:	// error
		pDstBuffer[3] = 0;	// just a guess, not sure what to put here for errors
		break;
	}
	pDstBuffer[4] = 0;
	pDstBuffer[5] = 0;
}
