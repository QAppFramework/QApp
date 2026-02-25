#[cxx_qt::bridge]
pub mod qobject {
    unsafe extern "C++" {
        include!("cxx-qt-lib/qstring.h");
        type QString = cxx_qt_lib::QString;
    }

    unsafe extern "C++" {
        include!("qt-helpers.h");
        fn qappWebFetch(url: &QString) -> QString;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qobject]
        #[qml_element]
        #[qproperty(bool, busy)]
        #[qproperty(QString, level)]
        #[qproperty(QString, name)]
        #[qproperty(QString, icon_url)]
        #[qproperty(QString, display_mode)]
        #[qproperty(QString, theme_color)]
        #[qproperty(QString, background_color)]
        #[qproperty(QString, start_url)]
        #[qproperty(QString, scope)]
        #[qproperty(QString, final_url)]
        #[qproperty(QString, status_message)]
        #[qproperty(QString, error_message)]
        #[qproperty(QString, classify_result_json)]
        type SiteClassifier = super::SiteClassifierRust;
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qsignal]
        fn result_changed(self: Pin<&mut SiteClassifier>);
    }

    #[auto_cxx_name]
    extern "RustQt" {
        #[qinvokable]
        fn classify(self: Pin<&mut SiteClassifier>, url: &QString);
    }

    impl cxx_qt::Threading for SiteClassifier {}
}

use core::pin::Pin;
use cxx_qt::Threading;
use cxx_qt_lib::QString;
use std::sync::mpsc;

use crate::classify_pipeline;

#[derive(Default)]
pub struct SiteClassifierRust {
    busy: bool,
    level: QString,
    name: QString,
    icon_url: QString,
    display_mode: QString,
    theme_color: QString,
    background_color: QString,
    start_url: QString,
    scope: QString,
    final_url: QString,
    status_message: QString,
    error_message: QString,
    classify_result_json: QString,
}

/// Helper to apply a ClassifyResult to the QObject properties.
fn apply_result(
    qobj: &mut Pin<&mut qobject::SiteClassifier>,
    r: &classify_pipeline::ClassifyResult,
) {
    let json_str = serde_json::to_string(&r.to_json()).unwrap_or_default();

    qobj.as_mut().set_level(QString::from(&r.level));
    qobj.as_mut().set_name(QString::from(&r.name));
    qobj.as_mut().set_icon_url(QString::from(&r.icon_url));
    qobj.as_mut().set_display_mode(QString::from(&r.display_mode));
    qobj.as_mut().set_theme_color(QString::from(&r.theme_color));
    qobj.as_mut()
        .set_background_color(QString::from(&r.background_color));
    qobj.as_mut().set_start_url(QString::from(&r.start_url));
    qobj.as_mut().set_scope(QString::from(&r.scope));
    qobj.as_mut().set_final_url(QString::from(&r.final_url));
    qobj.as_mut()
        .set_status_message(QString::from("Classification complete"));
    qobj.as_mut()
        .set_classify_result_json(QString::from(json_str.as_str()));
    qobj.as_mut().set_busy(false);
    qobj.as_mut().result_changed();
}

impl qobject::SiteClassifier {
    pub fn classify(mut self: Pin<&mut Self>, url: &QString) {
        let url_str = url.to_string();

        // Reset state
        self.as_mut().set_busy(true);
        self.as_mut().set_error_message(QString::from(""));
        self.as_mut().set_level(QString::from(""));
        self.as_mut().set_name(QString::from(""));
        self.as_mut().set_icon_url(QString::from(""));
        self.as_mut()
            .set_status_message(QString::from("Fetching page..."));

        // Run classification in a background thread
        let qt_thread = self.qt_thread();
        std::thread::spawn(move || {
            let result = classify_pipeline::classify(&url_str);

            match result {
                Ok(r) => {
                    qt_thread
                        .queue(move |mut qobj| {
                            apply_result(&mut qobj, &r);
                        })
                        .unwrap();
                }
                Err(_reqwest_err) => {
                    // Reqwest failed — try WebEngine fallback on Qt main thread
                    let (tx, rx) = mpsc::channel::<String>();
                    let url_for_webengine = url_str.clone();

                    qt_thread
                        .queue(move |mut qobj| {
                            qobj.as_mut()
                                .set_status_message(QString::from("Trying WebEngine fallback..."));
                            let q_url = QString::from(url_for_webengine.as_str());
                            let json = qobject::qappWebFetch(&q_url);
                            tx.send(json.to_string()).ok();
                        })
                        .unwrap();

                    // Block worker thread until WebEngine result arrives
                    let webengine_json = match rx.recv() {
                        Ok(j) => j,
                        Err(_) => {
                            let qt_thread2 = qt_thread.clone();
                            qt_thread2
                                .queue(move |mut qobj| {
                                    qobj.as_mut().set_error_message(QString::from(
                                        "WebEngine fallback: channel closed",
                                    ));
                                    qobj.as_mut().set_status_message(QString::from(""));
                                    qobj.as_mut().set_busy(false);
                                })
                                .unwrap();
                            return;
                        }
                    };

                    match classify_pipeline::classify_from_webengine_data(&webengine_json) {
                        Ok(r) => {
                            qt_thread
                                .queue(move |mut qobj| {
                                    apply_result(&mut qobj, &r);
                                })
                                .unwrap();
                        }
                        Err(e) => {
                            qt_thread
                                .queue(move |mut qobj| {
                                    qobj.as_mut()
                                        .set_error_message(QString::from(e.as_str()));
                                    qobj.as_mut().set_status_message(QString::from(""));
                                    qobj.as_mut().set_busy(false);
                                })
                                .unwrap();
                        }
                    }
                }
            }
        });
    }
}
