/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "DexMemberRefs.h"

#include "Debug.h"
#include "DexUtil.h"
#include "TypeUtil.h"

#include <sstream>

namespace dex_member_refs {

template <typename T>
static size_t expect(const std::string& s,
                     const T& needle,
                     size_t start_pos = 0) {
  auto pos = s.find(needle, start_pos);
  if (pos == std::string::npos) {
    std::ostringstream ss;
    ss << "Could not find \"" << needle << "\" in \"" << s << "\"";
    always_assert_log(false, ss.str().c_str());
  }
  return pos;
}

FieldDescriptorTokens parse_field(const std::string& s) {
  auto cls_end = expect(s, '.');
  auto name_start = cls_end + 1;
  auto name_end = expect(s, ':', name_start);
  auto type_start = name_end + 1;
  always_assert_log(type_start < s.size(), "No type found");

  FieldDescriptorTokens fdt;
  fdt.cls = s.substr(0, cls_end);
  fdt.name = s.substr(name_start, name_end - name_start);
  fdt.type = s.substr(type_start);
  return fdt;
}

namespace {

std::vector<std::string> split_args(std::string args) {
  std::vector<std::string> ret;
  auto begin = size_t{0};
  while (begin < args.length()) {
    auto ch = args[begin];
    auto end = begin + 1;
    if (ch == '[') {
      while (args[end] == '[') {
        ++end;
      }
      ch = args[end];
      ++end;
    }
    if (ch == 'L') {
      auto semipos = args.find(';', end);
      redex_assert(semipos != std::string::npos);
      end = semipos + 1;
    }
    ret.emplace_back(args.substr(begin, end - begin));
    begin = end;
  }
  return ret;
}

} // namespace

template <bool kCheckFormat>
MethodDescriptorTokens parse_method(const std::string& s) {
  auto cls_end = expect(s, '.');
  auto name_start = cls_end + 1;
  auto name_end = expect(s, ":(", name_start);
  auto args_start = name_end + 2;
  auto args_end = expect(s, ')', args_start);
  auto rtype_start = args_end + 1;
  always_assert_log(rtype_start < s.size(), "No return type found");

  MethodDescriptorTokens mdt;
  mdt.cls = s.substr(0, cls_end);
  mdt.name = s.substr(name_start, name_end - name_start);
  auto args_str = s.substr(args_start, args_end - args_start);
  mdt.args = split_args(args_str);
  mdt.rtype = s.substr(rtype_start);
  if (kCheckFormat) {
    // Macros are ugly, but it will print nicer since asserts are macros, too.
#define context_assert(e, local_ctx) \
  always_assert_log(e, "Invalid: %s (%s)", local_ctx.c_str(), s.c_str());

    context_assert(type::is_valid(mdt.cls), mdt.cls);
    // Class must not be a primitive.
    context_assert(mdt.cls.at(0) == 'L' || mdt.cls.at(0) == '[', mdt.cls);

    context_assert(!mdt.name.empty(), mdt.name);
    // Name must be a valid identifier.
    context_assert(is_valid_identifier(mdt.name), mdt.name);

    for (const std::string& t : mdt.args) {
      context_assert(type::is_valid(t), t);
    }
    context_assert(type::is_valid(mdt.rtype), mdt.rtype);
  }
  return mdt;
}
template MethodDescriptorTokens parse_method<false>(const std::string&);
template MethodDescriptorTokens parse_method<true>(const std::string&);

} // namespace dex_member_refs
