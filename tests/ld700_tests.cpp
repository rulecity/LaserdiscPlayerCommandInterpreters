#include "stdafx.h"
#include "ld700_test_interface.h"

class ld700_test_wrapper
{
public:
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void stop() { m_pInstance->Stop(); }
	static void eject() { m_pInstance->Eject(); }
	static void begin_search(uint32_t u32FrameNum) { m_pInstance->BeginSearch(u32FrameNum); }
	static LD700Status_t get_status() { return m_pInstance->GetStatus(); }
	static void on_ext_ack_changed(LD700_BOOL bActive) { m_pInstance->OnExtAckChanged(bActive); }
	static void on_error(LD700ErrCode_t err, uint8_t val) { m_pInstance->OnError(err, val); }

	static void setup(ILD700Test *pInstance)
	{
		m_pInstance = pInstance;
		g_ld700i_play = play;
		g_ld700i_pause = pause;
		g_ld700i_stop = stop;
		g_ld700i_eject = eject;
		g_ld700i_begin_search = begin_search;
		g_ld700i_get_status = get_status;
		g_ld700i_on_ext_ack_changed = on_ext_ack_changed;
		g_ld700i_error = on_error;
	}

private:
	static ILD700Test *m_pInstance;
};

ILD700Test *ld700_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void ld700_write_helper(uint8_t u8Cmd)
{
	// prefix
	ld700i_write(0xA8);
	ld700i_write(0xA8 ^ 0xFF);

	ld700i_write(u8Cmd);
	ld700i_write(u8Cmd ^ 0xFF);
}

void test_ld700_cmd_pattern1()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(3);

	ld700i_reset();

	// prefix
	ld700i_write(0xA8 + 1);	// error

	// implicitly we are testing that the next byte should be an 0xA8 when an error is received

	ld700i_write(0xA8);
	ld700i_write(0xA8 ^ 0xFF + 1);	// error

	ld700i_write(0xA8);
	ld700i_write(0xA8 ^ 0xFF);

	ld700i_write(0);	// arbitrary
	ld700i_write(0);	// error
}

TEST_CASE(ld700_cmd_pattern1)
{
	test_ld700_cmd_pattern1();
}

void ld700_cmd_wait_to_repeat()
{
	// so that command can be repeated without being dropped
	ld700i_on_vblank();
	ld700i_on_vblank();
	ld700i_on_vblank();
}

void test_ld700_reject_from_playing()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	{
		InSequence dummy;

		EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
		EXPECT_CALL(mockLD700, GetStatus()).WillOnce(Return(LD700_PLAYING));
		EXPECT_CALL(mockLD700, Stop());
	}

	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();

	ld700_write_helper(0x16);	// reject (to stop playing)
}

TEST_CASE(ld700_reject_from_playing)
{
	test_ld700_reject_from_playing();
}

void test_ld700_reject_from_stopped()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	{
		InSequence dummy;

		EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
		EXPECT_CALL(mockLD700, GetStatus()).WillOnce(Return(LD700_STOPPED));
		EXPECT_CALL(mockLD700, Eject());
	}

	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();

	ld700_write_helper(0x16);	// reject (to stop playing)
}

TEST_CASE(ld700_reject_from_stopped)
{
	test_ld700_reject_from_stopped();
}

void test_ld700_playing()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
	EXPECT_CALL(mockLD700, Play());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PLAYING));

	ld700i_reset();

	ld700_write_helper(0x17);

	// Observed on logic analyzer capture from real hardware:
	// About 3ms after the play command is finished transmitting, EXT_ACK' enables and stays enabled for about 49ms (about 3 vsync pulses)
	// The delay between the command ending and EXT_ACK' activating seems variable (ie it's not always 3ms)

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_TRUE)).Times(1);
	ld700i_on_vblank();

	ld700i_on_vblank();
	ld700i_on_vblank();

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);
	ld700i_on_vblank();
}

TEST_CASE(ld700_playing)
{
	test_ld700_playing();
}

void test_ld700_pause()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);
	EXPECT_CALL(mockLD700, Pause());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();

	ld700_write_helper(0x18);
}

TEST_CASE(ld700_pause)
{
	test_ld700_pause();
}

void test_ld700_search()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	{
		InSequence dummy;

		EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
		EXPECT_CALL(mockLD700, BeginSearch(12345));
	}

	ld700i_reset();

	// frame/time
	ld700_write_helper(0x41);

	// frame 12345
	ld700_write_helper(1);
	ld700_write_helper(2);
	ld700_write_helper(3);
	ld700_write_helper(4);
	ld700_write_helper(5);

	// begin search
	ld700_write_helper(0x42);
}

TEST_CASE(ld700_search)
{
	test_ld700_search();
}

void test_ld700_too_many_digits()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
	EXPECT_CALL(mockLD700, OnError(LD700_ERR_TOO_MANY_DIGITS, _)).Times(1);

	ld700i_reset();

	ld700_write_helper(1);
	ld700_write_helper(2);
	ld700_write_helper(3);
	ld700_write_helper(4);
	ld700_write_helper(5);
	ld700_write_helper(6);
}

TEST_CASE(ld700_too_many_digits)
{
	test_ld700_too_many_digits();
}

//////////////////////////////////////

void test_ld700_repeated_command_ignored()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
	EXPECT_CALL(mockLD700, Play()).Times(1);
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();

	ld700_write_helper(0x17);

	// the rest of these should be ignored because they repeat without an acceptable interval in between
	ld700_write_helper(0x17);
	ld700_write_helper(0x17);
	ld700_write_helper(0x17);
}

TEST_CASE(ld700_repeated_command_ignored)
{
	test_ld700_repeated_command_ignored();
}

void test_ld700_repeated_command_accepted()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
	EXPECT_CALL(mockLD700, Play()).Times(2);
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();

	ld700_write_helper(0x17);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_TRUE)).Times(1);
	ld700i_on_vblank();

	ld700i_on_vblank();
	ld700i_on_vblank();

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);
	ld700i_on_vblank();

	// this one should get accepted since we've waited long enough
	ld700_write_helper(0x17);
}

TEST_CASE(ld700_repeated_command_accepted)
{
	test_ld700_repeated_command_accepted();
}
