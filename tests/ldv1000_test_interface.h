#ifndef LDV1000_TEST_INTERFACE_H
#define LDV1000_TEST_INTERFACE_H

class ILDV1000Test
{
public:
	virtual LDV1000Status_t GetStatus() = 0;
	virtual uint32_t GetCurFrameNum() = 0;
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t) = 0;
	virtual void StepReverse() = 0;
	virtual void ChangeSpeed(uint8_t uNumerator, uint8_t uDenominator) = 0;
	virtual void SkipForward(uint8_t uTracks) = 0;
	virtual void SkipBackward(uint8_t uTracks) = 0;
	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	virtual void OnError(const char *cpszErrMsg) = 0;

	// extended commands
	virtual const uint8_t *QueryAvailableDiscs() = 0;
	virtual uint8_t QueryActiveDisc() = 0;
	virtual void BeginChangingToDisc(uint8_t id) = 0;
	virtual void ChangeSpinUpDelay(bool bEnabled) = 0;
	virtual void ChangeSeekDelay(bool bEnabled) = 0;
	virtual void ChangeSuperMode(bool bEnabled) = 0;
};

#endif // LDV1000_TEST_INTERFACE_H
