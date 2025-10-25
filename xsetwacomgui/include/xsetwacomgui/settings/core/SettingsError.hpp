#pragma once

#include <string_view>
#include <utility>

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

    constexpr explicit SettingsError(Type reason) : reason_ { reason } {}

    constexpr  SettingsError() noexcept = default;
    constexpr ~SettingsError() noexcept = default;

    constexpr SettingsError(SettingsError const& error) : reason_ { error.reason_ } {}
    constexpr SettingsError(SettingsError&& error) noexcept : reason_ { std::move(error.reason_) } {}

    constexpr SettingsError& operator=(SettingsError&& error) noexcept
    {
        reason_ = std::move(error.reason_);
        return *this;
    }

    constexpr SettingsError& operator=(SettingsError const& error)
    {
        reason_ = error.reason_;
        return *this;
    }

    [[nodiscard]] constexpr auto const& message() const noexcept { return reason_; }

private:
    Type reason_;
};
