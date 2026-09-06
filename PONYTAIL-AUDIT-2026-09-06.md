# Biotron firmware — ponytail-audit (2026-09-06)

Инструмент: скилл `ponytail-audit` (DietrichGebert/ponytail v4.9.0, коммит 974d940 от 2026-09-04), поставлен в `~/.claude/skills/ponytail*`.
Репо: `~/Projects/Claude/biotron-firmware`, ветка `codex/biotron-led-organic-beta-08`, рабочее дерево чистое.
Аудит на Opus, находки перепроверены grep'ом. Ничего не применено — только список.
Вне охвата по правилам скилла: баги, безопасность, производительность.

## Итог
**net: −265 строк, −0 зависимостей, ~3,8 КБ статической RAM.** pico-sdk и TinyUSB несущие — убирать нечего.

## Находки (по убыванию среза)

| # | Тег | Что срезать | Чем заменить | Где | Срез | Проверка |
|---|-----|-------------|--------------|-----|------|----------|
| 1 | delete | High-speed USB и чужие MCU-ветки дескрипторов. RP2040 = full-speed only, `TUD_OPT_HIGH_SPEED`=0, ветки не компилируются | оставить тела `#else` inline; wire-identity не меняется | PLSDK/src/usb_descriptors.c:94-144, 163-212, 221-226 | ~90 | `desc_hs_configuration` только внутри мёртвого `#if` |
| 2 | delete | `active_led_loop_classic()` + `ledsValue[200]`, `level`, `ASYNC_LEDS`, `NOTE_STRONG` | ничего | src/leds.c:84-87, 218-222, 275, 284-307; include/leds.h:33-34 | 35 + 800 Б RAM | единственный вызов закомментирован (:275) |
| 3 | delete | 22 макроса `DEF_*` — пресеты пишутся литералами, `DEF_SWING_FIRST_NOTE_PERCENT` вообще без значения | ничего | include/params.h:30-51 | 22 | каждое имя встречается 1 раз (своё `#define`) |
| 4 | yagni | Таргет `biotron_layout` / `DEBUG_LAYOUT_BUILD`: единственный потребитель делает `#define pulse 22`, что ломает `uint8_t pulse` — таргет не собирается | удалить опцию и `#ifndef`-блок | CMakeLists.txt:14-28; PLSDK/include/PLSDK/cap_buttons.h:25-29 | 20 | 1 упоминание вне CMake |
| 5 | yagni | Чередование `PulseSide`: `mute_plant()` и `mute_light()` побайтно одинаковы, `last_pulse_side` ни на что не влияет | один `mute_divisor()`; observer = `last_pulse_timestamp = time_us_32();` | src/leds.c:44-50, 311-329, 339 | 18 | читается только в :339 и в закомментированном блоке |
| 6 | shrink | `get_CC()` — 18 закомментированных строк вокруг одной живой | `return biotron_midi_7bit(63 + counter);` | src/music.c:62-81 | 17 | — |
| 7 | yagni | `cb_on_hold` в драйвере кнопок: все 3 кнопки передают `NULL`, заглушка дёргается каждый тик удержания | убрать поле, NULL-прокидку и вызов | PLSDK/src/cap_buttons.c:29, 136-140, 160-162; src/buttons.c:45-47 | 11 | 3 вызова `buttons_add_button`, все с NULL |
| 8 | yagni | `add_sys_ex_com()` / `add_sys_ex_query()` — обёртки с `minimum_length=0`, продакшн-вызовов ноль | тестам передавать `0` в `_len`-вариант | PLSDK/src/commands.c:37-43; PLSDK/include/PLSDK/commands.h:81-82 | 10 | 0 вызовов вне тестов |
| 9 | delete | Константы `MAX_COUNT_CHANNELS`, `DEFAULT_BPM`, `DEFAULT_VELOCITY`, `DEFAULT_PITCH_MSB/LSB`, `BPM_TO_MS`, `US_TO_BPM`, `MS_TO_BPM`, `MUSIC_SELECT` | ничего | PLSDK/include/PLSDK/constants.h:4-9, 34-36, 42 | 9 | по 1 вхождению |
| 10 | delete | `change_volume()` + `CC_VOLUME` | ничего | PLSDK/src/music.c:61-65; music.h:52; constants.h:20 | 7 | 0 вызовов, включая тесты |
| 11 | shrink | `biotron_effective_light_bpm()` ≡ `biotron_effective_light_range()` (`clamp(value,1,127)`) | одно имя | include/runtime_safety.h:31-37 | 6 | — |
| 12 | shrink | `encode_u32()` скопирован в два файла | один inline-хелпер рядом с константами payload | src/settings_readback.c:22-27; PLSDK/src/midi_health.c:40-45 | 6 | — |
| 13 | delete | `init_commands()` — объявлена, задокументирована, не определена (вызов = ошибка линковки) | ничего | PLSDK/include/PLSDK/commands.h:55-59 | 5 | 1 вхождение |
| 14 | delete | Недостижимая кастомная гамма: `custom[71]`, `len_of_custom`, `SCALE_CUSTOM`; `SCALES_COUNT`=13, индекс 13 клампится в MAJOR | ничего | PLSDK/src/music.c:21-22; music.h:29 | 3 + 142 Б RAM | по 1 вхождению |
| 15 | yagni | `recovery_panic_pending[16][16]` при 2 кабелях; скан 256 слотов вместо 32 в каждом `service_midi_tx()` | `[2][16]` | PLSDK/src/midi_tx.c:19, 64-65 | 0 + 224 Б RAM | все enqueue — cable 0/1 |
| 16 | yagni | `ButtonState_t _buttons[100]` при 3 кнопках; рядом `button_states[3]` — 4-я кнопка и так порвёт память | размер 3 | PLSDK/src/cap_buttons.c:62-63 | 0 + ~2,7 КБ RAM | — |
| 17 | shrink | `set_default_sys_ex()` зовёт `reset_bpm()` после `default_settings()`, который его уже звал; `case 125: break;` возвращает то же, что default | убрать | src/params.c:399-402; PLSDK/src/commands.c:214-215 | 3 | — |

