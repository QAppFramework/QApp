#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(bool, busy)]
        #[qproperty(bool, installed)]
        #[qproperty(QString, app_id)]
        #[qproperty(QString, app_name)]
        #[qproperty(QString, desktop_path)]
        #[qproperty(QString, error_message)]
        #[qproperty(QString, installed_apps)]
        #[qproperty(QString, update_status)]
        #[qproperty(QString, app_updates)]
        #[qproperty(bool, checking_updates)]
        type AppInstaller = super::AppInstallerRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qsignal]
        fn result_changed(self: Pin<&mut AppInstaller>);
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn reset(self: Pin<&mut AppInstaller>);

        #[qinvokable]
        fn install(
            self: Pin<&mut AppInstaller>,
            url: &QString,
            custom_icon_path: &QString,
            existing_app_id: &QString,
        );

        #[qinvokable]
        fn install_from_data(
            self: Pin<&mut AppInstaller>,
            classify_result_json: &QString,
            custom_icon_path: &QString,
        );

        #[qinvokable]
        fn uninstall(self: Pin<&mut AppInstaller>, app_id: &QString);

        #[qinvokable]
        fn list_apps(self: Pin<&mut AppInstaller>);

        #[qinvokable]
        fn launch(self: Pin<&mut AppInstaller>, app_id: &QString, url: &QString);

        #[qinvokable]
        fn update_qapp(self: Pin<&mut AppInstaller>);

        #[qinvokable]
        fn check_updates(self: Pin<&mut AppInstaller>);
    }

    impl cxx_qt::Threading for AppInstaller {}
}

use core::pin::Pin;
use cxx_qt::Threading;
use cxx_qt_lib::QString;
use qapp_common::{
    app_metadata, config_reader, desktop_entry, icon_manager, url_helpers, xdg_paths,
};

use crate::classify_pipeline::{self, ClassifyResult};

#[derive(Default)]
pub struct AppInstallerRust {
    busy: bool,
    installed: bool,
    app_id: QString,
    app_name: QString,
    desktop_path: QString,
    error_message: QString,
    installed_apps: QString,
    update_status: QString,
    app_updates: QString,
    checking_updates: bool,
}

impl qobject::AppInstaller {
    pub fn reset(mut self: Pin<&mut Self>) {
        self.as_mut().set_installed(false);
        self.as_mut().set_app_id(QString::from(""));
        self.as_mut().set_app_name(QString::from(""));
        self.as_mut().set_desktop_path(QString::from(""));
        self.as_mut().set_error_message(QString::from(""));
        self.as_mut().result_changed();
    }

