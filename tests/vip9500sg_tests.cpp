#include "stdafx.h"
#include "vip9500sg_test_interface.h"

class vip9500sg_test_wrapper
{
public:
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void stop() { m_pInstance->Stop(); }
	static void step_reverse() { m_pInstance->StepReverse(); }
	static void begin_search(uint32_t u32FrameNum) { m_pInstance->BeginSearch(u32FrameNum); }
	static void skip(int32_t i32TracksToSkip) { m_pInstance->Skip(i32TracksToSkip); }
	static void change_audio(uint8_t u8Channel, uint8_t u8Enabled) { m_pInstance->ChangeAudio(u8Channel, u8Enabled); }
	static VIP9500SGStatus_t GetStatus() { return m_pInstance->GetStatus(); }
	static uint32_t get_cur_frame() { return m_pInstance->GetCurFrame(); }
	static uint32_t get_vbi_line18() { return m_pInstance->GetVBILine18(); }

	static void OnError(VIP9500SGErrCode_t code, uint8_t u8Val) { m_pInstance->OnError(code, u8Val); }

	static void setup(IVIP9500SGTest *pInstance)
	{
		m_pInstance = pInstance;
		g_vip9500sgi_play = play;
		g_vip9500sgi_pause = pause;
		g_vip9500sgi_stop = stop;
		g_vip9500sgi_step_reverse = step_reverse;
		g_vip9500sgi_begin_search = begin_search;
		g_vip9500sgi_skip = skip;
		g_vip9500sgi_change_audio = change_audio;
		g_vip9500sgi_get_status = GetStatus;
		g_vip9500sgi_get_cur_frame_num = get_cur_frame;
		g_vip9500sgi_get_cur_vbi_line18 = get_vbi_line18;
		g_vip9500sgi_error = OnError;
	}

private:
	static IVIP9500SGTest *m_pInstance;
};

IVIP9500SGTest *vip9500sg_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void test_vip9500sg_playing()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, Play());

	{
		InSequence seq;
		EXPECT_CALL(mockVIP9500SG, GetStatus()).WillOnce(Return(VIP9500SG_SPINNING_UP));
		EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));
	}

	uint8_t u8;

	vip9500sgi_reset();
	vip9500sgi_write(0x25);	// send play

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// spinning up
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// let play complete
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0xA5);
}

TEST_CASE(vip9500sg_playing)
{
	test_vip9500sg_playing();
}

void test_vip9500sg_playing_1x()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, Play());
	EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));

	uint8_t u8;

	vip9500sgi_reset();
	vip9500sgi_write(0x53);	// send play 1x

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// let play complete
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0xD3);
}

TEST_CASE(vip9500sg_playing_1x)
{
	test_vip9500sg_playing_1x();
}

void test_vip9500sg_playing_with_frame_query()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, Play());

	{
		InSequence seq;
		EXPECT_CALL(mockVIP9500SG, GetStatus()).Times(2).WillRepeatedly(Return(VIP9500SG_SPINNING_UP));
		EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));
	}

	uint8_t u8;

	vip9500sgi_reset();
	vip9500sgi_write(0x25);	// send play

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// spinning up
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	vip9500sgi_write(0x6b);	// send frame query command before spin-up has completed

	// get the expected error from querying current frame during spin-up
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0x1D);	// error code

	// let spin-up complete
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0xA5);
}

TEST_CASE(vip9500sg_playing_with_frame_query)
{
	test_vip9500sg_playing_with_frame_query();
}

void test_vip9500sg_stop()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, Stop());

	uint8_t u8;

	vip9500sgi_write(0x2f);	// send stop

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0xAF);
}

TEST_CASE(vip9500sg_stop)
{
	test_vip9500sg_stop();
}

void test_vip9500sg_step_reverse()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, StepReverse());

	{
		InSequence seq;
		EXPECT_CALL(mockVIP9500SG, GetStatus()).WillOnce(Return(VIP9500SG_STEPPING));
		EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PAUSED));
	}

	uint8_t u8;

	vip9500sgi_reset();
	vip9500sgi_write(0x29);	// send step reverse

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// stepping
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// let operation complete
	vip9500sgi_think_after_vblank();
	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(u8, 0xA9);
}

