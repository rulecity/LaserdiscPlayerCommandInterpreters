#ifndef LDP1000_TEST_INTERFACE_H
#define LDP1000_TEST_INTERFACE_H

class ILDP1000Test
{
public:
	virtual void Play(uint8_t u8Numerator, uint8_t u8Denominator, bool bBackward, bool bAudioSquelched) = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t) = 0;
	virtual void StepForward() = 0;
	virtual void StepReverse() = 0;
	virtual void Skip(int16_t) = 0;
	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	virtual void ChangeVideo(bool bEnabled) = 0;
	virtual LDP1000Status_t GetStatus() = 0;
	virtual uint32_t GetCurFrame() = 0;

	virtual void TextEnableChanged(bool bEnabled) = 0;
	virtual void TextBufferContentsChanged(const uint8_t *p8Buf32Bytes) = 0;
	virtual void TextBufferStartIndexChanged(uint8_t u8StartIdx) = 0;
	virtual void TextModesChanged(uint8_t u8Mode, uint8_t u8X, uint8_t u8Y) = 0;

	virtual void OnError(LDP1000ErrCode_t code, uint8_t u8Val) = 0;
};

#endif // LDP1000_TEST_INTERFACE_H
