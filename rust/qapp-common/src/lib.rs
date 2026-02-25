pub mod config_reader;
pub mod xdg_paths;
pub mod url_helpers;
pub mod desktop_entry;
pub mod app_metadata;
pub mod html_parser;
pub mod manifest_parser;
pub mod icon_manager;

/// Generic result type matching the C++ Result<T> pattern.
/// Returns Ok(T) on success, Err(String) on failure.
pub type Result<T> = std::result::Result<T, String>;