    pub fn install(
        mut self: Pin<&mut Self>,
        url: &QString,
        custom_icon_path: &QString,
        existing_app_id: &QString,
    ) {
        if *self.as_ref().busy() {
            return;
        }

        self.as_mut().set_busy(true);
        self.as_mut().set_error_message(QString::from(""));

        let url_str = url.to_string();
        let icon_path = custom_icon_path.to_string();
        let existing_id = existing_app_id.to_string();

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            match classify_pipeline::classify(&url_str) {
                Ok(result) => {
                    let install_result = do_install_sync(&result, &icon_path, &existing_id);
                    qt_thread
                        .queue(move |mut qobj| {
                            apply_install_result(&mut qobj, install_result);
                        })
                        .unwrap();
                }
                Err(e) => {
                    let err = format!("Classification failed: {}", e);
                    qt_thread
                        .queue(move |mut qobj| {
                            qobj.as_mut().set_error_message(QString::from(err.as_str()));
                            qobj.as_mut().set_busy(false);
                            qobj.as_mut().result_changed();
                        })
                        .unwrap();
                }
            }
        });
    }

    pub fn install_from_data(
        mut self: Pin<&mut Self>,
        classify_result_json: &QString,
        custom_icon_path: &QString,
    ) {
        if *self.as_ref().busy() {
            return;
        }

        self.as_mut().set_busy(true);
        self.as_mut().set_error_message(QString::from(""));

        let json_str = classify_result_json.to_string();
        let icon_path = custom_icon_path.to_string();

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let result = parse_classify_json_and_install(&json_str, &icon_path);
            qt_thread
                .queue(move |mut qobj| {
                    apply_install_result(&mut qobj, result);
                })
                .unwrap();
        });
    }

    pub fn uninstall(mut self: Pin<&mut Self>, app_id: &QString) {
        if *self.as_ref().busy() {
            return;
        }

        let id = app_id.to_string();

        self.as_mut().set_busy(true);
        self.as_mut().set_error_message(QString::from(""));

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let result = do_uninstall_sync(&id);
            qt_thread
                .queue(move |mut qobj| {
                    match result {
                        Ok(()) => {
                            qobj.as_mut().set_installed(false);
                            qobj.as_mut().set_app_id(QString::from(""));
                            qobj.as_mut().set_app_name(QString::from(""));
                            qobj.as_mut().set_desktop_path(QString::from(""));
                            qobj.as_mut().set_busy(false);
                            qobj.as_mut().result_changed();
                            // Refresh list
                            let apps_json = list_apps_sync();
                            qobj.as_mut()
                                .set_installed_apps(QString::from(apps_json.as_str()));
                        }
                        Err(e) => {
                            qobj.as_mut().set_error_message(QString::from(e.as_str()));
                            qobj.as_mut().set_busy(false);
                            qobj.as_mut().result_changed();
                        }
                    }
                })
                .unwrap();
        });
    }

    pub fn list_apps(mut self: Pin<&mut Self>) {
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let apps_json = list_apps_sync();
            qt_thread
                .queue(move |mut qobj| {
                    qobj.as_mut()
                        .set_installed_apps(QString::from(apps_json.as_str()));
                })
                .unwrap();
        });
    }

    pub fn launch(self: Pin<&mut Self>, app_id: &QString, url: &QString) {
        let id = app_id.to_string();
        let url_str = url.to_string();

        // Read wrapperPath from metadata
        let wrapper = get_wrapper_path(&id);
        let args = vec![id, url_str];

        std::process::Command::new(&wrapper)
            .args(&args)
            .spawn()
            .ok();
    }

    pub fn update_qapp(mut self: Pin<&mut Self>) {
        if *self.as_ref().busy() {
            return;
        }

        self.as_mut().set_busy(true);
        self.as_mut()
            .set_update_status(QString::from("Downloading latest version..."));

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let result = do_update_qapp_sync();
            qt_thread
                .queue(move |mut qobj| {
                    qobj.as_mut()
                        .set_update_status(QString::from(result.as_str()));
                    qobj.as_mut().set_busy(false);
                })
                .unwrap();
        });
    }

    pub fn check_updates(mut self: Pin<&mut Self>) {
        if *self.as_ref().checking_updates() {
            return;
        }

        self.as_mut().set_checking_updates(true);

        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let results = do_check_updates_sync();
            qt_thread
                .queue(move |mut qobj| {
                    qobj.as_mut()
                        .set_app_updates(QString::from(results.as_str()));
                    qobj.as_mut().set_checking_updates(false);
                })
                .unwrap();
        });
    }
}

// --- Pure Rust sync functions (run on worker threads) ---

pub struct InstallResult {
    pub app_id: String,
    pub app_name: String,
    pub desktop_path: String,
    pub error: Option<String>,
}

fn apply_install_result(qobj: &mut Pin<&mut qobject::AppInstaller>, result: InstallResult) {
    if let Some(err) = result.error {
        qobj.as_mut().set_error_message(QString::from(err.as_str()));
        qobj.as_mut().set_busy(false);
        qobj.as_mut().result_changed();
    } else {
        qobj.as_mut().set_app_id(QString::from(result.app_id.as_str()));
        qobj.as_mut()
            .set_app_name(QString::from(result.app_name.as_str()));
        qobj.as_mut()
            .set_desktop_path(QString::from(result.desktop_path.as_str()));
        qobj.as_mut().set_installed(true);
        qobj.as_mut().set_busy(false);
        qobj.as_mut().result_changed();

        // Refresh list
        let apps_json = list_apps_sync();
        qobj.as_mut()
            .set_installed_apps(QString::from(apps_json.as_str()));
    }
}

