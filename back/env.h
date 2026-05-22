#pragma once

#include "servercommon.h"

namespace env
{
    static const std::string website_name{ sc::get_env("WEBSITE_NAME", "Simple Video CMS") };
    static const std::string icon_path{ sc::get_env("ICON_PATH", {}) }; // bundled from front/assets
    static const std::string back_host{ sc::get_env("BACK_HOST", "0.0.0.0") };
    static const std::string back_port{ sc::get_env("BACK_PORT", "8080") };
    static const std::string super_admin_username{ sc::get_env("SUPER_ADMIN_USERNAME", "admin") };
}
