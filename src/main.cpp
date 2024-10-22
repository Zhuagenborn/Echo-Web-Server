#include "config.h"
#include "log.h"
#include "util.h"
#include "web_server.h"

#include <cstdlib>
#include <filesystem>

using namespace ws;


namespace {

constexpr std::string_view port_tag {"server.port"};
constexpr std::string_view asset_folder_tag {"server.asset_folder"};
constexpr std::string_view alive_time_tag {"server.alive_time"};

//! Initialize the default configuration.
cfg::Config::Ptr InitDefaultConfig() noexcept {
    static constexpr std::uint16_t default_port {10000};
    static const std::string default_asset_folder {"assets"};
    static constexpr std::size_t default_alive_time {60};

    const auto config {cfg::RootConfig()};
    config->Lookup<std::uint16_t>(port_tag, default_port, "The listening port");
    config->Lookup<std::size_t>(alive_time_tag, default_alive_time,
                                "The alive time of client (in seconds)");
    config->Lookup<std::string>(asset_folder_tag, default_asset_folder,
                                "The asset folder");
    return config;
}

/**
 * @brief Load a local configuration.
 *
 * @param config An existing configuration.
 * @param file A configuration file.
 */
cfg::Config::Ptr LoadConfig(const cfg::Config::Ptr config,
                            const std::string& file) {
    config->LoadYaml(YAML::LoadFile(file));
    return config;
}

}  // namespace

int main() {
    log::Manager::InitConfig();
    auto config {InitDefaultConfig()};
    const auto curr_dir {std::filesystem::current_path()};

    try {
        // Load the local configuration if it exists.
        static constexpr std::string_view config_file {"config.yaml"};

        if (const auto config_path {curr_dir / config_file};
            std::filesystem::exists(config_path)) {
            config = LoadConfig(config, config_path);
        }
    } catch (const std::exception& err) {
        log::RootLogger()->Log(
            log::Event::Create(log::Level::Warn) << fmt::format(
                "Failed to load local configuration: {}", err.what()));
    }

    try {
        const auto port {config->Lookup<std::uint16_t>(port_tag)->GetValue()};
        const auto alive_time {
            config->Lookup<std::size_t>(alive_time_tag)->GetValue()};
        const auto asset_folder {
            config->Lookup<std::string>(asset_folder_tag)->GetValue()};

        WebServerBuilder<IPv4Addr> builder {};
        builder.SetPort(port)
            .SetAliveTime(std::chrono::seconds {alive_time})
            .SetRootDirectory(curr_dir / asset_folder);

        auto web_server {builder.Create()};
        web_server.Start();
        return EXIT_SUCCESS;

    } catch (const std::exception& err) {
        log::RootLogger()->Log(
            log::Event::Create(log::Level::Error)
            << fmt::format("Failed to run server: {}", err.what()));
        return EXIT_FAILURE;
    }
}