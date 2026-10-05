/*
VYRA Bible - OBS Studio plugin
Copyright (C) 2026 VYRA Concept
SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

// Minimal test support, no framework: a failed CHECK prints its line and makes the program exit
// with a non-zero code (so CTest and CI see it).

#include <cstdio>

namespace vyra::testing {

inline int &failures()
{
	static int n = 0;
	return n;
}
inline int &checks()
{
	static int n = 0;
	return n;
}

inline int finish()
{
	std::printf("%d checks, %d failure(s)\n", checks(), failures());
	return failures() == 0 ? 0 : 1;
}

} // namespace vyra::testing

#define CHECK(cond)                                                                                  \
	do {                                                                                         \
		++vyra::testing::checks();                                                           \
		if (!(cond)) {                                                                       \
			++vyra::testing::failures();                                                 \
			std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);                  \
		}                                                                                    \
	} while (0)
