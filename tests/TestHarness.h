/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: 2026 SEN Labs e.U.
 */
#pragma once

/**
 * A minimal test harness without dependencies, so that the same tests build on Linux, macOS and Haiku.
 * `TEST(name) { CHECK(...); }` registers a test, `TEST_MAIN()` runs all of them and returns the number of failures.
 */

#include <stdio.h>

#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace testing {

struct Case {
	const char* name;
	std::function<void()> body;
};

inline std::vector<Case>& Cases() { static std::vector<Case> cases; return cases; }
inline int& Failures() { static int failures = 0; return failures; }

struct Registrar {
	Registrar(const char* name, std::function<void()> body) { Cases().push_back({name, body}); }
};

template<typename A, typename B>
void CheckEqual(const A& a, const B& b, const char* expressionA, const char* expressionB, const char* file, int line)
{
	if (!(a == b)) {
		std::ostringstream out;
		out << a << " != " << b;
		printf("  FAIL %s:%d: %s == %s (%s)\n", file, line, expressionA, expressionB, out.str().c_str());
		Failures()++;
	}
}

}	// namespace testing

#define TEST(name) \
	static void test_##name(); \
	static testing::Registrar registrar_##name(#name, test_##name); \
	static void test_##name()

#define CHECK(condition) \
	do { if (!(condition)) { printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); testing::Failures()++; } } while (0)

#define CHECK_EQ(a, b) testing::CheckEqual((a), (b), #a, #b, __FILE__, __LINE__)
#define CHECK_STR(a, b) testing::CheckEqual(std::string(a), std::string(b), #a, #b, __FILE__, __LINE__)

#define TEST_MAIN() \
	int main() { \
		for (auto& testCase : testing::Cases()) { \
			int before = testing::Failures(); \
			testCase.body(); \
			printf("%s %s\n", testing::Failures() == before ? "ok  " : "FAIL", testCase.name); \
		} \
		printf("%d test(s), %d failure(s)\n", (int)testing::Cases().size(), testing::Failures()); \
		return testing::Failures() == 0 ? 0 : 1; \
	}
