#include "stdafx.h"
#include "vp932_test_interface.h"

using ::testing::StrictMock;

class vp932_test_wrapper
{
public:
	static void play(uint8_t u8Numerator, uint8_t u8Denominator, VP932_BOOL bBackward, VP932_BOOL bAudioSquelched)
	{
		m_pInstance->Play(u8Numerator, u8Denominator, bBackward == VP932_TRUE, bAudioSquelched == VP932_TRUE);
	}

	static void step(VP932_BOOL bBackward) { m_pInstance->Step(bBackward == VP932_TRUE); }
	static void pause() { m_pInstance->Pause(); }
	static void begin_search(uint32_t u32FrameNumber) { m_pInstance->BeginSearch(u32FrameNumber); }
	static void change_audio(uint8_t u8Channel, uint8_t u8Enable) { m_pInstance->ChangeAudio(u8Channel, u8Enable); }
	static uint32_t get_cur_frame_num() { return m_pInstance->GetCurFrameNum(); }
	static void OnError(VP932ErrCode_t code, uint8_t u8Val) { m_pInstance->OnError(code, u8Val); }

	static void setup(IVP932Test *pInstance)
	{
		m_pInstance = pInstance;
		g_vp932i_play = play;
		g_vp932i_step = step;
		g_vp932i_pause = pause;
		g_vp932i_begin_search = begin_search;
		g_vp932i_change_audio = change_audio;
		g_vp932i_get_cur_frame_num = get_cur_frame_num;
		g_vp932i_error = OnError;
	}

private:
	static IVP932Test *m_pInstance;
};

IVP932Test *vp932_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void test_write(const char *s)
{
	int idx = 0;

	while (s[idx] != 0)
	{
		vp932i_write(s[idx++]);
	}
}

void test_vp932_search_and_pause()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	EXPECT_CALL(mockVP932, BeginSearch(12345));

	vp932i_reset();

	test_write("F12345R\r");

	// send a few vblank status updates and make sure no other command (such as play) comes in
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

}

TEST_CASE(vp932_search_and_pause)
{
	test_vp932_search_and_pause();
}

void test_vp932_search_play_and_search()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP932, BeginSearch(166));
		EXPECT_CALL(mockVP932, Play(1, 1, false, true));
		EXPECT_CALL(mockVP932, BeginSearch(166));
	}

	vp932i_reset();

	test_write("F00166R\r");

	// send a few vblank status updates and make sure no other command (such as play) comes in
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

	// play silently
	test_write("U\r");
	vp932i_think_during_vblank(VP932_PLAYING);

	// now do another search; should work
	test_write("F00166R\r");
}

TEST_CASE(vp932_search_play_and_search)
{
	test_vp932_search_play_and_search();
}

void test_vp932_search_play_and_search2()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP932, BeginSearch(166));
		EXPECT_CALL(mockVP932, Play(1, 1, true, true));
		EXPECT_CALL(mockVP932, BeginSearch(166));
	}

	vp932i_reset();

	test_write("00166R\r");	// intentionally ommitting prefix 'F' to give variety to test

	// send a few vblank status updates and make sure no other command (such as play) comes in
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

	// play silently (backward)
	test_write("V\r");
	vp932i_think_during_vblank(VP932_PLAYING);

	// now do another search; should work
	test_write("F00166R\r");
}

TEST_CASE(vp932_search_play_and_search2)
{
	test_vp932_search_play_and_search2();
}

void test_vp932_search_and_play()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	EXPECT_CALL(mockVP932, BeginSearch(12345));
	EXPECT_CALL(mockVP932, Play(1, 1, false, false));

	vp932i_reset();

	// send a bunch of spurious spam like DL Sidam's test mode does to make sure it is handled
	vp932i_write('F');
	vp932i_write(0);
	vp932i_write('F');
	vp932i_write(0);

	test_write("F12345N\r");

	// send a few vblank status updates to trigger play once search is done
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('1', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

}

TEST_CASE(vp932_search_and_play)
{
	test_vp932_search_and_play();
}

void test_vp932_unknown_cmd()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	EXPECT_CALL(mockVP932, OnError(VP932_ERR_UNKNOWN_CMD_BYTE, '^'));

	vp932i_reset();

	test_write("^\r");	// unknown byte (for now)
}

TEST_CASE(vp932_unknown_cmd)
{
	test_vp932_unknown_cmd();
}

