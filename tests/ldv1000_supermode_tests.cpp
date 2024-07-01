#include "stdafx.h"
#include "ldv1000_test_wrapper.h"

void test_ldv1000super_spinup()
{
	MockLDV1000Test mockLDV1000;
	ldv1000_test_wrapper::setup(&mockLDV1000);

	{
		InSequence s;
		EXPECT_CALL(mockLDV1000, GetStatus()).WillOnce(Return(LDV1000_STOPPED));
		EXPECT_CALL(mockLDV1000, Play());
		EXPECT_CALL(mockLDV1000, GetStatus()).Times(2).WillRepeatedly(Return(LDV1000_SPINNING_UP));
		EXPECT_CALL(mockLDV1000, GetStatus()).WillOnce(Return(LDV1000_PLAYING));
	}

	reset_ldv1000i(LDV1000_EMU_SUPER);

	unsigned char ch;

	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xFC, ch);

	write_ldv1000i(0xFD);	// send play command

	// should be busy spinning up
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0x64, ch);

	ch = read_ldv1000i();
	TEST_REQUIRE_EQUAL(0x64, ch);

	// now the disc status should be playing based on our mock setup

	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xE4, ch);	// disc is full on in playing mode
}

TEST_CASE(ldv1000super_spinup)
{
	test_ldv1000super_spinup();
}

////////////////////////////

void test_ldv1000super_good_seek()
{
	MockLDV1000Test mockLDV1000;
	ldv1000_test_wrapper::setup(&mockLDV1000);

	{
		InSequence s;
		EXPECT_CALL(mockLDV1000, Play());
		EXPECT_CALL(mockLDV1000, BeginSearch(0));
		EXPECT_CALL(mockLDV1000, GetStatus()).Times(10).WillRepeatedly(Return(LDV1000_SEARCHING));
		EXPECT_CALL(mockLDV1000, GetStatus()).WillRepeatedly(Return(LDV1000_PAUSED));
	}

	reset_ldv1000i(LDV1000_EMU_SUPER);

	unsigned char ch;

	write_ldv1000i(0xFD);	// send play command

	// send a seek command (doesn't matter what it is because it should always succeed
	// intentionally do not send NO ENTRY

	write_ldv1000i(0xF7);	// seek to whatever frame

	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0x50, ch);	// seek busy

	unsigned int uBusyIterations = 0;

	while (ch == 0x50)
	{
		write_ldv1000i(0xFF);	// no entry (should not change status from 0x50)
		ch = read_ldv1000i();
		uBusyIterations++;
	};

	TEST_REQUIRE_EQUAL(10, uBusyIterations);

	TEST_CHECK_EQUAL(0xD0, ch);	// seek succeeded

	// successive reads should continue to return the same result
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xD0, ch);
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xD0, ch);

	// if we send a digit, then the status remain ready
	write_ldv1000i(0x3F);	// digit 0
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xD0, ch);
}

TEST_CASE(ldv1000super_good_seek)
{
	test_ldv1000super_good_seek();
}

//////////////////////////////////////////////////

void test_ldv1000super_delayed_good_seek()
{
	MockLDV1000Test mockLDV1000;
	ldv1000_test_wrapper::setup(&mockLDV1000);

	{
		InSequence s;
		EXPECT_CALL(mockLDV1000, Play());
		EXPECT_CALL(mockLDV1000, BeginSearch(0));
		EXPECT_CALL(mockLDV1000, GetStatus()).Times(11).WillRepeatedly(Return(LDV1000_SEARCHING));
		EXPECT_CALL(mockLDV1000, GetStatus()).WillRepeatedly(Return(LDV1000_PAUSED));
	}

	reset_ldv1000i(LDV1000_EMU_SUPER);

	unsigned char ch;

	write_ldv1000i(0xFD);	// send play command

	// send a seek command (doesn't matter what it is because it should always succeed
	// intentionally do not send NO ENTRY
	write_ldv1000i(0xF7);	// seek to whatever frame

	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0x50, ch);	// seek busy

	unsigned int uBusyIterations = 0;

	while (uBusyIterations < 10)
	{
		ch = read_ldv1000i();
		TEST_REQUIRE_EQUAL(0x50, ch);	// seek busy
		uBusyIterations++;
	};

	// status should automatically become ready once the search has finished
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0xD0, ch);	// seek succeeded
}

TEST_CASE(ldv1000super_delayed_good_seek)
{
	test_ldv1000super_delayed_good_seek();
}

///////////////////////////////////////////////////

void test_ldv1000super_disc_switch_during_seek()
{
	MockLDV1000Test mockLDV1000;
	ldv1000_test_wrapper::setup(&mockLDV1000);

	{
		InSequence s;
		EXPECT_CALL(mockLDV1000, Play());
		EXPECT_CALL(mockLDV1000, BeginSearch(0));
		EXPECT_CALL(mockLDV1000, GetStatus()).Times(2).WillRepeatedly(Return(LDV1000_SEARCHING));
	}

	reset_ldv1000i(LDV1000_EMU_SUPER);

	unsigned char ch;

	write_ldv1000i(0xFD);	// send play command

	// send a seek command (doesn't matter what it is because it should always succeed
	write_ldv1000i(0xF7);	// seek to whatever frame

	// make sure we're busy seeking
	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0x50, ch);

	// send disc switch command while we're busy (should get ignored)
	write_ldv1000i(0x93);
	write_ldv1000i(1);	// arbitrary disc id

	ch = read_ldv1000i();
	TEST_CHECK_EQUAL(0x50, ch);	// disc switch command should've been ignored
}

TEST_CASE(ldv1000super_disc_switch_during_seek)
{
	test_ldv1000super_disc_switch_during_seek();
}
