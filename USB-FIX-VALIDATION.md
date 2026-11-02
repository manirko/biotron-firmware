# Независимая проверка Biotron USB Fix · 02.11.2026

## Источники
- Official1.8.2/master: https://github.com/Playtronica/biotron-firmware/commit/67920c21d51866576075308529a055f10be164dc ; tag1.8.2.
- Official release, published14.12.2025: https://github.com/Playtronica/biotron-firmware/releases/tag/1.8.2 ; скачан заново02.11.2026, SHA256 совпал с GitHub asset digest и R03.
- Исходный F03 и manifest: `~/ProjectData/Playtronica/aiplay/playtron-opxy-usb-2026-11-02/delivery-current-r2/`; Claude source worktree `d38d1884-f306-4536-824a-5a076abb7dba/scratchpad/fw/bio-A` прочитан, не изменялся.
- Актуальный внутренний кандидат: https://github.com/manirko/biotron-firmware/commit/a7739040e07ba7ca04767682755eec8589c15234 (1.10.11,09.10.2026), ветка `codex/biotron-cue-channel-fix-20261009`, проверена свежим fetch02.11.2026. Это не official stable. Флаги A06–A08/Music+Pulse=ON и identity взяты из его `.github/workflows/pull_request_ci.yml`.
- OP-XY COM/M3: https://teenage.engineering/guides/op-xy/com ; подключение MIDI: https://teenage.engineering/guides/op-xy/how-to (прочитаны02.11.2026).

## Минимальная правка
Только `PLSDK/src/usb_descriptors.c`: bDeviceClass/SubClass/Protocol=EF/02/01; MIDI group перед CDC. Номера интерфейсов уже были0/1 и2/3; меняется порядок в массиве, не их номера. High-speed branch не используется RP2040 (full speed). VID/PID, строки продукта, endpoint addresses, два MIDI cable и формат settings не меняются. Никаких новых музыкальных функций.

## Что доказано программно
- Исходный F03 SHA256 и199-byte config разобраны независимо; envelope UF2/RP2040/последовательность блоков/границы flash/векторы PASS.
- Граница F03 `0x1000c100`, settings начинается `0x10080000`, пересечения нет. Device@`0x1000aa18`, config@`0x1000a950`.
- Все bytes MIDI и CDC function descriptors сохранены относительно official1.8.2 после единственной перестановки. Класс устройства EF/02/01; четыре interface descriptor идут0,1,2,3. Восемь MIDI jacks и associations `[1,5]`/`[3,7]`, bulk64, notification8, CDC association2/3, power100мА PASS.
- Build2 и build3 в разных чистых папках = исходный F03 byte-for-byte, SHA256 `e8ac496120541013df1e1f6aee134286ed7e945fc4535276f0d518d34bb87e76` (98816bytes).
- Build1 с `SOURCE_DATE_EPOCH` равным старому settings ID отличался ровно6байтами metadata build date. Это была неверная фиксация даты при воспроизведении; скрипт исправлен на02.11.2026UTC, settings ID оставлен прежним. Build1 не выдаётся.
- Официальный откат R03 скачан заново, SHA256 `d2406d66225f76779331714b0a9df814d1c72d7f3640178f29adc9d3b45b4251` (99840bytes). У него класс00/00/00 и порядок2,3,0,1; исправления USB нет.
- Сборки Release для RP2040/pico, firmware1.8.2, ID1765723554. Точные compile_commands/CMakeCache/ELF/логи хранятся в ProjectData. `tools/verify_uf2.py --selftest` проверяет также отказы на испорченные family/address/block sequence/payload length/magic.

## Почему отдельно нужен кандидат1.10.11
В1.10.11 та же ошибка USB layout, поэтому применим тот же patch. Эта база уже содержит ранее разработанные исправления MIDI/parser/storage/calibration/serial и Music+Pulse. Их не переносим в1.8.2 и не выдаём за новые функции этой задачи. Отдельная ветка1.10.11 должна отличаться отa773904 только USB runtime patch, regression expectation и документами/инструментами проверки. Аппаратный допуск кандидата A06–A08 остаётся открытым. У него другой набор исходников и версия, поэтому это отдельный эксперимент, не заменаF03.

## Проверки, которые не выполнены
На02.11.2026 для Biotron: OP-XY enumeration, MIDI notes/NoteOff, clock, CDC наWindows/Mac, settings roundtrip, физическая процедура BOOT/отката, калибровка/LED/слух — **NOT_RUN**. Playtron-A human PASS наOP-XY из общей карточки не переносится наBiotron. Публикаций, отправок и прошивки из этого проекта нет.

## Реальные ограничения исходной1.8.2
`src/params.c:get_sys_ex_and_behave` вызывает `clear_flash()` перед `reset_usb_boot()`: переход вBOOT сбрасывает пресеты независимо от отсутствия settings вUF2. Перед входом вBOOT сохранить настройки. `PLSDK/src/usb_descriptors.c` освобождает serial перед чтением строки; это старый дефект базы, USB-only patch его не исправляет. Внутренний1.10.11 уже содержит статический serial и сохранение pending settings передBOOT. Даже там сохранение/откат на железе пока не проверены.
