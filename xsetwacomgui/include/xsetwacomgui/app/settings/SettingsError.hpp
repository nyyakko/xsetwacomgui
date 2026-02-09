#pragma once

#include <magic_enum/magic_enum.hpp>

#include <utility>

class [[nodiscard]] SettingsError
{
public:
    enum class Category
    {
        FILE_NOT_FOUND,
        READ_FAILURE,
        WRITE_FAILURE,
        OUTDATED_SCHEMA
    };

    using enum Category;

public:
    using error_t = Category;

    constexpr explicit SettingsError(Category category) : error_{category} {}

    constexpr SettingsError() : error_{} {}

    constexpr SettingsError(SettingsError const& that) : error_{that.error_} {}
    constexpr SettingsError& operator=(SettingsError const& that)
    {
        this->error_ = that.error_;
        return *this;
    }

    constexpr SettingsError(SettingsError&& that) : error_{std::exchange(that.error_, {})} {}
    constexpr SettingsError& operator=(SettingsError&& that)
    {
        this->error_ = std::exchange(that.error_, {});
        return *this;
    }

    constexpr ~SettingsError() noexcept = default;

    [[nodiscard]] constexpr auto message() const noexcept { return magic_enum::enum_name<Category>(error_); }

    constexpr operator Category() const { return error_; }

private:
    Category error_;
};
