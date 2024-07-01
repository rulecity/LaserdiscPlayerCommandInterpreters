/*
* ldv1000-interpreter.c
*
* Copyright (C) 2009 Matt Ownby
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

// LD-V1000-interpreter.C

// This code emulates the Pioneer LD-V1000 laserdisc player which is used in these games:
// Dragon's Lair US
// Space Ace US
// Super Don Quixote
// Thayer's Quest
// Badlands
// Esh's Aurunmilla
// Interstellar
// Astron Belt (LD-V1000 roms)
// Starblazer

// Disc spin-up time: 13 seconds standard disc, 18 seconds aluminum

#ifdef WIN32
#pragma warning (disable:4996)	// disable the warning about sprintf being unsafe
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// memset
#include <assert.h>
#include <ldp-in/ldv1000-interpreter.h>

#define LDV1000_FRAMESIZE 5
#define LDV1000_QUEUESIZE 12		/* this should be small enough so as to not waste space but big enough to hold things like current frame number */

uint8_t g_ldv1000i_tx_buf[LDV1000_QUEUESIZE];
uint8_t *g_ldv1000i_pTxBufStart = 0;
uint8_t *g_ldv1000i_pTxBufEnd = 0;
const uint8_t *g_ldv1000i_pTxBufLastGoodPtr = (g_ldv1000i_tx_buf + sizeof(g_ldv1000i_tx_buf) - 1);
uint8_t g_ldv1000i_u8TxBufCount = 0;	// how many bytes are in the tx buffer

#define LDV1000I_TX_WRAP(var) 	if (var > g_ldv1000i_pTxBufLastGoodPtr) var = g_ldv1000i_tx_buf;

unsigned int g_ldv1000_autostop_frame = 0;	// which frame we need to stop on (if any)

LDV1000_BOOL audio1 = LDV1000_TRUE; // default audio status is on
LDV1000_BOOL audio2 = LDV1000_TRUE;

LDV1000_BOOL audio_temp_mute = LDV1000_FALSE;  // flag set on FORWARD 1X, 2X, etc., which don't play audio unless a PLAY command was given first

char ldv1000_frame[LDV1000_FRAMESIZE + 1]; // holds the digits sent to the LD-V1000

unsigned char g_ldv1000_output = 0xFC;	// LD-V1000 is PARK'd and READY

LDV1000_BOOL g_ldv1000_search_pending = LDV1000_FALSE;	// whether the LD-V1000 is currently in the middle of a search operation or not
LDV1000_DiscSwitchState_t g_ldv1000_discswitch_state = LDV1000_DISCSWITCH_NONE;	// state of extended disc switch command
LDV1000_BOOL g_ldv1000_discswitch_pending = LDV1000_FALSE;	// whether LD-V1000 is currently in the middle of a disc swap operation or not

// 30 means 0.5 seconds (since read_ldv1000 should be called at 60 Hz)
unsigned int g_ldv1000_search_delay_iterations = 0; // how many times read_ldv1000() is called before our search is finally finished

LDV1000_EmulationType_t g_ldv1000_emulation_type = LDV1000_EMU_STANDARD;

///////////////////////////////////////////

void reset_ldv1000i(LDV1000_EmulationType_t type)
{
	g_ldv1000i_pTxBufStart = g_ldv1000i_pTxBufEnd = g_ldv1000i_tx_buf;
	g_ldv1000i_u8TxBufCount = 0;
	g_ldv1000_autostop_frame = 0;
	audio1 = LDV1000_TRUE;
	audio2 = LDV1000_TRUE;
	audio_temp_mute = LDV1000_FALSE;
	g_ldv1000_output = 0xFC;
	g_ldv1000_search_pending = LDV1000_FALSE;
	g_ldv1000_discswitch_pending = LDV1000_FALSE;
	g_ldv1000_search_delay_iterations = 0;
	g_ldv1000_emulation_type = type;
	g_ldv1000_discswitch_state = LDV1000_DISCSWITCH_NONE;
}

///////////////////////////////////////////

// CALLBACKS THAT MUST BE DEFINED BY CALLER:

