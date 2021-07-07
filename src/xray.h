#ifndef XRAY_H
#define XRAY_H

// clang-format off
#include "checks.h"
// clang-format on

std::vector<std::string> RunXrayPackCheck(const wchar_t *resource_pack_directory/*,
	GenericCheck *generic_check*/);

#endif