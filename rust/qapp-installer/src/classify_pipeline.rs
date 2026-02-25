//! Pure Rust classify pipeline — no CXX-Qt dependency.
//! Uses reqwest blocking for HTTP, qapp-common for parsing.

use qapp_common::{html_parser, icon_manager, manifest_parser};
use serde_json::Value;
use url::Url;

const SW_PATHS: &[&str] = &["/sw.js", "/service-worker.js", "/serviceworker.js"];
const FAVICON_PATHS: &[&str] = &["/favicon.ico", "/apple-touch-icon.png"];
const USER_AGENT: &str = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 \
    (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

/// Classification result matching the C++ ClassifyResult struct.
#[derive(Debug, Clone, Default)]
pub struct ClassifyResult {
    pub level: String,
    pub has_manifest: bool,
    pub has_service_worker: bool,
    pub name: String,
    pub icon_url: String,
    pub display_mode: String,
    pub theme_color: String,
    pub background_color: String,
    pub start_url: String,
    pub scope: String,
    pub final_url: String,
    pub manifest_id: String,
    pub display_override: Vec<String>,
    pub shortcuts: Vec<Value>,
    pub mime_types: Vec<String>,
    pub protocol_handlers: Vec<Value>,
}

impl ClassifyResult {
    pub fn to_json(&self) -> Value {
        let mut classification = serde_json::Map::new();
        classification.insert("level".into(), self.level.clone().into());
        classification.insert("hasManifest".into(), self.has_manifest.into());
        classification.insert("hasServiceWorker".into(), self.has_service_worker.into());

        let mut metadata = serde_json::Map::new();
        metadata.insert("name".into(), self.name.clone().into());
        metadata.insert("iconUrl".into(), self.icon_url.clone().into());
        metadata.insert("displayMode".into(), self.display_mode.clone().into());
        metadata.insert("themeColor".into(), self.theme_color.clone().into());
        metadata.insert("backgroundColor".into(), self.background_color.clone().into());
        metadata.insert("startUrl".into(), self.start_url.clone().into());
        metadata.insert("scope".into(), self.scope.clone().into());
        if !self.manifest_id.is_empty() {
            metadata.insert("manifestId".into(), self.manifest_id.clone().into());
        }
        if !self.display_override.is_empty() {
            metadata.insert("displayOverride".into(), self.display_override.clone().into());
        }
        if !self.shortcuts.is_empty() {
            metadata.insert("shortcuts".into(), self.shortcuts.clone().into());
        }
        if !self.mime_types.is_empty() {
            metadata.insert("mimeTypes".into(), self.mime_types.clone().into());
        }
        if !self.protocol_handlers.is_empty() {
            metadata.insert(
                "protocolHandlers".into(),
                self.protocol_handlers.clone().into(),
            );
        }

        let mut root = serde_json::Map::new();
        root.insert("classification".into(), Value::Object(classification));
        root.insert("metadata".into(), Value::Object(metadata));
        root.insert("finalUrl".into(), self.final_url.clone().into());
        Value::Object(root)
    }
}

/// Run the full classification pipeline (blocking).
/// Returns Ok(ClassifyResult) or Err(error message).
pub fn classify(url_str: &str) -> Result<ClassifyResult, String> {
    if !qapp_common::url_helpers::is_valid_url(url_str) {
        return Err("Invalid URL".into());
    }

    let url = Url::parse(url_str).map_err(|e| format!("URL parse error: {}", e))?;

    // Step 1: Fetch page
    let client = reqwest::blocking::Client::builder()
        .user_agent(USER_AGENT)
        .timeout(std::time::Duration::from_secs(10))
        .redirect(reqwest::redirect::Policy::limited(10))
        .build()
        .map_err(|e| format!("HTTP client error: {}", e))?;

    let response = client
        .get(url_str)
        .send()
        .map_err(|e| format!("Failed to fetch: {}", e))?;

    let final_url = Url::parse(response.url().as_str())
        .unwrap_or_else(|_| url.clone());
    let html = response
        .text()
        .map_err(|e| format!("Failed to read response: {}", e))?;

    classify_from_html(&html, &final_url, &client)
}

/// Classify from WebEngine fallback JSON data.
/// The JSON contains: html, finalUrl, isHttps, manifestJson, swDetected.
pub fn classify_from_webengine_data(json_str: &str) -> Result<ClassifyResult, String> {
    let data: serde_json::Value =
        serde_json::from_str(json_str).map_err(|e| format!("WebEngine JSON parse: {}", e))?;

    let success = data["success"].as_bool().unwrap_or(false);
    if !success {
        let err = data["error"].as_str().unwrap_or("Unknown WebEngine error");
        return Err(format!("WebEngine fetch failed: {}", err));
    }

    let html = data["html"].as_str().unwrap_or("");
    let final_url_str = data["finalUrl"].as_str().unwrap_or("");
    let sw_detected = data["swDetected"].as_bool().unwrap_or(false);
    let manifest_json_str = data["manifestJson"].as_str().unwrap_or("");

    let final_url =
        Url::parse(final_url_str).map_err(|e| format!("WebEngine finalUrl parse: {}", e))?;

    // Parse HTML
    let html_meta = html_parser::parse(html, &final_url);

    // Parse manifest from pre-fetched JSON (if available)
    let mut manifest = manifest_parser::Manifest::default();
    let mut has_manifest = false;

    if !manifest_json_str.is_empty() {
        if let Ok(json) = serde_json::from_str::<serde_json::Value>(manifest_json_str) {
            if let Ok(m) = manifest_parser::validate(&json) {
                manifest = m;
                has_manifest = true;
            }
        }
    }

    // Build result using do_classify (same logic as HTTP path)
    let mut result = do_classify(&final_url, &html_meta, &manifest, has_manifest, sw_detected);

    // Icon fallback: letter icon (no HTTP client available for probing)
    if result.icon_url.is_empty() && !html_meta.og_image.is_empty() {
        result.icon_url = html_meta.og_image;
    }
    if result.icon_url.is_empty() {
        result.icon_url = icon_manager::generate_letter_icon(&result.name);
    }

    Ok(result)
}

/// Classify from pre-fetched HTML (used for WebEngine fallback path).
pub fn classify_from_html(
    html: &str,
    final_url: &Url,
    client: &reqwest::blocking::Client,
) -> Result<ClassifyResult, String> {
    // Step 2: Parse HTML
    let html_meta = html_parser::parse(html, final_url);

    // Step 3: Fetch manifest (if found)
    let mut manifest = manifest_parser::Manifest::default();
    let mut has_manifest = false;

    if !html_meta.manifest_url.is_empty() {
        if let Ok(resp) = client.get(&html_meta.manifest_url).send() {
            if let Ok(text) = resp.text() {
                if let Ok(json) = serde_json::from_str::<Value>(&text) {
                    if let Ok(m) = manifest_parser::validate(&json) {
                        manifest = m;
                        has_manifest = true;
                    }
                }
            }
        }
    }

    // Step 4: Detect service worker
    let mut sw_detected = html_meta.sw_hint;
    if !sw_detected {
        for path in SW_PATHS {
            let mut probe_url = final_url.clone();
            probe_url.set_path(path);
            probe_url.set_query(None);
            probe_url.set_fragment(None);
            if let Ok(resp) = client.head(probe_url.as_str()).send() {
                if resp.status().is_success() {
                    sw_detected = true;
                    break;
                }
            }
        }
    }

    // Step 5: Classify
    let mut result = do_classify(final_url, &html_meta, &manifest, has_manifest, sw_detected);

    // Step 6: Resolve icon (probe fallbacks)
    if result.icon_url.is_empty() {
        for path in FAVICON_PATHS {
            let mut probe_url = final_url.clone();
            probe_url.set_path(path);
            probe_url.set_query(None);
            probe_url.set_fragment(None);
            if let Ok(resp) = client.head(probe_url.as_str()).send() {
                if resp.status().is_success() {
                    if let Some(ct) = resp.headers().get("content-type") {
                        if let Ok(ct_str) = ct.to_str() {
                            if ct_str.starts_with("image/")
                                || ct_str.contains("octet-stream")
                            {
                                result.icon_url = probe_url.to_string();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // OG image fallback
    if result.icon_url.is_empty() && !html_meta.og_image.is_empty() {
        result.icon_url = html_meta.og_image;
    }

    // Letter icon as last resort
    if result.icon_url.is_empty() {
        result.icon_url = icon_manager::generate_letter_icon(&result.name);
    }

    Ok(result)
}

fn do_classify(
    url: &Url,
    html_meta: &html_parser::HtmlMeta,
    manifest: &manifest_parser::Manifest,
    has_manifest: bool,
    sw_detected: bool,
) -> ClassifyResult {
    let mut r = ClassifyResult::default();
    r.has_manifest = has_manifest;
    r.has_service_worker = sw_detected;

    r.level = if has_manifest && sw_detected {
        "PWAPP".into()
    } else if has_manifest {
        "WAPP".into()
    } else {
        "WS".into()
    };

    // Name: manifest.name → manifest.short_name → html title → hostname
    if !manifest.name.is_empty() {
        r.name = manifest.name.clone();
    } else if !manifest.short_name.is_empty() {
        r.name = manifest.short_name.clone();
    } else if !html_meta.title.is_empty() {
        r.name = html_meta.title.clone();
    } else {
        r.name = url.host_str().unwrap_or("Unknown").to_string();
    }

    // Display mode
    r.display_mode = if manifest.display.is_empty() {
        "browser".into()
    } else {
        manifest.display.clone()
    };

    // Theme color
    if !manifest.theme_color.is_empty() {
        r.theme_color = manifest.theme_color.clone();
    } else if !html_meta.theme_color.is_empty() {
        r.theme_color = html_meta.theme_color.clone();
    }

    // Background color
    if !manifest.background_color.is_empty() {
        r.background_color = manifest.background_color.clone();
    }

    // Start URL
    if !manifest.start_url.is_empty() {
        if let Ok(resolved) = url.join(&manifest.start_url) {
            r.start_url = resolved.to_string();
        }
    }

    // Scope
    if !manifest.scope.is_empty() {
        if let Ok(resolved) = url.join(&manifest.scope) {
            r.scope = resolved.to_string();
        }
    }

    // Manifest id
    r.manifest_id = manifest.id.clone();
    r.display_override = manifest.display_override.clone();

    // Shortcuts
    for sc in &manifest.shortcuts {
        let mut obj = serde_json::Map::new();
        obj.insert("name".into(), sc.name.clone().into());
        if let Ok(resolved) = url.join(&sc.url) {
            obj.insert("url".into(), resolved.to_string().into());
        }
        if !sc.description.is_empty() {
            obj.insert("description".into(), sc.description.clone().into());
        }
        r.shortcuts.push(Value::Object(obj));
    }

    // MIME types from file handlers
    for fh in &manifest.file_handlers {
        for mime in &fh.accept {
            if !r.mime_types.contains(mime) {
                r.mime_types.push(mime.clone());
            }
        }
    }

    // Protocol handlers
    for ph in &manifest.protocol_handlers {
        let mut obj = serde_json::Map::new();
        obj.insert("protocol".into(), ph.protocol.clone().into());
        if let Ok(resolved) = url.join(&ph.url) {
            obj.insert("url".into(), resolved.to_string().into());
        }
        r.protocol_handlers.push(Value::Object(obj));
    }

    // Final URL
    r.final_url = url.to_string();

    // Icon: manifest → HTML icons
    if !manifest.icons.is_empty() {
        r.icon_url = manifest_parser::pick_best_icon(&manifest.icons, url);
    } else if !html_meta.icons.is_empty() {
        r.icon_url = html_meta.icons[0].0.clone();
    }

    r
}
