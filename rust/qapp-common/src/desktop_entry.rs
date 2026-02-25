/// A shortcut action for the .desktop entry.
#[derive(Debug, Clone)]
pub struct DesktopAction {
    pub id: String,
    pub name: String,
    pub exec: String,
    pub icon: String,
}

/// Input for .desktop file generation.
#[derive(Debug, Clone, Default)]
pub struct DesktopEntryInput {
    pub name: String,
    pub exec: String,
    pub icon: String,
    pub comment: String,
    pub categories: String,
    pub mime_types: String,
    pub actions: Vec<DesktopAction>,
}

/// Generate a freedesktop .desktop entry file string.
pub fn generate(input: &DesktopEntryInput) -> crate::Result<String> {
    if input.name.is_empty() {
        return Err("Name is required".into());
    }
    if input.exec.is_empty() {
        return Err("Exec is required".into());
    }
    if input.icon.is_empty() {
        return Err("Icon is required".into());
    }

    let mut lines = vec![
        "[Desktop Entry]".to_string(),
        "Type=Application".to_string(),
        format!("Name={}", input.name),
        format!("Exec={}", input.exec),
        format!("Icon={}", input.icon),
        "Terminal=false".to_string(),
        "StartupNotify=true".to_string(),
    ];

    if !input.comment.is_empty() {
        lines.push(format!("Comment={}", input.comment));
    }

    let categories = if input.categories.is_empty() {
        "Network;WebBrowser;"
    } else {
        &input.categories
    };
    lines.push(format!("Categories={}", categories));

    if !input.mime_types.is_empty() {
        lines.push(format!("MimeType={}", input.mime_types));
    }

    if !input.actions.is_empty() {
        let action_ids: Vec<&str> = input.actions.iter().map(|a| a.id.as_str()).collect();
        lines.push(format!("Actions={};", action_ids.join(";")));

        for action in &input.actions {
            lines.push(String::new());
            lines.push(format!("[Desktop Action {}]", action.id));
            lines.push(format!("Name={}", action.name));
            lines.push(format!("Exec={}", action.exec));
            if !action.icon.is_empty() {
                lines.push(format!("Icon={}", action.icon));
            }
        }
    }

    // Trailing newline per freedesktop spec
    Ok(lines.join("\n") + "\n")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn basic_entry() {
        let input = DesktopEntryInput {
            name: "Test App".into(),
            exec: "/usr/bin/test".into(),
            icon: "/usr/share/icons/test.png".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("[Desktop Entry]"));
        assert!(result.contains("Name=Test App"));
        assert!(result.contains("Exec=/usr/bin/test"));
        assert!(result.contains("Icon=/usr/share/icons/test.png"));
        assert!(result.contains("Terminal=false"));
        assert!(result.contains("Categories=Network;WebBrowser;"));
    }

    #[test]
    fn with_comment() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            comment: "A test app".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("Comment=A test app"));
    }

    #[test]
    fn custom_categories() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            categories: "Office;Productivity;".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("Categories=Office;Productivity;"));
    }

    #[test]
    fn with_mime_types() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            mime_types: "image/png;text/plain;".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("MimeType=image/png;text/plain;"));
    }

    #[test]
    fn missing_name() {
        let input = DesktopEntryInput {
            exec: "app".into(),
            icon: "app.png".into(),
            ..Default::default()
        };
        assert!(generate(&input).is_err());
    }

    #[test]
    fn missing_exec() {
        let input = DesktopEntryInput {
            name: "App".into(),
            icon: "app.png".into(),
            ..Default::default()
        };
        assert!(generate(&input).is_err());
    }

    #[test]
    fn missing_icon() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            ..Default::default()
        };
        assert!(generate(&input).is_err());
    }

    #[test]
    fn with_actions() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            actions: vec![
                DesktopAction {
                    id: "new-window".into(),
                    name: "New Window".into(),
                    exec: "app --new-window".into(),
                    icon: String::new(),
                },
                DesktopAction {
                    id: "settings".into(),
                    name: "Settings".into(),
                    exec: "app --settings".into(),
                    icon: "settings.png".into(),
                },
            ],
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("Actions=new-window;settings;"));
        assert!(result.contains("[Desktop Action new-window]"));
        assert!(result.contains("Name=New Window"));
        assert!(result.contains("[Desktop Action settings]"));
        assert!(result.contains("Icon=settings.png"));
    }

    #[test]
    fn special_chars_in_name() {
        let input = DesktopEntryInput {
            name: "App & More <Test>".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.contains("Name=App & More <Test>"));
    }

    #[test]
    fn trailing_newline() {
        let input = DesktopEntryInput {
            name: "App".into(),
            exec: "app".into(),
            icon: "app.png".into(),
            ..Default::default()
        };
        let result = generate(&input).unwrap();
        assert!(result.ends_with('\n'));
    }
}
