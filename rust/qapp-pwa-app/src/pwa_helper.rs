#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappInitNetworkMonitoring() -> bool;
        fn qappIsNetworkOnline() -> bool;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(bool, metadata_loaded)]
        #[qproperty(QString, display_mode)]
        #[qproperty(QString, theme_color)]
        #[qproperty(QString, background_color)]
        #[qproperty(QString, icon_path)]
        #[qproperty(QString, start_url)]
        #[qproperty(QString, scope)]
        #[qproperty(QString, app_name)]
        #[qproperty(bool, is_online)]
        type PwaHelper = super::PwaHelperRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qsignal]
        fn metadata_changed(self: Pin<&mut PwaHelper>);

        #[qsignal]
        fn online_changed(self: Pin<&mut PwaHelper>);
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn load_metadata(self: Pin<&mut PwaHelper>, app_id: &QString);
    }

    impl cxx_qt::Threading for PwaHelper {}
    impl cxx_qt::Initialize for PwaHelper {}
}

use core::pin::Pin;
use cxx_qt::Threading;
use cxx_qt_lib::QString;
use qapp_common::{config_reader, xdg_paths};

#[derive(Default)]
pub struct PwaHelperRust {
    metadata_loaded: bool,
    display_mode: QString,
    theme_color: QString,
    background_color: QString,
    icon_path: QString,
    start_url: QString,
    scope: QString,
    app_name: QString,
    is_online: bool,
}

const SUPPORTED_DISPLAY_MODES: &[&str] = &["fullscreen", "standalone", "minimal-ui", "browser"];

impl cxx_qt::Initialize for qobject::PwaHelper {
    fn initialize(mut self: Pin<&mut Self>) {
        // Init QNetworkInformation backend (signal updates cached bool in C++)
        qobject::qappInitNetworkMonitoring();
        self.as_mut()
            .set_is_online(qobject::qappIsNetworkOnline());

        // Poll the cached bool to propagate changes to QML
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || loop {
            let online = qobject::qappIsNetworkOnline();
            qt_thread
                .queue(move |mut qobj| {
                    if *qobj.as_ref().is_online() != online {
                        qobj.as_mut().set_is_online(online);
                        qobj.as_mut().online_changed();
                    }
                })
                .ok();
            std::thread::sleep(std::time::Duration::from_secs(1));
        });
    }
}

impl qobject::PwaHelper {
    pub fn load_metadata(mut self: Pin<&mut Self>, app_id: &QString) {
        let app_id_str = app_id.to_string();
        if app_id_str.is_empty() {
            return;
        }

        let paths = match xdg_paths::resolve_app_paths(&app_id_str, "") {
            Ok(p) => p,
            Err(_) => {
                self.as_mut().set_metadata_loaded(true);
                self.as_mut().metadata_changed();
                return;
            }
        };

        let obj = match config_reader::read_json(&paths.metadata_file) {
            Ok(o) => o,
            Err(_) => {
                self.as_mut().set_metadata_loaded(true);
                self.as_mut().metadata_changed();
                return;
            }
        };

        // Resolve display_override: first supported mode wins, else fall back to display
        let mut resolved_display = String::new();
        if let Some(overrides) = obj.get("displayOverride").and_then(|v| v.as_array()) {
            for v in overrides {
                if let Some(mode) = v.as_str() {
                    if SUPPORTED_DISPLAY_MODES.contains(&mode) {
                        resolved_display = mode.to_string();
                        break;
                    }
                }
            }
        }
        if resolved_display.is_empty() {
            resolved_display = obj
                .get("displayMode")
                .and_then(|v| v.as_str())
                .unwrap_or("standalone")
                .to_string();
        }
        self.as_mut()
            .set_display_mode(QString::from(resolved_display.as_str()));

        if let Some(v) = obj.get("themeColor").and_then(|v| v.as_str()) {
            self.as_mut().set_theme_color(QString::from(v));
        }
        if let Some(v) = obj.get("backgroundColor").and_then(|v| v.as_str()) {
            self.as_mut().set_background_color(QString::from(v));
        }
        if let Some(v) = obj.get("iconPath").and_then(|v| v.as_str()) {
            self.as_mut().set_icon_path(QString::from(v));
        }
        if let Some(v) = obj.get("name").and_then(|v| v.as_str()) {
            self.as_mut().set_app_name(QString::from(v));
        }

        // Resolve scope
        let scope_str = obj
            .get("scope")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string();
        if !scope_str.is_empty() {
            let base_url = obj.get("url").and_then(|v| v.as_str()).unwrap_or("");
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
            self.as_mut().set_start_url(QString::from(start_url));
        }

        self.as_mut().set_metadata_loaded(true);
        self.as_mut().metadata_changed();
    }
}
