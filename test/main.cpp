//L=============================================================================
//L This software is distributed under the MIT license.
//L Copyright 2021 P�ter Kardos
//L=============================================================================

#pragma warning(disable: 4244)

#include <iostream>

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

using namespace std;

int main(int argc, char* argv[]) {
	int ret = Catch::Session().run(argc, argv);
	return ret;
}