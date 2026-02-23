#ifndef QAPP_COMMON_HPP
#define QAPP_COMMON_HPP

/**
 * Root namespace for QAppFramework shared types.
 *
 * Domain hierarchy:
 *   qapp::              — common utilities (this module)
 *   qapp::installer::   — installer-specific logic
 *   qapp::ws::          — website wrapper logic
 *   qapp::pwa::         — PWA app logic
 */

#include "result.hpp"
#include "config-reader.hpp"
#include "xdg-paths.hpp"
#include "url-helpers.hpp"
#include "desktop-entry.hpp"
#include "app-metadata.hpp"
#include "html-parser.hpp"
#include "manifest-parser.hpp"
#include "icon-manager.hpp"

#endif // QAPP_COMMON_HPP
