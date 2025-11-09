#include "platform/Notify.hpp"

#include "platform/Environment.hpp"

#include <filesystem>
#include <liberror/Try.hpp>
#include <libexec/Execute.hpp>

using namespace liberror;

std::filesystem::path APPLICATION_ICON_FILE = get_application_icon_path() / "64x64" / "apps" / NAME".png";

Result<void> notify_send(std::string_view title, std::string_view message, std::chrono::milliseconds timeout)
{
    TRY(libexec::execute("notify-send", {
        title.data(),
        message.data(),
        "--icon", APPLICATION_ICON_FILE,
        "--expire-time", std::to_string(timeout.count())
    }));
    return {};
}
