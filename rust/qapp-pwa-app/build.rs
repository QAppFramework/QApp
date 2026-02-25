use cxx_qt_build::{CxxQtBuilder, QmlModule};

fn main() {
    CxxQtBuilder::new()
        .qml_module(QmlModule {
            uri: "QApp.PWA",
            rust_files: &[
                "src/pwa_helper.rs",
                "src/pwa_permission.rs",
                "src/pwa_notification.rs",
                "src/pwa_os_integration.rs",
            ],
            qml_files: &[
                "qml/pwa-app.qml",
                "qml/PwaAppMenu.qml",
                "qml/PwaToolbar.qml",
                "qml/PwaSplashScreen.qml",
                "qml/PwaFullscreenHint.qml",
                "qml/PwaOfflineIndicator.qml",
            ],
            ..Default::default()
        })
        .cc_builder(|cc| {
            let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
            cc.include(format!("{}/../../cpp", dir));
        })
        .build();
}
