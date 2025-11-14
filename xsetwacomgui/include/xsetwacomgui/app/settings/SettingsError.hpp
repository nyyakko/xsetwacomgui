#pragma once

#include <magic_enum/magic_enum.hpp>
class [[nodiscard]] SettingsError
{
public:
    enum class Type
    {
        FILE_NOT_FOUND,
        READ_FAILURE,
        WRITE_FAILURE,
        OUTDATED_SCHEMA,
        PROFILE_NOT_FOUND
    };

public:
    using message_t = Type;

    constexpr explicit SettingsError(Type reason)
        : reason_{reason}
    {}

    constexpr SettingsError()
        : reason_{}
    {}

    constexpr SettingsError(SettingsError&& that)
        : reason_{std::exchange(that.reason_, {})}
    {}

    constexpr SettingsError& operator=(SettingsError&& that)
    {
        this->reason_ = std::exchange(that.reason_, {});
        return *this;
    }

    constexpr SettingsError(SettingsError const& that)
        : reason_{that.reason_}
    {}

    constexpr SettingsError& operator=(SettingsError const& that)
    {
        this->reason_ = that.reason_;
        return *this;
    }

    constexpr ~SettingsError() noexcept = default;

public:
    [[nodiscard]] constexpr auto message() const noexcept { return magic_enum::enum_name<Type>(reason_); }

public:
    constexpr operator Type() const { return reason_; }

private:
    Type reason_;
};
