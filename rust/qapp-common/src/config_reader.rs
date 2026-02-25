use serde_json::Value;
use std::fs;
use std::path::Path;

/// Read a JSON file and return the parsed object.
pub fn read_json(path: &str) -> crate::Result<Value> {
    if path.is_empty() {
        return Err("Path must not be empty".into());
    }

    let content = fs::read_to_string(path)
        .map_err(|e| format!("Cannot read file {}: {}", path, e))?;

    let val: Value =
        serde_json::from_str(&content).map_err(|e| format!("JSON parse error: {}", e))?;

    if !val.is_object() {
        return Err("JSON root is not an object".into());
    }

    Ok(val)
}

/// Write a JSON value to a file, creating parent directories if needed.
pub fn write_json(path: &str, obj: &Value) -> crate::Result<()> {
    if path.is_empty() {
        return Err("Path must not be empty".into());
    }

    let p = Path::new(path);
    if let Some(parent) = p.parent() {
        fs::create_dir_all(parent)
            .map_err(|e| format!("Cannot create directory: {}", e))?;
    }

    let data = serde_json::to_string_pretty(obj)
        .map_err(|e| format!("JSON serialize error: {}", e))?;

    fs::write(path, data).map_err(|e| format!("Write failed: {}", e))
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::TempDir;

    #[test]
    fn read_write_roundtrip() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("test.json");
        let path_str = path.to_str().unwrap();

        let obj = serde_json::json!({"key": "value", "num": 42});
        write_json(path_str, &obj).unwrap();

        let result = read_json(path_str).unwrap();
        assert_eq!(result["key"], "value");
        assert_eq!(result["num"], 42);
    }

    #[test]
    fn read_missing_file() {
        let result = read_json("/nonexistent/path.json");
        assert!(result.is_err());
    }

    #[test]
    fn read_empty_path() {
        let result = read_json("");
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("empty"));
    }

    #[test]
    fn read_invalid_json() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("bad.json");
        fs::write(&path, "not json").unwrap();
        let result = read_json(path.to_str().unwrap());
        assert!(result.is_err());
    }

    #[test]
    fn read_array_root() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("arr.json");
        fs::write(&path, "[1,2,3]").unwrap();
        let result = read_json(path.to_str().unwrap());
        assert!(result.is_err());
        assert!(result.unwrap_err().contains("not an object"));
    }

    #[test]
    fn write_creates_parent_dirs() {
        let dir = TempDir::new().unwrap();
        let path = dir.path().join("sub/dir/test.json");
        let obj = serde_json::json!({"ok": true});
        write_json(path.to_str().unwrap(), &obj).unwrap();
        assert!(path.exists());
    }

    #[test]
    fn write_empty_path() {
        let result = write_json("", &serde_json::json!({}));
        assert!(result.is_err());
    }
}
