use serde_json::Value;
use url::Url;

const VALID_DISPLAY_MODES: &[&str] = &["fullscreen", "standalone", "minimal-ui", "browser"];

#[derive(Debug, Clone, Default)]
pub struct ManifestIcon {
    pub src: String,
    pub sizes: String,
    pub icon_type: String,
    pub purpose: String,
}

#[derive(Debug, Clone, Default)]
pub struct ManifestShortcut {
    pub name: String,
    pub url: String,
    pub description: String,
    pub icon_src: String,
}

#[derive(Debug, Clone, Default)]
pub struct ManifestFileHandler {
    pub action: String,
    pub accept: Vec<String>,
}

#[derive(Debug, Clone, Default)]
pub struct ManifestProtocolHandler {
    pub protocol: String,
    pub url: String,
}

#[derive(Debug, Clone, Default)]
pub struct Manifest {
    pub name: String,
    pub short_name: String,
    pub start_url: String,
    pub scope: String,
    pub display: String,
    pub display_override: Vec<String>,
    pub id: String,
    pub icons: Vec<ManifestIcon>,
    pub shortcuts: Vec<ManifestShortcut>,
    pub file_handlers: Vec<ManifestFileHandler>,
    pub protocol_handlers: Vec<ManifestProtocolHandler>,
    pub theme_color: String,
    pub background_color: String,
}

fn get_str(obj: &Value, key: &str) -> String {
    obj.get(key)
        .and_then(Value::as_str)
        .unwrap_or("")
        .to_string()
}

/// Validate and parse a W3C manifest JSON object.
pub fn validate(json: &Value) -> crate::Result<Manifest> {
    let obj = json
        .as_object()
        .ok_or_else(|| "Empty manifest".to_string())?;
    if obj.is_empty() {
        return Err("Empty manifest".into());
    }

    let mut m = Manifest::default();

    m.name = get_str(json, "name");
    m.short_name = get_str(json, "short_name");
    m.start_url = get_str(json, "start_url");
    m.scope = get_str(json, "scope");
    m.theme_color = get_str(json, "theme_color");
    m.background_color = get_str(json, "background_color");
    m.id = get_str(json, "id");

    // Validate display mode
    let display = get_str(json, "display");
    if !display.is_empty() {
        if !VALID_DISPLAY_MODES.contains(&display.as_str()) {
            return Err(format!("Invalid display mode: {}", display));
        }
        m.display = display;
    }

    // display_override
    if let Some(arr) = json.get("display_override").and_then(Value::as_array) {
        for v in arr {
            if let Some(s) = v.as_str() {
                m.display_override.push(s.to_string());
            }
        }
    }

    // shortcuts
    if let Some(arr) = json.get("shortcuts").and_then(Value::as_array) {
        for val in arr {
            if !val.is_object() {
                continue;
            }
            let name = get_str(val, "name");
            let url = get_str(val, "url");
            if name.is_empty() || url.is_empty() {
                continue;
            }
            let mut sc = ManifestShortcut {
                name,
                url,
                description: get_str(val, "description"),
                ..Default::default()
            };
            if let Some(icons_arr) = val.get("icons").and_then(Value::as_array) {
                if let Some(first) = icons_arr.first() {
                    sc.icon_src = get_str(first, "src");
                }
            }
            m.shortcuts.push(sc);
        }
    }

    // file_handlers
    if let Some(arr) = json.get("file_handlers").and_then(Value::as_array) {
        for val in arr {
            if !val.is_object() {
                continue;
            }
            let action = get_str(val, "action");
            if action.is_empty() {
                continue;
            }
            let mut fh = ManifestFileHandler {
                action,
                ..Default::default()
            };
            if let Some(accept_obj) = val.get("accept").and_then(Value::as_object) {
                for key in accept_obj.keys() {
                    fh.accept.push(key.clone());
                }
            }
            m.file_handlers.push(fh);
        }
    }

    // protocol_handlers
    if let Some(arr) = json.get("protocol_handlers").and_then(Value::as_array) {
        for val in arr {
            if !val.is_object() {
                continue;
            }
            let protocol = get_str(val, "protocol");
            let url = get_str(val, "url");
            if protocol.is_empty() || url.is_empty() {
                continue;
            }
            m.protocol_handlers.push(ManifestProtocolHandler { protocol, url });
        }
    }

    // icons
    if let Some(arr) = json.get("icons").and_then(Value::as_array) {
        for val in arr {
            if !val.is_object() {
                continue;
            }
            let src = get_str(val, "src");
            if src.is_empty() {
                continue;
            }
            m.icons.push(ManifestIcon {
                src,
                sizes: get_str(val, "sizes"),
                icon_type: get_str(val, "type"),
                purpose: get_str(val, "purpose"),
            });
        }
    }

    Ok(m)
}

fn parse_size(sizes: &str) -> i32 {
    if sizes.is_empty() {
        return 0;
    }
    // Parse "192x192" -> 192
    let idx = sizes.find('x');
    match idx {
        Some(i) if i > 0 => sizes[..i].parse().unwrap_or(0),
        _ => 0,
    }
}

