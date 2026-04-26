[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/b842RB8g)

# Nimonspoli

Tugas Besar 1 IF2010 Pemrograman Berorientasi Objek 2025/2026.

Nimonspoli adalah permainan papan berbasis C++ yang terinspirasi dari Monopoly. Pemain dapat membeli dan mengelola properti, membayar sewa, membangun rumah/hotel, menggadaikan aset, mengikuti lelang, mengambil kartu, membayar pajak, masuk penjara, menyimpan/memuat permainan, dan menentukan pemenang berdasarkan kondisi akhir permainan.

## Identitas

- Kode kelompok: MPL
- Nama kelompok: Monopoli777
- Anggota:
  - 13524014 / Yusuf Faishal Listyardi
  - 13524025 / Moh. Hafizh Irham Perdana
  - 13524066 / Nathanael Gunawan
  - 13524070 / A. Fawwaz Azam Wicaksono
  - 13524090 / Nashiruddin Akram

## Fitur Utama

- CLI interaktif dengan command sesuai spesifikasi.
- GUI berbasis Qt Widgets untuk memainkan game melalui tampilan papan visual.
- Papan permainan dari file konfigurasi.
- Bonus papan dinamis dengan variasi konfigurasi 20 dan 60 petak.
- Properti Street, Railroad, dan Utility.
- Sistem sewa, monopoli color group, pembangunan rumah/hotel, gadai, dan tebus.
- Kartu Kesempatan, Dana Umum, dan Kartu Kemampuan Spesial.
- Lelang properti otomatis.
- Pajak, festival sewa, penjara, dan bebas parkir.
- Bankruptcy handling dengan liquidation planner.
- Save/load state permainan.
- Transaction logger untuk histori aksi pemain.
- Error handling berbasis exception.

## Struktur Folder

```text
.
|-- config/                 # Konfigurasi default papan 40 petak
|-- config_dynamic/         # Contoh konfigurasi papan dinamis
|   |-- board20/
|   `-- board60/
|-- data/                   # Save file permainan
|-- gui/                    # Aplikasi GUI Qt
|   |-- include/
|   |-- src/
|   |-- images/
|   |-- househotels/
|   `-- CMakeLists.txt
|-- include/                # Header core, model, view CLI, utils
|-- src/                    # Implementasi CLI dan core game
|-- makefile
`-- README.md
```

Ringkasan layer:

- User Interaction Layer: `GameUI`, `BoardRenderer`, `PropertyCardRenderer`, `GameWindow`, `BoardWidget`, `QtGameIO`.
- Game Logic Layer: `GameEngine`, `CommandProcessor`, `Board`, `TurnManager`, `AuctionManager`, `BankruptcyHandler`, `AssetManager`, `Tile`, `Card`, `Player`.
- Data Access Layer: `ConfigLoader`, `SaveManager`, `GameStateMapper`, `TransactionLogger`, dan kelas state/config.

## Requirement

Umum:

- C++17
- `make`
- Compiler C++ yang kompatibel, misalnya `g++`

Untuk GUI:

- CMake 3.16 atau lebih baru
- Qt 6 atau Qt 5 dengan komponen:
  - Qt Widgets
  - Qt Svg
  - Qt SvgWidgets
- MinGW 64-bit di Windows, atau paket Qt development di Linux/WSL

## Instalasi Dependency

### Linux / WSL Ubuntu-Debian

```bash
sudo apt update
sudo apt install -y build-essential make cmake qt6-base-dev qt6-svg-dev
```

### Windows PowerShell

Install CMake dan Git:

```powershell
winget install --id Kitware.CMake -e
winget install --id Git.Git -e
```

Install Qt Online Installer:

```powershell
Invoke-WebRequest https://download.qt.io/official_releases/online_installers/qt-online-installer-windows-x64-online.exe -OutFile qt-online-installer.exe
.\qt-online-installer.exe
```

Pada Qt Installer, pilih Qt MinGW 64-bit dan komponen Qt Widgets, Qt Svg, serta Qt SvgWidgets. Jika compiler Qt belum terbaca, tambahkan folder MinGW Qt ke `PATH`, contoh:

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;$env:Path"
```

## Build dan Run

### CLI

```bash
make run
```

Executable CLI akan dibuat di:

```text
bin/<platform>/game
```

### GUI

```bash
make gui-run
```

atau compile CLI dan GUI lalu jalankan GUI:

```bash
make BuildRun
```

Executable GUI akan dibuat di:

```text
gui/build/<platform>/nimonspoli_qt_ui
```

### Target Makefile

| Target | Fungsi |
| --- | --- |
| `make all` | Build CLI dan GUI |
| `make cli` | Build CLI saja |
| `make run` | Build dan jalankan CLI |
| `make gui-build` | Build GUI saja |
| `make gui-run` | Build dan jalankan GUI |
| `make BuildRun` | Build CLI + GUI, lalu jalankan GUI |
| `make clean` | Hapus output CLI platform aktif |
| `make clean-gui` | Hapus output build GUI |
| `make clean-all` | Hapus output CLI dan GUI |
| `make clean-legacy` | Hapus output build lama yang tercampur platform |
| `make rebuild` | Clean lalu build ulang |
| `make help` | Tampilkan ringkasan target |

