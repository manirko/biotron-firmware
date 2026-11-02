# Независимая проверка Biotron USB Fix · 02.11.2026

## Источники
- Official1.8.2/master: https://github.com/Playtronica/biotron-firmware/commit/67920c21d51866576075308529a055f10be164dc ; tag1.8.2.
- Official release, published14.12.2025: https://github.com/Playtronica/biotron-firmware/releases/tag/1.8.2 ; скачан заново 02.11.2026, SHA256 совпал с GitHub asset digest и R03.
- Исходный F03 и manifest: `~/ProjectData/Playtronica/aiplay/playtron-opxy-usb-2026-11-02/delivery-current-r2/`; Claude source worktree `d38d1884-f306-4536-824a-5a076abb7dba/scratchpad/fw/bio-A` прочитан, не изменялся.
- Актуальный внутренний кандидат: https://github.com/manirko/biotron-firmware/commit/a7739040e07ba7ca04767682755eec8589c15234 (1.10.11,09.10.2026), ветка `codex/biotron-cue-channel-fix-20261009`, проверена свежим fetch02.11.2026. Это не official stable. Флаги A06–A08/Music+Pulse=ON и identity взяты из его `.github/workflows/pull_request_ci.yml`.
- OP-XY COM/M3: https://teenage.engineering/guides/op-xy/com ; подключение MIDI: https://teenage.engineering/guides/op-xy/how-to (прочитаны 02.11.2026).

## Минимальная правка
Только `PLSDK/src/usb_descriptors.c`: bDeviceClass/SubClass/Protocol=EF/02/01; MIDI group перед CDC. Номера интерфейсов уже были 0/1 и 2/3; меняется порядок в массиве, не их номера. High-speed branch не используется RP2040 (full speed). VID/PID, строки продукта, endpoint addresses, два MIDI cable и формат settings не меняются. Никаких новых музыкальных функций.

## Что доказано программно
- Исходный F03 SHA256 и 199-byte config разобраны независимо; envelope UF2/RP2040/последовательность блоков/границы flash/векторы PASS.
- Граница F03 `0x1000c100`, settings начинается `0x10080000`, пересечения нет. Device@`0x1000aa18`, config@`0x1000a950`.
- Все bytes MIDI и CDC function descriptors сохранены относительно official1.8.2 после единственной перестановки. Класс устройства EF/02/01; четыре interface descriptor идут 0,1,2,3. Восемь MIDI jacks и associations `[1,5]`/`[3,7]`, bulk64, notification8, CDC association2/3, power100 мА PASS.
- Build2 и build3 в разных чистых папках = исходный F03 byte-for-byte, SHA256 `e8ac496120541013df1e1f6aee134286ed7e945fc4535276f0d518d34bb87e76` (98816bytes).
- Build1 с `SOURCE_DATE_EPOCH` равным старому settings ID отличался ровно 6 байтами metadata build date. Это была неверная фиксация даты при воспроизведении; скрипт исправлен на 02.11.2026UTC, settings ID оставлен прежним. Build1 не выдаётся.
- Официальный откат R03 скачан заново, SHA256 `d2406d66225f76779331714b0a9df814d1c72d7f3640178f29adc9d3b45b4251` (99840bytes). У него класс 00/00/00 и порядок 2,3,0,1; исправления USB нет.
- Сборки Release для RP2040/pico, firmware1.8.2, ID1765723554. Точные compile_commands/CMakeCache/ELF/логи хранятся в ProjectData. `tools/verify_uf2.py --selftest` проверяет также отказы на испорченные family/address/block sequence/payload length/magic.

## Почему отдельно нужен кандидат 1.10.11
В 1.10.11 та же ошибка USB layout, поэтому применим тот же patch. Эта база уже содержит ранее разработанные исправления MIDI/parser/storage/calibration/serial и Music+Pulse. Их не переносим в 1.8.2 и не выдаём за новые функции этой задачи. Отдельная ветка 1.10.11 должна отличаться от a773904 только USB runtime patch, regression expectation и документами/инструментами проверки. Аппаратный допуск кандидата A06–A08 остаётся открытым. У него другой набор исходников и версия, поэтому это отдельный эксперимент, не замена F03.

## Проверки, которые не выполнены
На 02.11.2026 для Biotron: OP-XY enumeration, MIDI notes/NoteOff, clock, CDC на Windows/Mac, settings roundtrip, физическая процедура BOOT/отката, калибровка/LED/слух — **NOT_RUN**. Playtron-A human PASS на OP-XY из общей карточки не переносится на Biotron. Публикаций, отправок и прошивки из этого проекта нет.

## Реальные ограничения исходной 1.8.2
`src/params.c:get_sys_ex_and_behave` вызывает `clear_flash()` перед `reset_usb_boot()`: переход в BOOT сбрасывает пресеты независимо от отсутствия settings в UF2. Перед входом в BOOT сохранить настройки. `PLSDK/src/usb_descriptors.c` освобождает serial перед чтением строки; это старый дефект базы, USB-only patch его не исправляет. Внутренний 1.10.11 уже содержит статический serial и сохранение pending settings перед BOOT. Даже там сохранение/откат на железе пока не проверены.


## Выполнено для кандидата 1.10.11 · 02.11.2026
- Две чистые сборки `candidate-build-1/2` совпали байт в байт: SHA256 `bf443ff30cb022675d81954e0bc4ccc157f50918f9c9a36dca250259c17f8383`, 114176 bytes. Settings ID 1765723554 и версия 1.10.11 проверены в compile_commands пяти runtime translation units, LED Music+Pulse=ON согласно существующему кандидату.
- `picotool info` подтвердил SDK2.3.0, RP2040/pico, Release, build date 02.11.2026, binary end 0x1000defc. UF2 padded end 0x1000df00, область settings от 0x10080000 не затронута.
- USB device @0x1000cb90, FS config @0x1000cac8; 199 bytes, EF/02/01, MIDI0/1 до CDC2/3. VID/PID CAFE/3011, два MIDI cable и все function bytes после перестановки совпали с official1.8.2. Это сравнение дескрипторов, не остальных исходников этих разных версий.
- Унаследованные 18 host-групп дважды (ASan+UBSan и optimized), release contract и syntax checks PASS. Добавленное ожидание ограничено real full-speed config и device initializer; отрицательные прогоны с CDC-first и class-zero оба отвергнуты. Никакие runtime файлы кроме PLSDK/src/usb_descriptors.c не менялись.
- Результаты: validation/candidate-audit.json, validation/candidate-checks.json; полные логи в ProjectData. Тест для человека: TEST-OPXY.md.
- В GitHub CI тот же source commit проверяется унаследованным workflow. Его Ubuntu UF2 — отдельный artifact с иным compiler, он не подменяет выданный macOS UF2; актуальную квитанцию CI сохранить в delivery manifest.
- Все аппаратные статусы Biotron остаются NOT_RUN; полных релизов, заводских изменений, отправок и автоматической прошивки нет.
