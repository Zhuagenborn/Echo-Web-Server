/**
 * @file config.h
 * @brief The configuration manager based on @p YAML.
 *
 * @author Chen Zhenshuo (chenzs108@outlook.com)
 * @par GitHub
 * https://github.com/Zhuagenborn
 * @version 1.0
 * @date 2022-05-10
 *
 * @example tests/config_test.cpp
 */

#pragma once

#include "util.h"

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <syncstream>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace ws {

namespace cfg {

/**
 * @brief
 * Convert a configuration variable into another type.
 * The default implementation is using @p static_cast.
 *
 * @tparam From The original type.
 * @tparam To A new type.
 *
 * @note
 * If a custom type needs to be saved to a configuration file,
 * it needs to be template-specialized to allow interconversion between it and strings.
 */
template <typename From, typename To>
class VarConverter {
public:
    To operator()(const From& val) const noexcept {
        return static_cast<To>(val);
    }
};

//! Convert a configuration variable into a string using @p to_string.
template <typename From>
class VarConverter<From, std::string> {
public:
    std::string operator()(const From& val) const noexcept {
        using std::to_string;
        return to_string(val);
    }
};

//! Return the variable itself if it is a string.
template <>
class VarConverter<std::string, std::string> {
public:
    std::string operator()(const std::string_view val) const noexcept {
        return val.data();
    }
};

/**
 * @brief Convert a string into a configuration variable using @p std::istringstream.
 *
 * @exception std::invalid_argument The conversion failed.
 */
template <typename To>
class VarConverter<std::string, To> {
public:
    To operator()(const std::string_view str) const {
        To val {};
        std::istringstream ss {str.data()};
        ss >> val;
        if (ss.fail()) {
            throw std::invalid_argument {
                fmt::format("Invalid value: '{}'", str)};
        } else {
            return val;
        }
    }
};

/**
 * @brief Convert a string into a list of configuration variables.
 *
 * @exception std::invalid_argument The string does not represent a list.
 */
template <typename T>
class VarConverter<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string_view str) const {
        const auto node {LoadYamlString(str)};
        if (!node.IsSequence()) {
            throw std::invalid_argument {
                fmt::format("Mismatched type: '{}'", str)};
        }

        std::list<T> vals;
        std::ranges::for_each(node, [&vals](const YAML::Node& child) {
            std::ostringstream ss;
            ss << child;
            vals.emplace_back(VarConverter<std::string, T> {}(ss.str()));
        });

        return vals;
    }
};

//! Convert a list of configuration variables into a string.
template <typename T>
class VarConverter<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& vals) const noexcept {
        YAML::Node node;
        std::ranges::for_each(vals, [&node](const T& val) {
            node.push_back(
                LoadYamlString(VarConverter<T, std::string> {}(val)));
        });

        std::ostringstream ss;
        ss << node;
        return ss.str();
    }
};

//! Convert a string into an array of configuration variables.
template <typename T>
class VarConverter<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string_view str) const {
        std::vector<T> vals;
        std::ranges::move(VarConverter<std::string, std::list<T>> {}(str),
                          std::back_inserter(vals));
        return vals;
    }
};

//! Convert an array of configuration variables into a string.
template <typename T>
class VarConverter<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& vals) const noexcept {
        return VarConverter<std::list<T>, std::string> {}(
            {vals.cbegin(), vals.cend()});
    }
};

//! Convert a string into a set of configuration variables.
template <typename T>
class VarConverter<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string_view str) const {
        std::set<T> vals;
        std::ranges::move(VarConverter<std::string, std::list<T>> {}(str),
                          std::inserter(vals, vals.end()));
        return vals;
    }
};

//! Convert a set of configuration variables into a string.
template <typename T>
class VarConverter<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& vals) const noexcept {
        return VarConverter<std::list<T>, std::string> {}(
            {vals.cbegin(), vals.cend()});
    }
};

//! Convert a string into an unordered set of configuration variables.
template <typename T>
class VarConverter<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string_view str) const {
        std::unordered_set<T> vals;
        std::ranges::move(VarConverter<std::string, std::list<T>> {}(str),
                          std::inserter(vals, vals.end()));
        return vals;
    }
};