**(intentional experiment)** LED music pulse A06–A08: ~510 строк под путь, выключенный по умолчанию (`led_engine.c` 212 + `.h` 42 + ~50 строк `#if` в leds/music + 398 строк тестов + CMake). Честная цена, не рекомендация резать.

## Оговорки
1. **PLSDK — общий код.** В Biotron он лежит встроенной копией (не submodule), но в org Playtronica есть отдельный репо `PLSDK` и копия в `touchme-firmware`. Находки 1, 7–10, 13–16 подтверждены «0 вызовов» только внутри Biotron. Резать — в копии Biotron (она уже расходится с апстримом: локальные коммиты «Remove redundant settings erase and dead declarations», «Resolve Biotron SysEx command collision»), либо сначала сверить с touchme/Playtron.
2. `Makefile release` (timestamp как ID настроек) и `Dockerfile` (pico-sdk/picotool с mutable HEAD) — ~65 строк пути сборки, который по DEVELOPING.md не должен давать релиз. Не посчитаны: DEVELOPING.md их явно оставляет как legacy-справку. Решение владельца прошивки (Сергей).
3. `include/persistence_scheduler.h` (30 строк, один потребитель, который оборачивает его ещё тремя функциями в params.c) — по форме «слой с одним вызывающим», но охраняет запись во flash и покрыт контрактным тестом. Оставлен намеренно.

## Что дальше (если резать)
- Порядок: сначала #2, #3, #6, #13, #14 (чистая мёртвая масса, ноль риска для wire/настроек), потом #1 и #5 (нужен пересбор + прогон `./tests/run_host_tests.sh` + проверка USB-identity на Маке), PLSDK-находки — после решения по оговорке 1.
- Никаких изменений wire-формата SysEx и раскладки настроек ни в одной находке нет.
- Ветка: не `origin`, не прод. Сначала свой worktree.

---

# Раунд 2 — ultra, три среза (2026-09-06)

Режим `/ponytail ultra`, три агента на Opus параллельно (приложение / PLSDK / тесты+сборка), каждому дан список раунда 1, чтобы не повторять. Находки перепроверены grep'ом. Тест-сьют прогнан: `./tests/run_host_tests.sh` → exit 0, 10,7 с.

**Итог раунда 2: −294 строки** (приложение −59, PLSDK −95, тесты −102, сборка −38) **+ ~7 КБ RAM** (очередь TX 2,6 КБ, парсер ~0,9 КБ, таблицы команд ~1 КБ, буфер события 0,3 КБ).
**Суммарно с раундом 1: ≈ −560 строк, ~10 КБ статической RAM, 0 зависимостей.**

## Главные новые находки (не по строкам, по весу)

1. **Логгер пишет в никуда.** `pico_enable_stdio_usb 1` + `stdio_init_all()` в main.c, но `PLSDK/include/tusb_config.h:109` ставит `LIB_TINYUSB_HOST 1`, и pico-sdk (`stdio_usb.c:9`) подменяет драйвер заглушкой `stdio_usb_init(){return false;}`. `plsdk_printf` → `vprintf` → заглушка. `tud_cdc_write` в коде нет вообще. Значит `LOGGER_FLAG`, `plsdk_printf`, SysEx-команды 3/4 (`LOGGER_ACTIVATE/DEACTIVATE`) и 14 вызовов printf — мёртвый аппарат (~25 строк). ⚠️ Сам define, похоже, намеренный хак (коммит 1a445ec «Split TinyUSB and Main Code»), чтобы pico_stdio_usb не перехватил TinyUSB — PLSDK сам зовёт `tusb_init()`. Резать логгер — да; убирать define — только с проверкой энумерации USB. SysEx ID 3/4 — wire-контракт, кейс оставить как no-op или решение владельца.
2. **Нет CI-гейта на тесты.** Шаг `"Build & test"` в versioning.yml — это `echo "done!"`. Ни один workflow не запускает `run_host_tests.sh`. Плюс `cp ../CC.md` в public_release.yml копирует несуществующий файл (латентный fail шага), `rename` с regex предыдущего шага не переименовывает ничего.
3. **30 SysEx-хендлеров в таблицу НЕ сворачиваются** (проверено): сигнатура PLSDK `void(const uint8_t[], uint8_t)` без ID команды, generic-хендлер не знает, какое поле писать. X-macro дал бы −34 строки ценой кодогенерации в файле, который релиз-гейт парсит как текст. Оставить как есть.
4. **`min_press` у кнопок не работает**: `buttons_add_button` пишет `min_press_interval`, а `_check_button` сравнивает с файловой константой `_minPressInterval=50`. Значения 60/40/40 в `src/buttons.c:45-47` не имеют эффекта. (Поведенчески = баг-кандидат, вне скоупа ponytail — отдать в обычный ревью.)

