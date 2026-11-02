# File map · 02.11.2026

| Файл | Зачем |
|---|---|
| `.github/workflows/public_release.yml` | Унаследованная автоматизация GitHub; не запускать публикацию. |
| `.github/workflows/versioning.yml` | Унаследованная автоматизация GitHub; не запускать публикацию. |
| `.gitignore` | Исключения git для локальных данных. |
| `CHANGELOG.md` | Унаследованная история версий. |
| `CMakeLists.txt` | RP2040 target и флаги сборки. |
| `Dockerfile` | Унаследованное окружение сборки. |
| `FILE-MAP.md` | Полный список файлов этого репозитория. |
| `LICENSE` | Лицензия GPL-3.0. |
| `Makefile` | Унаследованные Docker/release команды; не использовать release. |
| `PLSDK/.gitignore` | Служебный файл исходной базы. |
| `PLSDK/CMakeLists.txt` | Подключение исходников и библиотек этой папки. |
| `PLSDK/README.md` | Описание общей библиотеки Playtronica. |
| `PLSDK/include/PLSDK.h` | Объявления и константы модуля PLSDK. |
| `PLSDK/include/PLSDK/cap_buttons.h` | Объявления и константы модуля cap_buttons. |
| `PLSDK/include/PLSDK/commands.h` | Объявления и константы модуля commands. |
| `PLSDK/include/PLSDK/constants.h` | Объявления и константы модуля constants. |
| `PLSDK/include/PLSDK/music.h` | Объявления и константы модуля music. |
| `PLSDK/include/tusb_config.h` | Объявления и константы модуля tusb_config. |
| `PLSDK/src/PLSDK.c` | Реализация модуля PLSDK. |
| `PLSDK/src/cap_buttons.c` | Реализация модуля cap_buttons. |
| `PLSDK/src/commands.c` | Реализация модуля commands. |
| `PLSDK/src/music.c` | Реализация модуля music. |
| `PLSDK/src/usb_descriptors.c` | USB device/config/string descriptors; минимальный USB fix. |
| `README.md` | Вход в проект, параметры, проверки и ограничения. |
| `SettingsDescription.md` | CC/SysEx протокол настроек Biotron. |
| `USB-FIX-VALIDATION.md` | Источники, доказательства проверки, решение о двух базах. |
| `include/buttons.h` | Объявления и константы модуля buttons. |
| `include/global.h` | Объявления и константы модуля global. |
| `include/leds.h` | Объявления и константы модуля leds. |
| `include/music.h` | Объявления и константы модуля music. |
| `include/params.h` | Объявления и константы модуля params. |
| `include/raw_plant.h` | Объявления и константы модуля raw_plant. |
| `main.c` | Инициализация и главный цикл. |
| `pico_sdk_import.cmake` | Поиск и импорт Pico SDK. |
| `src/CMakeLists.txt` | Подключение исходников и библиотек этой папки. |
| `src/buttons.c` | Реализация модуля buttons. |
| `src/global.c` | Реализация модуля global. |
| `src/leds.c` | Реализация модуля leds. |
| `src/music.c` | Реализация модуля music. |
| `src/params.c` | Реализация модуля params. |
| `src/raw_plant.c` | Реализация модуля raw_plant. |
| `static/logo.png` | Унаследованное изображение логотипа. |
| `tools/build.sh` | Воспроизводимая Release сборка без обращения к устройству. |
| `tools/verify_uf2.py` | Независимый разбор UF2/USB и отрицательные самопроверки. |
| `validation/official-rollback-audit.json` | Результат разбора официального отката. |
| `validation/stable-audit.json` | Результат разбора точного F03 и сравнения с official. |
