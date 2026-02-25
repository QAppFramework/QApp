/// Resolved XDG paths for an installed app.
#[derive(Debug, Clone, PartialEq)]
pub struct AppPaths {
    pub desktop_file: String,
    pub icon_file: String,
    pub metadata_file: String,
    pub icons_dir: String,
    pub apps_dir: String,
    pub applications_dir: String,
}

/// Resolve XDG paths for an installed app.
/// Uses ~/.local/share/ as the base (XDG_DATA_HOME).
pub fn resolve_app_paths(app_id: &str, icon_ext: &str) -> crate::Result<AppPaths> {
    if app_id.is_empty() {
        return Err("App ID must not be empty".into());
    }

    let data_dir = dirs::data_dir()
        .ok_or_else(|| "Cannot determine XDG data directory".to_string())?;
    let local_share = data_dir.to_string_lossy().to_string();

    let applications_dir = format!("{}/applications", local_share);
    let qapp_base = format!("{}/qapp-framework", local_share);
    let icons_dir = format!("{}/icons", qapp_base);
    let apps_dir = format!("{}/apps", qapp_base);

    Ok(AppPaths {
        desktop_file: format!("{}/qapp-{}.desktop", applications_dir, app_id),
        icon_file: format!("{}/{}.{}", icons_dir, app_id, icon_ext),
        metadata_file: format!("{}/{}.json", apps_dir, app_id),
        icons_dir,
        apps_dir,
        applications_dir,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn resolve_basic() {
        let paths = resolve_app_paths("example-com", "png").unwrap();
        assert!(paths.desktop_file.ends_with("/qapp-example-com.desktop"));
        assert!(paths.icon_file.ends_with("/example-com.png"));
        assert!(paths.metadata_file.ends_with("/example-com.json"));
    }

    #[test]
    fn resolve_svg_ext() {
        let paths = resolve_app_paths("test-app", "svg").unwrap();
        assert!(paths.icon_file.ends_with("/test-app.svg"));
    }

    #[test]
    fn resolve_ico_ext() {
        let paths = resolve_app_paths("test-app", "ico").unwrap();
        assert!(paths.icon_file.ends_with("/test-app.ico"));
    }

    #[test]
    fn empty_app_id_fails() {
        let result = resolve_app_paths("", "png");
        assert!(result.is_err());
    }

    #[test]
    fn paths_under_local_share() {
        let paths = resolve_app_paths("app", "png").unwrap();
        assert!(paths.applications_dir.contains("/applications"));
        assert!(paths.icons_dir.contains("/qapp-framework/icons"));
        assert!(paths.apps_dir.contains("/qapp-framework/apps"));
    }
}