TEST_CASE(vip9500sg_step_reverse)
{
	test_vip9500sg_step_reverse();
}

void test_vip9500sg_searching()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, BeginSearch(12345));
	EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PAUSED));

	uint8_t u8;

	vip9500sgi_reset();

	vip9500sgi_write(0x2b);	// start search
	vip9500sgi_write('1');	// M1
	vip9500sgi_write('2');	// M2
	vip9500sgi_write('3');	// M3
	vip9500sgi_write('4');	// M4
	vip9500sgi_write('5');	// M5
	vip9500sgi_write(0x41);	// enter

	// let search complete
	vip9500sgi_think_after_vblank();

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(0x41, u8);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(0xb0, u8);
}

TEST_CASE(vip9500sg_searching)
{
	test_vip9500sg_searching();
}

void test_vip9500sg_searching_extra_digits()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, BeginSearch(123));
	EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PAUSED));

	uint8_t u8;

	vip9500sgi_reset();

	vip9500sgi_write(0x2b);	// start search
	vip9500sgi_write('9');	// extra digit
	vip9500sgi_write('9');	// extra digit
	vip9500sgi_write('9');	// extra digit
	vip9500sgi_write('9');	// extra digit
	vip9500sgi_write('9');	// extra digit

	vip9500sgi_write('0');	// M1
	vip9500sgi_write('0');	// M2
	vip9500sgi_write('1');	// M3
	vip9500sgi_write('2');	// M4
	vip9500sgi_write('3');	// M5
	vip9500sgi_write(0x41);	// enter

	// let search complete
	vip9500sgi_think_after_vblank();

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(0x41, u8);

	u8 = vip9500sgi_read();
	TEST_CHECK_EQUAL(0xb0, u8);
}

TEST_CASE(vip9500sg_searching_extra_digits)
{
	test_vip9500sg_searching_extra_digits();
}

void test_vip9500sg_get_cur_frame_playing()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, GetCurFrame()).WillOnce(Return(12345));
	EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));

	{
		InSequence seq;
		EXPECT_CALL(mockVIP9500SG, GetVBILine18()).WillOnce(Return(0));
		EXPECT_CALL(mockVIP9500SG, GetVBILine18()).WillRepeatedly(Return(0xF92345));
	}

	vip9500sgi_reset();
	vip9500sgi_write(0x6B);	// get current frame

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// get field that doesn't have VBI
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// get field that does have VBI
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	TEST_CHECK_EQUAL(0x6B, vip9500sgi_read());
	TEST_CHECK_EQUAL(0x30, vip9500sgi_read());	// 12345 is 0x3039
	TEST_CHECK_EQUAL(0x39, vip9500sgi_read());
}

TEST_CASE(vip9500sg_get_cur_frame_playing)
{
	test_vip9500sg_get_cur_frame_playing();
}

void test_vip9500sg_get_cur_frame_stopped()
{
	MockVIP9500SGTest mockVIP9500SG;

	vip9500sg_test_wrapper::setup(&mockVIP9500SG);

	EXPECT_CALL(mockVIP9500SG, GetStatus()).WillRepeatedly(Return(VIP9500SG_STOPPED));

	vip9500sgi_reset();
	vip9500sgi_write(0x6B);	// get current frame

	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	// not sure what a real LDP would do in this case so I have to fake it
	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	// real LDP returns 0x1D (error)
	TEST_CHECK_EQUAL(0x1D, vip9500sgi_read());
}

TEST_CASE(vip9500sg_get_cur_frame_stopped)
{
	test_vip9500sg_get_cur_frame_stopped();
}

void test_vip9500sg_skip_forward()
{
	MockVIP9500SGTest mockLDP;

	vip9500sg_test_wrapper::setup(&mockLDP);

	EXPECT_CALL(mockLDP, Skip(91));	// quirk of LDP
	EXPECT_CALL(mockLDP, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));

	vip9500sgi_reset();
	vip9500sgi_write(0x46);	// skip forward
	vip9500sgi_write('0');
	vip9500sgi_write('9');
	vip9500sgi_write('0');
	vip9500sgi_write(0x41);	// enter
	TEST_CHECK_EQUAL(0x41, vip9500sgi_read());

	// make sure there is a delay until vblank
	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	TEST_CHECK_EQUAL(0xc6, vip9500sgi_read());	// skip success
}

