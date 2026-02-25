use cxx_qt_build::{CxxQtBuilder, QmlModule};

fn main() {
    CxxQtBuilder::new()
        .qml_module(QmlModule {
            uri: "QApp.WS",
            rust_files: &["src/wrapper_helper.rs"],
            qml_files: &[
                "qml/wrapper.qml",
                "qml/WrapperHeader.qml",
                "qml/WrapperTabView.qml",
                "qml/WrapperSettingsDialog.qml",
                "qml/WrapperSaveDialog.qml",
                "qml/WrapperTab.qml",
                "qml/WrapperFullscreenHint.qml",
            ],
            ..Default::default()
        })
        .cc_builder(|cc| {
            let dir = std::env::var("CARGO_MANIFEST_DIR").unwrap();
            cc.include(format!("{}/../../cpp", dir));
        })
        .build();
}
