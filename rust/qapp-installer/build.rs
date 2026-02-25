use cxx_qt_build::{CxxQtBuilder, QmlModule};

fn main() {
    CxxQtBuilder::new()
        .qml_module(QmlModule {
            uri: "QApp.Installer",
            rust_files: &[
                "src/site_classifier.rs",
                "src/app_manager.rs",
                "src/cli.rs",
            ],
            qml_files: &[
                "qml/main.qml",
                "qml/InstallerInstallTab.qml",
                "qml/InstallerResultsPanel.qml",
                "qml/InstallerManageTab.qml",
                "qml/InstallerUpdateDialog.qml",
            ],
            ..Default::default()
        })
        .cc_builder(|cc| {
            let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
            cc.include(format!("{}/../../cpp", dir));
        })
        .qt_module("Network")
        .build();
}