fn parse_classify_json_and_install(json_str: &str, custom_icon_path: &str) -> InstallResult {
    let json: serde_json::Value = match serde_json::from_str(json_str) {
        Ok(v) => v,
        Err(_) => {
            return InstallResult {
                error: Some("Invalid classify result JSON".into()),
                ..Default::default()
            }
        }
    };

    let classification = &json["classification"];
    let metadata = &json["metadata"];

    let result = ClassifyResult {
        level: classification["level"].as_str().unwrap_or("").to_string(),
        has_manifest: classification["hasManifest"].as_bool().unwrap_or(false),
        has_service_worker: classification["hasServiceWorker"].as_bool().unwrap_or(false),
        name: metadata["name"].as_str().unwrap_or("").to_string(),
        icon_url: metadata["iconUrl"].as_str().unwrap_or("").to_string(),
        display_mode: metadata["displayMode"].as_str().unwrap_or("").to_string(),
        theme_color: metadata["themeColor"].as_str().unwrap_or("").to_string(),
        background_color: metadata["backgroundColor"].as_str().unwrap_or("").to_string(),
        start_url: metadata["startUrl"].as_str().unwrap_or("").to_string(),
        scope: metadata["scope"].as_str().unwrap_or("").to_string(),
        final_url: json["finalUrl"].as_str().unwrap_or("").to_string(),
        manifest_id: metadata["manifestId"].as_str().unwrap_or("").to_string(),
        ..Default::default()
    };

    if result.final_url.is_empty() {
        return InstallResult {
            error: Some("Missing finalUrl in classify result".into()),
            ..Default::default()
        };
    }

    do_install_sync(&result, custom_icon_path, "")
}

pub fn do_install_sync(
    result: &ClassifyResult,
    custom_icon_path: &str,
    existing_app_id: &str,
) -> InstallResult {
    // Generate app ID
    let app_id = if !existing_app_id.is_empty() {
        existing_app_id.to_string()
    } else {
        match url_helpers::generate_app_id(&result.final_url) {
            Ok(id) => id,
            Err(e) => {
                return InstallResult {
                    error: Some(format!("App ID: {}", e)),
                    ..Default::default()
                }
            }
        }
    };

    // Determine launch URL
    let install_url = if result.start_url.is_empty() {
        &result.final_url
    } else {
        &result.start_url
    };

    // Wrapper binary
    let binary_name = if result.level == "PWAPP" {
        "qapp-pwa-app"
    } else {
        "qapp-ws-wrapper"
    };
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_string_lossy().to_string()))
        .unwrap_or_else(|| "/usr/bin".to_string());
    let wrapper_path = format!("{}/{}", exe_dir, binary_name);

    // Icon format
    let icon_source = if custom_icon_path.is_empty() {
        &result.icon_url
    } else {
        custom_icon_path
    };
    let icon_ext = icon_manager::detect_format(icon_source);

    // Resolve XDG paths
    let paths = match xdg_paths::resolve_app_paths(&app_id, icon_ext) {
        Ok(p) => p,
        Err(e) => {
            return InstallResult {
                error: Some(format!("Paths: {}", e)),
                ..Default::default()
            }
        }
    };

    // Handle icon
    if !custom_icon_path.is_empty() {
        std::fs::create_dir_all(&paths.icons_dir).ok();
        let _ = std::fs::remove_file(&paths.icon_file);
        if std::fs::copy(custom_icon_path, &paths.icon_file).is_err() {
            return InstallResult {
                error: Some(format!("Icon: Failed to copy {}", custom_icon_path)),
                ..Default::default()
            };
        }
    } else if result.icon_url.starts_with("data:") {
        if let Err(e) = icon_manager::save_data_uri(&result.icon_url, &paths.icon_file) {
            return InstallResult {
                error: Some(format!("Icon: {}", e)),
                ..Default::default()
            };
        }
    } else if !result.icon_url.is_empty() {
        // Download icon via HTTP
        if let Err(_) = download_icon(&result.icon_url, &paths.icon_file) {
            // Fallback to letter icon
            let letter_icon = icon_manager::generate_letter_icon(&result.name);
            if let Ok(svg_paths) = xdg_paths::resolve_app_paths(&app_id, "svg") {
                icon_manager::save_data_uri(&letter_icon, &svg_paths.icon_file).ok();
                // Use SVG paths for the rest
                return finish_install(result, &app_id, install_url, &wrapper_path, &svg_paths);
            }
        }
    } else {
        // No icon — generate letter icon
        let letter_icon = icon_manager::generate_letter_icon(&result.name);
        if let Ok(svg_paths) = xdg_paths::resolve_app_paths(&app_id, "svg") {
            icon_manager::save_data_uri(&letter_icon, &svg_paths.icon_file).ok();
            return finish_install(result, &app_id, install_url, &wrapper_path, &svg_paths);
        }
    }

    finish_install(result, &app_id, install_url, &wrapper_path, &paths)
}

