#include "stdafx.h"
#include "vp931_test_interface.h"

class vp931_test_wrapper
{
public:
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void begin_search(uint32_t u32FrameNumber, VP931_BOOL bSquelch) { m_pInstance->BeginSearch(u32FrameNumber, (bSquelch != VP931_FALSE)); }
	static void skip(int16_t iTracks) { m_pInstance->Skip(iTracks); }
	static void skip_to_framenum(uint32_t u32FrameNum) { m_pInstance->SkipToFrameNum(u32FrameNum); }
	static void OnError(VP931ErrCode_t code, uint8_t u8Val) { m_pInstance->OnError(code, u8Val); }

	static void setup(IVP931Test *pInstance)
	{
		m_pInstance = pInstance;
		g_vp931i_play = play;
		g_vp931i_pause = pause;
		g_vp931i_begin_search = begin_search;
		g_vp931i_skip_tracks = skip;
		g_vp931i_skip_to_framenum = skip_to_framenum;
		g_vp931i_error = OnError;
	}

private:
	static IVP931Test *m_pInstance;
};

IVP931Test *vp931_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void test_vp931_basic_status()
{
	uint32_t u32VBILine18_1 = 0xf80001;
	uint8_t arrTestStatus[6];

	vp931i_reset();

	vp931i_get_status_bytes(u32VBILine18_1, VP931_PLAYING, arrTestStatus);

	// now check the value that was read
	TEST_CHECK_EQUAL(0xF8, arrTestStatus[0]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[1]);
	TEST_CHECK_EQUAL(0x01, arrTestStatus[2]);
	TEST_CHECK_EQUAL(0x04, arrTestStatus[3]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[4]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[5]);
}

TEST_CASE(vp931_basic_status)
{
	test_vp931_basic_status();
}

void test_vp931_seeking()
{
	uint32_t u32VBILine18_1 = 0xf80001;
	uint8_t arrTestStatus[6];

	vp931i_reset();

	vp931i_get_status_bytes(u32VBILine18_1, VP931_SEARCHING, arrTestStatus);

	// should get a seek status
	TEST_CHECK_EQUAL(0x00, arrTestStatus[0]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[1]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[2]);
	TEST_CHECK_EQUAL(0x08, arrTestStatus[3]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[4]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[5]);
}

TEST_CASE(vp931_seeking)
{
	test_vp931_seeking();
}

void test_vp931_paused()
{
	uint32_t u32VBILine18_1 = 0xf80001;
	uint8_t arrTestStatus[6];

	vp931i_reset();

	vp931i_get_status_bytes(u32VBILine18_1, VP931_PAUSED, arrTestStatus);

	// now check the value that was read
	TEST_CHECK_EQUAL(0xF8, arrTestStatus[0]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[1]);
	TEST_CHECK_EQUAL(0x01, arrTestStatus[2]);
	TEST_CHECK_EQUAL(0x05, arrTestStatus[3]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[4]);
	TEST_CHECK_EQUAL(0x00, arrTestStatus[5]);
}

TEST_CASE(vp931_paused)
{
	test_vp931_paused();
}

void test_vp931_seek_then_play()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);

	{
		InSequence dummy;
		EXPECT_CALL(mockVP931, Play());
		EXPECT_CALL(mockVP931, SkipToFrameNum(12345));
	}
	
	vp931i_reset();

	cmdbuf[0] = 0xF1;
	cmdbuf[1] = 0x23;
	cmdbuf[2] = 0x45;
	vp931i_on_vsync(cmdbuf, 3, VP931_ERROR);	// make sure we can recover from an error condition
}

TEST_CASE(vp931_seek_then_play)
{
	test_vp931_seek_then_play();
}

void test_vp931_seek_then_play_syi()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, SkipToFrameNum(1803));

	vp931i_reset();

	cmdbuf[0] = 0xF0;
	cmdbuf[1] = 0x18;
	cmdbuf[2] = 0x03;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);
}

TEST_CASE(vp931_seek_then_play_syi)
{
	test_vp931_seek_then_play_syi();
}

void test_vp931_seek_no_play()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, BeginSearch(12345, true));

	vp931i_reset();

	// cause a seek with a latch to play when seek finishes
	cmdbuf[0] = 0xD1;
	cmdbuf[1] = 0x23;
	cmdbuf[2] = 0x45;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);

	// now call it again with no command but with a 'paused' status; it should _not_ cause a play
	vp931i_on_vsync(NULL, 0, VP931_PAUSED);
}

TEST_CASE(vp931_seek_no_play)
{
	test_vp931_seek_no_play();
}

void test_vp931_skip_forward()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, Skip(90));

	vp931i_reset();

	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0xE0;
	cmdbuf[2] = 0x90;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);
}

TEST_CASE(vp931_skip_forward)
{
	test_vp931_skip_forward();
}

void test_vp931_skip_backward()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, Skip(-90));

	vp931i_reset();

	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0xF0;
	cmdbuf[2] = 0x90;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);
}

TEST_CASE(vp931_skip_backward)
{
	test_vp931_skip_backward();
}

void test_vp931_pause()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, Pause());

	vp931i_reset();

	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0x20;
	cmdbuf[2] = 0x00;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);
}

TEST_CASE(vp931_pause)
{
	test_vp931_pause();
}

void test_vp931_audio_options()
{
	MockVP931Test mockVP931;
	uint8_t cmdbuf[3];

	vp931_test_wrapper::setup(&mockVP931);
	
	EXPECT_CALL(mockVP931, OnError(VP931_ERR_UNSUPPORTED_CMD_BYTE, 0x02));

	vp931i_reset();

	cmdbuf[0] = 0x02;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	vp931i_on_vsync(cmdbuf, 3, VP931_PLAYING);
}

TEST_CASE(vp931_audio_options)
{
	test_vp931_audio_options();
}
