#pragma once

#include <magic_enum/magic_enum.hpp>

class [[nodiscard]] SettingsError
{
public:
    enum class Error
    {
        FILE_NOT_FOUND,
        OUTDATED_SCHEMA,
        READ_FAILURE,
        WRITE_FAILURE,
    };

    using enum Error;

public:
    using error_t = Error;

    constexpr explicit SettingsError(Error reason)
        : error_{reason}
    {}

    constexpr SettingsError()
        : error_{}
    {}

    constexpr SettingsError(SettingsError&& that)
        : error_{std::exchange(that.error_, {})}
    {}

    constexpr SettingsError& operator=(SettingsError&& that)
    {
        this->error_ = std::exchange(that.error_, {});
        return *this;
    }

    constexpr SettingsError(SettingsError const& that)
        : error_{that.error_}
    {}

    constexpr SettingsError& operator=(SettingsError const& that)
    {
        this->error_ = that.error_;
        return *this;
    }

    constexpr ~SettingsError() noexcept = default;

public:
    [[nodiscard]] constexpr auto message() const noexcept { return magic_enum::enum_name<Error>(error_); }

public:
    constexpr operator Error() const { return error_; }

private:
    Error error_;
};
