/*
* pr7820-interpreter.c
*
* Copyright (C) 2015 Matt Ownby
*
* This file is part of DAPHNE, a laserdisc arcade game emulator
*
* DAPHNE is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* DAPHNE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// memset
#include <ldp-in/pr7820-interpreter.h>
#include <ldp-in/datatypes.h>

#define PR7820_FRAMESIZE 5

PR7820_BOOL g_bPR7820AudioEnabled[2] = { PR7820_TRUE, PR7820_TRUE }; // default audio status is on

char pr7820_frame[PR7820_FRAMESIZE + 1]; // holds the digits sent to the player

///////////////////////////////////////////

// private functions

void pr7820_add_digit(char);
void pr7820_audio1();
void pr7820_audio2();
void pr7820_update_audio();
void pr7820_clear();

////////////////////////////

void pr7820i_reset()
{
	g_bPR7820AudioEnabled[0] = PR7820_TRUE;
	g_bPR7820AudioEnabled[1] = PR7820_TRUE;
	pr7820_clear();
}

///////////////////////////////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

PR7820Status_t (*g_pr7820i_get_status)() = NULL;
void (*g_pr7820i_play)() = NULL;
void (*g_pr7820i_pause)() = NULL;
void (*g_pr7820i_begin_search)(unsigned int uFrameNumber) = NULL;
void (*g_pr7820i_change_audio)(unsigned char uChannel, unsigned char uEnable) = NULL;
void (*g_pr7820i_enable_super_mode)() = NULL;
void (*g_pr7820i_on_error)(PR7820ErrCode_t code, unsigned char u8Val) = NULL;

///////////////////////////////////////////

//////////////////////////////////////////

PR7820_BOOL pr7820i_is_busy()
{
	PR7820Status_t stat = g_pr7820i_get_status();
	PR7820_BOOL result = ((stat == PR7820_SEARCHING) || (stat == PR7820_SPINNING_UP));
	return(result);
}

void pr7820i_write(unsigned char value)
{
	switch (value)
	{
	case 0x3F:	// 0
		pr7820_add_digit('0');
		break;
	case 0x0F:	// 1
		pr7820_add_digit('1');
		break;
	case 0x8F:	// 2
		pr7820_add_digit('2');
		break;
	case 0x4F:	// 3
		pr7820_add_digit('3');
		break;
	case 0x2F:	// 4
		pr7820_add_digit('4');
		break;
	case 0xAF:	// 5
		pr7820_add_digit('5');
		break;
	case 0x6F:	// 6
		pr7820_add_digit('6');
		break;
	case 0x1F:	// 7
		pr7820_add_digit('7');
		break;
	case 0x9F:	// 8
		pr7820_add_digit('8');
		break;
	case 0x5F:	// 9
		pr7820_add_digit('9');
		break;

	case 0x9E:	// EXTENDED COMMAND: enable super mode
		g_pr7820i_enable_super_mode();
		break;
	case 0xA0:	// mute audio (see $1D27 in thayer's quest ROM)
		g_bPR7820AudioEnabled[0] = PR7820_FALSE;
		g_bPR7820AudioEnabled[1] = PR7820_FALSE;
		pr7820_update_audio();
		break;
	case 0xA1:	// enable left audio only (see $1D0E in thayer's quest ROM)
		g_bPR7820AudioEnabled[0] = PR7820_TRUE;
		g_bPR7820AudioEnabled[1] = PR7820_FALSE;
		pr7820_update_audio();
		break;
	case 0xA2:	// enable right audio only (see $1CF5 in thayer's quest ROM)
		g_bPR7820AudioEnabled[0] = PR7820_FALSE;
		g_bPR7820AudioEnabled[1] = PR7820_TRUE;
		pr7820_update_audio();
		break;
	case 0xA3:	// enable both audio channels (see $1CC1 in Thayer's Quest ROM)
		g_bPR7820AudioEnabled[0] = PR7820_TRUE;
		g_bPR7820AudioEnabled[1] = PR7820_TRUE;
		pr7820_update_audio();
		break;
	case 0xE1:	// display off (see $1F30 in Thayer's Quest ROM)
		// ignored
		break;
	case 0xF4:	// Audio 1
		pr7820_audio1();
		break;
	case 0xFC:	// Audio 2
		pr7820_audio2();
		break;
	case 0xFD:	// Play
		g_pr7820i_play();
		break;
	case 0xF7:	// Search
		{
			uint32_t uFrame;
			uFrame = strtoul(pr7820_frame, NULL, 10);	// atoi is not sufficient for AVR because it is only 16-bit signed
			g_pr7820i_begin_search(uFrame);
			pr7820_clear();
		}
		break;
	case 0xF9:	// reject
		// ignored
		break;
	case 0xFB:	// Stop - this actually just goes into still-frame mode, so we pause
		g_pr7820i_pause();
		break;

	case 0xFF:	// no entry
		// thayer's quest sends this on startup, it should be ignored and is harmless
		break;

	case 0:	// null
	case 0x7F:	// recall
	case 0xCC:	// load
	case 0xCF:	// branch
	case 0xE0:	// display on (according to shaun wood)
	case 0xF0:	// DEC REG
	case 0xF1:	// display
	case 0xf2:	// slow fwd
	case 0xF3:	// Auto Stop
	case 0xf5:	// store
	case 0xf6:	// step fwd
	case 0xF8:	// input
	case 0xfa:	// slow rev
	case 0xFE:  // step reverse
		// unsupported commands
		g_pr7820i_on_error(PR7820_ERR_UNSUPPORTED_CMD_BYTE, value);
		break;

	default:	// Unknown Command
		g_pr7820i_on_error(PR7820_ERR_UNKNOWN_CMD_BYTE, value);
		break;
	}
}

// Adds a digit to the frame array that we will be seeking to.
// Digit should be in ASCII format
void pr7820_add_digit(char digit)
{
	int count;

	// shift existing digits over to the left
	for (count = 0; count < PR7820_FRAMESIZE - 1; count++)
	{
		if (!pr7820_frame[count + 1])
		{
			pr7820_frame[count + 1] = '0';
		}
		pr7820_frame[count] = pr7820_frame[count + 1];
	}
	pr7820_frame[PR7820_FRAMESIZE - 1] = digit;		
}

// Audio channel 1 on or off
void pr7820_audio1()
{
	// Check if we should just toggle
	if (!pr7820_frame[PR7820_FRAMESIZE - 1])
	{
		// Check status of audio and toggle accordingly
		if (g_bPR7820AudioEnabled[0])
		{
			g_bPR7820AudioEnabled[0] = PR7820_FALSE;
			g_pr7820i_change_audio(0, 0);	// disable left channel
		}
		else
		{
			g_bPR7820AudioEnabled[0] = PR7820_TRUE;
			g_pr7820i_change_audio(0, 1);	// enable left channel
		}
	}
	// Or if we have an explicit audio command
	else 
	{
		switch (pr7820_frame[PR7820_FRAMESIZE - 1] % 2)
		{
		case 0:
			g_bPR7820AudioEnabled[0] = PR7820_FALSE;
			g_pr7820i_change_audio(0, 0);
			break;
		default:
			g_bPR7820AudioEnabled[0] = PR7820_TRUE;
			g_pr7820i_change_audio(0, 1);
			break;
		}
		pr7820_clear();
	}
}

// Audio channel 2 on or off
void pr7820_audio2()
{
	// Check if we should just toggle
	if (!pr7820_frame[PR7820_FRAMESIZE - 1])
	{
		// Check status of audio and toggle accordingly
		if (g_bPR7820AudioEnabled[1])
		{
			g_bPR7820AudioEnabled[1] = PR7820_FALSE;
			g_pr7820i_change_audio(1, 0);
		}
		else
		{
			g_bPR7820AudioEnabled[1] = PR7820_TRUE;
			g_pr7820i_change_audio(1, 1);
		}
	}
	// Or if we have an explicit audio command
	else 
	{
		switch (pr7820_frame[PR7820_FRAMESIZE - 1] % 2)
		{
		case 0:
			g_bPR7820AudioEnabled[1] = PR7820_FALSE;
			g_pr7820i_change_audio(1, 0);
			break;
		default:
			g_bPR7820AudioEnabled[1] = PR7820_TRUE;
			g_pr7820i_change_audio(1, 1);
			break;
		}
		pr7820_clear();
	}
}

// clears any received digits from the frame array
void pr7820_clear()
{
	memset(pr7820_frame,0,sizeof(pr7820_frame));
}

void pr7820_update_audio()
{
	g_pr7820i_change_audio(0, g_bPR7820AudioEnabled[0]);
	g_pr7820i_change_audio(1, g_bPR7820AudioEnabled[1]);
}