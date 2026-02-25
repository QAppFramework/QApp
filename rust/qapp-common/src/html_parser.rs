use regex::Regex;
use std::sync::LazyLock;
use url::Url;

/// Parsed HTML metadata.
#[derive(Debug, Clone, Default)]
pub struct HtmlMeta {
    pub manifest_url: String,
    pub title: String,
    pub icons: Vec<(String, String)>, // (href, sizes)
    pub theme_color: String,
    pub og_image: String,
    pub sw_hint: bool,
}

fn resolve_url(href: &str, base_url: &Url) -> String {
    if href.is_empty() {
        return String::new();
    }
    match base_url.join(href) {
        Ok(u) => u.to_string(),
        Err(_) => href.to_string(),
    }
}

// Manifest: <link rel="manifest" href="...">
static MANIFEST_RE1: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<link[^>]*rel=["']manifest["'][^>]*href=["']([^"']+)["']"#).unwrap()
});
static MANIFEST_RE2: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<link[^>]*href=["']([^"']+)["'][^>]*rel=["']manifest["']"#).unwrap()
});

// Title
static TITLE_RE: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r"(?i)<title[^>]*>([^<]*)</title>").unwrap());

// Icons: <link rel="icon|apple-touch-icon|shortcut icon" href="..." sizes="...">
static ICON_RE1: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(
        r#"(?i)<link[^>]*rel=["'](icon|apple-touch-icon|shortcut icon)["'][^>]*href=["']([^"']+)["'][^>]*/?\s*>"#,
    )
    .unwrap()
});
static ICON_RE2: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(
        r#"(?i)<link[^>]*href=["']([^"']+)["'][^>]*rel=["'](icon|apple-touch-icon|shortcut icon)["'][^>]*/?\s*>"#,
    )
    .unwrap()
});
static SIZES_RE: LazyLock<Regex> =
    LazyLock::new(|| Regex::new(r#"(?i)sizes=["']([^"']+)["']"#).unwrap());

// Theme color
static THEME_RE1: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<meta[^>]*name=["']theme-color["'][^>]*content=["']([^"']+)["']"#).unwrap()
});
static THEME_RE2: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<meta[^>]*content=["']([^"']+)["'][^>]*name=["']theme-color["']"#).unwrap()
});

// OG Image
static OG_RE1: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<meta[^>]*property=["']og:image["'][^>]*content=["']([^"']+)["']"#).unwrap()
});
static OG_RE2: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(r#"(?i)<meta[^>]*content=["']([^"']+)["'][^>]*property=["']og:image["']"#).unwrap()
});

/// Parse HTML and extract metadata (manifest URL, title, icons, theme color, etc.)
pub fn parse(html: &str, base_url: &Url) -> HtmlMeta {
    let mut meta = HtmlMeta::default();

    // Manifest
    if let Some(cap) = MANIFEST_RE1.captures(html) {
        meta.manifest_url = resolve_url(&cap[1], base_url);
    } else if let Some(cap) = MANIFEST_RE2.captures(html) {
        meta.manifest_url = resolve_url(&cap[1], base_url);
    }

    // Title
    if let Some(cap) = TITLE_RE.captures(html) {
        meta.title = cap[1].trim().to_string();
    }

    // Icons (rel before href)
    for cap in ICON_RE1.captures_iter(html) {
        let href = resolve_url(&cap[2], base_url);
        let sizes = SIZES_RE
            .captures(&cap[0])
            .map_or(String::new(), |s| s[1].to_string());
        meta.icons.push((href, sizes));
    }
    // Icons (href before rel)
    for cap in ICON_RE2.captures_iter(html) {
        let href = resolve_url(&cap[1], base_url);
        let sizes = SIZES_RE
            .captures(&cap[0])
            .map_or(String::new(), |s| s[1].to_string());
        meta.icons.push((href, sizes));
    }

    // Theme color
    if let Some(cap) = THEME_RE1.captures(html) {
        meta.theme_color = cap[1].to_string();
    } else if let Some(cap) = THEME_RE2.captures(html) {
        meta.theme_color = cap[1].to_string();
    }

    // OG Image
    if let Some(cap) = OG_RE1.captures(html) {
        meta.og_image = resolve_url(&cap[1], base_url);
    } else if let Some(cap) = OG_RE2.captures(html) {
        meta.og_image = resolve_url(&cap[1], base_url);
    }

    // Service worker hint
    meta.sw_hint = html.contains("navigator.serviceWorker.register(");

    meta
}

#[cfg(test)]
mod tests {
    use super::*;

    fn base() -> Url {
        Url::parse("https://example.com").unwrap()
    }

    #[test]
    fn parse_manifest_rel_first() {
        let html = r#"<link rel="manifest" href="/manifest.json">"#;
        let meta = parse(html, &base());
        assert_eq!(meta.manifest_url, "https://example.com/manifest.json");
    }

    #[test]
    fn parse_manifest_href_first() {
        let html = r#"<link href="/manifest.json" rel="manifest">"#;
        let meta = parse(html, &base());
        assert_eq!(meta.manifest_url, "https://example.com/manifest.json");
    }

    #[test]
    fn parse_title() {
        let html = "<title>Test Page</title>";
        let meta = parse(html, &base());
        assert_eq!(meta.title, "Test Page");
    }

    #[test]
    fn parse_title_trimmed() {
        let html = "<title>  Spaced  </title>";
        let meta = parse(html, &base());
        assert_eq!(meta.title, "Spaced");
    }

    #[test]
    fn parse_icons_with_sizes() {
        let html = r#"<link rel="icon" href="/icon.png" sizes="192x192">"#;
        let meta = parse(html, &base());
        assert_eq!(meta.icons.len(), 1);
        assert_eq!(meta.icons[0].0, "https://example.com/icon.png");
        assert_eq!(meta.icons[0].1, "192x192");
    }

    #[test]
    fn parse_theme_color() {
        let html = r##"<meta name="theme-color" content="#ff0000">"##;
        let meta = parse(html, &base());
        assert_eq!(meta.theme_color, "#ff0000");
    }

    #[test]
    fn parse_theme_color_reverse() {
        let html = r##"<meta content="#00ff00" name="theme-color">"##;
        let meta = parse(html, &base());
        assert_eq!(meta.theme_color, "#00ff00");
    }

    #[test]
    fn parse_og_image() {
        let html = r#"<meta property="og:image" content="/og.png">"#;
        let meta = parse(html, &base());
        assert_eq!(meta.og_image, "https://example.com/og.png");
    }

    #[test]
    fn parse_sw_hint() {
        let html = r#"<script>navigator.serviceWorker.register('/sw.js')</script>"#;
        let meta = parse(html, &base());
        assert!(meta.sw_hint);
    }

    #[test]
    fn no_sw_hint() {
        let html = "<html><body>Hello</body></html>";
        let meta = parse(html, &base());
        assert!(!meta.sw_hint);
    }
}
