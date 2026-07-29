import os

file_path = "/home/efrail/Projeler/veyon/plugins/accesscontrol/AccessControlPlugin.cpp"

with open(file_path, "r", encoding="utf-8") as f:
    content = f.read()

# Replace the empty icons
content = content.replace(
    'tr("Click this button to block internet access on selected computers."),\n\t\t\t\t\t\t   QStringLiteral(""))',
    'tr("Click this button to block internet access on selected computers."),\n\t\t\t\t\t\t   QStringLiteral(":/core/toast-error.png"))'
)

content = content.replace(
    'tr("Click this button to block restricted applications on selected computers."),\n\t\t\t\t\t   QStringLiteral(""))',
    'tr("Click this button to block restricted applications on selected computers."),\n\t\t\t\t\t   QStringLiteral(":/core/edit-delete.png"))'
)

content = content.replace(
    'tr("Click this button to block USB storage devices on selected computers."),\n\t\t\t\t\t   QStringLiteral(""))',
    'tr("Click this button to block USB storage devices on selected computers."),\n\t\t\t\t\t   QStringLiteral(":/core/media-playback-stop.png"))'
)

with open(file_path, "w", encoding="utf-8") as f:
    f.write(content)
print("Icons updated in AccessControlPlugin.cpp")
