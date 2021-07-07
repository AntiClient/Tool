#ifndef CX_ENCRYPTED_STRING_H_
#define CX_ENCRYPTED_STRING_H_

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#define CONSTEXPR constexpr
#define FORCEINLINE __forceinline



namespace cxx {
namespace errr {
namespace {
const char *EncryptedStringRuntimeError;
} // namespace
} // namespace errr

namespace detail {
CONSTEXPR const auto kBitsInByte = 8;
CONSTEXPR const auto kMaxByte = std::numeric_limits<uint8_t>::max();

CONSTEXPR FORCEINLINE int32_t CharToInt(const char ch) { return ch - '0'; }

using KeyType = uint8_t;

template <KeyType K>
CONSTEXPR FORCEINLINE uint8_t GenerateXorKey(const size_t idx) {
  return ((K >> ((idx % sizeof(K)) * kBitsInByte)) % kMaxByte);
}

template <KeyType K, typename T>
CONSTEXPR FORCEINLINE T EncryptAt(const T *str, const size_t idx) {
#ifndef _DEBUG
  return (str[idx] ^ K);
#else
  return str[idx];
#endif
}

template <KeyType K, typename T>
CONSTEXPR FORCEINLINE T DecryptAt(const T *str, const size_t idx) {
#ifndef _DEBU G
  return (str[idx] ^ K);
#else
  return str[idx];
#endif
}
} // namespace detail

struct EncryptedStringBase {};

template <detail::KeyType K, typename T, typename I> struct EncryptedString;

template <detail::KeyType K, typename T, size_t... I>
struct EncryptedString<K, T, std::index_sequence<I...>> : EncryptedStringBase {
public:
  using SizeType = size_t;

public:
  explicit CONSTEXPR FORCEINLINE EncryptedString(const T *const s)
      : buffer_{detail::EncryptAt<K>(s, I)...} {}

  // returns the length of the string in T's
  CONSTEXPR FORCEINLINE SizeType Length() const { return (sizeof...(I)); }

  // returns the size of the string in bytes (without null terminator)
  CONSTEXPR FORCEINLINE SizeType Size() const { return (Length() * sizeof(T)); }

  // returns a decrypted character at idx
  FORCEINLINE T operator[](const SizeType idx) const {
    return detail::DecryptAt<K>(buffer_, idx);
  }

  // decrypts the string on the stack
  FORCEINLINE const T *c_str() const {
#ifndef _DEBUG
    auto *const stack_buffer = static_cast<T *>(_alloca(Size() + sizeof(T)));
    for (SizeType iii = 0; iii < Length(); ++iii) {
      stack_buffer[iii] = operator[](iii);
    }
    stack_buffer[Length()] = static_cast<T>(0);
    return stack_buffer;
#else
    return buffer_;
#endif
  }

  // decrypts the string into a basic_sting<T>
  FORCEINLINE std::basic_string<T> str() const {
    std::basic_string<T> str(Length(), static_cast<T>(' '));
    for (SizeType iii = 0; iii < Length(); ++iii) {
      str[iii] = operator[](iii);
    }
    return str;
  }

  // decrypts the string into a basic_sting<T>
  FORCEINLINE operator std::basic_string<T>() const { return str(); }

private:
  const T buffer_[sizeof...(I)];
};

template <detail::KeyType K, typename T, size_t N>
CONSTEXPR FORCEINLINE auto MakeEncryptedString(const T (&s)[N]) {
#ifndef _DEBUG
  constexpr auto length = (N - 1);
#else
  constexpr auto length = N;
#endif
  return true ? EncryptedString<K, T, std::make_index_sequence<length>>(s)
              : throw errr::EncryptedStringRuntimeError;
}
} // namespace cxx

#if defined _DEBUG
#define cx_make_encryption_key(t) 0
#else
  // clang-format off
#define cx_make_encryption_key(t)                                        \
  static_cast<t>((cxx::detail::CharToInt(__TIME__[7]) +           \
                  (cxx::detail::CharToInt(__TIME__[6]) * 10) +    \
                  (cxx::detail::CharToInt(__TIME__[4]) * 60) +    \
                  (cxx::detail::CharToInt(__TIME__[3]) * 600) +   \
                  (cxx::detail::CharToInt(__TIME__[1]) * 3600) +  \
                  (cxx::detail::CharToInt(__TIME__[0]) * 36000) + \
                  __LINE__ + __COUNTER__) % std::numeric_limits<t>::max())
  // clang-format on
#endif

#define cx_make_encrypted_string(str)                                          \
  cxx::MakeEncryptedString<cx_make_encryption_key(cxx::detail::KeyType)>(str)

#define charenc(s) cx_make_encrypted_string(s).c_str()
#define strenc(s) cx_make_encrypted_string(s).str()

#endif // CX_ENCRYPTED_STRING_H_