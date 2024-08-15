/*
Copyright 2018 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lib/cstring.h"

#include <gtest/gtest.h>

namespace P4::Test {

using namespace P4::literals;

TEST(cstring, construct) {
    cstring c(nullptr);
    EXPECT_TRUE(c.isNullOrEmpty());
    EXPECT_TRUE(c.isNull());
    EXPECT_EQ(c.size(), 0u);

    cstring c1("");
    EXPECT_TRUE(c1.isNullOrEmpty());
    EXPECT_FALSE(c1.isNull());
    EXPECT_EQ(c1.size(), 0u);
    EXPECT_FALSE(c == c1);
    EXPECT_TRUE(c != c1);

    c = nullptr;
    EXPECT_TRUE(c.isNullOrEmpty());
    EXPECT_TRUE(c.isNull());

    c = ""_cs;
    EXPECT_TRUE(c.isNullOrEmpty());
    EXPECT_FALSE(c.isNull());

    c1 = c;
    EXPECT_TRUE(c1.isNullOrEmpty());
    EXPECT_FALSE(c1.isNull());

    std::string s = "";
    c = s;
    EXPECT_TRUE(c.isNullOrEmpty());
    EXPECT_FALSE(c.isNull());
}

TEST(cstring, toupper) {
    cstring c = "simple tEst"_cs;
    cstring c1 = ""_cs;

    EXPECT_EQ(c.toUpper(), "SIMPLE TEST");
    EXPECT_EQ(c1.toUpper(), "");
}

TEST(cstring, capitalize) {
    cstring c = "simple tEst"_cs;
    cstring c1 = ""_cs;

    EXPECT_EQ(c.capitalize(), "Simple tEst");
    EXPECT_EQ(c1.capitalize(), "");
}

TEST(cstring, tolower) {
    cstring c = "SIMple tEsT"_cs;
    cstring c1 = ""_cs;

    EXPECT_EQ(c.toLower(), "simple test");
    EXPECT_EQ(c1.toLower(), "");
}

TEST(cstring, compare) {
    cstring c = "simple"_cs;
    cstring c1 = ""_cs;

    EXPECT_EQ(c, "simple");
    EXPECT_EQ(c, c);
    EXPECT_NE(c, c1);
    EXPECT_EQ(c.size(), strlen("simple"));

    std::string s = "simple";
    std::stringstream str;
    str << s;
    c = str;
    EXPECT_EQ(c, "simple");
    EXPECT_EQ(c, c);
    EXPECT_NE(c, c1);
    EXPECT_EQ(c.size(), strlen("simple"));
    EXPECT_FALSE(c != "simple");  // NOLINT
    EXPECT_FALSE(c == "other");   // NOLINT
    EXPECT_TRUE(c != "other");    // NOLINT

    EXPECT_TRUE(c < "zombie");     // NOLINT
    EXPECT_FALSE(c < "awesome");   // NOLINT
    EXPECT_TRUE(c <= "zombie");    // NOLINT
    EXPECT_FALSE(c <= "awesome");  // NOLINT
    EXPECT_TRUE(c >= "awesome");   // NOLINT
    EXPECT_TRUE(c > "awesome");    // NOLINT
    EXPECT_FALSE(c >= "zombie");   // NOLINT
    EXPECT_FALSE(c > "zombie");    // NOLINT

    const char *ptr = c.c_str();
    EXPECT_FALSE(strncmp(ptr, "simple", 7));
}

TEST(cstring, find) {
    cstring c = "simplest"_cs;
    EXPECT_EQ(c.find('s'), c.c_str());
    EXPECT_EQ(c.find('z'), nullptr);
    EXPECT_NE(c.findlast('s'), c.c_str());
    EXPECT_NE(c.findlast('s'), nullptr);
    EXPECT_EQ(c.find('z'), nullptr);
}

TEST(cstring, substr) {
    cstring c = "simplest"_cs;
    EXPECT_EQ(c.substr(3), "plest");
    EXPECT_EQ(c.substr(3, 2), "pl");
    EXPECT_EQ(c.substr(10), "");
    EXPECT_EQ(c.substr(3, 10), "plest");
    EXPECT_EQ(c.exceptLast(2), "simple");
}

TEST(cstring, replace) {
    cstring c = "Original"_cs;
    EXPECT_EQ(c.replace("in", "out"), "Origoutal");
    EXPECT_EQ(c.replace("", "out"), c);
    EXPECT_EQ(c.replace("i", "o"), "Orogonal");
    EXPECT_EQ(c.replace("i", ""), "Orgnal");
}

TEST(cstring, literalSuffix) {
    cstring c("test");
    EXPECT_EQ(c, "test"_cs);
    EXPECT_TRUE((std::is_same_v<cstring, decltype(""_cs)>));
}

TEST(cstring, is_cached) {
    [[maybe_unused]] cstring test = "test"_cs;
    EXPECT_FALSE(
        cstring::is_cached("we really do not expect that this string is already in cstring cache"));
    EXPECT_TRUE(cstring::is_cached("test"));
}

TEST(cstring, get_cached) {
    [[maybe_unused]] cstring test = "test"_cs;
    EXPECT_TRUE(
        cstring::get_cached("we really do not expect that this string is already in cstring cache")
            .isNull());
    EXPECT_FALSE(cstring::get_cached("test").isNullOrEmpty());
}

}  // namespace P4::Test
