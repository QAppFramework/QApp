#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappSetBadge(desktop_file_id: &QString, count: i32);
        fn qappOpenUrl(url: &QString);
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(QString, desktop_file_id)]
        type PwaOsIntegration = super::PwaOsIntegrationRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn set_badge(self: Pin<&mut PwaOsIntegration>, count: i32);

        #[qinvokable]
        fn share(
            self: Pin<&mut PwaOsIntegration>,
            title: &QString,
            text: &QString,
            url: &QString,
        );

        #[qinvokable]
        fn polyfill_script(self: &PwaOsIntegration) -> QString;
    }
}

use core::pin::Pin;
use cxx_qt_lib::QString;
use std::fmt::Write;

#[derive(Default)]
pub struct PwaOsIntegrationRust {
    desktop_file_id: QString,
}

impl qobject::PwaOsIntegration {
    pub fn set_badge(self: Pin<&mut Self>, count: i32) {
        qobject::qappSetBadge(self.desktop_file_id(), count);
    }

    pub fn share(self: Pin<&mut Self>, title: &QString, text: &QString, url: &QString) {
        // Compose mailto: URL with shared content
        let title_str = title.to_string();
        let text_str = text.to_string();
        let url_str = url.to_string();

        let mut params = Vec::new();
        if !title_str.is_empty() {
            params.push(format!("subject={}", percent_encode(&title_str)));
        }

        let mut body = String::new();
        if !text_str.is_empty() {
            body = text_str;
        }
        if !url_str.is_empty() {
            if !body.is_empty() {
                body.push_str("\n\n");
            }
            body.push_str(&url_str);
        }
        if !body.is_empty() {
            params.push(format!("body={}", percent_encode(&body)));
        }

        let mailto = if params.is_empty() {
            "mailto:".to_string()
        } else {
            format!("mailto:?{}", params.join("&"))
        };

        qobject::qappOpenUrl(&QString::from(mailto.as_str()));
    }

    pub fn polyfill_script(&self) -> QString {
        QString::from(concat!(
            "(function(){",
            "if(!navigator.setAppBadge){",
            "navigator.setAppBadge=function(c){",
            "console.info('__QAPP_CMD:badge:'+(c||0));return Promise.resolve();};",
            "}",
            "if(!navigator.clearAppBadge){",
            "navigator.clearAppBadge=function(){",
            "console.info('__QAPP_CMD:badge:0');return Promise.resolve();};",
            "}",
            "if(!navigator.share){",
            "navigator.canShare=function(){return true;};",
            "navigator.share=function(d){",
            "console.info('__QAPP_CMD:share:'+JSON.stringify(d||{}));return Promise.resolve();};",
            "}",
            "})();"
        ))
    }
}

fn percent_encode(s: &str) -> String {
    let mut result = String::new();
    for b in s.bytes() {
        match b {
            b'A'..=b'Z' | b'a'..=b'z' | b'0'..=b'9' | b'-' | b'_' | b'.' | b'~' => {
                result.push(b as char);
            }
            b' ' => result.push_str("%20"),
            _ => {
                let _ = write!(result, "%{:02X}", b);
            }
        }
    }
    result
}