//! Convert an unordered set of configuration variables into a string.
template <typename T>
class VarConverter<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& vals) const noexcept {
        return VarConverter<std::list<T>, std::string> {}(
            {vals.cbegin(), vals.cend()});
    }
};

/**
 * @brief Convert a string into a map of configuration variables.
 *
 * @exception std::invalid_argument The string does not represent a map.
 */
template <typename T>
class VarConverter<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string_view str) const {
        const auto node {LoadYamlString(str)};
        if (!node.IsMap()) {
            throw std::invalid_argument {
                fmt::format("Mismatched type: '{}'", str)};
        }

        std::map<std::string, T> vals;
        std::ranges::for_each(
            node, [&vals](const std::pair<YAML::Node, YAML::Node>& child) {
                std::ostringstream key;
                key << child.first;
                std::ostringstream val;
                val << child.second;
                vals.emplace(key.str(),
                             VarConverter<std::string, T> {}(val.str()));
            });

        return vals;
    }
};

//! Convert a map of configuration variables into a string.
template <typename T>
class VarConverter<std::map<std::string, T>, std::string> {
public:
    std::string operator()(
        const std::map<std::string, T>& vals) const noexcept {
        YAML::Node node;
        std::ranges::for_each(
            vals, [&node](const std::pair<std::string, T>& val) {
                node[val.first] =
                    LoadYamlString(VarConverter<T, std::string> {}(val.second));
            });

        std::ostringstream ss;
        ss << node;
        return ss.str();
    }
};

//! Convert a string into an unordered map of configuration variables.
template <typename T>
class VarConverter<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(
        const std::string_view str) const {
        std::unordered_map<std::string, T> vals;
        std::ranges::move(
            VarConverter<std::string, std::map<std::string, T>> {}(str),
            std::inserter(vals, vals.end()));
        return vals;
    }
};

//! Convert an unordered map of configuration variables into a string.
template <typename T>
class VarConverter<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(
        const std::unordered_map<std::string, T>& vals) const noexcept {
        return VarConverter<std::map<std::string, T>, std::string> {}(
            {vals.cbegin(), vals.cend()});
    }
};

//! Basic information about a configuration variable.
class VarBase {
public:
    using Ptr = std::shared_ptr<VarBase>;

    explicit VarBase(std::string_view name,
                     std::string_view description = "") noexcept;

    virtual ~VarBase() noexcept = default;

    //! Get the variable name.
    std::string_view Name() const noexcept;

    //! Get the variable description.
    std::string_view Description() const noexcept;

    //! Convert the variable into a string.
    virtual std::string ToString() const noexcept = 0;

    //! Set the variable from a string.
    virtual void FromString(std::string_view str) = 0;

protected:
    std::string name_;
    std::string description_;
};

/**
 * @brief The configuration variable.
 *
 * @tparam T The variable type.
 * @tparam FromStr A converter that can convert a string into a type-matching value.
 * @tparam ToStr A converter that can convert a type-matching value into a string.
 */
template <typename T, typename FromStr = VarConverter<std::string, T>,
          typename ToStr = VarConverter<T, std::string>>
requires std::is_invocable_r_v<T, FromStr, std::string> && std::
    is_nothrow_invocable_r_v<std::string, ToStr, T>
class Var : public VarBase {
public:
    using Ptr = std::shared_ptr<Var>;

    /**
     * @brief The listener for value change events.
     *
     * @param old_val The old value before changing.
     * @param new_val The new value after changing.
     */
    using OnChange = std::function<void(const T& old_val, const T& new_val)>;

    explicit Var(const std::string_view name, const T& default_val,
                 const std::string_view description = "") noexcept :
        VarBase {name, description}, val_ {default_val} {}

    Var(const Var&) = delete;

    Var(Var&&) = delete;

    Var& operator=(const Var&) = delete;

    Var& operator=(Var&&) = delete;

