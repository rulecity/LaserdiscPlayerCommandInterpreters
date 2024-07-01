#ifndef LDP_IN_MOCKS_H
#define LDP_IN_MOCKS_H

#include <gmock/gmock.h>
#include <ldp-in/vp931-interpreter.h>
#include <ldp-in/vp932-interpreter.h>
#include <ldp-in/pr8210-interpreter.h>
#include <ldp-in/pr7820-interpreter.h>
#include <ldp-in/vip9500sg-interpreter.h>
#include <ldp-in/ldv1000-interpreter.h>
#include <ldp-in/ldp1000-interpreter.h>

#include "vp931_test_interface.h"
#include "vp932_test_interface.h"
#include "ldp1000_test_interface.h"
#include "ldv1000_test_interface.h"
#include "pr7820_test_interface.h"
#include "vip9500sg_test_interface.h"
#include "pr8210_test_interface.h"

class MockVP931Test : public IVP931Test
{
public:
//	virtual void Play() = 0;
	MOCK_METHOD0(Play, void());

//	virtual void Pause() = 0;
	MOCK_METHOD0(Pause, void());

	MOCK_METHOD2(BeginSearch, void(uint32_t, bool));

//	virtual void Skip(int iTracks) = 0;
	MOCK_METHOD1(Skip, void(int));

	MOCK_METHOD1(SkipToFrameNum, void(uint32_t));

//	virtual void OnError(VP931ErrCode_t code, uint8_t u8Val) = 0;
	MOCK_METHOD2(OnError, void(VP931ErrCode_t, uint8_t));
};

class MockVP932Test : public IVP932Test
{
public:
	MOCK_METHOD4(Play, void(uint8_t u8Numerator, uint8_t u8Denominator, bool bBackward, bool bAudioSquelched));

	//virtual void Step(bool bBackward) = 0;
	MOCK_METHOD1(Step, void(bool));

//	virtual void Pause() = 0;
	MOCK_METHOD0(Pause, void());

//	virtual void BeginSearch(uint32_t u32FrameNumber) = 0;
	MOCK_METHOD1(BeginSearch, void(uint32_t));

	//virtual void ChangeAudio(uint8_t u8Channel, uint8_t u8Enabled) = 0;
	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));

	// virtual uint32_t GetCurFrameNum() = 0;
	MOCK_METHOD0(GetCurFrameNum, uint32_t());

	//virtual void OnError(VP932ErrCode_t code, uint8_t u8Val) = 0;
	MOCK_METHOD2(OnError, void(VP932ErrCode_t, uint8_t));
};

class MockLDP1000Test : public ILDP1000Test
{
public:
	MOCK_METHOD4(Play, void(uint8_t u8Numerator, uint8_t u8Denominator, bool bBackward, bool bAudioSquelched));

	MOCK_METHOD0(Pause, void());

	MOCK_METHOD1(BeginSearch, void(uint32_t));

	MOCK_METHOD0(StepForward, void());

	MOCK_METHOD0(StepReverse, void());

	MOCK_METHOD1(Skip, void(int16_t));

	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));

	MOCK_METHOD1(ChangeVideo, void(bool));

	MOCK_METHOD0(GetStatus, LDP1000Status_t());

	MOCK_METHOD0(GetCurFrame, uint32_t());

	MOCK_METHOD1(TextEnableChanged, void(bool));
	MOCK_METHOD1(TextBufferContentsChanged, void(const uint8_t *));
	MOCK_METHOD1(TextBufferStartIndexChanged, void(uint8_t));
	MOCK_METHOD3(TextModesChanged, void(uint8_t, uint8_t, uint8_t));

	MOCK_METHOD2(OnError, void(LDP1000ErrCode_t, uint8_t));
};

