#ifndef COMS_H
#define COMS_H
#include <string>

namespace ClientComms {
void DestroySession(const std::string& sid);
void UploadCheckResults(const std::string& json, const std::string& sid);
bool IsSessionValid(const std::string& sid);

void SendMachineName(const std::string& username, const std::string& machine_name);
}  // namespace ClientComms

#endif