# Проверить весь доступный путь, не повторяя одинаковые прогоны

Сначала W01–W03. Затем один software combined run на Chrome и, если есть, Edge.
Остальные проверки выполнять короткими сессиями по доступным средам.
Порядок: чтение → звук → освобождение портов → DAW/телефон → запись →
отдельно разрешённые изменения настроек/прошивка. Один MIDI owner за раз.

Каждый case получает PASS / FAIL / NOT_RUN / BLOCKED / INCONCLUSIVE.
PASS требует наблюдения именно на записанной связке версий.
Unsupported Web MIDI может быть ожидаемым поведением, но USB test остаётся NOT_RUN.
Проверка на Mac/эмуляция ширины не закрывает Windows или настоящий телефон.

## Первые4 действия Сергея

1. Открыть точный web из README. Записать OS/browser/audio route и firmware label.
2. Без прибора сыграть клавиатурой, выбрать заметно разные звуки.
3. С доступным собственным прибором: Start → сигнал/музыка → Stop10s → Start.
4. Записать первое отличие от ожидания либо PASS; затем дать Claude software-часть.

Если устройства сейчас нет, W03 = NOT_RUN; software и W02 всё равно полезны.

## Desktop, устройство и реальные выходы

| ID | Что сделать | Что зафиксировать |
| --- | --- | --- |
| W01 | До действий записать точный runtime, OS/browser, аудиовыход, USB/adapter, имя порта, PCB и label | Неизвестную PCB/binary оставить UNKNOWN; helper1.8.2 — synthetic input |
| W02 | Без прибора пройти15 звуков: Classic7, Timbres6, Handpan2. Одинаковые ноты/громкость. Включить Limit4, удержать5 клавиш, отпустить все. Сохранить локальный preset и Apply | Какие пары похожи; MIDI notes/октава; слышимость, финальная тишина. Поле прибора не проверять компьютерной клавиатурой |
| W03 | Собственный прибор, известный plant/light stimulus. Start; подержать ноты; Stop;10s касаться источника; Start снова | После Stop тихо, после Start звук возвращается. Измерение после Start записать отдельно от музыки |
| W04 | На Chrome и Edge отдельно удерживать один и тот же понятный stimulus5min | Время первого freeze/quiet/crackle, аудиовыход/нагрузка. При отказе не продолжать бессмысленную выдержку |
| W05 | После release web открыть имеющуюся DAW; записать MIDI clip и послушать получатель | NoteOn/Off, channels/cable, завершение нот, временные метки. MIDI clip не заменяет слух |
| W06 | Три цикла web Stop&release → DAW → закрыть свой DAW MIDI input → reconnect web | Порты освобождаются, музыка возвращается, нет зависших нот; каждый цикл отдельно |
| W07 | По одному: background/return; USB off5s/replug; отдельный Start | Тишина при потере связи, честное состояние UI, восстановление без чужого клиента |
| W08 | Сохранить первый успех; затем offline reopen и comparison sound | Не должно быть ложного Ready. Cold cache и уже открытый cache различать; поддержку offline не выдумывать |
| W09 | Только прочитать Settings/version/update/help/journal текущего устройства | Target/public vs installed, отсутствие downgrade, понятный unknown/unsupported Help. Не нажимать BOOT/install |
| W10 | Только после допуска FIRMWARE-CARD: install/recovery/rollback/reflash | По умолчанию BLOCKED; каждый реально выполненный переход имеет свой hash и backup/restore |
| W11 | Play quick sounds/sphere → All sounds → Classic/Timbres/Handpan; затем Settings | Есть понятный выбор, имя звука/возврат. Сравнить одинаковую фразу; отметить лишнее меню, не редизайнить |
| W12 | Keyboard-only: Tab/ShiftTab, native select arrows/Enter/Escape, details, sliders, copied diagnostics | UI отвечает, не застревает, keys не играют при вводе. На Windows native popup может поглотить Escape: сохранить фактический trace, не ослаблять oracle |
| W13 | Реальный телефон, отдельный проход ниже | OS/app/adapter/audio/USB и результаты по каждому шагу |
| W14 | Физическая active calibration cue/канал и оригинальный NoteOff | BLOCKED без отдельного допуска и контролируемой DAW/audio среды; не вызывать calibration/reset |
| W15 | Только после ownership+identity+свежий свой27-byte backup: одно обычное поле → readback → тот же stimulus → exact restore/readback | Без stimulus NOTMEASURABLE; restore failure STOP. Не тестировать все поля случайным перебором |
| W16 | Creator: Record plant → Stop → Listen → Save WAV → открыть внешним player | Реальный filename/size/duration/слышимость; cancel не теряет запись. MockWAV не заменяет plant take |
| W17 | Обычная камера+WAV → имеющийся CapCut → reopen → export → посмотреть вне редактора | Total time/help/retries, готовый ролик, добровольное желание повторить, что понял зритель |

