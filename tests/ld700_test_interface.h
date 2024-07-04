#ifndef LDP_IN_LD700_TEST_INTERFACE_H
#define LDP_IN_LD700_TEST_INTERFACE_H

#include <ldp-in/ld700-interpreter.h>

class ILD700Test
{
public:
	virtual void Play() = 0;
	virtual void Pause() = 0;
	virtual void BeginSearch(uint32_t) = 0;

	// g_ld700i_on_ext_ack_changed
	virtual void OnExtAckChanged(LD700_BOOL bActive) = 0;

	virtual void OnError(LD700ErrCode_t, uint8_t) = 0;
};

#endif //LDP_IN_LD700_TEST_INTERFACE_H
