/**
 * Copyright (C) 2016 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef DEVICE_TEST_H
#define DEVICE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class DeviceTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DeviceTest);
    CPPUNIT_TEST(testDevices);
    CPPUNIT_TEST_SUITE_END();
public:
    void setUp();
    void tearDown();

    void testDevices();
};

#endif // DEVICE_TEST_H