W02 и W11 — одинаковые ноты при одинаковом уровне. Для high-note treatment
отдельно сравнить вышеMIDI72; на72 blend ещё0. Записать реальные notes,
а не только название октавы. «Разные bytes» не доказывает разные ощущения.
Limit4 относится к browser synth, не к количеству USB MIDI событий прибора.
В feedback дать короткий A/B пример и слова Сергея, без требования одобрить дизайн.

### W15: поля только при отдельной согласованной проверке

Список26 полей и точная последовательность сохранены в `windows/README.md`.
Сначала read-only Export + Check saved settings. Исторический backup другого
дня не применять. Каждое разрешённое поле возвращать отдельно, не в конце серии.
Проверять persistence после отдельного reconnect только после exact restore.
Значок Saved и версия не заменяют прочитанные27bytes. Неизвестный stimulus
не оправдывает дополнительные записи. Factory defaults/reset/calibration исключены.

## Телефон: настоящий, отдельно от компьютера

Открыть точную HTTPS-ссылку. Localhost на телефоне ведёт на сам телефон.
PC Stop&release; не пытаться одновременно владеть одним USB устройством.
Android Chrome/USB-OTG, iOS обычный browser и уже установленный специальный
Web MIDI browser — разные строки отчёта. Ничего не устанавливать ради PASS.

1. Проверить доступность MIDI и честное состояние до connection. Затем Start/Stop10s/Start.
2. По одному проверить stimulus off7s/on10s, USB off5s/replug и background/lock-return.
3. Проверить portrait/landscape, доступ к Stop, Play↔Settings, sphere/Garden drag и выход.
4. Creator Record/Listen/Save/Share-cancel/внешнее открытие; затем W17, если есть CapCut.

Если Web MIDI unsupported, сохранить точный текст и browser version.
Проверки обычной клавиатуры/записи без USB можно отразить отдельно.
OS permission подтверждает владелец, обходов/настроек системы не делать.

## Дескрипторы сегодня и PLSDK

Этот UF2 сохраняет сегодня изменённые6a26781 дескрипторы буквально.
Добавлены только3 интеграционных исправления из REVIEW-PLSDK.
Если установка не допущена, hardware cases ниже = BLOCKED/NOT_RUN;
файловый audit не превращает их в PASS.

| F | Доступная проверка | Доказательство |
| --- | --- | --- |
| F01 | Без USB: hash, existing UF2 selftest, audit candidate+reference | stdout/exit, exactSHA, file PASS отдельно от installation |
| F02 | На уже допущенном exactUF2: Windows и Mac enumeration/Settings read-only | Имена обоих MIDI cable/ports, CDC availability без driver changes, версии и27bytes |
| F03 | После release ПК: имеющийся OP-XY, USB data cable, notes и replug | Видимость COM→M3, OS OP-XY, реальные channels, звучащий receiver, releases. Playtron PASS не Biotron PASS |
| F04 | Boot/recovery/rollback/reflash | Только отдельный допуск; label1.10.11 не отличает разные бинарники |
| F05 | Source review: USB before stdio, public C header, linkage, descriptors, USB task owner | File:line/риск/минимальный воспроизводитель. Review не physical acceptance |
| F06 | Остаточные SDK требования: provenance/product config/PID/HS/factory CDC | OPEN с причиной; без массового SDK update или смены USB identity |

Factory CDC protocol/калибровка/заводской стенд остаются отдельным scope.
Если нет OP-XY/Mac/телефона, не заменять их похожей средой.
Внешние устройства/программы использовать только уже имеющиеся и свободные.

## После первого отказа и на завершении

Сначала timestamp/первый симптом и логи, затем bounded screenshot/DOM,
затем cleanup. Отдельно записать Retry и изменения условий.
Не повторять PASS, чтобы скрыть исходный отказ или сделать красивую сводку.
При утрате cleanup остановить новые jobs; unknown/global processes не kill.
В конце Stop&release, закрыть свои окна/серверы и проверить свой cleanup.
Для отчёта собрать существующим `tester.py bundle` только проверенные человеком файлы.
