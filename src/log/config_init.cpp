#include "config_init.h"

#include <cassert>


namespace ws::log {

bool operator==(const AppenderConfig& lhs, const AppenderConfig& rhs) noexcept {
    return lhs.type == rhs.type && lhs.formatter == rhs.formatter
           && lhs.file == rhs.file;
}

bool operator!=(const AppenderConfig& lhs, const AppenderConfig& rhs) noexcept {
    return !(lhs == rhs);
}

bool operator==(const LoggerConfig& lhs, const LoggerConfig& rhs) noexcept {
    return lhs.name == rhs.name && lhs.level == rhs.level
           && lhs.capacity == rhs.capacity && lhs.formatter == rhs.formatter
           && lhs.appenders == rhs.appenders;
}

bool operator!=(const LoggerConfig& lhs, const LoggerConfig& rhs) noexcept {
    return !(lhs == rhs);
}

cfg::Var<std::unordered_set<LoggerConfig>>::Ptr SetListener(
    const cfg::Var<std::unordered_set<LoggerConfig>>::Ptr loggers,
    const ws::log::Manager::Ptr manager) noexcept {
    loggers->AddListener(
        [manager](const std::unordered_set<LoggerConfig>& old_cfgs,
                  const std::unordered_set<LoggerConfig>& new_cfgs) noexcept {
            for (const auto& logger_cfg : new_cfgs) {
                if (const auto cfg {old_cfgs.find(logger_cfg)};
                    cfg != old_cfgs.cend() && *cfg == logger_cfg) {
                    continue;
                }

                // Remove the old logger.
                manager->RemoveLogger(logger_cfg.name);

                // Create a new logger.
                const auto logger {manager->FindLogger(
                    logger_cfg.name, logger_cfg.level, logger_cfg.capacity)};
                if (!logger_cfg.formatter.empty()) {
                    logger->SetDefaultFormatter(logger_cfg.formatter);
                }

                // Add appenders.
                logger->ClearAppenders();
                for (const auto& appender_cfg : logger_cfg.appenders) {
                    Appender::Ptr appender;
                    switch (appender_cfg.type) {
                        case AppenderType::StdOut: {
                            appender = std::make_shared<StdOutAppender>(
                                logger->DefaultFormatter());
                            break;
                        }
                        case AppenderType::File: {
                            appender = std::make_shared<FileAppender>(
                                appender_cfg.file, logger->DefaultFormatter());
                            break;
                        }
                        default: {
                            assert(false);
                        }
                    }

                    logger->AddAppender(appender);
                }
            }
        });

    return loggers;
}

}  // namespace ws::log