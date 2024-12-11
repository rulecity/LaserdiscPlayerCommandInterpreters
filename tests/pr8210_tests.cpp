#include "stdafx.h"
#include "pr8210_test_interface.h"

class pr8210_test_wrapper
{
public:
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void step(int8_t i8Tracks) { m_pInstance->Step(i8Tracks); }
	static void begin_search(uint32_t u32FrameNum) { m_pInstance->BeginSearch(u32FrameNum); }
	static void change_audio(uint8_t u8Channel, uint8_t u8Enabled) { m_pInstance->ChangeAudio(u8Channel, u8Enabled); }
	static void skip(int8_t i8TracksToSkip) { m_pInstance->Skip(i8TracksToSkip); }
	static void change_auto_track_jump(PR8210_BOOL bAutoTrackJumpDisabled) { m_pInstance->ChangeAutoTrackJump(bAutoTrackJumpDisabled != PR8210_FALSE); }
	static PR8210_BOOL is_player_busy() { return m_pInstance->IsPlayerBusy() ? PR8210_TRUE : PR8210_FALSE; }
	static void change_standby(PR8210_BOOL bEnabled) { m_pInstance->ChangeStandby(bEnabled != PR8210_FALSE ? true : false); }
	static void OnError(PR8210ErrCode_t code, uint16_t u16Val) { m_pInstance->OnError(code, u16Val); }

	static void setup(IPR8210Test *pInstance)
	{
		m_pInstance = pInstance;
		g_pr8210i_play = play;
		g_pr8210i_pause = pause;
		g_pr8210i_step = step;
		g_pr8210i_begin_search = begin_search;
		g_pr8210i_change_audio = change_audio;
		g_pr8210i_skip = skip;
		g_pr8210i_change_auto_track_jump = change_auto_track_jump;
		g_pr8210i_is_player_busy = is_player_busy;
		g_pr8210i_change_standby = change_standby;
		g_pr8210i_error = OnError;
	}

private:
	static IPR8210Test *m_pInstance;
};

IPR8210Test *pr8210_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void test_pr8210_playing()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, Play());

	pr8210i_reset();
	pr8210i_write(4 | (5 << 3));	// PLAY (5)
	pr8210i_write(4 | (5 << 3));	// PLAY (5)
	pr8210i_write(4 | (0 << 3));	// EOC (0), cliffy/cobra styled
	pr8210i_write(0 | (0 << 3));	// EOC (0), MACH 3 styled
}

TEST_CASE(pr8210_playing)
{
	test_pr8210_playing();
}

void test_pr8210_pausing()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, Pause());

	pr8210i_reset();
	pr8210i_write(4 | (0xA << 3));	// PAUSE (A)
	pr8210i_write(4 | (0xA << 3));	// PAUSE (A)
	pr8210i_write(4 | (0 << 3));	// EOC (0), cliffy/cobra styled
}

TEST_CASE(pr8210_pausing)
{
	test_pr8210_pausing();
}

void test_pr8210_seeking()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	{
		InSequence dummy;
		EXPECT_CALL(mock, BeginSearch(12345));
		EXPECT_CALL(mock, ChangeStandby(true));
		EXPECT_CALL(mock, BeginSearch(67890));
		EXPECT_CALL(mock, ChangeStandby(true));
	}

	pr8210i_reset();
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x15 << 3));	// 5
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x10 << 3));	// 0
	pr8210i_write(4 | (0x10 << 3));	// 0
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
}

TEST_CASE(pr8210_seeking)
{
	test_pr8210_seeking();
}

void test_pr8210_seeking_extra_commands()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	{
		InSequence dummy;
		EXPECT_CALL(mock, BeginSearch(1));
		EXPECT_CALL(mock, ChangeStandby(true));
		EXPECT_CALL(mock, BeginSearch(67890));
		EXPECT_CALL(mock, ChangeStandby(true));
	}

	pr8210i_reset();

	// behave like goal to go
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	// intentionally only do 1 digit to test to make sure it works
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	// TODO : should we institute a delay here so the old search command can time out?

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x10 << 3));	// 0
	pr8210i_write(4 | (0x10 << 3));	// 0
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
}

