[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/b842RB8g)

# Tugas Besar 1 IF2010 Pemrograman Berorientasi Objek

Project Structure ini hanyalah referensi, Anda dapat menyesuaikannya dengan kebutuhan tim Anda

## Requirement GUI

Untuk menggunakan versi GUI Qt, siapkan hal-hal berikut:

- Qt 6 untuk MinGW 64-bit, dengan modul `Qt Widgets`, `Qt Svg`, dan `Qt SvgWidgets`
- MinGW toolchain yang kompatibel dengan instalasi Qt
- CMake versi 3.16 atau lebih baru

## Install GUI

PowerShell Windows:

```powershell
winget install --id Kitware.CMake -e
winget install --id Git.Git -e
Invoke-WebRequest https://download.qt.io/official_releases/online_installers/qt-online-installer-windows-x64-online.exe -OutFile qt-online-installer.exe
.\qt-online-installer.exe
```

Di Qt Installer, pilih Qt 6 MinGW 64-bit serta komponen `Qt Widgets`, `Qt Svg`, dan `Qt SvgWidgets`. Setelah install, tambahkan MinGW Qt ke PATH, contoh:

```powershell
$env:Path = "C:\Qt\Tools\mingw1310_64\bin;$env:Path"
```

Linux Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential make cmake qt6-base-dev qt6-svg-dev
```

Kalau menjalankan dari WSL seperti `/mnt/c/...`, command Linux di atas wajib dijalankan dulu di terminal WSL.

## Makefile

PowerShell Windows:

```powershell
mingw32-make BuildRun
```

Linux/Git Bash:

```bash
make BuildRun
```

Target yang tersedia:

- `make all` compile CLI dan GUI.
- `make run` compile lalu jalankan CLI.
- `make BuildRun` compile CLI + GUI lalu jalankan GUI.
- `make gui-build` compile GUI saja.
- `make gui-run` compile lalu jalankan GUI saja.
- `make clean-all` hapus hasil build CLI dan GUI.
- `make clean-legacy` hapus object lama yang mungkin tercampur Windows/Linux.

Jika di WSL muncul error seperti `dangerous relocation: R_AMD64_IMAGEBASE`, jalankan:

```bash
make clean-legacy
make BuildRun
```
