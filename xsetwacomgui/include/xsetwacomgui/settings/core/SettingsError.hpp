#pragma once

#include <libenum/Enum.hpp>

class [[nodiscard]] SettingsError
{
public:
    ENUM_CLASS(Type,
        READ_FAILURE,
        WRITE_FAILURE,
        OUTDATED_SCHEMA
    )

public:
    using message_t = Type;

    constexpr explicit SettingsError(Type reason) : reason { reason } {}

    constexpr  SettingsError() noexcept = default;
    constexpr ~SettingsError() noexcept = default;

    constexpr SettingsError(SettingsError const& error) : reason { error.reason } {}
    constexpr SettingsError(SettingsError&& error) noexcept : reason { std::move(error.reason) } {}

    constexpr SettingsError& operator=(SettingsError&& error) noexcept
    {
        reason = std::move(error.reason);
        return *this;
    }

    constexpr SettingsError& operator=(SettingsError const& error)
    {
        reason = error.reason;
        return *this;
    }

    [[nodiscard]] constexpr auto const& message() const noexcept { return reason; }

private:
    Type reason;
};
