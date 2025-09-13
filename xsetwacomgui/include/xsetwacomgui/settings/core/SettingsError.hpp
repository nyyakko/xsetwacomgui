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
        OUTDATED_SCHEMA
    };

public:
    using message_t = Type;

    constexpr explicit SettingsError(Type reason) : reason_m { reason } {}

    constexpr  SettingsError() noexcept = default;
    constexpr ~SettingsError() noexcept = default;

    constexpr SettingsError(SettingsError const& error) : reason_m { error.reason_m } {}
    constexpr SettingsError(SettingsError&& error) noexcept : reason_m { std::move(error.reason_m) } {}

    constexpr SettingsError& operator=(SettingsError&& error) noexcept
    {
        reason_m = std::move(error.reason_m);
        return *this;
    }

    constexpr SettingsError& operator=(SettingsError const& error)
    {
        reason_m = error.reason_m;
        return *this;
    }

    [[nodiscard]] constexpr auto const& message() const noexcept { return reason_m; }

private:
    Type reason_m;
};