/// Pick the best icon from a manifest icon list.
/// Prefers 192x192, then largest.
pub fn pick_best_icon(icons: &[ManifestIcon], base_url: &Url) -> String {
    if icons.is_empty() {
        return String::new();
    }

    let mut best_idx = 0;
    let mut best_size = parse_size(&icons[0].sizes);
    let mut found_192 = best_size == 192;

    for (i, icon) in icons.iter().enumerate().skip(1) {
        let s = parse_size(&icon.sizes);
        if !found_192 && s == 192 {
            best_idx = i;
            best_size = s;
            found_192 = true;
        } else if !found_192 && s > best_size {
            best_idx = i;
            best_size = s;
        }
    }

    let _ = best_size; // used only for selection
    let src = &icons[best_idx].src;
    if src.starts_with("http://") || src.starts_with("https://") {
        return src.clone();
    }
    match base_url.join(src) {
        Ok(u) => u.to_string(),
        Err(_) => src.clone(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_basic_manifest() {
        let json = serde_json::json!({
            "name": "Test App",
            "short_name": "Test",
            "start_url": "/",
            "display": "standalone"
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.name, "Test App");
        assert_eq!(m.short_name, "Test");
        assert_eq!(m.start_url, "/");
        assert_eq!(m.display, "standalone");
    }

    #[test]
    fn invalid_display_mode() {
        let json = serde_json::json!({
            "name": "Test",
            "display": "invalid"
        });
        assert!(validate(&json).is_err());
    }

    #[test]
    fn all_display_modes() {
        for mode in VALID_DISPLAY_MODES {
            let json = serde_json::json!({"name": "Test", "display": mode});
            assert!(validate(&json).is_ok());
        }
    }

    #[test]
    fn parse_colors() {
        let json = serde_json::json!({
            "name": "Test",
            "theme_color": "#ff0000",
            "background_color": "#ffffff"
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.theme_color, "#ff0000");
        assert_eq!(m.background_color, "#ffffff");
    }

    #[test]
    fn parse_icons() {
        let json = serde_json::json!({
            "name": "Test",
            "icons": [
                {"src": "/icon-192.png", "sizes": "192x192", "type": "image/png"},
                {"src": "/icon-512.png", "sizes": "512x512"}
            ]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.icons.len(), 2);
        assert_eq!(m.icons[0].src, "/icon-192.png");
        assert_eq!(m.icons[0].sizes, "192x192");
    }

    #[test]
    fn skip_icons_without_src() {
        let json = serde_json::json!({
            "name": "Test",
            "icons": [
                {"sizes": "192x192"},
                {"src": "/ok.png"}
            ]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.icons.len(), 1);
    }

    #[test]
    fn pick_best_icon_prefers_192() {
        let base = Url::parse("https://example.com").unwrap();
        let icons = vec![
            ManifestIcon {
                src: "/icon-512.png".into(),
                sizes: "512x512".into(),
                ..Default::default()
            },
            ManifestIcon {
                src: "/icon-192.png".into(),
                sizes: "192x192".into(),
                ..Default::default()
            },
        ];
        let result = pick_best_icon(&icons, &base);
        assert!(result.contains("icon-192"));
    }

    #[test]
    fn pick_best_icon_largest_fallback() {
        let base = Url::parse("https://example.com").unwrap();
        let icons = vec![
            ManifestIcon {
                src: "/icon-64.png".into(),
                sizes: "64x64".into(),
                ..Default::default()
            },
            ManifestIcon {
                src: "/icon-512.png".into(),
                sizes: "512x512".into(),
                ..Default::default()
            },
        ];
        let result = pick_best_icon(&icons, &base);
        assert!(result.contains("icon-512"));
    }

    #[test]
    fn pick_best_icon_empty() {
        let base = Url::parse("https://example.com").unwrap();
        assert!(pick_best_icon(&[], &base).is_empty());
    }

    #[test]
    fn parse_display_override() {
        let json = serde_json::json!({
            "name": "Test",
            "display_override": ["standalone", "minimal-ui"]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.display_override, vec!["standalone", "minimal-ui"]);
    }

    #[test]
    fn parse_shortcuts() {
        let json = serde_json::json!({
            "name": "Test",
            "shortcuts": [
                {"name": "Home", "url": "/"},
                {"name": "Settings", "url": "/settings", "description": "App settings"}
            ]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.shortcuts.len(), 2);
        assert_eq!(m.shortcuts[1].description, "App settings");
    }

    #[test]
    fn skip_invalid_shortcuts() {
        let json = serde_json::json!({
            "name": "Test",
            "shortcuts": [
                {"name": "No URL"},
                {"url": "/no-name"},
                {"name": "Valid", "url": "/ok"}
            ]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.shortcuts.len(), 1);
    }

    #[test]
    fn parse_file_handlers() {
        let json = serde_json::json!({
            "name": "Test",
            "file_handlers": [{
                "action": "/open",
                "accept": {"image/png": [".png"], "image/jpeg": [".jpg"]}
            }]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.file_handlers.len(), 1);
        assert_eq!(m.file_handlers[0].action, "/open");
        assert_eq!(m.file_handlers[0].accept.len(), 2);
    }

    #[test]
    fn parse_protocol_handlers() {
        let json = serde_json::json!({
            "name": "Test",
            "protocol_handlers": [
                {"protocol": "web+custom", "url": "/handle?url=%s"}
            ]
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.protocol_handlers.len(), 1);
        assert_eq!(m.protocol_handlers[0].protocol, "web+custom");
    }

    #[test]
    fn empty_manifest_fails() {
        let json = serde_json::json!({});
        assert!(validate(&json).is_err());
    }

    #[test]
    fn non_string_fields_ignored() {
        let json = serde_json::json!({
            "name": 42,
            "short_name": true,
            "start_url": "/"
        });
        let m = validate(&json).unwrap();
        assert!(m.name.is_empty());
        assert!(m.short_name.is_empty());
    }

    #[test]
    fn parse_id() {
        let json = serde_json::json!({
            "name": "Test",
            "id": "my-app-id"
        });
        let m = validate(&json).unwrap();
        assert_eq!(m.id, "my-app-id");
    }
}
