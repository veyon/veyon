import os
import re

plugins_dir = "/home/efrail/Projeler/veyon/plugins"
for root, dirs, files in os.walk(plugins_dir):
    for file in files:
        if file.endswith("Plugin.cpp"):
            path = os.path.join(root, file)
            with open(path, "r", encoding="utf-8") as f:
                content = f.read()
                # Find the m_xxxFeature(QStringLiteral("..."), ...) pattern
                matches = re.finditer(r'm_[a-zA-Z]+Feature\s*\(\s*QStringLiteral.*?\);', content, re.DOTALL)
                for match in matches:
                    print(f"--- {file} ---")
                    print(match.group(0))
