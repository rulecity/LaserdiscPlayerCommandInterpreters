#include "stdafx.h"
#include "ld700_test_interface.h"

class ld700_test_wrapper
{
public:
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void begin_search(uint32_t u32FrameNum) { m_pInstance->BeginSearch(u32FrameNum); }
	static void on_ext_ack_changed(LD700_BOOL bActive) { m_pInstance->OnExtAckChanged(bActive); }
	static void on_error(LD700ErrCode_t err, uint8_t val) { m_pInstance->OnError(err, val); }

	static void setup(ILD700Test *pInstance)
	{
		m_pInstance = pInstance;
		g_ld700i_play = play;
		g_ld700i_pause = pause;
		g_ld700i_begin_search = begin_search;
		g_ld700i_on_ext_ack_changed = on_ext_ack_changed;
		g_ld700i_error = on_error;
	}

private:
	static ILD700Test *m_pInstance;
};

ILD700Test *ld700_test_wrapper::m_pInstance = 0;

/////////////////////////////////////////////////////////////////

void test_ld700_playing()
{
	MockLD700Test mockLD700;

	ld700_test_wrapper::setup(&mockLD700);

	EXPECT_CALL(mockLD700, OnExtAckChanged(LD700_FALSE));
	EXPECT_CALL(mockLD700, Play());
	EXPECT_CALL(mockLD700, OnError(_, _)).Times(0);

	ld700i_reset();
	ld700i_write(0xA8);	// send prefix
	ld700i_write(0x57);	// send prefix
	ld700i_write(0x17);	// send play

}

TEST_CASE(ld700_playing)
{
	test_ld700_playing();
}