TEST_CASE(pr8210_seeking_extra_commands)
{
	test_pr8210_seeking_extra_commands();
}

void test_pr8210_seeking_too_many_digits()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, BeginSearch(12345));
	EXPECT_CALL(mock, ChangeStandby(true)).Times(2);
	EXPECT_CALL(mock, BeginSearch(67890));
	EXPECT_CALL(mock, OnError(PR8210_ERR_TOO_MANY_DIGITS, 0));
	EXPECT_CALL(mock, Step(1));
	EXPECT_CALL(mock, Step(-1));
	EXPECT_CALL(mock, Play());
	EXPECT_CALL(mock, Pause());

	pr8210i_reset();
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x16 << 3));	// 6	(too many digits)
	pr8210i_write(4 | (0x16 << 3));	// 6
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x4 << 3));	// step forward
	pr8210i_write(4 | (0x4 << 3));
	pr8210i_write(4 | (0x9 << 3));	// step backward
	pr8210i_write(4 | (0x9 << 3));
	pr8210i_write(4 | (5 << 3));	// PLAY (5)
	pr8210i_write(4 | (5 << 3));	// PLAY (5)
	pr8210i_write(4 | (0xA << 3));	// PAUSE
	pr8210i_write(4 | (0xA << 3));	// PAUSE

	// at this point we should be able to start a new seek and have it work

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x16 << 3));	// 6
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x17 << 3));	// 7
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x18 << 3));	// 8
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x19 << 3));	// 9
	pr8210i_write(4 | (0x10 << 3));	// 0
	pr8210i_write(4 | (0x10 << 3));	// 0
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
}

TEST_CASE(pr8210_seeking_too_many_digits)
{
	test_pr8210_seeking_too_many_digits();
}

void test_pr8210_audio1L()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(0, 0));

	pr8210i_reset();
	pr8210i_write(4 | (0xE << 3));	// TOGGLE L AUDIO (E)
	pr8210i_write(4 | (0xE << 3));	// TOGGLE L AUDIO (E)
}

TEST_CASE(pr8210_audio1L)
{
	test_pr8210_audio1L();
}

void test_pr8210_audio2R()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(1, 0));

	pr8210i_reset();
	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO
	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO
}

TEST_CASE(pr8210_audio2R)
{
	test_pr8210_audio2R();
}

void test_pr8210_audio2R2()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(1, 0));
	EXPECT_CALL(mock, ChangeAudio(1, 1));

	pr8210i_reset();
	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO
	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO

	pr8210i_write(4 | (0x0 << 3));	// filler

	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO
	pr8210i_write(4 | (0xD << 3));	// TOGGLE R AUDIO
}

TEST_CASE(pr8210_audio2R2)
{
	test_pr8210_audio2R2();
}

void test_pr8210_step_forward()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, Step(1));

	pr8210i_reset();
	pr8210i_write(4 | (0x4 << 3));	// step forward
	pr8210i_write(4 | (0x4 << 3));
}

TEST_CASE(pr8210_step_forward)
{
	test_pr8210_step_forward();
}

void test_pr8210_step_backward()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, Step(-1));

	pr8210i_reset();
	pr8210i_write(4 | (0x9 << 3));	// step backward
	pr8210i_write(4 | (0x9 << 3));
}

TEST_CASE(pr8210_step_backward)
{
	test_pr8210_step_backward();
}

void test_pr8210_jump_trigger_1back()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAutoTrackJump(false));
	EXPECT_CALL(mock, Skip(-1));

	pr8210i_reset();

	// assumed we are in internal mode
	pr8210i_on_jmp_trigger_changed(PR8210_FALSE, PR8210_FALSE);	// jump 1 track backward, this should have no effect
	pr8210i_on_jmp_trigger_changed(PR8210_TRUE, PR8210_FALSE);	// raise track jump line

	pr8210i_on_jmptrig_and_scanc_intext_changed(PR8210_FALSE);	// go external (which disables auto track jump)
	pr8210i_on_jmp_trigger_changed(PR8210_FALSE, PR8210_FALSE);	// jump 1 track backward, this should have effect
	pr8210i_on_jmp_trigger_changed(PR8210_FALSE, PR8210_FALSE);	// make sure that lowering an already low line has no effect
	pr8210i_on_jmp_trigger_changed(PR8210_TRUE, PR8210_FALSE);	// raise track jump line
}