fn download_icon(url: &str, dest: &str) -> Result<(), String> {
    let client = reqwest::blocking::Client::builder()
        .timeout(std::time::Duration::from_secs(10))
        .build()
        .map_err(|e| e.to_string())?;

    let resp = client.get(url).send().map_err(|e| e.to_string())?;
    if !resp.status().is_success() {
        return Err(format!("HTTP {}", resp.status()));
    }

    let bytes = resp.bytes().map_err(|e| e.to_string())?;
    let path = std::path::Path::new(dest);
    if let Some(parent) = path.parent() {
        std::fs::create_dir_all(parent).ok();
    }
    std::fs::write(dest, &bytes).map_err(|e| e.to_string())
}

fn finish_install(
    result: &ClassifyResult,
    app_id: &str,
    install_url: &str,
    wrapper_path: &str,
    paths: &xdg_paths::AppPaths,
) -> InstallResult {
    // Generate .desktop entry
    let exec_str = format!("{} {} {}", wrapper_path, app_id, install_url);
    let mut entry_input = desktop_entry::DesktopEntryInput {
        name: result.name.clone(),
        exec: exec_str,
        icon: paths.icon_file.clone(),
        ..Default::default()
    };

    // MIME types
    if !result.mime_types.is_empty() {
        entry_input.mime_types = format!("{};", result.mime_types.join(";"));
    }

    // Desktop actions from shortcuts
    for sc in &result.shortcuts {
        if let (Some(name), Some(url)) = (sc.get("name"), sc.get("url")) {
            let name_str = name.as_str().unwrap_or("");
            let url_str = url.as_str().unwrap_or("");
            if name_str.is_empty() || url_str.is_empty() {
                continue;
            }
            let action_id: String = name_str.chars().filter(|c| c.is_alphanumeric()).collect();
            entry_input.actions.push(desktop_entry::DesktopAction {
                id: action_id,
                name: name_str.to_string(),
                exec: format!("{} {} {}", wrapper_path, app_id, url_str),
                icon: String::new(),
            });
        }
    }

    let entry_content = match desktop_entry::generate(&entry_input) {
        Ok(c) => c,
        Err(e) => {
            return InstallResult {
                error: Some(format!("Desktop entry: {}", e)),
                ..Default::default()
            }
        }
    };

    // Write .desktop file
    std::fs::create_dir_all(&paths.applications_dir).ok();
    if std::fs::write(&paths.desktop_file, &entry_content).is_err() {
        return InstallResult {
            error: Some(format!("Cannot write: {}", paths.desktop_file)),
            ..Default::default()
        };
    }

    // Build metadata
    let mut meta_input = app_metadata::AppMetadataData {
        app_id: app_id.to_string(),
        name: result.name.clone(),
        url: install_url.to_string(),
        level: result.level.clone(),
        icon_path: paths.icon_file.clone(),
        desktop_path: paths.desktop_file.clone(),
        wrapper_path: wrapper_path.to_string(),
        ..Default::default()
    };

    // Optional fields
    if !result.display_mode.is_empty() && result.display_mode != "browser" {
        meta_input.display_mode = result.display_mode.clone();
    }
    if !result.theme_color.is_empty() {
        meta_input.theme_color = result.theme_color.clone();
    }
    if !result.background_color.is_empty() {
        meta_input.background_color = result.background_color.clone();
    }
    if !result.start_url.is_empty() {
        meta_input.start_url = result.start_url.clone();
    }
    if !result.scope.is_empty() {
        meta_input.scope = result.scope.clone();
    }
    if !result.manifest_id.is_empty() {
        meta_input.manifest_id = result.manifest_id.clone();
    }
    meta_input.display_override = result.display_override.clone();
    meta_input.shortcuts = result.shortcuts.clone();
    meta_input.mime_types = result.mime_types.clone();
    meta_input.protocol_handlers = result.protocol_handlers.clone();

    let meta = match app_metadata::build(&meta_input) {
        Ok(m) => m,
        Err(e) => {
            return InstallResult {
                error: Some(format!("Metadata: {}", e)),
                ..Default::default()
            }
        }
    };

    let meta_json = match app_metadata::to_json(&meta) {
        Ok(j) => j,
        Err(e) => {
            return InstallResult {
                error: Some(format!("JSON: {}", e)),
                ..Default::default()
            }
        }
    };

    if let Err(e) = config_reader::write_json(&paths.metadata_file, &meta_json) {
        return InstallResult {
            error: Some(format!("Config: {}", e)),
            ..Default::default()
        };
    }

    InstallResult {
        app_id: app_id.to_string(),
        app_name: result.name.clone(),
        desktop_path: paths.desktop_file.clone(),
        error: None,
    }
}

