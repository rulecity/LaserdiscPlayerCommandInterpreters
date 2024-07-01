#include "stdafx.h"
#include "pr7820_test_interface.h"

class pr7820_test_wrapper
{
public:
	static PR7820Status_t get_status() { return m_pInstance->GetStatus(); }
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void begin_search(uint32_t u32FrameNum) { m_pInstance->BeginSearch(u32FrameNum); }
	static void change_audio(uint8_t u8Channel, uint8_t u8Enabled) { m_pInstance->ChangeAudio(u8Channel, u8Enabled); }
	static void EnableSuperMode() { m_pInstance->EnableSuperMode(); }
	static void OnError(PR7820ErrCode_t code, uint8_t u8Val) { m_pInstance->OnError(code, u8Val); }

	static void setup(IPR7820Test *pInstance)
	{
		m_pInstance = pInstance;
		g_pr7820i_get_status = get_status;
		g_pr7820i_play = play;
		g_pr7820i_pause = pause;
		g_pr7820i_begin_search = begin_search;
		g_pr7820i_change_audio = change_audio;
		g_pr7820i_enable_super_mode = EnableSuperMode;
		g_pr7820i_on_error = OnError;
	}

private:
	static IPR7820Test *m_pInstance;
};

IPR7820Test *pr7820_test_wrapper::m_pInstance = 0;

///////////

void test_pr7820_playing()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, Play());

	pr7820i_reset();
	pr7820i_write(0xFD);	// play
}

TEST_CASE(pr7820_playing)
{
	test_pr7820_playing();
}

void test_pr7820_searching()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, BeginSearch(1));

	pr7820i_reset();
	pr7820i_write(0x0F);	// 1
	pr7820i_write(0xF7);	// search
}

TEST_CASE(pr7820_searching)
{
	test_pr7820_searching();
}

void test_pr7820_busy()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, GetStatus()).WillOnce(Return(PR7820_SEARCHING));

	pr7820i_reset();
	PR7820_BOOL b = pr7820i_is_busy();

	TEST_CHECK(b == PR7820_TRUE);

}

TEST_CASE(pr7820_busy)
{
	test_pr7820_busy();
}

void test_pr7820_spinup()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, GetStatus()).WillOnce(Return(PR7820_SPINNING_UP));

	pr7820i_reset();
	PR7820_BOOL b = pr7820i_is_busy();

	TEST_CHECK(b == PR7820_TRUE);

}

TEST_CASE(pr7820_spinup)
{
	test_pr7820_spinup();
}

void test_pr7820_search_then_pause()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, BeginSearch(1));
	EXPECT_CALL(mock, Pause());

	pr7820i_reset();
	pr7820i_write(0x0F);	// 1
	pr7820i_write(0xF7);	// search
	pr7820i_write(0xFB);	// pause
}

TEST_CASE(pr7820_search_then_pause)
{
	test_pr7820_search_then_pause();
}

void test_pr7820_mute_audio()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(0,0));
	EXPECT_CALL(mock, ChangeAudio(1,0));

	pr7820i_reset();
	pr7820i_write(0xA0);	// mute
}

TEST_CASE(pr7820_mute_audio)
{
	test_pr7820_mute_audio();
}

void test_pr7820_left_audio()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(0,1));
	EXPECT_CALL(mock, ChangeAudio(1,0));

	pr7820i_reset();
	pr7820i_write(0xA1);
}

TEST_CASE(pr7820_left_audio)
{
	test_pr7820_left_audio();
}

void test_pr7820_right_audio()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(0,0));
	EXPECT_CALL(mock, ChangeAudio(1,1));

	pr7820i_reset();
	pr7820i_write(0xA2);
}

TEST_CASE(pr7820_right_audio)
{
	test_pr7820_right_audio();
}

void test_pr7820_stereo_audio()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, ChangeAudio(0,1));
	EXPECT_CALL(mock, ChangeAudio(1,1));

	pr7820i_reset();
	pr7820i_write(0xA3);
}

TEST_CASE(pr7820_stereo_audio)
{
	test_pr7820_stereo_audio();
}

void test_pr7820_super_mode()
{
	MockPR7820Test mock;

	pr7820_test_wrapper::setup(&mock);

	EXPECT_CALL(mock, EnableSuperMode());

	pr7820i_reset();
	pr7820i_write(0x9E);
}

TEST_CASE(pr7820_super_mode)
{
	test_pr7820_super_mode();
}
