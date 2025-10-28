#pragma once

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

    constexpr SettingsError(SettingsError&& that) = delete;
    constexpr SettingsError& operator=(SettingsError&& that) = delete;

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
    [[nodiscard]] constexpr auto const& message() const noexcept { return reason_; }

private:
    Type reason_;
};
