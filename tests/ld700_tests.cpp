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
	static void change_audio(uint8_t u8Channel, LD700_BOOL bActive) { m_pInstance->ChangeAudio(u8Channel, bActive); }

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
		g_ld700i_change_audio = change_audio;
	}

private:
	static ILD700Test *m_pInstance;
};

ILD700Test *ld700_test_wrapper::m_pInstance = 0;

class LD700Tests : public ::testing::Test
{
public:
	void SetUp() override
	{
		ld700_test_wrapper::setup(&mockLD700);

		EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);

		ld700i_reset();
	}
protected:
	MockLD700Test mockLD700;
};

/////////////////////////////////////////////////////////////////

void ld700_write_helper(uint8_t u8Cmd)
{
	// prefix
	ld700i_write(0xA8);
	ld700i_write(0xA8 ^ 0xFF);

	ld700i_write(u8Cmd);
	ld700i_write(u8Cmd ^ 0xFF);
}

// when the number of vblanks doesn't matter that much and 1 is as good as any
void ld700_write_helper_with_vblank(MockLD700Test &mockLD700, LD700_BOOL bExtAckToActivate, uint8_t u8Cmd)
{
	ld700_write_helper(u8Cmd);

	// on the next vblank, ExtAck should change
	EXPECT_CALL(mockLD700, OnExtAckChanged(bExtAckToActivate)).Times(1);
	ld700i_on_vblank();
}

// simulate vblanks firing in the middle of sending data
void ld700_write_helper_with_vblanks(MockLD700Test &mockLD700, LD700_BOOL bExtAckToActivate, size_t uVblankCountBeforeChange, uint8_t u8Cmd)
{
	// the way our interpreter is currently written, there's no change in behavior whether the vblanks come between each write or all at once,
	//  so for clarity, I'm sending them all at once
	for (int i = 0; i < uVblankCountBeforeChange - 1; i++)
	{
		ld700i_on_vblank();
	}
	ld700_write_helper_with_vblank(mockLD700, bExtAckToActivate, u8Cmd);
}

// wait N number of vblanks, where EXT_ACK' changes on the last vblank
void wait_vblanks_for_ext_ack_change(MockLD700Test &mockLD700, LD700_BOOL bExtAckToActivate, size_t uVblankCount)
{
	for (int i = 0; i < uVblankCount - 1; i++)
	{
		ld700i_on_vblank();
	}
	EXPECT_CALL(mockLD700, OnExtAckChanged(bExtAckToActivate)).Times(1);
	ld700i_on_vblank();
}

////////////////////////////////////////////////////

TEST_F(LD700Tests, cmd_pattern1)
{
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(3);

	// prefix
	ld700i_write(0xA8 + 1);	// error

	// implicitly we are testing that the next byte should be an 0xA8 when an error is received

	ld700i_write(0xA8);
	ld700i_write(0xA9 ^ 0xFF);	// error

	ld700i_write(0xA8);
	ld700i_write(0xA8 ^ 0xFF);

	ld700i_write(0);	// arbitrary
	ld700i_write(0);	// error
}

TEST_F(LD700Tests, reject_from_playing)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillOnce(Return(LD700_PLAYING));
	EXPECT_CALL(mockLD700, Stop());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700_write_helper(0x16);	// reject (to stop playing)
}

TEST_F(LD700Tests, reject_from_stopped)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillOnce(Return(LD700_STOPPED));
	EXPECT_CALL(mockLD700, Eject());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700_write_helper(0x16);	// reject (to stop playing)
}

TEST_F(LD700Tests, playing)
{
	EXPECT_CALL(mockLD700, Play());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));

	// Observed on logic analyzer capture from real hardware:
	// About 3ms after the play command is finished transmitting, EXT_ACK' enables and stays enabled for about 49ms (about 3 vsync pulses)
	// The delay between the command ending and EXT_ACK' activating seems variable (ie it's not always 3ms)
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x17);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);
}

TEST_F(LD700Tests, pause)
{
	EXPECT_CALL(mockLD700, Pause());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PLAYING));

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x18);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);
}

TEST_F(LD700Tests, search)
{
	// This test ignores vblank and is more basic.
	// We have other tests that tests searching using vblank.

	EXPECT_CALL(mockLD700, BeginSearch(12345));
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

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

TEST_F(LD700Tests, search_after_disc_flip)
{
	EXPECT_CALL(mockLD700, ChangeAudio(0, LD700_TRUE));
	EXPECT_CALL(mockLD700, ChangeAudio(1, LD700_TRUE));
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));

	// NOTE: For this logic analyzer capture, commands are spaced out enough that EXT_ACK' deactivates before next command

	// a lot of Halcyon search commands start with this for some reason
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x4A);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);		// logic analyzer capture shows EXT_ACK' lasting 3 vblanks if disc is paused and 4A command is sent

	// frame/time
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x41);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 1);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 7);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 2);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 1);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 4);

	EXPECT_CALL(mockLD700, BeginSearch(1721));
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x42);
}