## Приложение (−59)

| Тег | Что | Чем | Где | Срез |
|---|---|---|---|---|
| delete | rate-limiter отладочного лога в горячем пути бита: два `time_us_64()` за бит ради лога, который и так молчит | убрать блок | src/music.c:276-283 | 8 |
| yagni | `load_settings()` = комментарий вокруг `reset_bpm()` | звать `reset_bpm()` напрямую | src/global.c:205-211; include/global.h:29 | 7 |
| shrink | `set_button_mode_state_{sys_ex,cc}` и `set_light_pitch_mode_{sys_ex,cc}` — одинаковые тела попарно | по одному хелперу + тонкие адаптеры (регистрации сохраняются) | src/params.c:461-469, 563-577 | 7 |
| yagni | `initFrequencyTimer()` — вызывается один раз пятью строками ниже | влить в `init_plant()` | src/raw_plant.c:86-97 | 6 |
| yagni | `setup()` — один вызов, файл из двух функций | влить в `main()` (порядок `read_settings→init_midi→init_plant` сохранить, его проверяет release_contract.py:85) | main.c:17-45 | 6 |
| shrink | `get_plant_counter` считает `a*((extra+10)/10)` четыре раза, одна ветка пустая | один `const double` | src/music.c:104-117 | 5 |
| delete | пять неиспользуемых include: `pico/printf.h` (main.c:3, params.c:15, music.c:4), `pico/stdio.h` (music.c:6), `stdlib.h` (buttons.c:5) | — | — | 5 |
| yagni | `human_channel()` повторно клампит поле, которое парсер уже отверг (`params.c:503`), а пресеты пинят к 1/2 | `u7(stored+1)` | src/settings_readback.c:16-20 | 4 |
| yagni | `service_settings_persistence()` — 4-строчная обёртка над `save_pending_settings_now()`, один вызов | слить (debounce-проверку оставить — это защита от потери данных) | src/params.c:230-238 | 4 |
| yagni | `get_CC()` после чистки = одна строка с одним вызовом | инлайн в :183 | src/music.c:62-81 | 3 |
| delete | `LastCount` — объявлен, не читается, не пишется | — | src/raw_plant.c:16 | 1 |
| delete | `filter_val = (filter_val<<1)>>1` = `&= 0x7fffffff`, бит 31 недостижим (вход ≤ 655 350) | — | src/global.c:103 | 1 |
| shrink | хвостовой `u7()` в `percent()` мёртв — гварды выше уже держат 0…127 | прямой cast | src/settings_readback.c:10-14 | 1 |
| yagni | `save_settings()` экспортирован, вызовы только внутри params.c | `static`, убрать из params.h | include/params.h:89 | 1 |

Проверено и НЕ режется: суммирующий цикл в `change_plant_bpm_sys_ex` (BPM >127 нужен multi-byte); `STABILIZATION_COUNTER`/`AVERAGE_COUNTER` (калибровочные ручки); `start_plant_calibration_sys_ex` (пинит release_contract.py:105); `persistence_scheduler.debounce_us` (варьируется тестом); `plant_is_ready()` (несущий).

## PLSDK (−95, ~3,5 КБ RAM + ~1 КБ таблиц) — все с оговоркой «общий код»

