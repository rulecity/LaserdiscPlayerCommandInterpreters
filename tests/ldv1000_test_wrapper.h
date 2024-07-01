#ifndef LDV1000_TEST_WRAPPER_H
#define LDV1000_TEST_WRAPPER_H

class ldv1000_test_wrapper
{
public:
	static LDV1000Status_t get_status() { return m_pInstance->GetStatus(); }
	static uint32_t get_cur_frame_num() { return m_pInstance->GetCurFrameNum(); }
	static void play() { m_pInstance->Play(); }
	static void pause() { m_pInstance->Pause(); }
	static void begin_search(uint32_t frame) { m_pInstance->BeginSearch(frame); }
	static void step_reverse() { m_pInstance->StepReverse(); }
	static void change_speed(uint8_t uNumerator, uint8_t uDenominator) { m_pInstance->ChangeSpeed(uNumerator, uDenominator); }
	static void skip_forward(uint8_t uTracks) { m_pInstance->SkipForward(uTracks); }
	static void skip_backward(uint8_t uTracks) { m_pInstance->SkipBackward(uTracks); }
	static void change_audio(uint8_t uChannel, uint8_t uEnable) { m_pInstance->ChangeAudio(uChannel, uEnable); }
	static void on_error(const char *cpszMsg) { m_pInstance->OnError(cpszMsg); }
	static const uint8_t *query_available_discs() { return m_pInstance->QueryAvailableDiscs(); }
	static uint8_t query_active_disc() { return m_pInstance->QueryActiveDisc(); }
	static void begin_changing_to_disc(uint8_t id) { m_pInstance->BeginChangingToDisc(id); }
	static void change_seek_delay(LDV1000_BOOL bEnabled) { m_pInstance->ChangeSeekDelay(bEnabled == LDV1000_TRUE); }
	static void change_spinup_delay(LDV1000_BOOL bEnabled) { m_pInstance->ChangeSpinUpDelay(bEnabled == LDV1000_TRUE); }
	static void change_super_mode(LDV1000_BOOL bEnabled) { m_pInstance->ChangeSuperMode(bEnabled == LDV1000_TRUE); }

	static void setup(ILDV1000Test *pInstance)
	{
		m_pInstance = pInstance;
		g_ldv1000i_get_status = get_status;
		g_ldv1000i_get_cur_frame_num = get_cur_frame_num;
		g_ldv1000i_play = play;
		g_ldv1000i_pause = pause;
		g_ldv1000i_begin_search = begin_search;
		g_ldv1000i_step_reverse = step_reverse;
		g_ldv1000i_change_speed = change_speed;
		g_ldv1000i_skip_forward = skip_forward;
		g_ldv1000i_skip_backward = skip_backward;
		g_ldv1000i_change_audio = change_audio;
		g_ldv1000i_on_error = on_error;
		g_ldv1000i_query_available_discs = query_available_discs;
		g_ldv1000i_query_active_disc = query_active_disc;
		g_ldv1000i_begin_changing_to_disc = begin_changing_to_disc;
		g_ldv1000i_change_seek_delay = change_seek_delay;
		g_ldv1000i_change_spinup_delay = change_spinup_delay;
		g_ldv1000i_change_super_mode = change_super_mode;
	}

private:
	static ILDV1000Test *m_pInstance;
};

#endif // LDV1000_TEST_WRAPPER_H
