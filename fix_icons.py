import os
import re

plugins = {
    "chat": ":/core/user-group-new.png",
    "gamification": ":/core/toast-success.png",
    "appblocker": ":/core/toast-error.png",
    "kioskmode": ":/core/view-fullscreen.png",
    "quickquiz": ":/core/help-about.png",
    "randompicker": ":/core/media-playback-start.png",
    "focustracker": ":/core/edit-find.png",
    "annotation": ":/core/document-edit.png",
    "usblock": ":/core/toast-warning.png",
    "inventory": ":/core/list-add.png"
}

base_dir = "/home/efrail/Projeler/veyon/plugins"

for plugin, icon_path in plugins.items():
    file_path = os.path.join(base_dir, plugin, f"{plugin.capitalize()}Plugin.cpp")
    if not os.path.exists(file_path):
        # some plugins might have different casing, try to find it
        files = [f for f in os.listdir(os.path.join(base_dir, plugin)) if f.endswith("Plugin.cpp")]
        if files:
            file_path = os.path.join(base_dir, plugin, files[0])
        else:
            continue
            
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Find the m_xxxFeature(..., QStringLiteral("...")) pattern
    # It might span multiple lines. The icon is the last argument before the closing parenthesis.
    
    # We will just replace all QStringLiteral(":/...") or QStringLiteral("") or {}
    # inside the Feature(...) constructor?
    # It's safer to use regex to find the string just before the `), m_features` or `))`
    
    # Actually, let's just do a simple replace since I know what I put in them:
    
    # Replace any icon path with the correct one
    # The pattern looks like: tr("..."), \s*QStringLiteral("...")\)
    
    new_content = re.sub(
        r'(tr\("[^"]+"\)),\s*(?:\{\}|QStringLiteral\("[^"]*"\))\s*\)',
        fr'\1, QStringLiteral("{icon_path}"))',
        content,
        flags=re.MULTILINE
    )
    
    # Randompicker case might be:
    new_content = re.sub(
        r'(tr\("[^"]+"\)),\s*QStringLiteral\(":/core/dialog-information.png"\)\)',
        fr'\1, QStringLiteral("{icon_path}"))',
        new_content
    )
    
    # Chat case:
    new_content = re.sub(
        r'(tr\("[^"]+"\)),\s*QStringLiteral\(":/textmessage/dialog-information.png"\)\)',
        fr'\1, QStringLiteral("{icon_path}"))',
        new_content
    )
    
    # Kioskmode case might have no description maybe?
    # Let's just find the m_xxxFeature constructor explicitly
    
    if new_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(new_content)
        print(f"Updated icon for {plugin} to {icon_path}")
    else:
        # Check if already correct
        if f'QStringLiteral("{icon_path}")' in content:
            print(f"{plugin} already has the correct icon.")
        else:
            print(f"Could not automatically update {plugin}. Manual review needed.")