LDV1000Status_t (*g_ldv1000i_get_status)() = NULL;
uint32_t (*g_ldv1000i_get_cur_frame_num)() = NULL;
void (*g_ldv1000i_play)() = NULL;
void (*g_ldv1000i_pause)() = NULL;
void (*g_ldv1000i_begin_search)(uint32_t uFrameNumber) = NULL;
void (*g_ldv1000i_step_reverse)() = NULL;
void (*g_ldv1000i_change_speed)(uint8_t uNumerator, uint8_t uDenominator) = NULL;
void (*g_ldv1000i_skip_forward)(uint8_t uTracks) = NULL;
void (*g_ldv1000i_skip_backward)(uint8_t uTracks) = NULL;
void (*g_ldv1000i_change_audio)(uint8_t uChannel, uint8_t uEnable) = NULL;
void (*g_ldv1000i_on_error)(const char *pszErrMsg) = NULL;
const uint8_t *(*g_ldv1000i_query_available_discs)() = NULL;
uint8_t (*g_ldv1000i_query_active_disc)() = NULL;
void (*g_ldv1000i_begin_changing_to_disc)(uint8_t idDisc) = NULL;
void (*g_ldv1000i_change_seek_delay)(LDV1000_BOOL bEnabled) = NULL;
void (*g_ldv1000i_change_spinup_delay)(LDV1000_BOOL bEnabled) = NULL;
void (*g_ldv1000i_change_super_mode)(LDV1000_BOOL bEnabled) = NULL;

///////////////////////////////////////////

// private functions

unsigned int get_buffered_frame(void);
void ldv1000_add_digit(char);
void pre_audio1();
void pre_audio2();
void clear();

//////////////////////////////////////////

void ldv1000_push_queue(uint8_t u8Val)
{
	*g_ldv1000i_pTxBufEnd = u8Val;
	g_ldv1000i_pTxBufEnd++;
	LDV1000I_TX_WRAP(g_ldv1000i_pTxBufEnd);
	g_ldv1000i_u8TxBufCount++;

	// we should never get close to this
	assert(g_ldv1000i_u8TxBufCount < sizeof(g_ldv1000i_tx_buf));
}

uint8_t ldv1000_pop_queue()
{
	uint8_t u8Res = *g_ldv1000i_pTxBufStart;
	g_ldv1000i_pTxBufStart++;
	LDV1000I_TX_WRAP(g_ldv1000i_pTxBufStart);
	g_ldv1000i_u8TxBufCount--;

	// sanity check
	assert(g_ldv1000i_u8TxBufCount != -1);

	return u8Res;
}

