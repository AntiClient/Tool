#include "coms.h"

#include <openssl/aes.h>

#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <algorithm>
#include <iomanip>
#include <string>

#include "cx_encrypted_string.h"

constexpr const auto kHostname = cx_make_encrypted_string("api.anticlient.xyz");
constexpr const auto kPort = cx_make_encrypted_string("443");

constexpr const auto kDestroy = cx_make_encrypted_string("/destroy.php");

constexpr const auto kLog = cx_make_encrypted_string("/log.php");
constexpr const auto kBlacklist =
    cx_make_encrypted_string("/blacklist.php");
constexpr const auto kCheckSession =
    cx_make_encrypted_string("/checksession.php");

using tcp = boost::asio::ip::tcp;     // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;     // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;  // from <boost/beast/http.hpp>  // from
                                      // <boost/beast/http.hpp>

std::string UrlEncode(const std::string& value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n;
       ++i) {
    std::string::value_type c = (*i);

    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }

    escaped << std::uppercase;
    escaped << '%' << std::setw(2) << int((unsigned char)c);
    escaped << std::nouppercase;
  }

  return escaped.str();
}



void ClientComms::DestroySession(const std::string& sid) {
  boost::asio::io_context ioc;

  ssl::context ctx{ssl::context::sslv23_client};

  tcp::resolver resolver{ioc};

  ssl::stream<tcp::socket> stream{ioc, ctx};

  SSL_set_tlsext_host_name(stream.native_handle(), kHostname.c_str());

  const auto results = resolver.resolve(kHostname.c_str(), kPort.c_str());

  try {
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
  } catch (std::exception& e) {
    MessageBoxA(nullptr,
                ("Connection to host failed, check if you have an internet "
                 "connection available."),
                ("Connection Failed"), MB_OK);
    ExitProcess(0);
  }
  stream.handshake(ssl::stream_base::client);

  auto request_url = (std::string(kDestroy.c_str()) +
	  (charenc("?SID=") + UrlEncode(sid)));

  http::request<http::string_body> req{http::verb::get, request_url, 11};

  req.set(http::field::host, kHostname.c_str());
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(stream, req);

  boost::system::error_code ec;
  stream.shutdown(ec);
}


void ClientComms::UploadCheckResults(const std::string& json, const std::string& sid) {
  boost::asio::io_context ioc;

  ssl::context ctx{ssl::context::sslv23_client};

  tcp::resolver resolver{ioc};

  ssl::stream<tcp::socket> stream{ioc, ctx};

  SSL_set_tlsext_host_name(stream.native_handle(), kHostname.c_str());

  const auto results = resolver.resolve(kHostname.c_str(), kPort.c_str());

  try {
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
  } catch (std::exception& e) {
    MessageBoxA(nullptr,
                ("Connection to host failed, check if you have an internet "
                 "connection available."),
                ("Connection Failed"), MB_OK);
    ExitProcess(0);
  }
  stream.handshake(ssl::stream_base::client);

  http::request<http::string_body> req{http::verb::post, (std::string(kLog.c_str()) +
							  (charenc("?SID=") + UrlEncode(sid))), 11};

  req.set(http::field::host, kHostname.c_str());
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(http::field::content_type, "application/json");
  req.set(http::field::content_length, json.length());

  req.body() = json;

  http::write(stream, req);

  boost::system::error_code ec;
  stream.shutdown(ec);
}


bool ClientComms::IsSessionValid(const std::string& sid) {
  boost::asio::io_context ioc;

  ssl::context ctx{ssl::context::sslv23_client};

  tcp::resolver resolver{ioc};

  ssl::stream<tcp::socket> stream{ioc, ctx};

  SSL_set_tlsext_host_name(stream.native_handle(), kHostname.c_str());

  const auto results = resolver.resolve(kHostname.c_str(), kPort.c_str());

  try {
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
  } catch (std::exception& e) {
    MessageBoxA(nullptr,
                ("Connection to host failed, check if you have an internet "
                 "connection available."),
                ("Connection Failed"), MB_OK);
    ExitProcess(0);
  }
  stream.handshake(ssl::stream_base::client);

  auto request_url = (std::string(kCheckSession.c_str()) +
							  (charenc("?SID=") + UrlEncode(sid)));

  http::request<http::string_body> req{http::verb::get, request_url, 11};

  req.set(http::field::host, kHostname.c_str());
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(stream, req);

  boost::beast::flat_buffer buffer;

  http::response<http::dynamic_body> res;

  http::read(stream, buffer, res);

  auto response = boost::beast::buffers_to_string(res.body().data());

  boost::system::error_code ec;
  stream.shutdown(ec);

  return response.empty();
}


void ClientComms::SendMachineName(const std::string& username,
                                  const std::string& machine_name) {
  boost::asio::io_context ioc;

  ssl::context ctx{ssl::context::sslv23_client};

  tcp::resolver resolver{ioc};

  ssl::stream<tcp::socket> stream{ioc, ctx};

  SSL_set_tlsext_host_name(stream.native_handle(), kHostname.c_str());

  const auto results = resolver.resolve(kHostname.c_str(), kPort.c_str());

  try {
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
  } catch (std::exception& e) {
    MessageBoxA(nullptr,
                ("Connection to host failed, check if you have an internet "
                 "connection available."),
                ("Connection Failed"), MB_OK);
    ExitProcess(0);
  }
  stream.handshake(ssl::stream_base::client);

  auto request_url = (std::string(kBlacklist.c_str()) +
                      (charenc("?user=") + UrlEncode(username) +
                       "&pcname=" + UrlEncode(machine_name)));

  http::request<http::string_body> req{http::verb::get, request_url, 11};

  req.set(http::field::host, kHostname.c_str());
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(stream, req);

  boost::system::error_code ec;
  stream.shutdown(ec);
}
