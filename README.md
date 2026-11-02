# Biotron OP-XY USB Fix · 02.11.2026

Отдельная тестовая прошивка Biotron для OP-XY: только USB Fix на официальной базе 1.8.2. Не релиз.

## Pipeline
`Playtronica/biotron-firmware@67920c2 (tag 1.8.2)` → USB descriptor patch → Release RP2040 UF2 → независимый разбор UF2 + две чистые пересборки → ручной тест Андрея на OP-XY.

## Результат на 02.11.2026
F03 воспроизведён **байт в байт**, две чистые сборки совпали с исходным файлом:
`e8ac496120541013df1e1f6aee134286ed7e945fc4535276f0d518d34bb87e76`.
Официальный R03 независимо скачан с GitHub, совпадает:
`d2406d66225f76779331714b0a9df814d1c72d7f3640178f29adc9d3b45b4251`.

UF2/header/family/flash bounds/векторы/дескрипторы — PASS. MIDI0/1 → CDC2/3, EF/02/01; VID/PID CAFE/3011, два MIDI-кабеля, CDC, endpoints и 100мА сохранены. USB-функции побайтово совпадают с официальными после перестановки. Физические OP-XY/Mac/Windows, звук, настройки и откат — NOT_RUN.

## File map
Полный список каждого файла и его назначения: [FILE-MAP.md](FILE-MAP.md).
Главная правка: `PLSDK/src/usb_descriptors.c` — класс устройства и порядок двух групп.
`tools/build.sh` — фиксированные параметры и чистая сборка; `tools/verify_uf2.py` — проверка файла без USB-доступа.
[USB-FIX-VALIDATION.md](USB-FIX-VALIDATION.md) — доказательства, ограничения и отдельный выбор внутреннего кандидата.

## Данные
Сборки/ELF/логи/первичный GitHub JSON: `~/ProjectData/biotron-opxy-usb-fix-20261102/`.
Исходный F03/R03: `~/ProjectData/Playtronica/aiplay/playtron-opxy-usb-2026-11-02/delivery-current-r2/`.
SDK и compiler: `~/ProjectData/toolchains/`. В git нет бинарников, логов и секретов.

## Как собрать и проверить
Pico SDK2.3.0 `98a542c1a62fb549ffb5d66a3e5892b06276b670`, TinyUSB `86ad6e56c1700e85f1c5678607a762cfe3aa2f47`, Arm GNU13.2.Rel1, picotool2.3.0; cmake4.4.3/ninja1.13.2. SDK/TinyUSB рабочие деревья чистые при проверке.

```sh
./tools/build.sh stable "$HOME/ProjectData/biotron-opxy-usb-fix-20261102/my-stable-build"
python3 tools/verify_uf2.py --selftest
python3 tools/verify_uf2.py "$HOME/ProjectData/biotron-opxy-usb-fix-20261102/my-stable-build/biotron.uf2" --reference /absolute/path/R03_Biotron_ROLLBACK_1.8.2_official.uf2
```

Использовать новую папку сборки. Версия1.8.2, `FLASH_ID_STARTUP=1765723554`; дата `SOURCE_DATE_EPOCH=1793577600` (02.11.2026UTC) закреплена отдельно от settings ID. Первая проверка с датой2025 дала отличие только6байт метаданных даты; она сохранена как diagnostic build1, не выдаётся. build2 и build3 — точный F03. Обычный `make release` не использовать: он меняет settings ID.

## Safety rails
Прошивает только человек, по одному файлу. До BOOT сохранить пресеты: **в1.8.2 BOOT вызывает `clear_flash()`**. Сами UF2 не пишут в область settings от0x10080000; это не гарантия сохранения пресетов при BOOT. Исходная1.8.2 сохраняет старый дефект времени жизни serial string; этот USB-only patch его не меняет.

Не менять основной Playtronica repo/worktree родителя, музыкальные режимы, завод, stable/latest, settings.playtronica.com. Команде ничего не отправлять. Не создавать релизы и не шить устройство автоматически. Полный firmware release — только после явного «да» Сергея.
Внутренний кандидат1.10.11 рассматривается в отдельной ветке и не заменяет stable без выбора Андрея.

## Git
Независимая папка `~/Projects/biotron-opxy-usb-fix-20261102`; ветка `codex/biotron-opxy-usb-fix-20261102` → fork `manirko/biotron-firmware`.
Upstream `Playtronica/biotron-firmware` только для чтения (push URL заблокирован). В main/master не писать. Push только отдельной ветки; не выполнять команды release. Унаследованные workflows публикуют только по master/release, этот push их не запускает.
Договор: `~/.agents/skills/aiplay/SKILL.md`, `~/ProjectData/Playtronica/aiplay/TEAM-CONTRACT-v0.md`.
Карточка: `PVTI_lAHOAGy_zM4Bm3w-zhFcBUY` на https://github.com/users/manirko/projects/1/views/2.
