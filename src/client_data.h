#ifndef CLIENT_DATA_H
#define CLIENT_DATA_H
// clang-format off
#include <json.hpp>
#include <optional>
// clang-format on

std::optional<nlohmann::json> GetClientData(
	const std::string &pin);
#endif