| Тег | Что | Чем | Где | Срез |
|---|---|---|---|---|
| delete | мёртвый логгер (см. главную находку 1): `plsdk_printf`, `LOGGER_FLAG`, кейсы SysEx 3/4 | — (кейсы 3/4 → no-op, ID не трогать) | PLSDK/src/PLSDK.c:6,35-41; PLSDK.h:4,22; commands.c:198-213 | ~25 |
| native | опции tusb_config.h, дословно повторяющие дефолты TinyUSB: `CFG_TUSB_MEM_SECTION/ALIGN`, `CFG_TUSB_OS`, `CFG_TUD_ENDPOINT0_SIZE 64`, `CFG_TUD_MSC/HID/VENDOR 0`, `CFG_TUD_CDC_EP_BUFSIZE` | удалить, TinyUSB даёт те же | tusb_config.h:67-69, 74-87, 97-99, 103-104, 107, 119-120 | ~20 |
| delete | `BOARD_DEVICE_RHPORT_SPEED` — `#if` по списку чужих MCU (LPC/MIMXRT/NUC505/CXD56), всегда даёт FULL_SPEED; `BOARD_TUD_RHPORT` читает только ft9xx | `#define BOARD_DEVICE_RHPORT_SPEED OPT_MODE_FULL_SPEED` | tusb_config.h:47-56, 89-91 | ~12 |
| native | четыре тернарника `(TUD_OPT_HIGH_SPEED ? 512 : 64)` — константа 64 | `64` | tusb_config.h:112-117 | 4 |
| yagni | `MIDI_TX_MAX_MESSAGE_BYTES 259` × 16 слотов = 4,2 КБ под сообщения ≤ 87 байт (health page 0 = 87, readback 47, info 8) | 96 | midi_tx.h:8 | 0 + 2,6 КБ RAM |
| yagni | `MIDI_PARSER_SYSEX_CAPACITY 300` × (2 парсера + копия в событии) ≈ 930 Б при максимальном входящем SysEx 7 байт; отказ по переполнению остаётся, срабатывает раньше | 32–64 | midi_parser.h:8, 27, 35 | 0 + ~0,8 КБ RAM |
| shrink | `midi_event_t.data[300]` — вторая копия `parser->sysex` через memcpy | `const uint8_t *data` в буфер парсера | midi_parser.c:61, 82 | ~5 + 300 Б |
| delete | `dropped_count`/`midi_tx_dropped()` = `tx_rejected+tx_evicted`, оба уже в диагностике; `midi_tx_pending()`, `midi_tx_recovery_pending()` — только тесты читают | тестам читать snapshot | midi_tx.c:18, 86, 125, 138, 144, 181-187; midi_tx.h:14-16 | ~12 |
| delete | `counters.ignored_cable_packets` — инкрементится, но ни на одной health-странице не кодируется (0 вхождений в midi_health.c), хост прочитать не может | — | midi_diagnostics.c:76; .h:51 | 2 |
| delete | `ButtonState_t.min_press_interval` + параметр `min_press` — пишется, не читается (см. главную находку 4) | — | cap_buttons.c:31, 122; cap_buttons.h:61 | 3 |
| delete | `print_sys_ex()` — 0 вызовов в проде, всё идёт через `print_sys_ex_reply()` | — | commands.c:75-77; commands.h:99 | 4 |
| yagni | вложенный `project(PLSDK VERSION 2.0.0 … LANGUAGES C CXX ASM)` — 0 файлов .cpp/.S | оставить `add_library` + два `target_*` | PLSDK/CMakeLists.txt:1-6 | 6 |
| yagni | `MAX_COUNT_COMMANDS 50` × 2 таблицы ≈ 1 КБ при 18 CC + 30 SysEx | 32 | commands.h:19 | 0 + ~0,7 КБ |
| yagni | `init_midi()` — однострочный делегат `tusb_init()`, один вызов | звать `tusb_init()` в main | PLSDK.c:9-11; PLSDK.h:12 | 3 |
| shrink | `uint8_t pulse = 5` — дефолт сразу перетирается единственным `buttons_init(5)` | без инициализатора | cap_buttons.c:16 | 0 |

Проверено и НЕ режется: `midi_parser.c` не дублирует TinyUSB (tud_midi_packet_read отдаёт сырые 4-байтные пакеты без сборки multi-packet SysEx и классификации overflow/abort); CDC-интерфейс пустой, но заморожен правилом F1 (бит 0 PID `0x3011`) — кандидат в миграционный релиз, не срез. Побочное: `commands.c:108` зовёт лишний `tud_task()` на каждый принятый пакет внутри 32-пакетного drain, который main.c:53 и так качает раз в цикл — поведенческое решение, не слепой срез.

## Тесты (−102) и сборка (−38)