// retrieves the status from our virtual LD-V1000
unsigned char read_ldv1000i()
{
	unsigned char result = 0;

	// if we don't have anything in the queue to return, then return current player status
	if (g_ldv1000i_u8TxBufCount == 0)
	{
		LDV1000Status_t stat = g_ldv1000i_get_status();

		// we are in the middle of a search operation ...
		if (g_ldv1000_search_pending)
		{
			// if the ld-v1000 has been "searching" for long enough
			//   then check to see if it's time to change our search from 'busy' to 'finished'
			if (g_ldv1000_search_delay_iterations == 0)
			{
				// if we finished seeking and found success
				if (stat == LDV1000_PAUSED)
				{
					g_ldv1000_output = (g_ldv1000_output & 0x80) | 0x50;	// seek succeeded (but don't change the high bit in case they have not sent a NO ENTRY command since initiating the search, cobraconv does this a lot)
					g_ldv1000_search_pending = LDV1000_FALSE;
				}
				// search failed for whatever reason ...
				else if (stat == LDV1000_ERROR)
				{
					g_ldv1000_output = 0x90;	// seek failed and ready (TODO : this is incorrect, the ready bit should be changeable, but I need to add a unit test to prove it before I fix it here)
					g_ldv1000_search_pending = LDV1000_FALSE;
				}

				// else if we're not still searching, it's an error
				else if (stat != LDV1000_SEARCHING)
				{
					char s[50];
					sprintf(s, "Unknown state after search: %x", stat);
					g_ldv1000i_on_error(s);
				}
			}
			// else search is still going so don't change status
			else
			{
				g_ldv1000_search_delay_iterations--;
			}
		}

		else if (g_ldv1000_discswitch_pending)
		{
			if (stat == LDV1000_STOPPED)
			{
				g_ldv1000_output = (g_ldv1000_output & 0x80) | 0x50;	// seek succeeded (but don't change the high bit in case they have not sent a NO ENTRY command since initiating the search, cobraconv does this a lot)
				g_ldv1000_discswitch_pending = LDV1000_FALSE;
			}
			else if (stat == LDV1000_ERROR)
			{
				g_ldv1000_output = 0x90;	// seek failed and ready (TODO : this is incorrect, the ready bit should be changeable, but I need to add a unit test to prove it before I fix it here)
				g_ldv1000_discswitch_pending = LDV1000_FALSE;
			}
			else if (stat != LDV1000_DISC_SWITCHING)
			{
				char s[50];
				sprintf(s, "Unknown state after disc switch: %x", stat);
				g_ldv1000i_on_error(s);

				g_ldv1000_output = 0x90;	// seek failed and ready (TODO : this is incorrect, the ready bit should be changeable, but I need to add a unit test to prove it before I fix it here)
				g_ldv1000_discswitch_state = LDV1000_DISCSWITCH_NONE;
			}
		}

		// if autostop is active, we need to check to see if we need to stop
		else if ((g_ldv1000_output & 0x7F) == 0x54)
		{
			// if we've hit the frame we need to stop on (or gone too far) then stop
			if (g_ldv1000i_get_cur_frame_num() >= g_ldv1000_autostop_frame)
			{
				g_ldv1000i_pause();
				g_ldv1000_output = (unsigned char) ((g_ldv1000_output & 0x80) | 0x65);	// preserve ready bit and set status to paused
				g_ldv1000_autostop_frame = 0;
			}
		}

		// else we can just return the previous output

		result = g_ldv1000_output;

		// status is always 'ready' in super mode (this will be overridden by spinning up/seeking status)
		if (g_ldv1000_emulation_type == LDV1000_EMU_SUPER)
		{
			result |= 0x80;
		}

		// it's legal to start sending new commands during a busy operation but we want our status to stay as 0x50
		// (I'm not sure if this is correct)
		if ((g_ldv1000_search_pending) || (g_ldv1000_discswitch_pending))
		{
			result = 0x50;
		}
		// if the disc is spinning up, return 0x64 no matter how many 0xFF's they've sent
		else if (stat == LDV1000_SPINNING_UP)
		{
			result = 0x64;
		}
	}
	// else if we have something in the queue (like the current frame)
	else 
	{
		result = ldv1000_pop_queue();
	}

	return(result);
}