void test_vp932_R_without_F()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP932, BeginSearch(166));
	}

	vp932i_reset();

	// Logic analyzer shows dl euro board sending this on power up for some reason.
	// Teo says that we need to return 'A0' to make the game work.  Apparently original player could partially handle it.
	vp932i_write(0);
	vp932i_write('X');
	vp932i_write(0);
	vp932i_write('0');
	vp932i_write('0');
	vp932i_write('1');
	vp932i_write('6');
	vp932i_write('6');
	vp932i_write('R');
	vp932i_write('\r');

	// make the seek finish
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

}

TEST_CASE(vp932_R_without_F)
{
	test_vp932_R_without_F();
}

void test_vp932_silent_play()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP932, Play(1, 1, false, true));
		EXPECT_CALL(mockVP932, BeginSearch(1210));
		EXPECT_CALL(mockVP932, BeginSearch(1281));
		EXPECT_CALL(mockVP932, Play(1, 1, false, false));
	}

	vp932i_reset();

	test_write("S002\r");
	test_write("U\r");
	test_write("F01210R\r");

	// make the seek finish
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	test_write("F01281N\r");

	// make the seek finish
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

}

TEST_CASE(vp932_silent_play)
{
	test_vp932_silent_play();
}

void test_vp932_repeated_seek()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP932, BeginSearch(1210));

		// no seek should occur if we're already on our target frame
		EXPECT_CALL(mockVP932, Play(1, 1, false, false));
	}

	vp932i_reset();

	test_write("S002\r");
	test_write("F01210R\r");

	// make the seek finish
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

	// spam search to the same frame and make sure we get a response
	test_write("F01210R\r");

	// status should never change from paused
	vp932i_think_during_vblank(VP932_PAUSED);

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('0', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

	test_write("F01210N\r");

	// make no-op seek 'finish', issue play command
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('1', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

}

TEST_CASE(vp932_repeated_seek)
{
	test_vp932_repeated_seek();
}

void test_vp932_search_play_and_pause()
{
	MockVP932Test mockVP932;

	vp932_test_wrapper::setup(&mockVP932);

	EXPECT_CALL(mockVP932, BeginSearch(12345));
	EXPECT_CALL(mockVP932, Play(1, 1, false, false));
	EXPECT_CALL(mockVP932, Pause());

	vp932i_reset();

	test_write("F12345N\r");

	// send a few vblank status updates to trigger play once search is done
	vp932i_think_during_vblank(VP932_SEARCHING);
	vp932i_think_during_vblank(VP932_PAUSED);

	// make sure we get an expected response
	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('A', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('1', vp932i_read());

	TEST_REQUIRE_EQUAL(VP932_TRUE, vp932i_can_read());
	TEST_CHECK_EQUAL('\r', vp932i_read());

	test_write("*\r");

}

TEST_CASE(vp932_search_play_and_pause)
{
	test_vp932_search_play_and_pause();
}

void test_vp932_null_byte()
{
	MockVP932Test mockVP932;

	// we should not get an error
	EXPECT_CALL(mockVP932, OnError(_, _)).Times(0);

	vp932_test_wrapper::setup(&mockVP932);

	vp932i_reset();

	// send a NULL (DL Euro does this spuriously)
	vp932i_write(0);
	vp932i_write('\r');
}

TEST_CASE(vp932_null_byte)
{
	test_vp932_null_byte();
}

void test_vp932_multispeed()
{
	MockVP932Test mockVP932;

	{
		InSequence dummy;

		EXPECT_CALL(mockVP932, Play(1, 1, false, true));
		EXPECT_CALL(mockVP932, Play(1, 1, true, true));
	}

	vp932_test_wrapper::setup(&mockVP932);

	vp932i_reset();

	// multispeed forward
	test_write("U\r");

	// multispeed reverse
	test_write("V\r");
}

TEST_CASE(vp932_multispeed)
{
	test_vp932_multispeed();
}

void test_vp932_step()
{
	MockVP932Test mockVP932;

	{
		InSequence dummy;

		EXPECT_CALL(mockVP932, Step(false));
		EXPECT_CALL(mockVP932, Step(true));
	}

	vp932_test_wrapper::setup(&mockVP932);

	vp932i_reset();

	// step forward
	test_write("L\r");

	// step reverse
	test_write("M\r");
}

TEST_CASE(vp932_step)
{
	test_vp932_step();
}

void test_vp932_display()
{
	MockVP932Test mockVP932;

	// make sure that receiving a 'D' does not result in an error
	EXPECT_CALL(mockVP932, OnError(_, _)).Times(0);

	vp932_test_wrapper::setup(&mockVP932);

	vp932i_reset();

	// disply
	test_write("D\r");
}

TEST_CASE(vp932_display)
{
	test_vp932_display();
}