pub fn do_uninstall_sync(app_id: &str) -> Result<(), String> {
    let paths = xdg_paths::resolve_app_paths(app_id, "png")?;

    let meta_json = config_reader::read_json(&paths.metadata_file)?;
    let meta = app_metadata::validate(&meta_json)?;

    // Delete files (best effort)
    std::fs::remove_file(&meta.desktop_path).ok();
    std::fs::remove_file(&meta.icon_path).ok();
    std::fs::remove_file(&paths.metadata_file).ok();

    Ok(())
}

pub fn list_apps_sync() -> String {
    let paths = match xdg_paths::resolve_app_paths("_", "png") {
        Ok(p) => p,
        Err(_) => return "[]".to_string(),
    };

    let apps_dir = std::path::Path::new(&paths.apps_dir);
    if !apps_dir.exists() {
        return "[]".to_string();
    }

    let mut apps: Vec<serde_json::Value> = Vec::new();

    if let Ok(entries) = std::fs::read_dir(apps_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|e| e.to_str()) != Some("json") {
                continue;
            }
            let path_str = path.to_string_lossy().to_string();
            if let Ok(json) = config_reader::read_json(&path_str) {
                if let Ok(meta) = app_metadata::validate(&json) {
                    if let Ok(obj) = app_metadata::to_json(&meta) {
                        apps.push(obj);
                    }
                }
            }
        }
    }

    // Sort by name (case-insensitive)
    apps.sort_by(|a, b| {
        let na = a["name"].as_str().unwrap_or("").to_lowercase();
        let nb = b["name"].as_str().unwrap_or("").to_lowercase();
        na.cmp(&nb)
    });

    serde_json::to_string(&apps).unwrap_or_else(|_| "[]".to_string())
}

