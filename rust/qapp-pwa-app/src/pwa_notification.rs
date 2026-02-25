#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappShowNotification(
            app_name: &QString,
            icon_path: &QString,
            title: &QString,
            body: &QString,
        );
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(QString, app_name)]
        #[qproperty(QString, icon_path)]
        type PwaNotificationHandler = super::PwaNotificationHandlerRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn show(self: Pin<&mut PwaNotificationHandler>, title: &QString, body: &QString);
    }
}

use core::pin::Pin;
use cxx_qt_lib::QString;

#[derive(Default)]
pub struct PwaNotificationHandlerRust {
    app_name: QString,
    icon_path: QString,
}

impl qobject::PwaNotificationHandler {
    pub fn show(self: Pin<&mut Self>, title: &QString, body: &QString) {
        qobject::qappShowNotification(self.app_name(), self.icon_path(), title, body);
    }
}
