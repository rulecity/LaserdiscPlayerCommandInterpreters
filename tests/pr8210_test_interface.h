#ifndef PR8210_TEST_INTERFACE_H
#define PR8210_TEST_INTERFACE_H

class IPR8210Test
{
public:
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Step(int8_t) = 0;
	virtual void BeginSearch(uint32_t) = 0;
	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	virtual void Skip(int8_t) = 0;
	virtual void ChangeAutoTrackJump(bool bAutoTrackJumpEnabled) = 0;
	virtual bool IsPlayerBusy() = 0;
	virtual void ChangeStandby(bool bEnabled) = 0;
	virtual void ChangeVideoSquelch(bool bRaised) = 0;
	virtual void OnError(PR8210ErrCode_t code, uint16_t u16Val) = 0;
};

#endif // test interface