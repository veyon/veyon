# Insight Teacher - Sistem Manajemen Laboratorium Komputer
### SMA Muhammadiyah 1 Palembang

Aplikasi monitoring, demonstrasi interaktif, pengawasan, dan manajemen laboratorium komputer performa tinggi berbasis C++ / Qt6 & Windows Background Service, dikustomisasi secara khusus untuk **SMA Muhammadiyah 1 Palembang** dengan basis inti Veyon v4.8.3.

---

## 🌟 Fitur Utama (Fase 1 Core)

1. **Pemantauan Layar Real-time (Computer Monitoring)**:
   - Pantau seluruh layar PC siswa secara serentak dalam bentuk tata letak thumbnail adaptif.
   - Deteksi nama komputer dan nama user aktif secara instan.
2. **Kendali Jarak Jauh (Full HD Remote Control)**:
   - Buka layar siswa dalam resolusi penuh untuk memberikan bantuan langsung.
   - Kendali penuh pointer mouse dan input keyboard secara mulus dan responsif.
3. **Demonstrasi Layar Guru (Teacher Screen Broadcast / Demo)**:
   - Siaran layar guru ke seluruh PC siswa di lab secara real-time.
   - Pilihan mode: **Layar Penuh Terkunci (Fullscreen)** atau **Mode Jendela Apung (Window)** agar siswa dapat mempraktikkan materi secara langsung.
4. **Pengunci Layar Perhatian (School-Branded Attention Lock Screen)**:
   - Kunci layar siswa dengan tampilan resmi SMA Muhammadiyah 1 Palembang (Hijau Muhammadiyah `#0A5C36` dan Emas `#F59E0B`).
   - Pesan resmi: *"LAYAR TERKUNCI - Harap Perhatikan Instruksi Guru di Depan Kelas"*.
   - Mengunci input mouse & keyboard siswa agar fokus penuh ke penjelasan di depan kelas.
5. **Kontrol Akses Internet (Block Web & Internet)**:
   - Satu tombol cepat untuk memutus atau mengizinkan akses browsing siswa.
6. **Distribusi Bahan Ajar (File Transfer)**:
   - Kirim materi modul latihan, file dokumen, atau starter code ke folder dokumen/desktop siswa secara massal.
7. **Komunikasi Kelas (Text Chat & Broadcast Messages)**:
   - Kirim pesan teks pengumuman penting langsung ke layar komputer siswa.
8. **Manajemen Daya Lab (Power Control & WoL)**:
   - Menghidupkan komputer lab via **Wake-on-LAN (WoL)**.
   - **Reboot Massal** saat pergantian sesi kelas.
   - **Shutdown Massal** seluruh PC lab secara serentak dan aman saat jam pulang sekolah.
   - **Logoff Windows Massal**.
9. **Zero-Config Deployment (Autentikasi Otomatis)**:
   - Installer Windows (.exe) telah menyematkan kunci autentikasi bawaan sekolah (*pre-configured RSA key pair*) dan ruangan *"Lab Komputer SMA Muhammadiyah 1 Palembang"*. Begitu diinstal di PC Siswa, PC langsung terhubung ke konsol Guru tanpa perlu ekspor-impor kunci manual di 40 PC.

---

## 🏗️ Struktur Repositori

```
Insight Teacher/
├── core/                        # Library inti Insight (C++/Qt, networking, security)
├── master/                      # Aplikasi Konsol Guru (Insight Teacher)
├── service/                     # Background Service PC Siswa (Insight Student)
├── configurator/                # Insight Konfigurator Lab
├── plugins/                     # Plugin modul (screenlock, demo, filetransfer, chat, dll)
├── default-keys/                # Pasangan kunci RSA bawaan laboratorium sekolah
├── nsis/                        # Script installer Windows (.nsi) & visual branding
├── translations/                # File lokalisasi Bahasa Indonesia (veyon_id.ts)
├── .github/workflows/build.yml  # Pipeline CI/CD kompilasi otomatis installer Windows
└── legacy-dotnet/               # Arsip proyek sistem .NET C# sebelumnya (referensi Fase 2)
```

---

## 🚀 Alur Kompilasi (Build Windows Installer)

Proyek ini telah dikonfigurasi dengan alur **Hybrid CI/CD**:

1. **Kompilasi Otomatis via GitHub Actions (Sangat Disarankan)**:
   - Cukup dorong (*git push*) perubahan kode ke repositori GitHub Anda.
   - GitHub Actions akan memicu workflow `.github/workflows/build.yml` menggunakan image builder Windows MinGW resmi (`veyon/ci-mingw-w64:4.8`).
   - File installer siap pakai: `InsightTeacher-Windows-Setup.exe` akan langsung tersedia untuk diunduh pada tab **Actions Artifacts / Releases**.

2. **Kompilasi Lokal (Manual)**:
   - Menggunakan MSYS2 atau MinGW-w64 x86_64 dengan Qt6 dan CMake.
   - Menjalankan script: `.ci/windows/build.sh x86_64`.
