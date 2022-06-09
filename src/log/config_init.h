/**
 * @file config_init.h
 * @brief The logger configuration initializer.
 *
 * @details
 * @p YAML configuration.
 * An example is shown below:
 * @code {.yaml}
 * loggers:
 *   - name: root
 *     level: info
 *     capacity: 50
 *     appenders:
 *       - type: stdout
 *       - type: file
 *         file: log.txt
 *   - name: system
 *     level: debug
 *     formatter: "%d%T%m%n"
 *     appenders:
 *       - type: stdout
 * @endcode
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-23
 *
 * @example src/log/config_init_test.cpp
 */

#pragma once

#include "log.h"


namespace ws::log {

/**
 * The appender configuration.
 */
struct AppenderConfig {
    AppenderType type;

    //! Optional.
    std::string formatter;

    //! Only required for file writters.
    std::string file;
};

bool operator==(const AppenderConfig&, const AppenderConfig&) noexcept;

bool operator!=(const AppenderConfig&, const AppenderConfig&) noexcept;

/**
 * The logger configuration.
 */
struct LoggerConfig {
    std::string name;

    Level level;

    //! Optional.
    std::size_t capacity;

    //! Optional.
    std::string formatter;

    std::list<AppenderConfig> appenders;
};

bool operator==(const LoggerConfig&, const LoggerConfig&) noexcept;

bool operator!=(const LoggerConfig&, const LoggerConfig&) noexcept;

}  // namespace ws::log

/**
 * The hash for unordered sets.
 */
template <>
struct std::hash<ws::log::LoggerConfig> {
    std::size_t operator()(const ws::log::LoggerConfig& logger) const noexcept {
        return std::hash<std::string> {}(logger.name);
    }
};

namespace ws {

template <>
class cfg::VarConverter<std::string, log::AppenderConfig> {
public:
    log::AppenderConfig operator()(const std::string_view str) const {
        const YAML::Node node {LoadYamlString(str, {"type"})};

        log::AppenderConfig appender {};
        ThrowIfYamlFieldIsNotScalar(node, "type");
        appender.type =
            log::StringToAppenderType(node["type"].as<std::string>());

        if (node["file"]) {
            ThrowIfYamlFieldIsNotScalar(node, "file");
            appender.file = node["file"].as<std::string>();
        }

        if (node["formatter"]) {
            ThrowIfYamlFieldIsNotScalar(node, "formatters");
            appender.formatter = node["formatters"].as<std::string>();
        }

        return appender;
    }
};

template <>
class cfg::VarConverter<log::AppenderConfig, std::string> {
public:
    std::string operator()(const log::AppenderConfig& appender) const noexcept {
        YAML::Node node;
        node["type"] = log::AppenderTypeToString(appender.type).data();

        if (!appender.file.empty()) {
            node["file"] = appender.file;
        }

        if (!appender.formatter.empty()) {
            node["formatter"] = appender.formatter;
        }

        std::ostringstream ss;
        ss << node;
        return ss.str();
    }
};

template <>
class cfg::VarConverter<std::string, log::LoggerConfig> {
public:
    log::LoggerConfig operator()(const std::string_view str) const {
        const YAML::Node node {
            LoadYamlString(str, {"name", "level", "appenders"})};

        log::LoggerConfig logger {};
        ThrowIfYamlFieldIsNotScalar(node, "name");
        logger.name = node["name"].as<std::string>();

        ThrowIfYamlFieldIsNotScalar(node, "level");
        logger.level = log::StringToLevel(node["level"].as<std::string>());

        if (node["capacity"]) {
            ThrowIfYamlFieldIsNotScalar(node, "capacity");
            logger.capacity = node["capacity"].as<std::size_t>();
        }

        if (node["formatter"]) {
            ThrowIfYamlFieldIsNotScalar(node, "formatter");
            logger.formatter = node["formatter"].as<std::string>();
        }

        std::ostringstream ss;
        ss << node["appenders"];
        logger.appenders =
            VarConverter<std::string, std::list<log::AppenderConfig>> {}(
                ss.str());
        return logger;
    }
};

template <>
class cfg::VarConverter<log::LoggerConfig, std::string> {
public:
    std::string operator()(const log::LoggerConfig& logger) const noexcept {
        YAML::Node node;
        node["name"] = logger.name;
        node["level"] = log::LevelToString(logger.level).data();
        node["capacity"] = logger.capacity;
        node["formatter"] = logger.formatter;
        node["appenders"] =
            VarConverter<std::list<log::AppenderConfig>, std::string> {}(
                logger.appenders);

        std::ostringstream ss;
        ss << node;
        return ss.str();
    }
};

namespace log {

/**
 * @brief Set the listener callback for a logger configuration.
 *
 * @param loggers   A logger configuration.
 * @param manager   A logger manager. It will adjust its loggers when the configuration changes.
 */
cfg::Var<std::unordered_set<LoggerConfig>>::Ptr SetListener(
    cfg::Var<std::unordered_set<LoggerConfig>>::Ptr loggers,
    Manager::Ptr manager) noexcept;

}  // namespace log

}  // namespace ws