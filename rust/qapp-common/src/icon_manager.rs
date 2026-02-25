use base64::Engine;
use std::fs;
use std::path::Path;
use url::form_urlencoded;

/// Deterministic hue from string hash (0-359).
pub fn hash_to_hue(s: &str) -> i32 {
    let mut hash: i32 = 0;
    for ch in s.chars() {
        hash = hash.wrapping_mul(31).wrapping_add(ch as i32);
        hash &= 0x7FFFFFFF; // Keep positive
    }
    ((hash % 360) + 360) % 360
}

/// Generate a letter icon as an SVG data URI.
pub fn generate_letter_icon(name: &str) -> String {
    let letter = if name.is_empty() {
        '?'
    } else {
        name.chars().next().unwrap().to_uppercase().next().unwrap()
    };
    let hue = hash_to_hue(if name.is_empty() { "default" } else { name });

    let svg = format!(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 192 192'>\
         <rect width='192' height='192' rx='32' fill='hsl({},65%,45%)'/>\
         <text x='96' y='96' font-size='96' text-anchor='middle' \
         dominant-baseline='central' fill='white' font-family='sans-serif'>{}</text>\
         </svg>",
        hue, letter
    );

    let encoded: String = form_urlencoded::byte_serialize(svg.as_bytes()).collect();
    format!("data:image/svg+xml,{}", encoded)
}

/// Default globe icon as SVG data URI.
pub fn default_icon() -> String {
    "data:image/svg+xml,\
     %3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 192 192'%3E\
     %3Ccircle cx='96' cy='96' r='80' fill='%23607d8b' stroke='white' stroke-width='4'/%3E\
     %3Ctext x='96' y='96' font-size='64' text-anchor='middle' \
     dominant-baseline='central' fill='white' font-family='sans-serif'%3E\
     %F0%9F%8C%90%3C/text%3E%3C/svg%3E"
        .to_string()
}

/// Detect image format from source (data URI or file extension).
pub fn detect_format(source: &str) -> &str {
    if source.starts_with("data:image/svg") {
        return "svg";
    }
    if source.starts_with("data:image/png") {
        return "png";
    }
    if source.starts_with("data:image/ico") {
        return "ico";
    }
    if source.starts_with("data:image/webp") {
        return "webp";
    }
    if source.ends_with(".svg") {
        return "svg";
    }
    if source.ends_with(".ico") {
        return "ico";
    }
    if source.ends_with(".webp") {
        return "webp";
    }
    "png"
}

/// Save a data URI to a file, creating parent directories if needed.
pub fn save_data_uri(data_uri: &str, dest_path: &str) -> crate::Result<()> {
    let comma_idx = data_uri
        .find(',')
        .ok_or_else(|| "Invalid data URI".to_string())?;

    let header = &data_uri[..comma_idx];
    let payload = &data_uri[comma_idx + 1..];

    let path = Path::new(dest_path);
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent).map_err(|e| format!("Cannot create directory: {}", e))?;
    }

    let bytes = if header.contains(";base64") {
        base64::engine::general_purpose::STANDARD
            .decode(payload)
            .map_err(|e| format!("Base64 decode error: {}", e))?
    } else {
        percent_decode(payload)
    };

    fs::write(dest_path, bytes).map_err(|e| format!("Write failed: {}", e))
}

fn percent_decode(input: &str) -> Vec<u8> {
    let mut bytes = Vec::with_capacity(input.len());
    let input_bytes = input.as_bytes();
    let mut i = 0;
    while i < input_bytes.len() {
        if input_bytes[i] == b'%' && i + 2 < input_bytes.len() {
            if let Ok(val) = u8::from_str_radix(
                std::str::from_utf8(&input_bytes[i + 1..i + 3]).unwrap_or(""),
                16,
            ) {
                bytes.push(val);
                i += 3;
                continue;
            }
        }
        bytes.push(input_bytes[i]);
        i += 1;
    }
    bytes
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::TempDir;

    #[test]
    fn hash_to_hue_deterministic() {
        let h1 = hash_to_hue("test");
        let h2 = hash_to_hue("test");
        assert_eq!(h1, h2);
    }

    #[test]
    fn hash_to_hue_range() {
        for name in &["a", "hello", "test", "example.com", "ZZZ"] {
            let h = hash_to_hue(name);
            assert!((0..360).contains(&h), "Hue {} out of range for {}", h, name);
        }
    }

    #[test]
    fn hash_to_hue_different_inputs() {
        let h1 = hash_to_hue("apple");
        let h2 = hash_to_hue("banana");
        // Different inputs should (usually) give different hues
        // Not guaranteed, but extremely likely for these inputs
        assert_ne!(h1, h2);
    }

    #[test]
    fn generate_letter_icon_svg() {
        let icon = generate_letter_icon("Example");
        assert!(icon.starts_with("data:image/svg+xml,"));
        assert!(icon.contains("E")); // first letter uppercase
    }

    #[test]
    fn generate_letter_icon_empty() {
        let icon = generate_letter_icon("");
        assert!(icon.contains("%3F") || icon.contains("?")); // question mark
    }

    #[test]
    fn default_icon_is_data_uri() {
        let icon = default_icon();
        assert!(icon.starts_with("data:image/svg+xml,"));
    }

    #[test]
    fn detect_format_data_uri() {
        assert_eq!(detect_format("data:image/svg+xml,..."), "svg");
        assert_eq!(detect_format("data:image/png;base64,..."), "png");
        assert_eq!(detect_format("data:image/ico,..."), "ico");
        assert_eq!(detect_format("data:image/webp,..."), "webp");
    }

    #[test]
    fn detect_format_extension() {
        assert_eq!(detect_format("/path/icon.svg"), "svg");
        assert_eq!(detect_format("/path/icon.ico"), "ico");
        assert_eq!(detect_format("/path/icon.webp"), "webp");
        assert_eq!(detect_format("/path/icon.png"), "png");
    }

    #[test]
    fn detect_format_default() {
        assert_eq!(detect_format("https://example.com/icon"), "png");
    }

    #[test]
    fn save_data_uri_base64() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("test.png");
        let data_uri = "data:image/png;base64,iVBORw0KGgo=";
        save_data_uri(data_uri, path.to_str().unwrap()).unwrap();
        assert!(path.exists());
    }

    #[test]
    fn save_data_uri_percent_encoded() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("test.svg");
        let data_uri = "data:image/svg+xml,%3Csvg%3E%3C/svg%3E";
        save_data_uri(data_uri, path.to_str().unwrap()).unwrap();
        assert!(path.exists());
        let content = fs::read_to_string(&path).unwrap();
        assert!(content.contains("<svg>"));
    }

    #[test]
    fn save_data_uri_creates_dirs() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("sub/dir/test.svg");
        let data_uri = "data:image/svg+xml,%3Csvg/%3E";
        save_data_uri(data_uri, path.to_str().unwrap()).unwrap();
        assert!(path.exists());
    }

    #[test]
    fn save_data_uri_invalid() {
        let result = save_data_uri("not-a-data-uri", "/tmp/test");
        assert!(result.is_err());
    }
}