| Тег | Что | Чем | Где | Срез |
|---|---|---|---|---|
| shrink | 17 рукописных блоков `run_pair` в раннере | here-doc `name<TAB>args` + `while read` (флаги по лейнам сохраняются дословно) | tests/run_host_tests.sh:27-56, 73-109 | 40 |
| yagni | Python проверяет наличие подстрок прозы в трёх .md — ловит орфографию, не истину | убрать, либо `grep -qF` в раннере как средний путь | test_release_contract.py:41-43, 172-184, 192-193 | 18 |
| native | Python сверяет список лейнов раннера с захардкоженным списком из 17 имён; `set -eu` уже валит прогон при падении лейна | если нужно: `[ "$(grep -c '^run_pair' "$0")" = 17 ]` в раннере | test_release_contract.py:44, 185-191 | 8 |
| shrink | три копипасты `-fsyntax-only` | функция `syntax_only()` + 3 вызова | run_host_tests.sh:57-71 | 8 |
| stdlib | Python переизобретает препроцессор, чтобы посчитать USB PID из tusb_config.h (`_PID_MAP`) | `assert(desc_device.idProduct == 0x3011)` в test_usb_string_descriptor.c, который и так линкует usb_descriptors.c | test_release_contract.py:152-159 | 7 |
| delete | `tests/stubs/malloc.h` — 0 включений | — | stubs/malloc.h | 6 |
| delete | `stubs/pico/printf.h` второй раз определяет `MIN/MAX`, уже есть в `stubs/pico/stdlib.h:11-16` | `#include "pico/stdlib.h"` | stubs/pico/printf.h:4-9 | 6 |
| delete | `stubs/tusb.h` дублирует пять `CFG_TUD_*` + `ENDPOINT0_SIZE` из tusb_config.h (PLSDK/include уже в `-I`) — это причина, по которой существует Python-PID | включить настоящий tusb_config.h (не компилировал — проверить) | stubs/tusb.h:7-12 | 6 |
| yagni | `run_pair` гоняет каждый тест дважды (ASan/UBSan `-O1`, потом `-O2`); хостовый `-O2` не моделирует `arm-none-eabi -O2` | одна дорожка; 10,7 с → ~5 с | run_host_tests.sh:20-22 | 3 (судейское, разумно и оставить) |
| native | четыре шага для вычисления тега (action get-previous-tag → awk → strip v → echo) | `git describe --tags --abbrev=0` + один awk (fetch-depth: 0 уже есть) | versioning.yml:18-39 | 16 |
| native | сторонний `winterjung/split@v2` чтобы разбить `1.8.3` по точке | `IFS=. read -r MAJ MIN PAT <<<"$CLEAN_TAG"` | versioning.yml:44-49 | 5 |
| delete | шаг `"Build & test"` = `echo "done!"` (см. главную находку 2) | — либо настоящий `./tests/run_host_tests.sh` | versioning.yml:60-62 | 3 |
| delete | `apt-get install rename` + `rename "s/\^.\*.uf2//g"` — паттерн предыдущего шага как имя файла, ничего не переименовывает | — | public_release.yml:18-19, 31-33 | 5 |
| delete | `add_compile_definitions(BIOTRON_LED_MUSIC_PULSE=0)` + status — `include/leds.h:4-5` уже дефолтит через `#ifndef`, все use-site — `#if` | — | CMakeLists.txt:36-39 | 4 |
| delete | два голых `ls` в release push | — | public_release.yml:44-45 | 2 |
| delete | `cp ../CC.md CC.md` — файла нет в репо (латентный fail) | — | public_release.yml:53 | 1 |
| legacy | шаг `Build Firmware` зовёт `make release` (timestamp как ID настроек) — вызывающий уже найденного Makefile-таргета; DEVELOPING.md:121-124 говорит «не релизное свидетельство» | решение владельца | versioning.yml:51-55 | 5 |

Проверено и НЕ режется: assert-harness (все 17 тестов на `<assert.h>` + одна `puts`); раннер (`mktemp -d` + `trap` + `set -eu` — нативная форма, CTest был бы добавлением); флаги лейнов реально различаются (`-DBIOTRON_LED_MUSIC_PULSE=1` только там, где компилируются leds.c/music.c); пары `*_v1_contract.c` ↔ `*.c` не пересекаются (wire-байты vs alarm/swing; `offsetof` ABI vs pack/pad); фейки `time_us_64`/`add_alarm_in_us` намеренно разные в разных файлах.

## Порядок, если резать (обновлённый)
1. **Ноль риска, чистая мёртвая масса:** раунд 1 #2, #3, #6, #13, #14 + раунд 2: unused include, `LastCount`, `malloc.h`, `MIN/MAX` дубль, `print_sys_ex`, `ignored_cable_packets`, `midi_tx_dropped` & co, CMake `LANGUAGES`, `BIOTRON_LED_MUSIC_PULSE=0`, `ls`/`rename`/`CC.md` в workflow.
2. **Пересбор + `run_host_tests.sh` + USB-identity на Маке:** HS-дескрипторы, `PulseSide`, tusb_config-дефолты, размеры буферов (259→96, 300→64, 50→32), `setup()`/`init_midi()` инлайн.
3. **Решение владельца (Сергей):** мёртвый логгер + `LIB_TINYUSB_HOST`, `min_press` (баг-кандидат), лишний `tud_task()` в drain, `-O2`-лейн, Python-проверки прозы, `make release` в CI, CDC-интерфейс (миграционный релиз).
4. **Отдельно от ponytail, но важнее всего:** поставить настоящий `./tests/run_host_tests.sh` в CI вместо `echo "done!"`.

---

# Раунд 3 — верификация и опровержение (2026-09-06)

