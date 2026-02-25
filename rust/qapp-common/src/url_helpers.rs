use regex::Regex;
use std::sync::LazyLock;
use url::Url;

static URL_PATTERN: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(
        r"^https?://[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*(:\d{1,5})?([\w\-._~:/?#\[\]@!$&'()*+,;=%]*)?$"
    ).unwrap()
});

static NON_ALNUM: LazyLock<Regex> = LazyLock::new(|| Regex::new(r"[^a-z0-9]+").unwrap());

/// Check whether a string is a valid HTTP(S) URL.
pub fn is_valid_url(text: &str) -> bool {
    if text.is_empty() {
        return false;
    }
    URL_PATTERN.is_match(text)
}

/// Generate an app ID from a URL string.
/// Rules: hostname only, dots->hyphens, lowercase, non-default port appended.
pub fn generate_app_id(url_str: &str) -> crate::Result<String> {
    let parsed = Url::parse(url_str).map_err(|_| "Invalid URL".to_string())?;

    let scheme = parsed.scheme();
    if scheme != "http" && scheme != "https" {
        return Err("URL must use http or https protocol".into());
    }

    let host = parsed
        .host_str()
        .ok_or_else(|| "URL has no hostname".to_string())?;

    if host.is_empty() {
        return Err("URL has no hostname".into());
    }

    let mut app_id = host.to_lowercase().replace('.', "-");

    // Append non-default port (skip 80 for http, 443 for https)
    if let Some(port) = parsed.port() {
        let is_default = (scheme == "http" && port == 80) || (scheme == "https" && port == 443);
        if !is_default {
            app_id = format!("{}-{}", app_id, port);
        }
    }

    Ok(app_id)
}

/// Generate an app ID from a custom name string.
/// Rules: lowercase, non-alphanumeric->hyphens, collapse, trim edges.
pub fn generate_app_id_from_name(name: &str) -> crate::Result<String> {
    let trimmed = name.trim();
    if trimmed.is_empty() {
        return Err("App name must not be empty".into());
    }

    let lower = trimmed.to_lowercase();
    let slug = NON_ALNUM.replace_all(&lower, "-");
    let slug = slug.trim_matches('-');

    if slug.is_empty() {
        return Err("App name must contain at least one alphanumeric character".into());
    }

    Ok(slug.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    // --- is_valid_url ---
    #[test]
    fn valid_http_url() {
        assert!(is_valid_url("http://example.com"));
    }

    #[test]
    fn valid_https_url() {
        assert!(is_valid_url("https://example.com"));
    }

    #[test]
    fn valid_url_with_path() {
        assert!(is_valid_url("https://example.com/path/to/page"));
    }

    #[test]
    fn valid_url_with_query() {
        assert!(is_valid_url("https://example.com?key=value"));
    }

    #[test]
    fn valid_url_with_fragment() {
        assert!(is_valid_url("https://example.com#section"));
    }

    #[test]
    fn valid_bare_hostname() {
        assert!(is_valid_url("http://mimer"));
    }

    #[test]
    fn valid_localhost() {
        assert!(is_valid_url("http://localhost"));
    }

    #[test]
    fn valid_ip_with_port() {
        assert!(is_valid_url("http://192.168.1.1:8080"));
    }

    #[test]
    fn valid_hostname_with_port() {
        assert!(is_valid_url("http://mimer:3000"));
    }

    #[test]
    fn invalid_empty() {
        assert!(!is_valid_url(""));
    }

    #[test]
    fn invalid_no_scheme() {
        assert!(!is_valid_url("example.com"));
    }

    #[test]
    fn invalid_ftp() {
        assert!(!is_valid_url("ftp://example.com"));
    }

    // --- generate_app_id ---
    #[test]
    fn app_id_basic() {
        assert_eq!(generate_app_id("https://example.com").unwrap(), "example-com");
    }

    #[test]
    fn app_id_lowercase() {
        assert_eq!(
            generate_app_id("https://Example.COM").unwrap(),
            "example-com"
        );
    }

    #[test]
    fn app_id_with_port() {
        assert_eq!(
            generate_app_id("http://localhost:3000").unwrap(),
            "localhost-3000"
        );
    }

    #[test]
    fn app_id_skip_default_80() {
        assert_eq!(
            generate_app_id("http://example.com:80").unwrap(),
            "example-com"
        );
    }

    #[test]
    fn app_id_skip_default_443() {
        assert_eq!(
            generate_app_id("https://example.com:443").unwrap(),
            "example-com"
        );
    }

    #[test]
    fn app_id_invalid_url() {
        assert!(generate_app_id("not-a-url").is_err());
    }

    // --- generate_app_id_from_name ---
    #[test]
    fn name_basic() {
        assert_eq!(generate_app_id_from_name("My App").unwrap(), "my-app");
    }

    #[test]
    fn name_special_chars() {
        assert_eq!(
            generate_app_id_from_name("App (Test) #1").unwrap(),
            "app-test-1"
        );
    }

    #[test]
    fn name_unicode() {
        assert_eq!(
            generate_app_id_from_name("Ørsted App").unwrap(),
            "rsted-app"
        );
    }

    #[test]
    fn name_empty() {
        assert!(generate_app_id_from_name("").is_err());
    }

    #[test]
    fn name_whitespace_only() {
        assert!(generate_app_id_from_name("   ").is_err());
    }

    #[test]
    fn name_all_special() {
        assert!(generate_app_id_from_name("@#$%").is_err());
    }
}