TEST_CASE(vip9500sg_skip_forward)
{
	test_vip9500sg_skip_forward();
}

void test_vip9500sg_skip_forward_paused()
{
	MockVIP9500SGTest mockLDP;

	vip9500sg_test_wrapper::setup(&mockLDP);

	EXPECT_CALL(mockLDP, Skip(91));	// quirk of LDP
	EXPECT_CALL(mockLDP, GetStatus()).WillRepeatedly(Return(VIP9500SG_PAUSED));

	vip9500sgi_reset();
	vip9500sgi_write(0x46);	// skip forward
	vip9500sgi_write('0');
	vip9500sgi_write('9');
	vip9500sgi_write('0');
	vip9500sgi_write(0x41);	// enter
	TEST_CHECK_EQUAL(0x41, vip9500sgi_read());

	// make sure there is a delay until vblank
	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	TEST_CHECK_EQUAL(0xc6, vip9500sgi_read());	// skip success
}

TEST_CASE(vip9500sg_skip_forward_paused)
{
	test_vip9500sg_skip_forward_paused();
}

void test_vip9500sg_skip_backward()
{
	MockVIP9500SGTest mockLDP;

	vip9500sg_test_wrapper::setup(&mockLDP);

	EXPECT_CALL(mockLDP, Skip(-89));	// quirk of LDP
	EXPECT_CALL(mockLDP, GetStatus()).WillRepeatedly(Return(VIP9500SG_PLAYING));

	vip9500sgi_reset();
	vip9500sgi_write(0x47);	// skip backward
	vip9500sgi_write('0');
	vip9500sgi_write('9');
	vip9500sgi_write('0');
	vip9500sgi_write(0x41);	// enter
	TEST_CHECK_EQUAL(0x41, vip9500sgi_read());

	// make sure there is a delay until vblank
	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	TEST_CHECK_EQUAL(0xc7, vip9500sgi_read());	// skip success
}

TEST_CASE(vip9500sg_skip_backward)
{
	test_vip9500sg_skip_backward();
}

void test_vip9500sg_skip_backward_paused()
{
	MockVIP9500SGTest mockLDP;

	vip9500sg_test_wrapper::setup(&mockLDP);

	EXPECT_CALL(mockLDP, Skip(-89));	// quirk of LDP
	EXPECT_CALL(mockLDP, GetStatus()).WillRepeatedly(Return(VIP9500SG_PAUSED));

	vip9500sgi_reset();
	vip9500sgi_write(0x47);	// skip backward
	vip9500sgi_write('0');
	vip9500sgi_write('9');
	vip9500sgi_write('0');
	vip9500sgi_write(0x41);	// enter
	TEST_CHECK_EQUAL(0x41, vip9500sgi_read());

	// make sure there is a delay until vblank
	VIP9500SG_BOOL b = vip9500sgi_can_read();
	TEST_REQUIRE(b == 0);

	vip9500sgi_think_after_vblank();

	b = vip9500sgi_can_read();
	TEST_REQUIRE(b != 0);

	TEST_CHECK_EQUAL(0xc7, vip9500sgi_read());	// skip success
}

TEST_CASE(vip9500sg_skip_backward_paused)
{
	test_vip9500sg_skip_backward_paused();
}

void test_vip9500sg_reset()
{
	MockVIP9500SGTest mockLDP;

	vip9500sg_test_wrapper::setup(&mockLDP);

	vip9500sgi_reset();
	vip9500sgi_write(0x68);	// reset
	TEST_CHECK_EQUAL(0xe8, vip9500sgi_read());
	vip9500sgi_write(0x75);	// another type of reset
	TEST_CHECK_EQUAL(0xf5, vip9500sgi_read());
}

TEST_CASE(vip9500sg_reset)
{
	test_vip9500sg_reset();
}
