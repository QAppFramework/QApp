#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappAppDataLocation() -> QString;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(bool, busy)]
        #[qproperty(QString, status_message)]
        #[qproperty(bool, metadata_loaded)]
        #[qproperty(QString, display_mode)]
        #[qproperty(QString, theme_color)]
        #[qproperty(QString, metadata_start_url)]
        #[qproperty(QString, scope)]
        #[qproperty(QString, icon_path)]
        type WrapperHelper = super::WrapperHelperRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn load_metadata(self: Pin<&mut WrapperHelper>, app_id: &QString);

        #[qinvokable]
        fn save_as_app(self: Pin<&mut WrapperHelper>, url: &QString, name: &QString);

        #[qinvokable]
        fn clear_app_data_and_restart(self: Pin<&mut WrapperHelper>, app_id: &QString);
    }
}

use core::pin::Pin;
use cxx_qt_lib::QString;
use qapp_common::{config_reader, xdg_paths};

#[derive(Default)]
pub struct WrapperHelperRust {
    busy: bool,
    status_message: QString,
    metadata_loaded: bool,
    display_mode: QString,
    theme_color: QString,
    metadata_start_url: QString,
    scope: QString,
    icon_path: QString,
}

impl qobject::WrapperHelper {
    pub fn load_metadata(mut self: Pin<&mut Self>, app_id: &QString) {
        let app_id_str = app_id.to_string();
        if app_id_str.is_empty() {
            return;
        }

        let paths = match xdg_paths::resolve_app_paths(&app_id_str, "") {
            Ok(p) => p,
            Err(_) => {
                self.as_mut().set_metadata_loaded(true);
                return;
            }
        };

        let obj = match config_reader::read_json(&paths.metadata_file) {
            Ok(o) => o,
            Err(_) => {
                self.as_mut().set_metadata_loaded(true);
                return;
            }
        };

        if let Some(v) = obj.get("displayMode").and_then(|v| v.as_str()) {
            self.as_mut().set_display_mode(QString::from(v));
        }
        if let Some(v) = obj.get("themeColor").and_then(|v| v.as_str()) {
            self.as_mut().set_theme_color(QString::from(v));
        }
        if let Some(v) = obj.get("iconPath").and_then(|v| v.as_str()) {
            self.as_mut().set_icon_path(QString::from(v));
        }

        // Resolve scope
        let scope_str = obj
            .get("scope")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string();
        if !scope_str.is_empty() {
            let base_url = obj
                .get("url")
                .and_then(|v| v.as_str())
                .unwrap_or("");
            if !base_url.is_empty() {
                if let Ok(base) = url::Url::parse(base_url) {
                    if let Ok(resolved) = base.join(&scope_str) {
                        self.as_mut().set_scope(QString::from(resolved.as_str()));
                    }
                }
            } else {
                self.as_mut().set_scope(QString::from(scope_str.as_str()));
            }
        }

        // Resolve startUrl
        let start_url = obj
            .get("startUrl")
            .and_then(|v| v.as_str())
            .unwrap_or("");
        if !start_url.is_empty() {
            self.as_mut()
                .set_metadata_start_url(QString::from(start_url));
        }

        self.as_mut().set_metadata_loaded(true);
    }

    pub fn save_as_app(mut self: Pin<&mut Self>, url: &QString, name: &QString) {
        if *self.as_ref().busy() {
            return;
        }

        let url_str = url.to_string();
        let name_str = name.to_string();

        // Find installer binary (sibling to current binary)
        let exe = std::env::current_exe().unwrap_or_default();
        let dir = exe.parent().unwrap_or(std::path::Path::new("."));
        let installer_path = dir.join("qapp-installer");

        let mut args = vec![url_str];
        if !name_str.is_empty() {
            args.push("--name".into());
            args.push(name_str);
        }

        let launched = std::process::Command::new(&installer_path)
            .args(&args)
            .spawn()
            .is_ok();

        if launched {
            self.as_mut()
                .set_status_message(QString::from("Launching installer..."));
        } else {
            self.as_mut()
                .set_status_message(QString::from("Failed to launch installer"));
        }
    }

    pub fn clear_app_data_and_restart(self: Pin<&mut Self>, app_id: &QString) {
        let app_id_str = app_id.to_string();
        if app_id_str.is_empty() {
            return;
        }

        // Use QStandardPaths::AppDataLocation (respects per-app applicationName)
        let app_data = qobject::qappAppDataLocation().to_string();
        let data_path = std::path::PathBuf::from(&app_data)
            .join("QtWebEngine")
            .join(&app_id_str);

        // Relaunch args: same binary + same arguments
        let exe = std::env::current_exe().unwrap_or_default();
        let args: Vec<String> = std::env::args().skip(1).collect();

        // Spawn a detached shell that waits for us to exit, deletes data, relaunches
        let data_path_str = data_path.to_string_lossy();
        let exe_str = exe.to_string_lossy();
        let args_str = args
            .iter()
            .map(|a| format!("'{}'", a.replace('\'', "'\\''")))
            .collect::<Vec<_>>()
            .join(" ");

        let cmd = format!(
            "sleep 0.5 && rm -rf '{}' && exec '{}' {}",
            data_path_str, exe_str, args_str
        );

        let _ = std::process::Command::new("bash")
            .args(["-c", &cmd])
            .spawn();
        std::process::exit(0);
    }
}