## Снято: «логгер пишет в никуда» (раунд 2, PLSDK #1) — ОПРОВЕРГНУТО
Проверка по готовому ELF (`build-arm-led-on-arm15-v197/biotron.elf`, 31.08.2026) и препроцессору с реальными флагами сборки:
- `stdio_usb_init` = 68 байт, `stdio_usb_out_chars` = 248 байт и вызывает `tud_cdc_n_write` / `tud_cdc_n_write_flush` (objdump). Это настоящий драйвер, а не заглушка `return false`.
- `ninja -t deps`: stdio_usb.c включил именно `PLSDK/include/tusb_config.h`, и `-dM` показывает `LIB_TINYUSB_HOST 1` определённым.
- Причина: условие pico-sdk `#if !defined(LIB_TINYUSB_HOST) || (defined(LIB_TINYUSB_HOST) && defined(CFG_TUH_RPI_PIO_USB))` — TinyUSB сам определяет `CFG_TUH_RPI_PIO_USB 0`, а проверка на `defined`, не на значение. Условие истинно → реальный драйвер.
**Вывод:** `plsdk_printf`, `LOGGER_FLAG`, SysEx 3/4 — живой отладочный канал через CDC (когда хост открыл порт). Не резать. Из −95 строк PLSDK возвращаются ~25.
**Остаётся:** `delete:` `#define LIB_TINYUSB_HOST 1` — 1 строка, no-op в pico-sdk 2.3.0 и cargo-cult: в SDK, где `CFG_TUH_RPI_PIO_USB` не предопределён, эта строка молча убьёт stdio. [PLSDK/include/tusb_config.h:109]

## PLSDK — статус «общего кода» уточнён
- Апстрим `Playtronica/PLSDK`: последний коммит a563c76 от 2025-03-04, 5 файлов src + 4 заголовка. В нём НЕТ `midi_tx/midi_parser/midi_diagnostics/midi_health` — эти четыре файла существуют только в Biotron. Находки по ним (очередь TX, парсер, счётчики) — без оговорки, чисто локальные.
- Копия в touchme-firmware = тот же старый апстрим (diff: отличаются только Biotron-правки). Приложение touchme из «мёртвых» символов использует только `add_sys_ex_com` (12 вызовов); `print_sys_ex`, `change_volume`, `min_press_interval`, `init_commands`, `SCALE_CUSTOM`, `cb_on_hold`, HS-дескрипторы, `CC_VOLUME`, `DEFAULT_BPM`, `MUSIC_SELECT` — в touchme-app 0 вызовов.
- Biotron-копия уже расходится с апстримом в 8 файлах (commands.c, cap_buttons.c, music.c, usb_descriptors.c, PLSDK.c, constants.h, commands.h, music.h). Это де-факто форк, апстрим 18 месяцев не двигался.
**Вывод:** резать в копии Biotron можно без синхронизации с апстримом; единственное, что стоит оставить ради touchme-совместимости API — `add_sys_ex_com()` (или удалить сознательно, форк есть форк).

## Доки: детектор дрейфа охраняет уже ложное утверждение
- DEVELOPING.md:9-10 и README.md:54: «кандидат = `cf264aa` (1.8.3); коммиты после — только тесты/доки/тулинг». Факт: `git rev-list --count cf264aa..HEAD -- src include main.c PLSDK` = **14** коммитов в код (self-heal scheduler, organic LED, zones, calibration…). Утверждение устарело.
- `tests/test_release_contract.py:176,192` требует подстроки `cf264aa` и `1765723554` в DEVELOPING.md — то есть обновить доку до правды нельзя, не переписав тест. Это живой пример находки «Python проверяет прозу» из раунда 2: спелл-чек вместо проверки истины.
- CHANGELOG.md: верхняя секция «Unreleased — 1.8.3 candidate», ни одного упоминания 1.9.x, при этом в репо TEST-ARTIFACT-1.9.2…1.9.8 и сборки v196/v197. Последний git-тег — v1.7.9.
**Вывод (решение владельца):** либо объявить новый кандидат и обновить три места (DEVELOPING/README/CHANGELOG + пины теста), либо честно пометить ветку 1.9.x как «не кандидат». Ponytail-срез: заменить подстрочные пины в Python на одну проверку `git rev-parse --verify <SHA из DEVELOPING>` — проверяет существование, не орфографию.

## LED-срез (ultra, раунд 3): −79 строк
Оба раунда прошли leds.c/led_engine.c поверхностно; отдельный проход. Проверено grep'ом.