// sends a byte to our virtual LD-V1000
void write_ldv1000i(unsigned char value)
{
	// if high-bit is set, it means we are ready and so we accept input
	// (super mode is always ready)
	if ((g_ldv1000_output & 0x80) || (g_ldv1000_emulation_type == LDV1000_EMU_SUPER))
	{
		// if we are in the middle of a 'switch disc' extended command
		if (g_ldv1000_discswitch_state == LDV1000_DISCSWITCH_WAITING_FOR_DISC_ID)
		{
			// we have received the multi-part command so back to normal command processing
			g_ldv1000_discswitch_state = LDV1000_DISCSWITCH_NONE;

			// we can only initiate a disc switch if a previous disc switch is not in progress
			if (g_ldv1000_discswitch_pending == LDV1000_TRUE)
			{
				return;
			}

			// we can only initiate a disc switch if a search is not in progress
			if (g_ldv1000_search_pending == LDV1000_TRUE)
			{
				return;
			}

			// if we are really changing to a new disc
			if (g_ldv1000i_query_active_disc() != value)
			{
				g_ldv1000_discswitch_pending = LDV1000_TRUE;
				g_ldv1000i_begin_changing_to_disc(value);
			}
			// else we are changing to the current disc, so 'instantly' succeed

			// the status will changed to 'busy' regardless or whether we complete the change instantly or not
			g_ldv1000_output = 0x50;

			return;
		}

		g_ldv1000_output &= 0x7F;	// clear high bit
		// because when we receive a non 0xFF command we are no longer ready

		switch (value)
		{
		case 0xBF:	// clear
			clear();
			break;
		case 0x3F:	// 0
			ldv1000_add_digit('0');
			break;
		case 0x0F:	// 1
			ldv1000_add_digit('1');
			break;
		case 0x8F:	// 2
			ldv1000_add_digit('2');
			break;
		case 0x4F:	// 3
			ldv1000_add_digit('3');
			break;
		case 0x2F:	// 4
			ldv1000_add_digit('4');
			break;
		case 0xAF:	// 5
			ldv1000_add_digit('5');
			break;
		case 0x6F:	// 6
			ldv1000_add_digit('6');
			break;
		case 0x1F:	// 7
			ldv1000_add_digit('7');
			break;
		case 0x9F:	// 8
			ldv1000_add_digit('8');
			break;
		case 0x5F:	// 9
			ldv1000_add_digit('9');
			break;
		case 0xF4:	// Audio 1
			pre_audio1();
			break;
		case 0xFC:	// Audio 2
			pre_audio2();
			break;
		case 0xA0:	// play at 0X (pause)
			g_ldv1000i_pause();
			// TODO : change status?
			break;
		case 0xA1:	// play at 1/4X
			g_ldv1000i_change_speed(1, 4);
			break;
		case 0xA2:	// play at 1/2X
			g_ldv1000i_change_speed(1, 2);
			break;
		case 0xA3:	// play at 1X
			// it is necessary to set the playspeed because otherwise the ld-v1000 ignores skip commands
			if (g_ldv1000i_get_status() != LDV1000_PLAYING) 
			{
				// if not already playing, FORWARD 1X plays with no audio (until the next PLAY),
				// so we need to set a temporary mute
				audio_temp_mute = LDV1000_TRUE;
				g_ldv1000i_play();
				g_ldv1000i_change_audio(0, 0);// disable audio 1
				g_ldv1000i_change_audio(1, 0);// disable audio 2
				g_ldv1000_output = 0x2e;	// not ready and in FORWARD (variable speed) mode
			}
			g_ldv1000i_change_speed(1, 1);
			break;
		case 0xA4:	// play at 2X
			g_ldv1000i_change_speed(2, 1);
			break;
		case 0xA5:	// play at 3X
			g_ldv1000i_change_speed(3, 1);
			break;
		case 0xA6:	// play at 4X
			g_ldv1000i_change_speed(4, 1);
			break;
		case 0xA7:	// play at 5X
			g_ldv1000i_change_speed(5, 1);
			break;
		case 0xF9:	// Reject - Stop the laserdisc player from playing
			g_ldv1000_output = 0x7c;	// LD-V1000 is PARK'd and NOT READY
			// NOTE: laserdisc state should go into parked mode, but most people probably don't want this to happen (I sure don't)
			break;
		case 0xF3:	// Auto Stop (used by Esh's)
			// This command accepts a frame # as an argument and also begins playing the disc
			g_ldv1000_autostop_frame = get_buffered_frame();
			clear();
			g_ldv1000i_play();
			g_ldv1000_output = 0x54;	// autostop is active
			break;
		case 0xFD:	// Play
			g_ldv1000i_play();

			// if a FORWARD 1X caused playing with no audio, PLAY will turn audio back on
			if (audio_temp_mute)
			{
				audio_temp_mute = LDV1000_FALSE;
				if (audio1)  //make sure we don't have a normal mute going as well
				{
					g_ldv1000i_change_audio(0, 1);// enable audio 1
				}
				if (audio2)
				{
					g_ldv1000i_change_audio(1, 1); // enable audio 2
				}
			}
			g_ldv1000_output = 0x64;	// not ready and playing
			break;
		case 0xFE:  // step reverse
			{
				g_ldv1000i_step_reverse();
				g_ldv1000_output = 0x65; // 0x65 is stop
				break;
			}
		case 0xF7:	// Search
			{
				uint32_t uFrame;

				// Esh's and Astron belt require searches to last at least 4 delay iterations
				g_ldv1000_search_delay_iterations = 4;
			
				uFrame = strtoul(ldv1000_frame, NULL, 10);	// atoi is not sufficient for AVR because it is only 16-bit signed
				g_ldv1000i_begin_search(uFrame);
				g_ldv1000_search_pending = LDV1000_TRUE;
				g_ldv1000_output = 0x50;
				clear();
			}
			break;
		case 0xC2:	// get current frame
			{
				uint32_t curframe = g_ldv1000i_get_cur_frame_num();

				// this conversion is expensive so I've forced the loop to be unrolled
				char s[5];
				s[4] = (curframe % 10) + '0';
				curframe /= 10;
				s[3] = (curframe % 10) + '0';
				curframe /= 10;
				s[2] = (curframe % 10) + '0';
				curframe /= 10;
				s[1] = (curframe % 10) + '0';
				curframe /= 10;
				s[0] = (curframe % 10) + '0';
				// discard the rest as we only want the lower 5 digits

				ldv1000_push_queue(s[0]);
				ldv1000_push_queue(s[1]);
				ldv1000_push_queue(s[2]);
				ldv1000_push_queue(s[3]);
				ldv1000_push_queue(s[4]);
			}
			break;
		case 0xB1:	// Skip Forward 10
		case 0xB2:	// Skip Forward 20
		case 0xB3:	// Skip Forward 30
		case 0xB4:	// Skip Forward 40
		case 0xB5:	// Skip Forward 50
		case 0xB6:	// Skip Forward 60
		case 0xB7:	// Skip Forward 70
		case 0xB8:	// Skip Forward 80
		case 0xB9:	// Skip Forward 90
		case 0xBa:	// Skip Forward 100
			// FIXME: ignore skip command if forward command has not been issued
			{
				// LD-V1000 does add 1 when skipping
				// UPDATE : I've decided it adds 1 because the disc is playing, so we should not add 1 here.
				unsigned int tracks_to_skip = (unsigned int) (10 * (value & 0x0f));
				g_ldv1000i_skip_forward(tracks_to_skip);
			}
			break;
		case 0xCD:	// Display Disable
			break;
		case 0xCE:	// Display Enable
			break;
		case 0xFB:	// Stop - this actually just goes into still-frame mode, so we pause
			g_ldv1000i_pause();
			g_ldv1000_output = 0x65;	// stopped and not ready
			break;
			/*
			* From Ernesto Corvi (MAME team)
			* Commands 20-27: Same as commands A0-A7, but direction is reverse, instead of forward.
			* Commands 31-3A: Same as commands B1-BA, but it seeks back, instead of forward.
			*/

		case 0x20:	// Badlands custom command (disc paused, reverse)
			g_ldv1000i_pause();
			// TODO : change status? this was a Badlands-only command
			break;
		case 0x31:	// Badlands custom command (skip backward 10)
		case 0x32:// skip back 20
		case 0x33: // skip back 30
		case 0x34: // skip back 40
		case 0x35: // skip back 50
		case 0x36: // skip back 60
		case 0x37: // skip back 70
		case 0x38: // skip back 80
		case 0x39: // skip back 90
			{
				unsigned int tracks_to_skip = (unsigned int) (10 * (value & 0x0f));
				g_ldv1000i_skip_backward(tracks_to_skip);
			}
			break;

			// EXTENDED (NON-STANDARD) COMMANDS DEVELOPED FOR DEXTER
		case 0x90:	// hello
			ldv1000_push_queue(0xa2);
			break;
		case 0x91:	// query available discs
			if (!g_ldv1000_discswitch_pending)
			{
				const uint8_t *pDiscs = g_ldv1000i_query_available_discs();

				for (;;)
				{
					uint8_t val = *pDiscs;
					pDiscs++;
					ldv1000_push_queue(val);
					if (val == 0)
					{
						break;
					}
				}
			}
			break;
		case 0x92:	// query active disc
			if (!g_ldv1000_discswitch_pending)
			{
				ldv1000_push_queue(g_ldv1000i_query_active_disc());
			}
			break;
		case 0x93:	// prepare to switch discs
			// yes, we always have to go into this state because we need to 'eat' the subsequent byte even if we will be ignoring this command later
			g_ldv1000_discswitch_state = LDV1000_DISCSWITCH_WAITING_FOR_DISC_ID;
			break;

		case 0x94:	// change spin-up delay
			g_ldv1000i_change_spinup_delay((ldv1000_frame[LDV1000_FRAMESIZE - 1] & 1));
			clear();
			break;

		case 0x95:	// change seek delay
			g_ldv1000i_change_seek_delay((ldv1000_frame[LDV1000_FRAMESIZE - 1] & 1));
			clear();
			break;

		case 0x9D:	// disable super mode
			g_ldv1000i_change_super_mode(LDV1000_FALSE);
			break;

		case 0x9E:	// enable super mode
			g_ldv1000i_change_super_mode(LDV1000_TRUE);
			break;

		case 0xFF:	// NO ENTRY
			// it's legal to send the LD-V1000 as many of these as you want, we just ignore 'em
			g_ldv1000_output |= 0x80;	// set highbit just in case
			break;
		default:	// Unsupported Command
			{
				// this should never happen :)
				char s[3];
				g_ldv1000i_on_error("Unsupported Command");
				sprintf(s, "%2x", value);
				g_ldv1000i_on_error(s);
			}
			break;
		}
	}

	// if we are not ready, we can become ready by receiving a 0xFF command
	else
	{
		// if we got 0xFF (NO ENTRY) as expected
		if (value == 0xFF)
		{
				g_ldv1000_output |= 0x80;	// set high bit, we are now ready
		}

		// if we got a non NO ENTRY, we just ignore it, only the first non-NO ENTRY matters
		else
		{
			g_ldv1000_output &= 0x7F;	// clear high bit, we are no longer ready
		}
	}

}

