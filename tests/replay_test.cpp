#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#include "traceforge/generator/workload_generator.hpp"
#include "traceforge/recording/log.hpp"
#include "traceforge/replay/replay.hpp"

namespace {

class TemporaryLog {
  public:
    explicit TemporaryLog(std::string label) {
        static std::atomic<std::uint64_t> next_id{0};
        path_ = std::filesystem::temp_directory_path() /
                ("traceforge_replay_" + std::move(label) + "_" +
                 std::to_string(next_id.fetch_add(1)) + ".tflog");
    }

    ~TemporaryLog() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

  private:
    std::filesystem::path path_;
};

bool equals(const std::vector<std::uint64_t>& actual,
            std::initializer_list<std::uint64_t> expected) {
    return actual == std::vector<std::uint64_t>{expected};
}

}  // namespace

int main() {
    traceforge::generator::GeneratorConfig config;
    config.seed = 42;
    config.duration = std::chrono::milliseconds{100};
    const auto workload =
        traceforge::generator::WorkloadGenerator{config}.generate();
    if (workload.events.size() < 4) {
        std::cerr << "Generator did not create enough replay fixtures\n";
        return 1;
    }

    TemporaryLog log{"ordering"};
    {
        traceforge::recording::LogWriter writer{
            log.path(), {.flush_every_records = 1, .created_unix_ns = 123}};
        const std::int64_t arrivals[] = {
            30'000'000, 10'000'000, 10'000'000, 20'000'000};
        for (std::size_t index = 0; index < 4; ++index) {
            if (!writer.append(workload.events[index], arrivals[index])) {
                std::cerr << "Failed to write replay fixture: "
                          << writer.error() << '\n';
                return 1;
            }
        }
    }

    const traceforge::replay::ReplayEngine engine{log.path()};
    if (!engine.good() || engine.index().size() != 4) {
        std::cerr << "Replay engine failed to index a valid log: "
                  << engine.error() << '\n';
        return 1;
    }
    if (engine.index()[0].record_index != 1 ||
        engine.index()[1].record_index != 2 ||
        engine.index()[2].record_index != 3 ||
        engine.index()[3].record_index != 0 ||
        engine.index()[0].file_offset <=
            traceforge::recording::kFileHeaderSize) {
        std::cerr << "Timestamp index ordering or file offsets are wrong\n";
        return 1;
    }

    std::vector<std::uint64_t> first_order;
    const auto first = engine.replay(
        {}, [&first_order](const traceforge::recording::LogRecord& record) {
            first_order.push_back(record.record_index);
            return true;
        });
    std::vector<std::uint64_t> second_order;
    const auto second = engine.replay(
        {}, [&second_order](const traceforge::recording::LogRecord& record) {
            second_order.push_back(record.record_index);
            return true;
        });
    if (first.status != traceforge::replay::ReplayStatus::complete ||
        second.status != traceforge::replay::ReplayStatus::complete ||
        first.hash != second.hash || first.replayed_records != 4 ||
        !equals(first_order, {1, 2, 3, 0}) || first_order != second_order) {
        std::cerr << "Repeated replay was not deterministic\n";
        return 1;
    }

    std::vector<std::uint64_t> seek_order;
    const auto seek = engine.replay(
        {.from_collector_timestamp_ns = 20'000'000,
         .until_collector_timestamp_ns = 30'000'001,
         .speed = 0.0},
        [&seek_order](const traceforge::recording::LogRecord& record) {
            seek_order.push_back(record.record_index);
            return true;
        });
    if (seek.status != traceforge::replay::ReplayStatus::complete ||
        seek.replayed_records != 2 || !equals(seek_order, {3, 0}) ||
        seek.first_timestamp_ns != 20'000'000 ||
        seek.last_timestamp_ns != 30'000'000 || seek.hash == first.hash) {
        std::cerr << "Timestamp seek returned the wrong replay window\n";
        return 1;
    }

    const auto pacing_started = std::chrono::steady_clock::now();
    const auto paced = engine.replay({.speed = 10.0});
    const auto pacing_elapsed =
        std::chrono::steady_clock::now() - pacing_started;
    if (paced.status != traceforge::replay::ReplayStatus::complete ||
        paced.hash != first.hash ||
        pacing_elapsed < std::chrono::milliseconds{1}) {
        std::cerr << "Replay speed changed deterministic output\n";
        return 1;
    }

    const auto invalid_window = engine.replay(
        {.from_collector_timestamp_ns = 20'000'000,
         .until_collector_timestamp_ns = 20'000'000});
    if (invalid_window.status !=
        traceforge::replay::ReplayStatus::invalid_options) {
        std::cerr << "Invalid replay window was accepted\n";
        return 1;
    }

    const auto rejected = engine.replay(
        {}, [](const traceforge::recording::LogRecord&) { return false; });
    if (rejected.status != traceforge::replay::ReplayStatus::consumer_error ||
        rejected.replayed_records != 0) {
        std::cerr << "Consumer rejection was not propagated\n";
        return 1;
    }

    TemporaryLog truncated{"truncated"};
    std::filesystem::copy_file(log.path(), truncated.path());
    std::filesystem::resize_file(
        truncated.path(), std::filesystem::file_size(truncated.path()) - 1);
    const traceforge::replay::ReplayEngine bad_engine{truncated.path()};
    if (bad_engine.good()) {
        std::cerr << "Truncated log was accepted for replay\n";
        return 1;
    }

    return 0;
}
