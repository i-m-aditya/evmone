// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2023 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "blockchaintest.hpp"
#include <CLI/CLI.hpp>
#include <evmone/evmone.h>
#include <evmone/execution_state.hpp>
#include <gtest/gtest.h>
#include <iostream>

namespace fs = std::filesystem;

namespace
{
class BlockchainGTest : public testing::Test
{
    fs::path m_json_test_file;
    evmone::MegaContext& m_mega_ctx;
    evmc::VM& m_vm;

public:
    explicit BlockchainGTest(
        fs::path json_test_file, evmone::MegaContext& mega_ctx, evmc::VM& vm) noexcept
      : m_json_test_file{std::move(json_test_file)}, m_mega_ctx{mega_ctx}, m_vm{vm}
    {}

    void TestBody() final
    {
        std::ifstream f{m_json_test_file};

        try
        {
            evmone::test::run_blockchain_tests(
                evmone::test::load_blockchain_tests(f), m_mega_ctx, m_vm);
        }
        catch (const evmone::test::UnsupportedTestFeature& ex)
        {
            GTEST_SKIP() << ex.what();
        }
    }
};

void register_test(const std::string& suite_name, const fs::path& file,
    evmone::MegaContext& mega_ctx, evmc::VM& vm)
{
    testing::RegisterTest(suite_name.c_str(), file.stem().string().c_str(), nullptr, nullptr,
        file.string().c_str(), 0, [file, &mega_ctx, &vm]() -> testing::Test* {
            return new BlockchainGTest(file, mega_ctx, vm);
        });
}

void register_test_files(const fs::path& root, evmone::MegaContext& mega_ctx, evmc::VM& vm)
{
    if (is_directory(root))
    {
        std::vector<fs::path> test_files;
        std::copy_if(fs::recursive_directory_iterator{root}, fs::recursive_directory_iterator{},
            std::back_inserter(test_files), [](const fs::directory_entry& entry) {
                return entry.is_regular_file() && entry.path().extension() == ".json";
            });
        std::sort(test_files.begin(), test_files.end());

        for (const auto& p : test_files)
            register_test(fs::relative(p, root).parent_path().string(), p, mega_ctx, vm);
    }
    else  // Treat as a file.
    {
        register_test(root.parent_path().string(), root, mega_ctx, vm);
    }
}
}  // namespace


int main(int argc, char* argv[])
{
    try
    {
        testing::InitGoogleTest(&argc, argv);  // Process GoogleTest flags.

        CLI::App app{"evmone blockchain test runner"};

        std::vector<std::string> paths;
        app.add_option("path", paths, "Path to test file or directory")
            ->required()
            ->check(CLI::ExistingPath);

        bool trace_flag = false;
        app.add_flag("--trace", trace_flag, "Enable EVM tracing");

        CLI11_PARSE(app, argc, argv);

        evmone::MegaContext mega_ctx = {};
        evmc::VM vm{evmc_create_evmone()};

        if (trace_flag)
            vm.set_option("trace", "1");

        for (const auto& p : paths)
            register_test_files(p, mega_ctx, vm);

        return RUN_ALL_TESTS();
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << "\n";
        return -1;
    }
}
