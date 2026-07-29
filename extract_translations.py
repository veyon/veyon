import xml.etree.ElementTree as ET

tree = ET.parse('/home/efrail/Projeler/veyon/translations/veyon_tr.ts')
root = tree.getroot()

unfinished_count = 0
for context in root.findall('context'):
    context_name = context.find('name').text
    for message in context.findall('message'):
        translation = message.find('translation')
        if translation is not None and translation.get('type') == 'unfinished':
            source = message.find('source').text
            print(f"CTX: {context_name} | SRC: {source}")
            unfinished_count += 1

print(f"\nTotal unfinished: {unfinished_count}")