// Adds a digit to the frame array that we will be seeking to.
// Digit should be in ASCII format
void ldv1000_add_digit(char digit)
{
	int count;
	// we need to set the high bit for Badlands - it might be caused by some emulation problem
	if (g_ldv1000_emulation_type == LDV1000_EMU_BADLANDS)
	{
		g_ldv1000_output |= 0x80;	
	}

	for (count = 0; count < LDV1000_FRAMESIZE - 1; count++)
	{
		if (!ldv1000_frame[count + 1])
		{
			ldv1000_frame[count + 1] = '0';
		}
		ldv1000_frame[count] = ldv1000_frame[count + 1];
	}
	ldv1000_frame[LDV1000_FRAMESIZE - 1] = digit;		
}

// Audio channel 1 on or off
void pre_audio1()
{
	// Check if we should just toggle
	if (!ldv1000_frame[LDV1000_FRAMESIZE - 1])
	{
		// Check status of audio and toggle accordingly
		if (audio1)
		{
			audio1 = LDV1000_FALSE;
			g_ldv1000i_change_audio(0, 0);	// disable left channel
		}
		else
		{
			audio1 = LDV1000_TRUE;
			g_ldv1000i_change_audio(0, 1);	// enable left channel
		}
	}
	// Or if we have an explicit audio command
	else 
	{
		switch (ldv1000_frame[LDV1000_FRAMESIZE - 1] & 1)
		{
		case 0:
			audio1 = LDV1000_FALSE;
			g_ldv1000i_change_audio(0, 0);
			break;
		default:
			audio1 = LDV1000_TRUE;
			g_ldv1000i_change_audio(0, 1);
			break;
		}
		clear();
	}
}