TEST_F(LD700Tests, too_many_digits)
{
	EXPECT_CALL(mockLD700, OnError(LD700_ERR_TOO_MANY_DIGITS, _)).Times(1);

	ld700_write_helper(1);
	ld700_write_helper(2);
	ld700_write_helper(3);
	ld700_write_helper(4);
	ld700_write_helper(5);
	ld700_write_helper(6);
}

TEST_F(LD700Tests, repeated_command_ignored)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));
	EXPECT_CALL(mockLD700, Play()).Times(1);
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700_write_helper(0x17);

	// the rest of these should be ignored because they repeat without an acceptable interval in between
	ld700_write_helper(0x17);
	ld700_write_helper(0x17);
	ld700_write_helper(0x17);
}

TEST_F(LD700Tests, repeated_command_accepted)
{
	EXPECT_CALL(mockLD700, Play()).Times(2);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));	// we know how long EXT_ACK' lasts when play command is sent when disc is paused
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x17);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);

	// this one should get accepted since we've waited long enough
	ld700_write_helper(0x17);
}

TEST_F(LD700Tests, tray_ejected)
{
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, ChangeAudio(0, LD700_TRUE)).Times(1);
	EXPECT_CALL(mockLD700, ChangeAudio(1, LD700_TRUE)).Times(1);

	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_TRAY_EJECTED));
	ld700_write_helper(0x4A);

	// EXT_ACK' should not enable because the tray is ejected
	ld700i_on_vblank();
}

TEST_F(LD700Tests, enable_left)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));
	EXPECT_CALL(mockLD700, ChangeAudio(0, LD700_TRUE)).Times(1);
	EXPECT_CALL(mockLD700, ChangeAudio(1, LD700_FALSE)).Times(1);
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x4B);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);
}

TEST_F(LD700Tests, enable_right)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));
	EXPECT_CALL(mockLD700, ChangeAudio(1, LD700_TRUE)).Times(1);
	EXPECT_CALL(mockLD700, ChangeAudio(0, LD700_FALSE)).Times(1);
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x49);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);
}

TEST_F(LD700Tests, boot1)
{
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, ChangeAudio(0, LD700_TRUE)).Times(1);
	EXPECT_CALL(mockLD700, ChangeAudio(1, LD700_TRUE)).Times(1);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_STOPPED));

	// EXT_ACK' should activate because tray is not ejected
	ld700_write_helper_with_vblank(mockLD700, LD700_TRUE, 0x4A);

	ld700_write_helper_with_vblanks(mockLD700, LD700_FALSE, 5, 0x5F);
	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 5, 0x02);
	ld700_write_helper_with_vblanks(mockLD700, LD700_FALSE, 5, 0x5F);
	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 5, 0x04);

	EXPECT_CALL(mockLD700, Play());
	ld700_write_helper_with_vblanks(mockLD700, LD700_FALSE, 5, 0x17);
}

TEST_F(LD700Tests, boot2)
{
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PLAYING)); // NOTE: For this test, the disc starts out playing (this is from a logic analyzer capture)

	// EXT_ACK' is already inactive and sending this command won't change that
	ld700_write_helper(0x5F);

	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 2, 0x06);	// seen on real hardware

	EXPECT_CALL(mockLD700, Pause());
	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE)).Times(1);
	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 3, 0x18);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 1);

	// on logic analyzer capture, there was a gap here that I am filling with vblanks to match what happened in the capture
	ld700i_on_vblank();
	ld700i_on_vblank();

	// EXT_ACK' is already inactive and sending this command won't change that
	ld700_write_helper(0x5F);

	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 3, 0x03);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);

	// EXT_ACK' is already inactive and sending this command won't change that
	ld700_write_helper(0x5F);

	ld700_write_helper_with_vblanks(mockLD700, LD700_TRUE, 3, 0x05);
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_FALSE, 3);
}

TEST_F(LD700Tests, disc_searching)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_SEARCHING));

	// EXT_ACK' should enable
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_TRUE, 1);

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockLD700));
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));
}

TEST_F(LD700Tests, disc_spinning_up)
{
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_SPINNING_UP));

	// EXT_ACK' should enable
	wait_vblanks_for_ext_ack_change(mockLD700, LD700_TRUE, 1);

	// make it easy to troubleshoot problems
	ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockLD700));
	EXPECT_CALL(mockLD700, GetStatus()).WillRepeatedly(Return(LD700_PAUSED));
}
