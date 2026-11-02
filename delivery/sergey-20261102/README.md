# Biotron · полный пакет Сергея · 02.11.2026 · v2

Открой **START-HERE.html**. Распакуй весь архив в обычную короткую папку,
например `C:\Biotron\Sergey-v2`. Запускать из окна ZIP нельзя.

Четыре действия: **первый звук → автоматические проверки → ручные проверки
доступных устройств → один отчёт**. Ночной запуск не нужен.

## Как устроен пакет

`windows/runtime` → существующий локальный tester → установленные Chrome/Edge →
`windows/results` → просмотр человеком → `tester.py bundle`.
Телефон проверяет человек по HTTPS. Прошивка проверяется как файл;
физическая установка требует отдельного допуска из FIRMWARE-CARD.md.

| Путь | Что открыть |
| --- | --- |
| START-HERE.html | Первый проход и ссылки на всё остальное; работает офлайн |
| CLAUDE.md | Готовое задание Claude, команды, остановки и границы |
| TEST-PLAN.md | Полная матрица W01–W17, Windows/телефон/DAW/OP-XY |
| REPORT-TEMPLATE.md | Скопировать в `windows/results/REPORT.md`, заполнить фактами |
| FIRMWARE-CARD.md | Точный новый UF2, ограничения плат, backup/recovery, отдельный допуск |
| REVIEW-PLSDK-20261102.md | Разбор интеграции SDK и оставшиеся требования |
| windows/ | Неизменный, уже проверенный Windows r2 от30Oct; ещё раз распаковывать не надо |
| source/firmware/ | Полные исходники firmware exact81a7f820, GPL и существующие host-тесты |
| windows/source-review.zip | Исходники web exacte02b11e, лицензии и source manifest |
| firmware/test/ | Только новый внутренний кандидат; не устанавливается тестером |
| firmware/rollback/ | Официальный1.8.2; не эквивалент прежней внутренней прошивке |
| tools/verify_uf2.py | Существующая проверка файла без доступа к USB |
| evidence/ | Прежние software-доказательства, diff и происхождение файлов |
| packet-manifest.json | SHA256 всех исходных файлов этого комплекта |

## Зафиксированные версии

- Web: `e02b11eff36f99623eabbabced154475b06ed7a4`.
- Helper: `a00680b5b90f13c6c3347583279059527e76ab54`.
- Firmware source: `81a7f820c6004fcd6602b02f0748bbce62c472de`;
  собранный runtime — `1fe17a762ac583856cc000d3d066bf4cd4c560a9`.
- Новый UF2: `7900c23989635b8a35890a17bc2e3cc84735c781f77be2f4bee7ca94198c7f42`.
- [Точный web](https://ea4501a5.biotron-settings-beta.pages.dev/#/biotron/play?sound=1),
  [Creator](https://ea4501a5.biotron-settings-beta.pages.dev/creator),
  [журнал](https://ea4501a5.biotron-settings-beta.pages.dev/firmware-journal).

Сайт может требовать интернет. Его hosted bytes в этой упаковке заново не
проверялись; локальный runtime закреплён SHA. Постоянный домен не использовать
для сравнения версий. Web updater работает с public-stable, а не с новым UF2.
Поле `firmware_version=1.8.2` в helper environment описывает его mock-input,
**не прошивку подключённого прибора**.

## Что уже проверено ИИ и что предстоит

Ранее проверены18 firmware host-групп в sanitizer/optimized, fault controls,
две одинаковые ARM-сборки, бинарные дескрипторы, CI и45 selftests Windows helper.
Их receipts приложены. Эти результаты не заменяют прогоны Сергея.

В этой v2 меняются упаковка и инструкции. Web/helper/firmware bytes сохранены.
Windows, реальный телефон, USB, слух, DAW, OP-XY, запись и установка прошивки
не получают PASS при подготовке архива. Неизвестное остаётся UNKNOWN;
недоступное — NOT_RUN; закрытый допуск — BLOCKED.

Приложенные старые README/тестовые карточки в `source/firmware` и `windows/`
сохранены как документы своих версий. Текущие команды брать из корневого
CLAUDE.md; кандидат прошивки — только из корневого FIRMWARE-CARD.md.

## Разработка и безопасность

Не менять runtime, тестовые входы, manifest и первый неуспешный результат.
Правки — в отдельной work-копии после воспроизведения; один writer на репозиторий.
Без автоматических установок, обновления зависимостей, upload, рассылок,
BOOT/flash/reset/calibration и изменения OS permissions.
Исходники не содержат Git credentials или `.git`. Новых сервисов и реестров нет.

## Проверка упаковки v2 · 02Nov

257 исходных файлов Windows r2 совпали SHA;115 firmware blobs совпали с Git81a7f820.
Из перемещённой копии выполнены verify, doctor, cases, UF2 selftest/audit и
экспорт REPORT существующим bundle. Стартовая страница просмотрена на1440/320px:
локальные ссылки, отсутствие горизонтального overflow, disclosure мышью и
клавиатурой проверены. Собственные browser contexts/API закрыты.
Новых runtime/hardware прогонов эта документационная упаковка не заявляет.
