// clang-format off

#include <openssl/aes.h>
#include <aes.h>
#include <hex.h>
#include <filters.h>
#include <modes.h>
#include <base64.h>
#include <osrng.h> 

#include <vector>
#include <random>
#include <climits>
#include <algorithm>
#include <functional>
#include <ThemidaSDK.h>


#define BOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <fstream>

#define NOMINMAX
#include "third_party/fastpbkdf2/fastpbkdf2.h"
#include "cx_encrypted_string.h"
#include "client_data.h"

// clang-format on

using tcp = boost::asio::ip::tcp;     // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;     // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;  // from <boost/beast/http.hpp>  // from
                                      // <boost/beast/http.hpp>
using namespace CryptoPP;
// 37.187.185.219
constexpr const auto kHostname = cx_make_encrypted_string("<Server IP Address>");
constexpr const auto kPort = cx_make_encrypted_string("443");
constexpr const auto kEndpoint = cx_make_encrypted_string("/api/client_data/");

constexpr char UrlAlphabet[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_' };


void AesDecrypt(const std::string &cipher, const uint8_t *key,
                const size_t key_length, std::string *decrypted) {
	//VM_DOLPHIN_RED_START
  try {
    std::string nigger;
    AutoSeededRandomPool prng;
    byte iv[AES::BLOCKSIZE];
    prng.GenerateBlock(iv, sizeof(iv));

    CFB_Mode<AES>::Decryption dec;
    dec.SetKeyWithIV(key, key_length, iv);
    StringSource string_source(
        cipher, true,
        new StreamTransformationFilter(dec, new StringSink(nigger)));
    *decrypted = nigger.data() + 16;
  } catch (CryptoPP::Exception &e) {
	  MessageBoxA(nullptr, charenc("Decryption Error"), nullptr, 0);
	  __fastfail(-1);
  }
  //VM_DOLPHIN_RED_END
}

std::string Base64Decode(const std::string &in) {
	//VM_DOLPHIN_RED_START
  std::string out;	
  std::vector<int> T(256, -1);
  for (auto i = 0u; i < 64; i++) T[UrlAlphabet[i]] = i;

  auto val = 0, valb = -8;
  for (auto i = 0u; i < in.length(); i++) {
    unsigned char c = in[i];
    if (T[c] == -1) break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  //VM_DOLPHIN_RED_END
  return out;
}

std::optional<nlohmann::json> GetClientData(const std::string &pin) {
	//VM_DOLPHIN_RED_START
	
  if (pin.length() == 0) return std::nullopt;

  boost::asio::io_context ioc;

  ssl::context ctx{ssl::context::sslv23_client};

  tcp::resolver resolver{ioc};

  ssl::stream<tcp::socket> stream{ioc, ctx};

  SSL_set_tlsext_host_name(stream.native_handle(), kHostname.c_str());

  auto const results = resolver.resolve(kHostname.c_str(), kPort.c_str());

  try {
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
  } catch (std::exception &e) {
    MessageBoxA(nullptr,
                ("Connection to host failed, check if you have an internet "
                 "connection available."),
                ("Connection Failed"), MB_OK);
    ExitProcess(0);
  }
  stream.handshake(ssl::stream_base::client);

  auto request_url = std::string(kEndpoint.c_str()) + charenc("?pin=") + pin;

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

  if (response == charenc("Invalid Pin") ||
      response == charenc("Invalid Session") ||
      response == charenc("Invalid Pin Length")) {
    return std::nullopt;
  }

  std::vector<uint8_t> key(pin.begin(), pin.end());

	//Must match the bytes on the server, used for updates.
  uint8_t salt[32] = {0x51, 0x4a, 0x39, 0x70, 0x9d, 0x6f, 0x50, 0x9f,//0x57 replaced by 0x37 - 0x38 - 0x39
                      0x6e, 0x2f, 0x7c, 0x36, 0xcf, 0xc3, 0xa0, 0xf6,//0xd4 replaced by 0xf4 - 0xf5 - 0xf6
                      0xca, 0x4d, 0x97, 0x1, 0x43, 0x75, 0x90, 0x41,//0xb1 replaced by 0x1
                      0x48, 0x31, 0xbf, 0xcb, 0xa2, 0x10, 0xbe, 0x5b};//0x6b replaced by 0x5b

  uint8_t hashed_key[32];
  fastpbkdf2_hmac_sha256(key.data(), key.size(), salt, 32, 2520, hashed_key,//2250 replaced by 2500 - 2520
                         32);


  std::string decrypted{};

AesDecrypt(Base64Decode(response), hashed_key, AES::MAX_KEYLENGTH,
             &decrypted);
  nlohmann::json j{};
  try {
    j = nlohmann::json::parse(decrypted);
  } catch (std::exception &e) {
    MessageBoxA(
        nullptr,
        charenc("Please get the latest download from the anticlient website."),
        charenc("Update Required"), MB_ICONINFORMATION);
    __fastfail(-1);
    return std::nullopt;
  }

  //VM_DOLPHIN_RED_END
  return j;
}
