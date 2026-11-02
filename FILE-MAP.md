# File map · 02.11.2026

| Файл | Зачем |
|---|---|
| `.github/workflows/public_release.yml` | Унаследованный GitHub workflow: public_release. |
| `.github/workflows/pull_request_ci.yml` | Унаследованный GitHub workflow: pull_request_ci. |
| `.github/workflows/versioning.yml` | Унаследованный GitHub workflow: versioning. |
| `.gitignore` | Исключения git для generated файлов. |
| `CHANGELOG.md` | Унаследованная история версий. |
| `CLEAN-FIRMWARE.md` | Состав существующего внутреннего кандидата1.10.11. |
| `CMakeLists.txt` | RP2040 target и флаги. |
| `DEVELOPING.md` | Унаследованная карта runtime и протоколов. |
| `Dockerfile` | Унаследованное окружение Docker. |
| `FILE-MAP.md` | Каждый файл и его назначение. |
| `LED-MUSIC-PULSE.md` | Существующий LED эксперимент A06–A08. |
| `LICENSE` | Лицензия GPL-3.0. |
| `Makefile` | Унаследованная сборка; release не использовать. |
| `PLSDK/.gitignore` | Служебный файл исходной базы. |
| `PLSDK/CMakeLists.txt` | Подключение исходников и библиотек папки. |
| `PLSDK/README.md` | Унаследованная документация: README. |
| `PLSDK/include/PLSDK.h` | Объявления и константы модуля PLSDK. |
| `PLSDK/include/PLSDK/cap_buttons.h` | Объявления и константы модуля cap_buttons. |
| `PLSDK/include/PLSDK/commands.h` | Объявления и константы модуля commands. |
| `PLSDK/include/PLSDK/constants.h` | Объявления и константы модуля constants. |
| `PLSDK/include/PLSDK/midi_diagnostics.h` | Объявления и константы модуля midi_diagnostics. |
| `PLSDK/include/PLSDK/midi_health.h` | Объявления и константы модуля midi_health. |
| `PLSDK/include/PLSDK/midi_parser.h` | Объявления и константы модуля midi_parser. |
| `PLSDK/include/PLSDK/midi_tx.h` | Объявления и константы модуля midi_tx. |
| `PLSDK/include/PLSDK/music.h` | Объявления и константы модуля music. |
| `PLSDK/include/tusb_config.h` | Объявления и константы модуля tusb_config. |
| `PLSDK/src/PLSDK.c` | Реализация модуля PLSDK. |
| `PLSDK/src/cap_buttons.c` | Реализация модуля cap_buttons. |
| `PLSDK/src/commands.c` | Реализация модуля commands. |
| `PLSDK/src/midi_diagnostics.c` | Реализация модуля midi_diagnostics. |
| `PLSDK/src/midi_health.c` | Реализация модуля midi_health. |
| `PLSDK/src/midi_parser.c` | Реализация модуля midi_parser. |
| `PLSDK/src/midi_tx.c` | Реализация модуля midi_tx. |
| `PLSDK/src/music.c` | Реализация модуля music. |
| `PLSDK/src/usb_descriptors.c` | USB descriptor callbacks; единственный runtime USB patch. |
| `PONYTAIL-AUDIT-2026-09-06.md` | Унаследованная документация: PONYTAIL-AUDIT-2026-09-06. |
| `README.md` | Вход в USB Fix проект: база, сборка, проверки, ограничения. |
| `SettingsDescription.md` | CC/SysEx протокол Biotron. |
| `TEST-ARTIFACT-1.9.2-RECALIBRATION.md` | Унаследованная документация: TEST-ARTIFACT-1.9.2-RECALIBRATION. |
| `TEST-ARTIFACT-1.9.3-READBACK-RECALIBRATION.md` | Унаследованная документация: TEST-ARTIFACT-1.9.3-READBACK-RECALIBRATION. |
| `TEST-ARTIFACT-1.9.4-CALIBRATION-CUE.md` | Унаследованная документация: TEST-ARTIFACT-1.9.4-CALIBRATION-CUE. |
| `TEST-ARTIFACT-1.9.6-QUIET-CALIBRATION.md` | Унаследованная документация: TEST-ARTIFACT-1.9.6-QUIET-CALIBRATION. |
| `TEST-ARTIFACT-1.9.7-SCHEDULER-RECOVERY.md` | Унаследованная документация: TEST-ARTIFACT-1.9.7-SCHEDULER-RECOVERY. |
| `TEST-ARTIFACT-1.9.8-ORGANIC-LED.md` | Унаследованная документация: TEST-ARTIFACT-1.9.8-ORGANIC-LED. |
| `TEST-OPXY.md` | Тест Андрея на OP-XY до четырёх шагов. |
| `USB-FIX-VALIDATION.md` | Первичные источники и доказательства проверки обеих баз. |
| `include/buttons.h` | Объявления и константы модуля buttons. |
| `include/global.h` | Объявления и константы модуля global. |
| `include/led_engine.h` | Объявления и константы модуля led_engine. |
| `include/leds.h` | Объявления и константы модуля leds. |
| `include/midi_note_lifecycle.h` | Объявления и константы модуля midi_note_lifecycle. |
| `include/music.h` | Объявления и константы модуля music. |
| `include/params.h` | Объявления и константы модуля params. |
| `include/persistence_scheduler.h` | Объявления и константы модуля persistence_scheduler. |
| `include/raw_plant.h` | Объявления и константы модуля raw_plant. |
| `include/runtime_safety.h` | Объявления и константы модуля runtime_safety. |
| `include/settings_readback.h` | Объявления и константы модуля settings_readback. |
| `include/settings_storage.h` | Объявления и константы модуля settings_storage. |
| `main.c` | Инициализация и главный цикл. |
| `pico_sdk_import.cmake` | Поиск и импорт Pico SDK. |
| `src/CMakeLists.txt` | Подключение исходников и библиотек папки. |
| `src/buttons.c` | Реализация модуля buttons. |
| `src/global.c` | Реализация модуля global. |
| `src/led_engine.c` | Реализация модуля led_engine. |
| `src/leds.c` | Реализация модуля leds. |
| `src/music.c` | Реализация модуля music. |
| `src/params.c` | Реализация модуля params. |
| `src/raw_plant.c` | Реализация модуля raw_plant. |
| `src/settings_readback.c` | Реализация модуля settings_readback. |
| `static/logo.png` | Логотип исходного проекта. |
| `tests/run_host_tests.sh` | 18 групп в sanitizer и optimized вариантах. |
| `tests/stubs/PLSDK/cap_buttons.h` | Host stub для cap_buttons.h. |
| `tests/stubs/hardware/adc.h` | Host stub для adc.h. |
| `tests/stubs/hardware/flash.h` | Host stub для flash.h. |
| `tests/stubs/hardware/gpio.h` | Host stub для gpio.h. |
| `tests/stubs/hardware/irq.h` | Host stub для irq.h. |
| `tests/stubs/hardware/pwm.h` | Host stub для pwm.h. |
| `tests/stubs/hardware/sync.h` | Host stub для sync.h. |
| `tests/stubs/hardware/timer.h` | Host stub для timer.h. |
| `tests/stubs/malloc.h` | Host stub для malloc.h. |
| `tests/stubs/pico/bootrom.h` | Host stub для bootrom.h. |
| `tests/stubs/pico/printf.h` | Host stub для printf.h. |
| `tests/stubs/pico/stdio.h` | Host stub для stdio.h. |
| `tests/stubs/pico/stdlib.h` | Host stub для stdlib.h. |
| `tests/stubs/pico/time.h` | Host stub для time.h. |
| `tests/stubs/pico/unique_id.h` | Host stub для unique_id.h. |
| `tests/stubs/tusb.h` | Host stub для tusb.h. |
| `tests/test_calibration_cue_lifecycle.c` | Регрессионная проверка calibration_cue_lifecycle. |
| `tests/test_commands_integration.c` | Регрессионная проверка commands_integration. |
| `tests/test_led_adapter.c` | Регрессионная проверка led_adapter. |
| `tests/test_led_engine.c` | Регрессионная проверка led_engine. |
| `tests/test_midi_diagnostics.c` | Регрессионная проверка midi_diagnostics. |
| `tests/test_midi_health.c` | Регрессионная проверка midi_health. |
| `tests/test_midi_parser.c` | Регрессионная проверка midi_parser. |
| `tests/test_midi_tx.c` | Регрессионная проверка midi_tx. |
| `tests/test_music_scheduler.c` | Регрессионная проверка music_scheduler. |
| `tests/test_music_v1_contract.c` | Регрессионная проверка music_v1_contract. |
| `tests/test_note_lifecycle.c` | Регрессионная проверка note_lifecycle. |
| `tests/test_persistence_scheduler.c` | Регрессионная проверка persistence_scheduler. |
| `tests/test_raw_plant_runtime.c` | Регрессионная проверка raw_plant_runtime. |
| `tests/test_release_contract.py` | Регрессионная проверка release_contract. |
| `tests/test_runtime_safety.c` | Регрессионная проверка runtime_safety. |
| `tests/test_settings_readback.c` | Регрессионная проверка settings_readback. |
| `tests/test_settings_storage.c` | Регрессионная проверка settings_storage. |
| `tests/test_storage_v1_contract.c` | Регрессионная проверка storage_v1_contract. |
| `tests/test_usb_string_descriptor.c` | Регрессионная проверка usb_string_descriptor. |
| `tools/build.sh` | Воспроизводимая Release сборка без USB доступа. |
| `tools/verify_uf2.py` | Разбор UF2/USB и отрицательные самопроверки. |
| `validation/candidate-audit.json` | Отчёт независимой проверки: candidate-audit. |
| `validation/candidate-checks.json` | Отчёт независимой проверки: candidate-checks. |
| `validation/official-rollback-audit.json` | Отчёт независимой проверки: official-rollback-audit. |
| `validation/stable-audit.json` | Отчёт независимой проверки: stable-audit. |
