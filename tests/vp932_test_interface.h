#ifndef VP932_TEST_INTERFACE_H
#define VP932_TEST_INTERFACE_H

class IVP932Test
{
public:
	virtual void Play(uint8_t u8Numerator, uint8_t u8Denominator, bool bBackward, bool bAudioSquelched) = 0;
	virtual void Step(bool bBackward) = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t u32FrameNumber) = 0;
	virtual void ChangeAudio(uint8_t u8Channel, uint8_t u8Enabled) = 0;
	virtual uint32_t GetCurFrameNum() = 0;
	virtual void OnError(VP932ErrCode_t code, uint8_t u8Val) = 0;
};

#endif // VP932_TEST_INTERFACE_H