class MockLDV1000Test : public ILDV1000Test
{
public:
//	virtual LDV1000Status_t GetStatus() = 0;
	MOCK_METHOD0(GetStatus, LDV1000Status_t());

//	virtual uint32_t GetCurFrame() = 0;
	MOCK_METHOD0(GetCurFrameNum, uint32_t());

//	virtual void Play() = 0;
	MOCK_METHOD0(Play, void());

//	virtual void Pause() = 0;
	MOCK_METHOD0(Pause, void());

//	virtual void BeginSearch(uint32_t) = 0;
	MOCK_METHOD1(BeginSearch, void(uint32_t));

//	virtual void StepReverse() = 0;
	MOCK_METHOD0(StepReverse, void());

//	virtual void ChangeSpeed(uint8_t uNumerator, uint8_t uDenominator) = 0;
	MOCK_METHOD2(ChangeSpeed, void(uint8_t, uint8_t));

//	virtual void SkipForward(uint8_t uTracks) = 0;
	MOCK_METHOD1(SkipForward, void(uint8_t));

//	virtual void SkipBackward(uint8_t uTracks) = 0;
	MOCK_METHOD1(SkipBackward, void(uint8_t));

//	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));

//	virtual void OnError(const char *cpszErrMsg) = 0;
	MOCK_METHOD1(OnError, void(const char *));

	MOCK_METHOD0(QueryAvailableDiscs, const uint8_t *());

	MOCK_METHOD0(QueryActiveDisc, uint8_t ());

	MOCK_METHOD1(BeginChangingToDisc, void (uint8_t));

//	virtual void ChangeSpinUpDelay(bool bEnabled) = 0;
	MOCK_METHOD1(ChangeSpinUpDelay, void(bool));

//	virtual void ChangeSeekDelay(bool bEnabled) = 0;
	MOCK_METHOD1(ChangeSeekDelay, void(bool));

	MOCK_METHOD1(ChangeSuperMode, void(bool));
};

class MockPR7820Test : public IPR7820Test
{
public:
	MOCK_METHOD0(GetStatus, PR7820Status_t());
	MOCK_METHOD0(Play, void());
	MOCK_METHOD0(Pause, void());
	MOCK_METHOD1(BeginSearch, void(uint32_t));
	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));
	MOCK_METHOD0(EnableSuperMode, void());
	MOCK_METHOD2(OnError, void(PR7820ErrCode_t, uint8_t));
};

class MockPR8210Test : public IPR8210Test
{
public:
	MOCK_METHOD0(Play, void());
	MOCK_METHOD0(Pause, void());
	MOCK_METHOD1(Step, void(int8_t));
	MOCK_METHOD1(BeginSearch, void(uint32_t));
	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));
	MOCK_METHOD1(Skip, void(int8_t));
	MOCK_METHOD1(ChangeAutoTrackJump, void(bool));
	MOCK_METHOD0(IsPlayerBusy, bool());
	MOCK_METHOD1(ChangeStandby, void(bool));
	MOCK_METHOD1(ChangeVideoSquelch, void(bool));
	MOCK_METHOD2(OnError, void(PR8210ErrCode_t, uint16_t));
};

class MockVIP9500SGTest : public IVIP9500SGTest
{
public:
	MOCK_METHOD0(Play, void());

	MOCK_METHOD0(Pause, void());

	MOCK_METHOD0(Stop, void());

	MOCK_METHOD0(StepReverse, void());

	MOCK_METHOD1(BeginSearch, void(uint32_t));

	MOCK_METHOD1(Skip, void(int32_t));

	MOCK_METHOD2(ChangeAudio, void(uint8_t, uint8_t));

	MOCK_METHOD0(GetStatus, VIP9500SGStatus_t());

	MOCK_METHOD0(GetCurFrame, uint32_t());

	MOCK_METHOD0(GetVBILine18, uint32_t());

	MOCK_METHOD2(OnError, void(VIP9500SGErrCode_t, uint8_t));
};

#endif //LDP_IN_MOCKS_H
