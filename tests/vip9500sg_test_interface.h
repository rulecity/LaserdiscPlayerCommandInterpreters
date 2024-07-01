#ifndef VIP9500SG_TEST_INTERFACE_H
#define VIP9500SG_TEST_INTERFACE_H

#include <ldp-in/vip9500sg-interpreter.h>

class IVIP9500SGTest
{
public:
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void Stop() = 0;
	virtual void StepReverse() = 0;
	virtual void BeginSearch(uint32_t) = 0;
	virtual void Skip(int32_t) = 0;
	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	virtual VIP9500SGStatus_t GetStatus() = 0;
	virtual uint32_t GetCurFrame() = 0;
	virtual uint32_t GetVBILine18() = 0;

	virtual void OnError(VIP9500SGErrCode_t code, uint8_t u8Val) = 0;
};

#endif // VIP9500SG_TEST_INTERFACE_H