fn get_wrapper_path(app_id: &str) -> String {
    if let Ok(paths) = xdg_paths::resolve_app_paths(app_id, "png") {
        if let Ok(json) = config_reader::read_json(&paths.metadata_file) {
            if let Some(wp) = json["wrapperPath"].as_str() {
                if !wp.is_empty() {
                    return wp.to_string();
                }
            }
        }
    }
    // Fallback
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_string_lossy().to_string()))
        .unwrap_or_else(|| "/usr/bin".to_string());
    format!("{}/qapp-ws-wrapper", exe_dir)
}

fn do_update_qapp_sync() -> String {
    let exe_dir = std::env::current_exe()
        .ok()
        .and_then(|p| p.parent().map(|d| d.to_string_lossy().to_string()))
        .unwrap_or_else(|| "/usr/bin".to_string());

    let candidates = [
        format!("{}/../share/qapp-framework/update.sh", exe_dir),
        format!("{}/../bin/update.sh", exe_dir),
    ];

    let script_path = candidates
        .iter()
        .find(|p| std::path::Path::new(p).exists())
        .cloned()
        .unwrap_or_default();

    if script_path.is_empty() {
        return "Update script not found".to_string();
    }

    let output = std::process::Command::new("bash")
        .args([&script_path])
        .output();

    match output {
        Ok(out) if out.status.success() => {
            "Updated! Restart QApp to use the new version.".to_string()
        }
        Ok(out) => {
            let stderr = String::from_utf8_lossy(&out.stderr);
            let last_line = stderr.lines().last().unwrap_or("Unknown error");
            format!("Update failed: {}", last_line)
        }
        Err(e) => format!("Failed to start update: {}", e),
    }
}

pub fn do_check_updates_sync() -> String {
    let paths = match xdg_paths::resolve_app_paths("_", "png") {
        Ok(p) => p,
        Err(_) => return "[]".to_string(),
    };

    let apps_dir = std::path::Path::new(&paths.apps_dir);
    if !apps_dir.exists() {
        return "[]".to_string();
    }

    let mut results: Vec<serde_json::Value> = Vec::new();

    if let Ok(entries) = std::fs::read_dir(apps_dir) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path.extension().and_then(|e| e.to_str()) != Some("json") {
                continue;
            }
            let path_str = path.to_string_lossy().to_string();
            if let Ok(json) = config_reader::read_json(&path_str) {
                if let Ok(meta) = app_metadata::validate(&json) {
                    let mut changes: Vec<String> = Vec::new();

                    // WS apps: only version check
                    if meta.level == "WS" {
                        results.push(serde_json::json!({
                            "appId": meta.app_id,
                            "name": meta.name,
                            "hasUpdate": !changes.is_empty(),
                            "changes": changes
                        }));
                        continue;
                    }

                    // WAPP/PWAPP: re-classify and compare
                    if let Ok(fresh) = classify_pipeline::classify(&meta.url) {
                        if !fresh.name.is_empty() && fresh.name != meta.name {
                            changes.push("name".into());
                        }
                        let stored_display = if meta.display_mode.is_empty() {
                            "browser"
                        } else {
                            &meta.display_mode
                        };
                        let fresh_display = if fresh.display_mode.is_empty() {
                            "browser"
                        } else {
                            &fresh.display_mode
                        };
                        if stored_display != fresh_display {
                            changes.push("displayMode".into());
                        }
                        if meta.theme_color != fresh.theme_color {
                            changes.push("themeColor".into());
                        }
                        if meta.start_url != fresh.start_url {
                            changes.push("startUrl".into());
                        }
                        if meta.scope != fresh.scope {
                            changes.push("scope".into());
                        }
                    }

                    results.push(serde_json::json!({
                        "appId": meta.app_id,
                        "name": meta.name,
                        "hasUpdate": !changes.is_empty(),
                        "changes": changes
                    }));
                }
            }
        }
    }

    serde_json::to_string(&results).unwrap_or_else(|_| "[]".to_string())
}

impl Default for InstallResult {
    fn default() -> Self {
        Self {
            app_id: String::new(),
            app_name: String::new(),
            desktop_path: String::new(),
            error: None,
        }
    }
}