// Audio channel 2 on or off
void pre_audio2()
{
	// Check if we should just toggle
	if (!ldv1000_frame[LDV1000_FRAMESIZE - 1])
	{
		// Check status of audio and toggle accordingly
		if (audio2)
		{
			audio2 = LDV1000_FALSE;
			g_ldv1000i_change_audio(1, 0);
		}
		else
		{
			audio2 = LDV1000_TRUE;
			g_ldv1000i_change_audio(1, 1);
		}
	}
	// Or if we have an explicit audio command
	else 
	{
		switch (ldv1000_frame[LDV1000_FRAMESIZE - 1] & 1)
		{
		case 0:
			audio2 = LDV1000_FALSE;
			g_ldv1000i_change_audio(1, 0);
			break;
		default:
			audio2 = LDV1000_TRUE;
			g_ldv1000i_change_audio(1, 1);
			break;
		}
		clear();
	}
}

// returns the frame that has been entered in by add_digit thus far
unsigned int get_buffered_frame()
{
	ldv1000_frame[LDV1000_FRAMESIZE] = 0;	// terminate string
	return ((unsigned int) atoi(ldv1000_frame));
}

// clears any received digits from the frame array
void clear(void)
{
	memset(ldv1000_frame,0,sizeof(ldv1000_frame));
}
