#pragma once
#include <Windows.h>
#ifndef XRAY_H
#define XRAY_H

// clang-format off
#include "checks.h"
#endif

namespace ss
{

	void run_scan(HANDLE process, GenericCheck *generic_check);

}