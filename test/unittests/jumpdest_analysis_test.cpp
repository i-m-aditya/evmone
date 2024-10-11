// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "test/experimental/jumpdest_analysis.hpp"
#include "test/utils/bytecode.hpp"
#include <evmone/baseline.hpp>
#include <gtest/gtest.h>

using namespace evmone;
using namespace evmone::experimental;
using namespace evmone::test;

namespace
{
struct JumpdestAnalysisImpl
{
    static constexpr auto name = "baseline";
    static auto analyze(bytes_view code) { return baseline::analyze(code, false); }
};


constexpr auto tail_code_padding = 100;

inline bool is_jumpdest(const bitset32& a, size_t index) noexcept
{
    return (index < a.size() && a[index]);
}

const bytecode bytecode_test_cases[]{
    push(0x5b),
    {},
    OP_JUMPDEST,
    push(0),
    push(0x5b) + OP_JUMPDEST,
    push(0x60) + OP_JUMPDEST,
    "5b000000000000000000000000000000",
    "005b0000000000000000000000000000",
    "605b00605b000000000000000000000000000000000000000000000000000000",
    "5b14000000000000005badadad0000000000000000000000606060606060ff5b",
    "5b00000000000000000000000000000000000000000000000000000000000060",
    "605b000000000000000000000000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000000000000000000000007f5b",
    "00605b000000000000000000000000000000000000000000000000000000605b",
    "005b5b5b00000000000000000000000000000000000000000000000000000000",
    "5b5b000000000000000000000000000000000000000000000000000000000000",
    "0000000000000000000000000000000000000000000000000000000000000000"  // vvv
    "7000000000000000005b000000000000005b5b00000000000000000000000000",
};
}  // namespace

template <typename>
class ja_test : public testing::Test
{};

using test_types = testing::Types<JumpdestAnalysisImpl>;

class NameGenerator
{
public:
    template <typename T>
    static std::string GetName(int)
    {
        return T::name;
    }
};

TYPED_TEST_SUITE(ja_test, test_types, NameGenerator);

TYPED_TEST(ja_test, validate)
{
    for (const auto& code : bytecode_test_cases)
    {
        const auto a0 = official_analyze_jumpdests(code.data(), code.size());
        const auto analysis = TypeParam::analyze(code);

        for (size_t i = 0; i < code.size() + tail_code_padding; ++i)
        {
            SCOPED_TRACE(i);
            EXPECT_EQ(analysis.check_jumpdest(i), is_jumpdest(a0, i));
        }
    }
}

TEST(jumpdest_analysis, compare_implementations)
{
    for (const auto& t : bytecode_test_cases)
    {
        SCOPED_TRACE(hex(t));
        const auto data = t.data();
        const auto data_size = t.size();

        const auto xxx = JumpdestAnalysisImpl::analyze(t);

        const auto a0 = official_analyze_jumpdests(data, data_size);
        const auto a2 = build_jumpdest_map_vec1(data, data_size);
        const auto v2 = build_jumpdest_map_vec2(data, data_size);
        const auto x3 = build_jumpdest_map_vec3(data, data_size);
        const auto v3 = build_jumpdest_map_sttni(data, data_size);
        const auto v4 = build_jumpdest_map_str_avx2(data, data_size);
        const auto v5 = build_jumpdest_map_str_avx2_mask(data, data_size);
        const auto v5a = build_jumpdest_map_str_avx2_mask_v2(data, data_size);
        const auto v6 = build_jumpdest_map_str_avx2_mask2(data, data_size);
        const auto a3 = build_jumpdest_map_bitset1(data, data_size);
        const auto a4 = build_internal_code_v1(data, data_size);
        const auto a5 = build_internal_code_v2(data, data_size);
        const auto a6 = build_internal_code_v3(data, data_size);
        const auto ic4 = build_internal_code_v4(data, data_size);
        const auto ic8 = build_internal_code_v8(data, data_size);
        const auto s1 = build_jumpdest_map_simd1(data, data_size);
        const auto s2 = build_jumpdest_map_simd2(data, data_size);
        const auto s3 = build_jumpdest_map_simd3(data, data_size);
        const auto s4 = build_jumpdest_map_simd4(data, data_size);

        for (size_t i = 0; i < data_size + tail_code_padding; ++i)
        {
            SCOPED_TRACE(i);
            const bool expected = is_jumpdest(a0, i);
            EXPECT_EQ(xxx.check_jumpdest(i), expected);
            EXPECT_EQ(is_jumpdest(a2, i), expected);
            EXPECT_EQ(is_jumpdest(v2, i), expected);
            EXPECT_EQ(is_jumpdest(x3, i), expected);
            EXPECT_EQ(is_jumpdest(v3, i), expected);
            EXPECT_EQ(is_jumpdest(v4, i), expected);
            EXPECT_EQ(is_jumpdest(v5, i), expected);
            EXPECT_EQ(is_jumpdest(v5a, i), expected);
            EXPECT_EQ(is_jumpdest(v6, i), expected);
            EXPECT_EQ(is_jumpdest(a3, i), expected);
            EXPECT_EQ(is_jumpdest(a4.get(), t.size(), i), expected);
            EXPECT_EQ(is_jumpdest(a5.get(), t.size(), i), expected);
            EXPECT_EQ(is_jumpdest(a6.get(), t.size(), i), expected);
            EXPECT_EQ(is_jumpdest(ic4.get(), t.size(), i), expected);
            EXPECT_EQ(is_jumpdest(ic8.get(), t.size(), i), expected);
            EXPECT_EQ(is_jumpdest(s1, i), expected);
            EXPECT_EQ(is_jumpdest(s2, i), expected);
            EXPECT_EQ(is_jumpdest(s3, i), expected);
            EXPECT_EQ(is_jumpdest(s4, i), expected);
        }
    }
}
