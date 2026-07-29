import os

# Update AccessControlPlugin
ac_file = "/home/efrail/Projeler/veyon/plugins/accesscontrol/AccessControlPlugin.cpp"
with open(ac_file, "r", encoding="utf-8") as f:
    ac_content = f.read()

ac_content = ac_content.replace('QStringLiteral(":/core/toast-error.png"))', 'QStringLiteral(":/core/block-internet.png"))')
ac_content = ac_content.replace('QStringLiteral(":/core/edit-delete.png"))', 'QStringLiteral(":/core/block-apps.png"))')
ac_content = ac_content.replace('QStringLiteral(":/core/media-playback-stop.png"))', 'QStringLiteral(":/core/block-usb.png"))')

with open(ac_file, "w", encoding="utf-8") as f:
    f.write(ac_content)


# Update SurveyPlugin
survey_file = "/home/efrail/Projeler/veyon/plugins/survey/SurveyPlugin.cpp"
with open(survey_file, "r", encoding="utf-8") as f:
    survey_content = f.read()

# Replace empty icon or any icon in SurveyPlugin
import re
survey_content = re.sub(
    r'(tr\("[^"]+"\)),\s*(?:\{\}|QStringLiteral\("[^"]*"\))\s*\)',
    r'\1, QStringLiteral(":/core/survey.png"))',
    survey_content,
    flags=re.MULTILINE
)

with open(survey_file, "w", encoding="utf-8") as f:
    f.write(survey_content)
    
print("Updated icons")