TEST_CASE(pr8210_jump_trigger_1back)
{
	test_pr8210_jump_trigger_1back();
}

void test_pr8210_jump_trigger_1forward()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAutoTrackJump(false));
	EXPECT_CALL(mock, Skip(1));

	pr8210i_reset();

	// assumed we are in internal mode
	pr8210i_on_jmp_trigger_changed(PR8210_FALSE, PR8210_TRUE);	// jump 1 track forward, this should have no effect until we go external
	pr8210i_on_jmptrig_and_scanc_intext_changed(PR8210_FALSE);	// go external (this should trigger a skip)
	pr8210i_on_jmp_trigger_changed(PR8210_TRUE, PR8210_FALSE);	// raise track jump line
}

TEST_CASE(pr8210_jump_trigger_1forward)
{
	test_pr8210_jump_trigger_1forward();
}

void test_pr8210_standby_blink()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, IsPlayerBusy()).WillRepeatedly(Return(true));

	{
		InSequence dummy;
		EXPECT_CALL(mock, BeginSearch(12345));
		EXPECT_CALL(mock, ChangeStandby(true));
		EXPECT_CALL(mock, ChangeStandby(false));
	}

	pr8210i_reset();

	// do a search to trigger stand by going high

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x15 << 3));	// 5
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	// this should trigger stand by to become disabled
	for (int i = 0; i < 13; i++)
	{
		pr8210i_on_vblank();
	}
}

TEST_CASE(pr8210_standby_blink)
{
	test_pr8210_standby_blink();
}

void test_pr8210_standby_blink_long()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, IsPlayerBusy()).WillRepeatedly(Return(true));

	{
		InSequence dummy;
		EXPECT_CALL(mock, BeginSearch(12345));
		EXPECT_CALL(mock, ChangeStandby(true));
		EXPECT_CALL(mock, ChangeStandby(false));
		EXPECT_CALL(mock, ChangeStandby(true));
	}

	pr8210i_reset();

	// do a search to trigger stand by going high

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x15 << 3));	// 5
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	// this should trigger stand by to become disabled
	for (int i = 0; i < 11; i++)
	{
		pr8210i_on_vblank();
	}

	// this should trigger stand by to become enabled
	for (int i = 0; i < 11; i++)
	{
		pr8210i_on_vblank();
	}

	// this should almost trigger stand by to become disabled (but not quite)
	for (int i = 0; i < 10; i++)
	{
		pr8210i_on_vblank();
	}
}

TEST_CASE(pr8210_standby_blink_long)
{
	test_pr8210_standby_blink_long();
}

void test_pr8210_standby_end()
{
	MockPR8210Test mock;

	pr8210_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, IsPlayerBusy()).WillRepeatedly(Return(false));

	{
		InSequence dummy;
		EXPECT_CALL(mock, BeginSearch(12345));
		EXPECT_CALL(mock, ChangeStandby(true));
		EXPECT_CALL(mock, ChangeStandby(false));
	}

	pr8210i_reset();

	// do a search to trigger stand by going high

	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x11 << 3));	// 1
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x12 << 3));	// 2
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x13 << 3));	// 3
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x14 << 3));	// 4
	pr8210i_write(4 | (0x15 << 3));	// 5
	pr8210i_write(4 | (0x15 << 3));	// 5
	
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)
	pr8210i_write(4 | (0xB << 3));	// SEARCH (B)

	// this should trigger stand by to become disabled because player won't be busy
	pr8210i_on_vblank();
}

TEST_CASE(pr8210_standby_end)
{
	test_pr8210_standby_end();
}
