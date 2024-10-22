#include "config.h"


namespace ws {

namespace cfg {

namespace {

/**
 * @brief Recursively extract members of a @p YAML node.
 *
 * @details
 * The return value recursively contains members on every level, not just leaves.
 * For example, for the following @p YAML file:
 *
 * @code {.yaml}
 * system:
 *   port: 80
 *   ip: "127.0.0.1"
 * @endcode
 *
 * This method will return a list of fours items:
 * - A node with an empty name representing the entire node.
 * - A node with named @p system representing the port and ip address.
 * - A node with named @p system.port representing the port.
 * - A node with named @p system.ip representing the ip address.
 *
 * @param node A @p YAML node.
 * @param prefix A prefix representing the node level.
 * @return A list of members and names representing their levels.
 */
std::list<std::pair<std::string, YAML::Node>> ExtractMembers(
    const YAML::Node& node, const std::string& prefix = "") {
    std::list<std::pair<std::string, YAML::Node>> members;
    members.emplace_back(prefix, node);
    if (node.IsMap()) {
        for (auto it {node.begin()}; it != node.end(); ++it) {
            std::ostringstream ss;
            ss << it->first;
            const auto key {ss.str()};
            auto sub_members {ExtractMembers(
                it->second, prefix.empty() ? key : prefix + "." + key)};
            members.merge(
                sub_members,
                [](const std::pair<std::string, YAML::Node>& lhs,
                   const std::pair<std::string, YAML::Node>& rhs) noexcept {
                    return lhs.first < rhs.first;
                });
        }
    }

    return members;
}

}  // namespace

VarBase::VarBase(const std::string_view name,
                 const std::string_view description) noexcept :
    name_ {name}, description_ {description} {}

std::string_view VarBase::Name() const noexcept {
    return name_;
}

std::string_view VarBase::Description() const noexcept {
    return description_;
}

Config::Config(const std::string_view name) noexcept : name_ {name} {}

std::string_view Config::Name() const noexcept {
    return name_;
}

VarBase::Ptr Config::LookupBase(const std::string_view name) const noexcept {
    const std::shared_lock locker {mtx_};
    if (const auto var {vars_.find(name.data())}; var != vars_.cend()) {
        return var->second;
    } else {
        return nullptr;
    }
}

void Config::LoadYaml(const YAML::Node& root) {
    for (const auto& node : ExtractMembers(root)) {
        const auto key {node.first};
        if (!key.empty()) {
            if (const auto var {LookupBase(key)}; var) {
                std::ostringstream ss;
                ss << node.second;
                var->FromString(ss.str());
            }
        }
    }
}

void Config::Visit(
    const std::function<void(VarBase::Ptr)> visitor) const noexcept {
    const std::shared_lock locker {mtx_};
    std::ranges::for_each(vars_, [&visitor](const auto& var) noexcept {
        try {
            visitor(var.second);
        } catch (const std::exception& err) {
            std::osyncstream {std::cerr} << err.what() << std::endl;
        }
    });
}

Config::Ptr RootConfig() noexcept {
    static const auto ins {std::make_shared<Config>("root")};
    return ins;
}

}  // namespace cfg

}  // namespace ws