## Menjalankan Program

Saat program CLI dijalankan, pengguna akan memilih:

1. New Game
2. Load Game

Pada New Game, program meminta folder konfigurasi. Jika input dikosongkan, folder `config/` digunakan. Untuk bonus papan dinamis, masukkan salah satu folder berikut:

```text
config_dynamic/board20
config_dynamic/board60
```

Setelah konfigurasi valid, program meminta jumlah pemain, username pemain, lalu permainan dimulai.

Pada Load Game, gunakan command `MUAT <filename>` sesuai mekanisme save/load. Save file disimpan melalui `SaveManager` di folder `data/`.

## Command CLI

| Command | Keterangan |
| --- | --- |
| `HELP` | Menampilkan bantuan command yang tersedia |
| `CETAK_PAPAN` | Menampilkan papan permainan |
| `LEMPAR_DADU` | Melempar dadu |
| `ATUR_DADU X Y` | Mengatur nilai dadu manual untuk testing |
| `CETAK_AKTA KODE` | Menampilkan kartu akta properti berdasarkan kode |
| `CETAK_PROPERTI` | Menampilkan daftar properti pemain |
| `GADAI` | Menggadaikan properti ke bank |
| `TEBUS` / `UNMORTGAGE` | Menebus properti yang sedang digadaikan |
| `BANGUN` | Membangun rumah atau upgrade hotel |
| `BAYAR_DENDA` | Membayar denda keluar penjara |
| `GUNAKAN_KEMAMPUAN` | Menggunakan kartu kemampuan spesial |
| `SIMPAN filename` | Menyimpan state permainan |
| `MUAT filename` | Memuat state permainan |
| `CETAK_LOG [n]` | Menampilkan semua atau n log terakhir |
| `KELUAR` | Keluar dari permainan |

## Konfigurasi

Setiap folder konfigurasi minimal berisi:

| File | Fungsi |
| --- | --- |
| `property.txt` | Data properti, harga, nilai gadai, biaya bangun, dan tabel sewa |
| `aksi.txt` | Data petak aksi seperti GO, kartu, pajak, festival, penjara |
| `railroad.txt` | Tabel sewa Railroad berdasarkan jumlah Railroad milik owner |
| `utility.txt` | Faktor pengali Utility berdasarkan jumlah Utility milik owner |
| `tax.txt` | Besaran PPH flat, PPH persentase, dan PBM |
| `special.txt` | Gaji GO dan denda penjara |
| `misc.txt` | Batas turn dan saldo awal pemain |

Validasi papan dinamis:

- Jumlah petak minimal 20 dan maksimal 60.
- Jumlah petak harus kelipatan 4 agar dapat dirender sebagai papan persegi.
- Harus ada tepat 1 petak GO dan 1 petak PEN.
- Semua kode properti/aksi yang dipakai harus terdefinisi.
- Tidak boleh ada ID petak duplikat atau petak kosong.

## Mekanisme Game

### Properti

- Street dapat dibeli ketika pemain mendarat di petak milik bank.
- Railroad dan Utility otomatis menjadi milik pemain pertama yang mendarat.
- Properti berstatus `MORTGAGED` tidak menghasilkan sewa.
- Street dapat dibangun jika pemain memonopoli seluruh color group dan semua properti group aktif/tidak digadaikan.
- Pembangunan rumah harus merata antarpetak dalam satu color group.
- Upgrade hotel dapat dilakukan setelah seluruh petak dalam color group memiliki 4 rumah.

### Gadai dan Tebus

- Properti yang masih memiliki bangunan pada color group yang sama harus menjual seluruh bangunan group terlebih dahulu sebelum digadaikan.
- Pemain menerima nilai gadai sesuai akta.
- Untuk menebus, pemain membayar harga beli penuh properti.

### Lelang

Lelang berjalan otomatis ketika properti Street tidak dibeli atau ketika aset pemain bangkrut ke bank perlu dilelang. Pemain dapat memilih pass atau bid. Pemenang lelang membayar bid tertinggi dan menerima properti.

### Penjara

Pemain dapat masuk penjara karena:

- Mendarat di petak Pergi ke Penjara.
- Mendapat kartu masuk penjara.
- Mendapat double tiga kali berturut-turut.

Pemain di penjara dapat mencoba keluar dengan double atau membayar denda melalui `BAYAR_DENDA`.

### Bangkrut dan Likuidasi

Jika pemain tidak mampu membayar kewajiban, sistem menghitung potensi likuidasi aset. Pemain dapat menjual properti ke bank atau menggadaikan aset yang memenuhi syarat. Jika total aset tetap tidak cukup, pemain dinyatakan bangkrut:

