//! CLI functions exposed to C++ via CXX extern "Rust".
//! Each function runs synchronously and returns a JSON string.

#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    extern "Rust" {
        fn cli_classify(url: &QString) -> QString;
        fn cli_install(url: &QString) -> QString;
        fn cli_uninstall(app_id: &QString) -> QString;
        fn cli_list() -> QString;
        fn cli_check_updates() -> QString;
    }
}

use cxx_qt_lib::QString;

use crate::app_manager;
use crate::classify_pipeline;

fn cli_classify(url: &QString) -> QString {
    let url_str = url.to_string();
    match classify_pipeline::classify(&url_str) {
        Ok(r) => {
            let json = serde_json::to_string_pretty(&r.to_json()).unwrap_or_default();
            QString::from(json.as_str())
        }
        Err(e) => {
            let err = serde_json::json!({"error": e});
            QString::from(err.to_string().as_str())
        }
    }
}

fn cli_install(url: &QString) -> QString {
    let url_str = url.to_string();
    match classify_pipeline::classify(&url_str) {
        Ok(result) => {
            let install = app_manager::do_install_sync(&result, "", "");
            match install.error {
                Some(e) => {
                    let err = serde_json::json!({"success": false, "error": e});
                    QString::from(err.to_string().as_str())
                }
                None => {
                    let ok = serde_json::json!({
                        "success": true,
                        "appId": install.app_id,
                        "appName": install.app_name,
                        "desktopPath": install.desktop_path,
                    });
                    QString::from(serde_json::to_string_pretty(&ok).unwrap_or_default().as_str())
                }
            }
        }
        Err(e) => {
            let err = serde_json::json!({"success": false, "error": format!("Classification failed: {}", e)});
            QString::from(err.to_string().as_str())
        }
    }
}

fn cli_uninstall(app_id: &QString) -> QString {
    let id = app_id.to_string();
    match app_manager::do_uninstall_sync(&id) {
        Ok(()) => {
            let ok = serde_json::json!({"success": true, "appId": id});
            QString::from(serde_json::to_string_pretty(&ok).unwrap_or_default().as_str())
        }
        Err(e) => {
            let err = serde_json::json!({"success": false, "error": e});
            QString::from(err.to_string().as_str())
        }
    }
}

fn cli_list() -> QString {
    let json = app_manager::list_apps_sync();
    QString::from(json.as_str())
}

fn cli_check_updates() -> QString {
    let json = app_manager::do_check_updates_sync();
    QString::from(json.as_str())
}
