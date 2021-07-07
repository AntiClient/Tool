#ifndef CHECKS_H
#define CHECKS_H

// clang-format off
#include <string>
#include <vector>
#include <json.hpp>
// clang-format on

enum CheckStatus { kFailed = 0, kSuccess = 1, kWarning = 2 };

struct GenericCheck {
  CheckStatus status;
  std::string message;
  std::string client;

  void Reset() {
    status = kSuccess;
    message.clear();
  }
};

struct StartTime {
  std::string process;
  std::string start_time;
};

struct Directory {
  std::string dir;
  std::string write_date;
};

void RunWindowChecks(const nlohmann::json &data, GenericCheck *generic_check);
void RunProcessesCheck(nlohmann::json &data, GenericCheck *generic_check);

void RunRecordingSoftwareCheck(nlohmann::json &data,
                               GenericCheck *generic_check);

std::vector<Directory> GetRecycleBinWriteDates();
std::vector<std::string> executed_files();
std::vector<StartTime> GetProcessStartTimes(nlohmann::json &data);

std::vector<std::string> GetPlayerAccounts(
    const nlohmann::json &launcher_profile);

std::vector<std::string> GetEventsChangeTime(
	const nlohmann::json &launcher_profile);

void runPatternScan(GenericCheck *generic_check);

#endif
