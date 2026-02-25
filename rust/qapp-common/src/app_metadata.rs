use chrono::Utc;
use serde::{Deserialize, Serialize};
use serde_json::Value;

const VALID_LEVELS: &[&str] = &["WS", "WAPP", "PWAPP"];
const VALID_DISPLAY_MODES: &[&str] = &["fullscreen", "standalone", "minimal-ui", "browser"];

/// Per-app metadata stored as JSON.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct AppMetadataData {
    pub app_id: String,
    pub name: String,
    pub url: String,
    pub level: String,
    pub icon_path: String,
    pub desktop_path: String,
    pub wrapper_path: String,
    pub installed_at: String,

    // Optional manifest fields
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub display_mode: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub theme_color: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub background_color: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub start_url: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub scope: String,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub manifest_id: String,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub display_override: Vec<String>,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub shortcuts: Vec<Value>,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub mime_types: Vec<String>,
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub protocol_handlers: Vec<Value>,
    #[serde(default, skip_serializing_if = "String::is_empty")]
    pub qapp_version: String,
}

fn validate_fields(data: &AppMetadataData) -> crate::Result<()> {
    if data.app_id.is_empty() {
        return Err("Validation failed at appId: Invalid length".into());
    }
    if data.name.is_empty() {
        return Err("Validation failed at name: Invalid length".into());
    }
    if data.url.is_empty() {
        return Err("Validation failed at url: Invalid URL".into());
    }
    if !VALID_LEVELS.contains(&data.level.as_str()) {
        return Err("Validation failed at level: Invalid input".into());
    }
    if data.installed_at.is_empty() {
        return Err("Validation failed at installedAt: Invalid timestamp".into());
    }
    // Validate ISO 8601 timestamp
    if chrono::DateTime::parse_from_rfc3339(&data.installed_at).is_err()
        && chrono::NaiveDateTime::parse_from_str(&data.installed_at, "%Y-%m-%dT%H:%M:%S%.fZ")
            .is_err()
    {
        return Err("Validation failed at installedAt: Invalid timestamp".into());
    }
    if !data.display_mode.is_empty() && !VALID_DISPLAY_MODES.contains(&data.display_mode.as_str())
    {
        return Err("Validation failed at displayMode: Invalid input".into());
    }
    Ok(())
}

/// Build metadata from input, adding current ISO timestamp.
pub fn build(input: &AppMetadataData) -> crate::Result<AppMetadataData> {
    let mut data = input.clone();
    data.installed_at = Utc::now().to_rfc3339_opts(chrono::SecondsFormat::Millis, true);
    validate_fields(&data)?;
    Ok(data)
}

/// Validate existing metadata from a JSON Value.
pub fn validate(obj: &Value) -> crate::Result<AppMetadataData> {
    let data: AppMetadataData =
        serde_json::from_value(obj.clone()).map_err(|e| format!("Deserialization error: {}", e))?;
    validate_fields(&data)?;
    Ok(data)
}

/// Convert metadata to a JSON Value.
pub fn to_json(data: &AppMetadataData) -> crate::Result<Value> {
    serde_json::to_value(data).map_err(|e| format!("Serialization error: {}", e))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample_input() -> AppMetadataData {
        AppMetadataData {
            app_id: "example-com".into(),
            name: "Example".into(),
            url: "https://example.com".into(),
            level: "WS".into(),
            icon_path: "/home/user/icons/example.png".into(),
            desktop_path: "/home/user/apps/example.desktop".into(),
            wrapper_path: "/usr/bin/qapp-ws-wrapper".into(),
            ..Default::default()
        }
    }

    #[test]
    fn build_adds_timestamp() {
        let result = build(&sample_input()).unwrap();
        assert!(!result.installed_at.is_empty());
        assert!(result.installed_at.contains('T'));
    }

    #[test]
    fn build_validates_required_fields() {
        let mut input = sample_input();
        input.app_id = String::new();
        assert!(build(&input).is_err());
    }

    #[test]
    fn build_ws_level() {
        let result = build(&sample_input()).unwrap();
        assert_eq!(result.level, "WS");
    }

    #[test]
    fn build_wapp_level() {
        let mut input = sample_input();
        input.level = "WAPP".into();
        assert!(build(&input).is_ok());
    }

    #[test]
    fn build_pwapp_level() {
        let mut input = sample_input();
        input.level = "PWAPP".into();
        assert!(build(&input).is_ok());
    }

    #[test]
    fn build_invalid_level() {
        let mut input = sample_input();
        input.level = "INVALID".into();
        assert!(build(&input).is_err());
    }

    #[test]
    fn build_with_display_mode() {
        let mut input = sample_input();
        input.display_mode = "standalone".into();
        let result = build(&input).unwrap();
        assert_eq!(result.display_mode, "standalone");
    }

    #[test]
    fn build_invalid_display_mode() {
        let mut input = sample_input();
        input.display_mode = "invalid".into();
        assert!(build(&input).is_err());
    }

    #[test]
    fn to_json_omits_empty_optional() {
        let result = build(&sample_input()).unwrap();
        let json = to_json(&result).unwrap();
        assert!(json.get("displayMode").is_none());
        assert!(json.get("themeColor").is_none());
        assert!(json.get("shortcuts").is_none());
    }

    #[test]
    fn to_json_includes_set_optional() {
        let mut input = sample_input();
        input.display_mode = "standalone".into();
        input.theme_color = "#ff0000".into();
        let result = build(&input).unwrap();
        let json = to_json(&result).unwrap();
        assert_eq!(json["displayMode"], "standalone");
        assert_eq!(json["themeColor"], "#ff0000");
    }

    #[test]
    fn roundtrip_validate() {
        let original = build(&sample_input()).unwrap();
        let json = to_json(&original).unwrap();
        let restored = validate(&json).unwrap();
        assert_eq!(original.app_id, restored.app_id);
        assert_eq!(original.name, restored.name);
        assert_eq!(original.level, restored.level);
    }

    #[test]
    fn validate_backward_compatible() {
        // Minimal JSON without optional fields (old format)
        let json = serde_json::json!({
            "appId": "test",
            "name": "Test",
            "url": "https://test.com",
            "level": "WS",
            "iconPath": "/path/icon.png",
            "desktopPath": "/path/test.desktop",
            "wrapperPath": "/usr/bin/qapp-ws-wrapper",
            "installedAt": "2026-02-24T12:00:00.000Z"
        });
        let result = validate(&json).unwrap();
        assert_eq!(result.app_id, "test");
        assert!(result.display_mode.is_empty());
    }
}
