# Biotron 1.10.11 · OP-XY USB Fix · тест 02.11.2026

Отдельная прошивка для проверки USB MIDI на OP-XY, поверх существующего внутреннего кандидата Biotron1.10.11. Не официальный релиз.

## Pipeline
`manirko/biotron-firmware@a773904 (existing internal1.10.11)` → USB descriptor patch → host suite → Release RP2040 UF2 → независимый разбор бинарника + две чистые сборки → ручной тест Андрея.

## База и статус
База: `a7739040e07ba7ca04767682755eec8589c15234`, проверена fresh fetch02.11.2026. Сохраняет прежние 1.10.11/ID1765723554/LED Music+Pulse ON (только A06–A08). Новых музыкальных функций в этой задаче нет. Runtime diff к базе — только 9 строк `PLSDK/src/usb_descriptors.c`: EF/02/01 и MIDI перед CDC. Обновлены только ожидания регрессионного теста и три константы host stub.

Все 18 групп host-тестов в ASan/UBSan и optimized lanes, release contract и syntax checks — PASS. Две чистые Release-сборки дают один UF2. Дескрипторы, UF2/RP2040/границы flash/векторы, две MIDI cable и CDC — PASS. Физические OP-XY, MIDI/clock/CDC, settings/BOOT/откат, LED/калибровка/слух — NOT_RUN.

SHA256 тестового 1.10.11UF2: `bf443ff30cb022675d81954e0bc4ccc157f50918f9c9a36dca250259c17f8383`.
Отдельно воспроизведён прежний стабильный F03(1.8.2) байт в байт: `e8ac496120541013df1e1f6aee134286ed7e945fc4535276f0d518d34bb87e76`. Его ветка — `codex/biotron-opxy-usb-fix-20261102`, commit73d2200. Stable и завод не менялись.

## File map
Каждый файл перечислен в [FILE-MAP.md](FILE-MAP.md).
- `PLSDK/src/usb_descriptors.c` — единственное runtime-изменение.
- `tests/run_host_tests.sh` — унаследованный полный suite; `tests/test_release_contract.py` проверяет новый порядок и класс; `tests/stubs/tusb.h` знает три class constant.
- `tools/build.sh`, `tools/verify_uf2.py` — воспроизведение и независимый разбор файла без USB-доступа.
- `validation/` — результаты проверки точных UF2 и compiler definitions.
- [USB-FIX-VALIDATION.md](USB-FIX-VALIDATION.md), [TEST-OPXY.md](TEST-OPXY.md) — источники/ограничения и проверка до 4 шагов.
- `DEVELOPING.md`, `CLEAN-FIRMWARE.md`, `LED-MUSIC-PULSE.md` — унаследованный контекст внутреннего кандидата.

## Где данные
Сборки/ELF/compile_commands/логи/официальный download/первичный GitHub JSON: `~/ProjectData/biotron-opxy-usb-fix-20261102/`.
Исходные F03/R03 и manifest родителя: `~/ProjectData/Playtronica/aiplay/playtron-opxy-usb-2026-11-02/delivery-current-r2/`.
SDK/compiler: `~/ProjectData/toolchains/`. В git только исходники/документы/небольшие отчёты; секретов нет.

## Как собрать и проверить
Pico SDK2.3.0@`98a542c1a62fb549ffb5d66a3e5892b06276b670`, TinyUSB@`86ad6e56c1700e85f1c5678607a762cfe3aa2f47`, Arm GNU13.2.Rel1, picotool2.3.0. Локально cmake4.4.3/ninja1.13.2. SDK/TinyUSB clean при проверке. SettingsID и build date закреплены раздельно:1765723554 и `SOURCE_DATE_EPOCH=1793577600` (02.11.2026UTC).

```sh
./tests/run_host_tests.sh
./tools/build.sh candidate "$HOME/ProjectData/biotron-opxy-usb-fix-20261102/my-candidate-build"
python3 tools/verify_uf2.py --selftest
python3 tools/verify_uf2.py "$HOME/ProjectData/biotron-opxy-usb-fix-20261102/my-candidate-build/biotron.uf2" --reference /absolute/path/R03_Biotron_ROLLBACK_1.8.2_official.uf2
```

Нужна новая папка сборки. GitHub унаследованный CI выполняет те же host-тесты и build того же commit с pinned SDK. LinuxCI использует другой ARM compiler и делает отдельный artifact, не заменяет macOSUF2 для Андрея. Унаследованные `make release`/timestamp settings ID не использовать.

## Safety rails
Тестовая 1.10.11 — для подтверждённых A06–A08 и отдельного выбора этой базы. Если прибор на stable1.8.2, не менять его на 1.10.11 автоматически. Сначала сохранить пресеты; в 1.8.2 вход в BOOT стирает settings. Official rollbackR03=1.8.2, не предыдущая 1.10.11; сохранение настроек при процедуре на железе не проверялось.

Прошивает только человек, по одному файлу. Не менять основной Playtronica repo/worktree родителя, завод/stable/latest, settings.playtronica.com. Не отправлять файлы команде. Не создавать релизы. Полный firmware release — после явного «да» Сергея. HumanPASS от Playtron не переносится на Biotron.

## Git
Независимая папка `~/Projects/biotron-opxy-usb-fix-20261102`; текущая ветка `codex/biotron-opxy-usb-fix-1.10.11-20261102` → fork `manirko/biotron-firmware`. Отдельная stable ветка остаётся на 1.8.2. Upstream push URL заблокирован. В main/master не писать; push только тестовой ветки. Не создавать release/tag релиза.
Договор: `~/.agents/skills/aiplay/SKILL.md`, `~/ProjectData/Playtronica/aiplay/TEAM-CONTRACT-v0.md`. Карточка `PVTI_lAHOAGy_zM4Bm3w-zhFcBUY`: https://github.com/users/manirko/projects/1/views/2 .
