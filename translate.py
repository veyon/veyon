import xml.etree.ElementTree as ET

translations = {
    "About Veyon %1": "Veyon Hakkında %1",
    "Move selected computer up": "Seçilen bilgisayarı yukarı taşı",
    "Move selected computer down": "Seçilen bilgisayarı aşağı taşı",
    "Move selected location up": "Seçilen konumu yukarı taşı",
    "Move selected location down": "Seçilen konumu aşağı taşı",
    "Veyon Service %1 at %2:%3": "%2:%3 adresinde Veyon Servisi %1",
    "No default network object directory plugin was found. Please check your installation or configure a different network object directory backend via Veyon Configurator.": "Varsayılan ağ nesnesi dizini eklentisi bulunamadı. Lütfen kurulumunuzu kontrol edin veya Veyon Configurator üzerinden farklı bir ağ nesne dizini ayarlayın.",
    "Could not modify the autostart property for the Veyon Service.": "Veyon Servisi için otomatik başlatma özelliği değiştirilemedi.",
    "Could not configure the firewall configuration for the Veyon Server.": "Veyon Sunucusu için güvenlik duvarı yapılandırılamadı.",
    "Could not configure the firewall configuration for the Veyon Worker.": "Veyon Worker (İşçi) için güvenlik duvarı yapılandırılamadı.",
    "Veyon Demo": "Veyon Demo",
    "Subfolder handling": "Alt klasör işleme",
    "*.* or *.docx;*.pdf (leave empty for all files)": "*.* veya *.docx;*.pdf (tüm dosyalar için boş bırakın)",
    "File pattern": "Dosya deseni",
    "Local destination directory": "Yerel hedef dizini",
    "Relative (Documents/) or absolute (/tmp/ or C:\\TMP) or empty for configured directory": "Göreceli (Belgeler/) veya mutlak (/tmp/ veya C:\\TMP) ya da yapılandırılmış dizin için boş bırakın",
    "Source directory on remote computers": "Uzak bilgisayarlardaki kaynak dizin",
    "Files in source directory only": "Sadece kaynak dizindeki dosyalar",
    "Files in source directory and subdirectories": "Kaynak dizindeki ve alt dizinlerdeki dosyalar",
    "Open output directory": "Çıktı dizinini aç",
    "Number of files": "Dosya sayısı",
    "First part of user name": "Kullanıcı adının ilk kısmı",
    "Last part of user name": "Kullanıcı adının son kısmı",
    "Grouping attribute 3:": "Gruplama özniteliği 3:",
    "Destination directory:": "Hedef dizini:",
    "Store collected files in:": "Toplanan dosyaları şuraya kaydet:",
    "Group collected files:": "Toplanan dosyaları gruplandır:",
    "Grouping attribute 1:": "Gruplama özniteliği 1:",
    "Grouping attribute 2:": "Gruplama özniteliği 2:",
    "Directly in the destination directory": "Doğrudan hedef dizine",
    "Files to collect:": "Toplanacak dosyalar:",
    "Files to exclude:": "Hariç tutulacak dosyalar:",
    "e.g. *.lnk or *.desktop": "örn. *.lnk veya *.desktop",
    "Destination directory (remote):": "Hedef dizini (uzak):",
    "Default source directory:": "Varsayılan kaynak dizini:",
    "Source directory (remote):": "Kaynak dizini (uzak):",
    "Could not open file %1 for reading! Please check your permissions!": "%1 dosyası okumak için açılamadı! Lütfen izinlerinizi kontrol edin!",
    "Destination directory on remote computers:": "Uzak bilgisayarlardaki hedef dizin:",
    "Relative (Desktop/) or absolute (/tmp/ or C:\\TMP) or empty for configured directory": "Göreceli (Masaüstü/) veya mutlak (/tmp/ veya C:\\TMP) ya da yapılandırılmış dizin için boş bırakın",
    "Received file %1.": "%1 dosyası alındı.",
    "The file %1 is to be collected, but is still open in an application.": "%1 dosyası toplanacak ancak bir uygulamada hala açık.",
    "The file %1 is to be collected, but is still open in the application <b>%2</b>.": "%1 dosyası toplanacak ancak <b>%2</b> uygulamasında hala açık.",
    "Please save your changes and close the program so that the transfer can be completed.": "Lütfen değişikliklerinizi kaydedin ve aktarımın tamamlanabilmesi için programı kapatın.",
    "Are you sure you want to skip transferring the file %1?": "%1 dosyasını aktarmayı atlamak istediğinizden emin misiniz?",
    "Could not receive file %1 as it already exists.": "%1 dosyası zaten mevcut olduğundan alınamadı.",
    "Could not receive file %1 as it could not be opened for writing!": "%1 dosyası yazmak için açılamadığından alınamadı!",
    "Veyon service": "Veyon servisi",
    "The Veyon service needs to be stopped temporarily in order to remove the log files. Continue?": "Günlük dosyalarını kaldırmak için Veyon servisinin geçici olarak durdurulması gerekiyor. Devam edilsin mi?",
    "LDAP/AD support for Veyon": "Veyon için LDAP/AD desteği",
    "Key press interval for text input": "Metin girişi için tuş basma aralığı",
    "Key press interval to control input fields": "Giriş alanlarını kontrol etmek için tuş basma aralığı",
    "Veyon Configurator %1": "Veyon Yapılandırıcısı (Configurator) %1",
    "The local configuration backend reported that the configuration is not writable! Please run Veyon Configurator with higher privileges.": "Yerel yapılandırma arka ucu, yapılandırmanın yazılamaz olduğunu bildirdi! Lütfen Veyon Configurator'ü yönetici (root/admin) haklarıyla çalıştırın.",
    "No authentication key files were found or your current ones are outdated. Please create new key files using Veyon Configurator. Alternatively set up logon authentication using Veyon Configurator. Otherwise you won't be able to access computers using Veyon.": "Kimlik doğrulama anahtar dosyaları bulunamadı veya mevcut olanlar güncel değil. Lütfen Veyon Configurator'ü kullanarak yeni anahtar dosyaları oluşturun veya oturum açma kimlik doğrulamasını ayarlayın. Aksi takdirde, Veyon kullanarak bilgisayarlara erişemezsiniz.",
    "The feature \"%1\" is still active. Please s": "\"%1\" özelliği hala etkin. Lütfen",
    "The feature \"%1\" is still active. Please stop it before closing Veyon.": "\"%1\" özelliği hala etkin. Lütfen Veyon'u kapatmadan önce o özelliği durdurun.",
    "Identify users in guest sessions": "Misafir oturumlarındaki kullanıcıları tanımla",
    "Never": "Asla",
    "If login name matches": "Kullanıcı adı eşleşirse",
    "If full name matches": "Tam adı eşleşirse",
    "Guest": "Misafir",
    "Guest user identity extension": "Misafir kullanıcı kimliği uzantısı",
    "Prefix": "Önek",
    "Suffix": "Sonek",
    "Identification request": "Kimlik tanımlama isteği",
    "Please enter your name:": "Lütfen adınızı girin:",
    "First name + last name": "Ad + Soyad",
    "Wayland VNC server (PipeWire/XDG Desktop Portal)": "Wayland VNC sunucusu (PipeWire/XDG Desktop Portal)",
    "%1 - Veyon Remote Access": "%1 - Veyon Uzaktan Erişim",
    "%1 - %2 - Veyon Remote Access": "%1 - %2 - Veyon Uzaktan Erişim",
    "All settings were saved successfully. In order to take effect the Veyon service needs to be restarted. Restart it now?": "Tüm ayarlar başarıyla kaydedildi. Değişikliklerin geçerli olabilmesi için Veyon servisinin yeniden başlatılması gerekiyor. Şimdi yeniden başlatılsın mı?",
    "Veyon Master – Slideshow": "Veyon Master – Slayt Gösterisi",
    "Could not save your personal settings! Please check the user configuration file path using Veyon Configurator.": "Kişisel ayarlarınız kaydedilemedi! Lütfen Veyon Configurator'ü kullanarak kullanıcı yapılandırma dosya yolunu kontrol edin.",
    "Use custom power scheme with disabled power and sleep buttons": "Güç ve uyku düğmeleri devre dışı bırakılmış özel güç planı kullan",
    "Disable touchpads and touchscreens": "Dokunmatik yüzeyleri ve dokunmatik ekranları devre dışı bırak",
    "Disable keyboard devices": "Klavye cihazlarını devre dışı bırak",
    "Disable mouse devices": "Fare cihazlarını devre dışı bırak"
}

tree = ET.parse('/home/efrail/Projeler/veyon/translations/veyon_tr.ts')
root = tree.getroot()

modified_count = 0
for context in root.findall('context'):
    for message in context.findall('message'):
        translation = message.find('translation')
        if translation is not None and translation.get('type') == 'unfinished':
            source = message.find('source')
            if source is not None and source.text in translations:
                translation.text = translations[source.text]
                del translation.attrib['type']
                modified_count += 1

tree.write('/home/efrail/Projeler/veyon/translations/veyon_tr.ts', encoding='utf-8', xml_declaration=True)
print(f"Successfully translated {modified_count} strings.")
