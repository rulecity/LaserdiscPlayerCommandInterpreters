#ifndef VP931_TEST_INTERFACE_H
#define VP931_TEST_INTERFACE_H

class IVP931Test
{
public:
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t u32FrameNumber, bool bAudioSquelchedOnComplete) = 0;
	virtual void Skip(int iTracks) = 0;
	virtual void SkipToFrameNum(uint32_t u32FrameNumber) = 0;
	virtual void OnError(VP931ErrCode_t code, uint8_t u8Val) = 0;
};

#endif // VP931_TEST_INTERFACE_H
