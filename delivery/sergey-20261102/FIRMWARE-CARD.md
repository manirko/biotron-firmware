# Отдельная карточка прошивки · по умолчанию BLOCKED

Сначала проверить web и текущую прошивку. Этот пакет **не разрешает установку**.
Новый кандидат нужен для ручной проверки today USB descriptors + PLSDK fixes.
Не понижать неизвестную/более новую прошивку, в частности label1.10.19, ради теста.

## Точные файлы

| Назначение | Путь / размер / SHA256 |
| --- | --- |
| Новый PLSDK кандидат | `firmware/test/biotron-firmware_v1.10.11.uf2`;114176bytes; `7900c23989635b8a35890a17bc2e3cc84735c781f77be2f4bee7ca94198c7f42` |
| Официальный rollback1.8.2 | `firmware/rollback/biotron-firmware_v1.8.2.uf2`;99840bytes; `d2406d66225f76779331714b0a9df814d1c72d7f3640178f29adc9d3b45b4251` |

Source exact81a7f820; built runtime1fe17a76. Pre-release:
https://github.com/manirko/biotron-firmware/releases/tag/1.10.11-plsdk-test.20261102

**Не путать** с прежним descriptor-only1.10.11 /6a26781:
SHA `bf443ff30cb022675d81954e0bc4ccc157f50918f9c9a36dca250259c17f8383`.
Он не лежит в `firmware/test`. Одинаковый номер версии не доказывает один бинарник.
Web updater этого пакета использует public-stable, не новый internalUF2.
Не выбирать latest или «самый новый filename» вместо точногоSHA.

## До любого BOOT/flash

Нужны подтверждённые **собственные A06–A08**, свежие ownership и свободный USB,
считанный с этого прибора полный backup26fields/27bytes, проверенный restore,
выбранный владельцем exact target, подходящий rollback и доказанный recovery.
Точные собственные PCB/BOOT photos, contacts/tool/power/removal sequence должны
быть известны заранее. Generic `RPI-RP2` не идентифицирует плату Biotron.
Не угадывать контакты, не давать общую инструкцию «замкнуть два пина».

Rollback1.8.2 возвращает stable, **не прежнюю внутреннюю прошивку**.
В старой1.8.2 вход в BOOT может очистить presets. Полный возврат старого
состояния этим файлом не гарантирован; без нужного собственного rollback — BLOCKED.
Лейбл версии не заменяет backup, PCB, происхождение бинарника или совместимость.
Если хоть один пункт неизвестен, только file audit и read-only; W10/F04 BLOCKED.

## Четыре шага после отдельного допуска владельца

1. **Зафиксировать baseline.** PCB, exact installed identity насколько известно,
   свой backup/hash, текущая игра и Settings readback. Повторно сверить targetSHA.
2. **Выполнить один переход.** Только заранее подтверждённый способ BOOT и
   вручную выбранный exactUF2. Не повторять запись после неясного результата.
3. **Проверить результат.** Enumeration, label, Settings readback, игра и replug;
   каждое наблюдение отдельно. Для OP-XY предварительно release ПК.
4. **Зафиксировать recovery.** BOOT/rollback/reflash выполнять только если
   эти конкретные переходы также допущены. Каждый имеет свой fileSHA и результат.

При недоступном recovery, несовпавшем readback, потере restore, неожиданном
потреблении/поведении — STOP. Не reset/calibration/JIG/OS drivers.
Не тянуть старый backup на другой schema. «Диск исчез» не installation PASS;
одного version reply тоже недостаточно. Обязательны реальные USB/звук/readback.

Физические переходы в этой подготовке NOT_RUN. Ранее PASS относятся к host,
бинарному audit и сборкам. Никаких автоматических flashing scripts здесь нет.
