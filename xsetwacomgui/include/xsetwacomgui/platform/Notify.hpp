#pragma once

#include <chrono>
#include <liberror/Result.hpp>
#include <string_view>

liberror::Result<void> notify_send(std::string_view title, std::string_view message, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));
