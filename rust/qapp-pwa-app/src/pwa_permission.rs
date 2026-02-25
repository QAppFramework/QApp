#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappReadSettingBool(key: &QString, default_value: bool) -> bool;
        fn qappWriteSettingBool(key: &QString, value: bool);
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        type PwaPermissionHandler = super::PwaPermissionHandlerRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn should_grant(self: &PwaPermissionHandler, permission_type: i32) -> bool;

        #[qinvokable]
        fn store_decision(
            self: Pin<&mut PwaPermissionHandler>,
            permission_type: i32,
            granted: bool,
        );
    }
}

use core::pin::Pin;
use cxx_qt_lib::QString;

#[derive(Default)]
pub struct PwaPermissionHandlerRust {}

fn permission_key(permission_type: i32) -> Option<&'static str> {
    match permission_type {
        1 => Some("geolocation"),
        2 => Some("mediaAudioCapture"),
        3 => Some("mediaVideoCapture"),
        4 => Some("mediaAudioVideo"),
        5 => Some("desktopVideoCapture"),
        6 => Some("desktopAudioVideo"),
        7 => Some("notifications"),
        8 => Some("clipboardReadWrite"),
        9 => Some("localFontsAccess"),
        _ => None,
    }
}

impl qobject::PwaPermissionHandler {
    pub fn should_grant(&self, permission_type: i32) -> bool {
        let key = match permission_key(permission_type) {
            Some(k) => k,
            None => return false,
        };

        // QSettings key format matches C++ version: "permissions/<name>"
        let settings_key = QString::from(format!("permissions/{}", key).as_str());
        // Default: true (installed PWAs get permissions auto-granted)
        qobject::qappReadSettingBool(&settings_key, true)
    }

    pub fn store_decision(self: Pin<&mut Self>, permission_type: i32, granted: bool) {
        let key = match permission_key(permission_type) {
            Some(k) => k,
            None => return,
        };

        let settings_key = QString::from(format!("permissions/{}", key).as_str());
        qobject::qappWriteSettingBool(&settings_key, granted);
    }
}