| Тег | Что | Чем | Где | Срез |
|---|---|---|---|---|
| shrink | 22 рукописных триплета `pwm_set_gpio_level_invert` (74 вызова в файле), пишущих одно значение в одну группу из трёх LED | 4-строчный `set_group(base, value)` поверх уже существующего `ALL_LEDS[]` (раскладка 0-2 синие, 3-5 зелёные-1, 6-8 зелёные-2); `blue_leds()` растворяется | src/leds.c:91-95, 144-152, 156-164, 170-172, 176-183, 224-229, 269-274, 342-348 | 30 (6 триплетов из раунда 1 не считаны дважды) |
| shrink | `previous_target/previous_level` + четыре блока сравнения ради `dirty`; в `service` огибающие только спадают к нулю, «что-то изменилось» ⟺ «что-то ненулевое» | `moving = beat_target \| beat_level` внутри существующего цикла по лейнам | src/led_engine.c:167-192 | 10 ⚠️ инвариант выведен чтением, прогнать `test_led_engine` |
| yagni | `music_pulse_suppressed` — второй state-machine мьюта в адаптере, охраняющий и так идемпотентный `led_engine_clear_notes` (memset + dirty) | `if (isMutedByButton) led_engine_clear_notes(&engine);` | src/leds.c:23, 258-267 | 8 |
| shrink | `spread_weight()` — тело выбирает одну из двух 3-элементных таблиц | `static const uint8_t SPREAD_WEIGHT[2][LANES]` на единственном месте вызова; числа {112,34,14}/{38,16,8} дословно | src/led_engine.c:37-46 | 6 |
| yagni | два из четырёх `#if BIOTRON_LED_MUSIC_PULSE` в leds.h охраняют только include и два прототипа — кода не порождают, OFF-UF2 остаётся байт-в-байт | безусловно | include/leds.h:8-12, 41, 44 | 5 |
| shrink | factory-test if/else — две ветки по 9 вызовов, отличаются только тем, куда идёт значение | `green = isTestModeGreen ? MAX_LIGHT : 0` + три `set_group` | src/leds.c:143-165 | 4 |
| yagni | `led_engine_init()` = дословный алиас `led_engine_reset()` | удалить, единственный вызов (leds.c:68) → reset | src/led_engine.c:106-108; led_engine.h:32 | 4 |
| delete | четыре `if (engine == NULL) return;` — один статический экземпляр в проде, ни один тест не передаёт NULL (0 вхождений NULL в обоих тестах) | — | src/led_engine.c:100, 147, 154, 163 | 4 |
| yagni | `led_frame_t.blue[3]` — три копии одного числа (`frame->blue[lane] = beat_level` без зависимости от лейна); спец называет это «полная синяя дуга» | `uint16_t blue;`, запись вне цикла, `BLUE_SPATIAL[]` не нужен | led_engine.h:19; led_engine.c:201-202; leds.c:25-28, 105 | 3 + 4 Б |
| delete | недостижимый `else { return; }` в `led_music_note_on` — у `led_source_t` два значения, оба обработаны выше, движок ещё раз валидирует | обычный `else` | src/leds.c:125-127 | 3 |
| delete | двойная проверка нулевой velocity в `note_attack` — единственный вызов уже отсекает `velocity == 0` | одно выражение | src/led_engine.c:25-30 | 2 |
| native (0, не считано) | `pwm_set_gpio_level_invert()` переизобретает полярность PWM, которую pico-sdk делает железом (`pwm_config_set_output_polarity`). НЕ рекомендуется как есть: `MAX_LIGHT`=65436≠65536 (сдвиг ~0,3 % на краях), `intro_leds` намеренно не инвертирует зелёные, `test_led_adapter.c:64` пинит `MAX_LIGHT - level` как контракт | решение железо/продукт | src/leds.c:53-55, 61-66 | 0 |

Зависимое: после удаления `ledsValue/level` (раунд 1) `#include "raw_plant.h"` в leds.c:5 мёртв (−1).

Проверено и оставлено: `led_engine.c` отдельным файлом с одним потребителем — контракт LED-MUSIC-PULSE.md:27-32 (без SDK/USB/MIDI/heap/float/flash, хост-тесты); параметр `engine*` вместо статика — тест поднимает два движка рядом; все константы decay/attack/beat, `MAX_LIGHT/MIN_LIGHT`, `led_step`, бюджеты — tuning-поверхность по спеку; `MAX_DECAY_STEPS` early-out — контрактная эквивалентность при пропуске кадра; `_Static_assert(sizeof ≤ 128)` — бюджет RAM; дублированный ping-pong ramp — после `set_group` выигрыш 1 строка, не стоит индирекции; три `#if` в music.c — однострочные гарды, правильная форма.

## Опровержение раунда 2 (Opus-ревизор, 67 проверок, эксперименты в scratch-копии, суммарно прогнал сьют)

