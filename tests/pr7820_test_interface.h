#ifndef PR7820_TEST_INTERFACE_H
#define PR7820_TEST_INTERFACE_H

class IPR7820Test
{
public:
	virtual PR7820Status_t GetStatus() = 0;
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t) = 0;
	virtual void ChangeAudio(uint8_t, uint8_t) = 0;
	virtual void EnableSuperMode() = 0;
	virtual void OnError(PR7820ErrCode_t code, uint8_t u8Val) = 0;
};

#endif // test interface