- Jika kreditur adalah pemain lain, aset dialihkan ke kreditur.
- Jika kreditur adalah bank, aset dikembalikan ke bank dan dapat dilelang.

### Kondisi Menang

Permainan selesai ketika:

- Batas maksimum turn tercapai.
- Hanya satu pemain yang belum bangkrut.

Pada akhir permainan, pemenang ditentukan berdasarkan kekayaan/rekap pemain atau pemain terakhir yang tersisa.

## GUI Qt

GUI menggunakan core game yang sama dengan CLI melalui abstraksi `GameIO`. Komponen penting:

- `GuiGameSession`: controller sesi GUI dan penghubung ke core game.
- `QtGameIO`: implementasi input/output GUI untuk dialog, popup, bid, pajak, kartu, dan pemilihan tile.
- `GameWindow`: shell utama aplikasi.
- `BoardWidget`: custom-painted board yang menampilkan petak, pion, bangunan, owner, mortgage, dan festival.
- `PropertyCardWidget`: tampilan kartu akta properti.
- `PropertyPortfolioWidget`: tampilan portofolio aset pemain.

GUI menyediakan popup untuk aksi penting, termasuk kartu, pembayaran, error handling, masuk penjara, dan bangkrut.

## Save dan Load

Save file disimpan di folder `data/` dalam format teks. State yang disimpan meliputi:

- Turn dan batas turn.
- Urutan giliran.
- Data pemain, saldo, posisi, status penjara, dan kartu di tangan.
- Data properti, owner, status gadai, bangunan, festival.
- State deck kartu.
- Log transaksi.
- Path konfigurasi yang digunakan.

Contoh:

```text
SIMPAN game_sesi1.txt
MUAT game_sesi1.txt
```

## Penerapan OOP

Konsep OOP yang digunakan:

- Inheritance dan polymorphism pada `Tile`, `PropertyTile`, `ActionTile`, `ActionCard`, dan `SkillCard`.
- Abstract class dan virtual function melalui `Tile::onLanded()` dan `PropertyTile::calculateRent()`.
- Operator overloading pada `Player` untuk operasi uang dan perbandingan kekayaan.
- Generic class `CardDeck<T>` untuk deck ActionCard dan SkillCard.
- STL `vector` untuk board, player list, properti, kartu, log, dan state.
- STL `map` untuk lookup properti, tabel sewa Railroad, multiplier Utility, dan mapping konfigurasi.
- Exception hierarchy untuk validasi command, config, uang tidak cukup, deck kosong, kartu penuh, dan error game init.

## Testing Manual

Beberapa skenario yang dapat diuji:

- New game dengan 2-4 pemain.
- Membeli Street dan memicu lelang jika tidak membeli.
- Railroad/Utility otomatis berpindah kepemilikan saat pertama diinjak.
- Monopoli color group, build rumah, upgrade hotel.
- Gadai properti dan tebus properti.
- Validasi build saat ada properti group yang digadaikan.
- Bayar sewa hingga memicu likuidasi/bangkrut.
- Pajak PPH/PBM.
- Festival pengganda sewa.
- Masuk penjara dari tile, kartu, dan double 3x.
- Save lalu load game.
- Papan dinamis `board20` dan `board60`.

## Troubleshooting

### WSL/Qt GUI tidak tampil atau OpenGL bermasalah

Makefile menjalankan GUI Linux dengan environment berikut:

```bash
QT_OPENGL=software LIBGL_ALWAYS_SOFTWARE=1 MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
```

Jika GUI masih gagal, pastikan WSL mendukung aplikasi GUI atau jalankan versi Windows dengan Qt MinGW.

### Error build campuran Windows/Linux

Jika muncul error object lama atau relocation, bersihkan output lama:

```bash
make clean-legacy
make clean-all
make BuildRun
```

### CMake tidak ditemukan

Install CMake sesuai OS:

```bash
sudo apt install -y cmake
```

atau Windows:

```powershell
winget install --id Kitware.CMake -e
```

### Qt tidak ditemukan di Windows

Pastikan Qt MinGW terinstall dan path Qt dapat ditemukan oleh CMake. `gui/CMakeLists.txt` mencoba mendeteksi beberapa lokasi umum seperti:

```text
C:/Qt/6.x.x/mingw_64
C:/msys64/ucrt64
C:/msys64/mingw64
```

Jika Qt berada di lokasi lain, set `CMAKE_PREFIX_PATH` atau `QTDIR`.

## Lampiran Laporan

README ini dapat digunakan sebagai lampiran ringkas untuk laporan M2 karena memuat:

- Identitas tim.
- Ringkasan fitur wajib dan bonus.
- Struktur folder dan arsitektur layer.
- Cara build/run CLI dan GUI.
- Format konfigurasi.
- Daftar command.
- Ringkasan mekanisme utama.
- Catatan troubleshooting.