**Снято (8):**
| Находка | Почему |
|---|---|
| Прил. #1 rate-limiter лога в music.c | опиралась на «логгер мёртв» — а он живой CDC-канал; это единственная телеметрия AverageFreq/Freq/Note |
| Прил. #2 `load_settings()` | `test_music_scheduler.c:162-163` зовёт её дважды и считает `schedule_count` |
| Прил. #8 `human_channel()` → `u7(stored+1)` | `plant_channel` — `int`; тест readback:83-94 подаёт −1/99 и ждёт 1/16. `u7(0)=0`, `u7(100)=100` — поведение и тест ломаются |
| Прил. #9 слить `service_settings_persistence` | у `save_pending_settings_now()` второй вызов params.c:663 (RESET_DEVICE), пин release_contract.py:124 |
| Прил. #12 маска бита 31 в `filter_val` | это feedback-аккумулятор `f' = k·f + (1−k)·v`, `filterPercent` до 1.27 → расходится; симуляция точным кодом: бит 31 на 13-м сэмпле. Маска живая |
| PLSDK #1 мёртвый логгер | см. выше: `CFG_TUH_RPI_PIO_USB` определён TinyUSB как 0, `defined()` истинно, реальный драйвер в ELF |
| PLSDK #8 `midi_tx_dropped/pending/recovery_pending` | у snapshot нет полей текущей глубины/ожидающих panic; `test_midi_tx.c:110-126` строит на них инвариант |
| PLSDK #9 `ignored_cable_packets` | `test_midi_diagnostics.c:57` и `test_commands_integration.c:259` ассертят `== 1` |
| PLSDK #14 `init_midi()` → `tusb_init()` | `release_contract.py:85` делает `main_source.index("init_midi();")` — `ValueError` при отсутствии |

**Ужато (10):** хелперы `set_*_mode` −4 вместо −7 (тела не идентичны: `data[0]>0` vs `value>63`); `get_plant_counter` −2 вместо −5 (`b*2` не унифицируется); unused include `pico/printf.h` в music.c держит хост-лейн (`MIN/MAX` приходят из стаба) — только вместе с дедупом стаба; tusb_config-дефолты −12, а `CFG_TUD_MSC/HID/VENDOR` только после снятия Python-PID (иначе `AttributeError` в release_contract.py:153, проверено); **TX-буфер 259 → не 96, а ≥104** (`MIDI_HEALTH_MAX_PAYLOAD_BYTES 96` + 4 байта конверта = 100, 96 молча уронит страницу на заявленном лимите SDK), RAM −2,5 КБ; парсер 300 → 64 валит `test_midi_parser.c:78` (счётчики 99/100 захардкожены), RAM −0,46 КБ, не 0,93 (копия события — на стеке); `midi_event_t.data`-указатель должен алиасить два буфера (sysex и `packet`), ~3 строки, 0 RAM; `MAX_COUNT_COMMANDS` 50 → 32 проходит сьют, но запас всего 2 слота, RAM −288 Б; `print_sys_ex` — 0 в проде, но `test_commands_integration.c:179-180` пинит легаси-контракт; `min_press` — мёртвая запись подтверждена, но удаление цементирует баг-кандидат #4 и меняет публичную сигнатуру SDK → решение владельца.

**Подтверждено экспериментом:** `malloc.h` удалён — сьют зелёный; `printf.h` → `#include "pico/stdlib.h"` — зелёный; `stubs/tusb.h` → настоящий `tusb_config.h` компилируется, но нужно `CFG_TUSB_MCU` выше include и перевод строки в конце tusb_config.h (`-Werror=newline-eof`); препроцессор с боевыми флагами: `rhport_speed = 0x0200`, `TUD_OPT_HIGH_SPEED = 0`, PID `0x3011` не меняется.

**Цепочки зависимостей при резке:** `stubs/tusb.h` → Python-PID → `CFG_TUD_*`; here-doc раннера только после снятия lane-list проверки (`re.findall(r"^run_pair …")` иначе видит 0 лейнов — и предложенный `grep -c '^run_pair'` тоже); `pico/printf.h` в music.c только после дедупа стаба.

**Скорректированный раунд 2:** приложение −26, PLSDK −37, тесты −97 (40 из них сцеплены), сборка −36 → **≈ −196 строк, ≈ 3,3 КБ RAM** (было заявлено −294 / ~7 КБ).

# ИТОГ ПО ТРЁМ РАУНДАМ (после опровержения)

| Раунд | Заявлено | После проверки | RAM |
|---|---|---|---|
| 1 (full, весь репо) | −265 | −265 (grep-сверка; НЕ прогнан через опровергателя) | ~3,8 КБ |
| 2 (ultra, 3 среза) | −294 | **−196** | ~3,3 КБ |
| 3 LED (ultra) | −79 | −79 (grep-сверка; НЕ прогнан через опровергателя) | ~0 |
| **Всего** | −638 | **≈ −540 строк** | **≈ 7 КБ** |

Не удалось проверить: шлёт ли реальный хост (Settings app / WebMidiBiotron) входящий SysEx длиннее 27 байт payload (ограничено только со стороны прошивки); ре-энумерация USB после срезов в tusb_config (препроцессор идентичен, устройство не прошивалось); из какого именно дерева собран отгруженный бинарь.

Урок раунда 2 → раунда 3: 8 из 46 находок ultra-режима оказались ложными, все восемь — потому что агент не читал тесты как вызывающих. Любую «0 вызовов» проверять и по `tests/`, и по `test_release_contract.py` (он парсит исходники как текст).