    std::string ToString() const noexcept override {
        return ToStr {}(GetValue());
    }

    void FromString(const std::string_view str) override {
        SetValue(FromStr {}(str));
    }

    T GetValue() const noexcept {
        const std::shared_lock locker {mtx_};
        return val_;
    }

    void SetValue(const T& val) noexcept {
        {
            const std::shared_lock locker {mtx_};
            if (val != val_) {
                std::ranges::for_each(
                    listeners_, [&val, this](const auto& listener) noexcept {
                        try {
                            listener.second(val_, val);
                        } catch (const std::exception& err) {
                            std::osyncstream {std::cerr} << err.what()
                                                         << std::endl;
                        }
                    });
            }
        }

        const std::unique_lock locker {mtx_};
        val_ = val;
    }

    //! Get a unique string representing the variable type.
    std::string_view TypeName() const noexcept {
        return typeid(T).name();
    }

    /**
     * @brief Remove a listener.
     *
     * @param key A unique key corresponding to the listener.
     */
    void RemoveListener(const std::uint64_t key) noexcept {
        const std::unique_lock locker {mtx_};
        listeners_.erase(key);
    }

    /**
     * @brief Add a listener for value change events.
     *
     * @param listener A listener.
     * @return A unique key corresponding to the listener.
     */
    std::uint64_t AddListener(OnChange listener) noexcept {
        const std::unique_lock locker {mtx_};
        static std::uint64_t key {0};
        listeners_[key] = std::move(listener);
        return key++;
    }

    //! Remove all listeners.
    void ClearListeners() noexcept {
        const std::unique_lock locker {mtx_};
        listeners_.clear();
    }

private:
    mutable std::shared_mutex mtx_;

    T val_;
    std::unordered_map<std::uint64_t, OnChange> listeners_;
};

//! The configuration.
class Config {
public:
    using Ptr = std::shared_ptr<Config>;

    explicit Config(std::string_view name) noexcept;

    Config(const Config&) = delete;

    Config(Config&&) = delete;

    Config& operator=(const Config&) = delete;

    Config& operator=(Config&&) = delete;

    std::string_view Name() const noexcept;

    //! Lookup a variable by its name, creating it if it does not exist.
    template <typename T>
    typename Var<T>::Ptr Lookup(const std::string_view name,
                                const T& default_val,
                                const std::string_view description = "") {
        if (const auto var {Lookup<T>(name)}; !var) {
            const auto new_var {
                std::make_shared<Var<T>>(name, default_val, description)};
            const std::unique_lock locker {mtx_};
            vars_.emplace(name, new_var);
            return new_var;
        } else {
            return var;
        }
    }

    /**
     * @brief Lookup a variable by its name.
     *
     * @tparam T A variable type.
     * @param name A variable name.
     * @return The found variable or @p nullptr.
     *
     * @exception std::invalid_argument The type of the found variable does not match @p T.
     */
    template <typename T>
    typename Var<T>::Ptr Lookup(const std::string_view name) const {
        if (const auto base {LookupBase(name)}; base) {
            if (const auto var {std::dynamic_pointer_cast<Var<T>>(base)}; var) {
                return var;
            } else {
                throw std::invalid_argument {
                    fmt::format("Mismatched type: '{}'", name)};
            }
        } else {
            return nullptr;
        }
    }

    //! Lookup the basic information about a variable.
    VarBase::Ptr LookupBase(std::string_view name) const noexcept;

    /**
     * @brief Set the values of existing variables from a @p YAML node.
     *
     * @warning
     * If a variable in the @p YAML node does not exist in the current configuration,
     * this method will not create it.
     */
    void LoadYaml(const YAML::Node& root);

    //! Visit all variables.
    void Visit(std::function<void(VarBase::Ptr)> visitor) const noexcept;

private:
    using VarMap = std::unordered_map<std::string, VarBase::Ptr>;

    mutable std::shared_mutex mtx_;
    std::string name_;
    VarMap vars_;
};

//! Get the root configuration.
Config::Ptr RootConfig() noexcept;

}  // namespace cfg

}  // namespace ws