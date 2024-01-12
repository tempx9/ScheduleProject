#pragma once
#include <ctime>
#include <string>

using std::string;

const string WeekDay[] = { "Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота" };

// Высчитываем чётная сейчас неделя или нечётная
std::string WhatWeek() {
    std::string Week = "Нечётная неделя"; // Начальное значение

    // Текущее время
    time_t now = time(nullptr);
    struct tm CurrentTime;
    localtime_r(&now, &CurrentTime);

    // Опорная дата - 1 сентября текущего учебного года
    tm StartOfTheYear = {};
    StartOfTheYear.tm_year = CurrentTime.tm_year; // Установим текущий год
    StartOfTheYear.tm_mon = 8; // Сентябрь (с нуля)
    StartOfTheYear.tm_mday = 4; // Четвёртое число
    mktime(&StartOfTheYear); // Нормализация структуры tm

    // Вычисление разницы в днях
    int dayDiff = CurrentTime.tm_yday - StartOfTheYear.tm_yday;
    if (dayDiff < 0) {
        // Если текущий день года меньше, чем день года 1 сентября,
        // это означает, что сейчас новый год, но StartOfTheYear еще не обновлен.
        StartOfTheYear.tm_year = CurrentTime.tm_year - 1; // Установим предыдущий год
        mktime(&StartOfTheYear); // Нормализация структуры tm
        dayDiff = CurrentTime.tm_yday + (365 - StartOfTheYear.tm_yday);
    }

    // Определение чётности недели
    if ((dayDiff / 7) % 2 == 0) {
        Week = "Чётная неделя";
    }

    return Week;
}

int NumberDayOfWeek() {
    time_t now = time(nullptr);

    struct tm CurrentTime;
    localtime_r(&now, &CurrentTime); // Используем localtime_r

    return CurrentTime.tm_wday;
}

std::string GetTomorrow() {
    time_t now = time(nullptr);
    struct tm CurrentTime;
    localtime_r(&now, &CurrentTime);

    int tomorrow_wday = CurrentTime.tm_wday + 1;
    if (tomorrow_wday > 6) { // Воскресенье имеет индекс 0
        tomorrow_wday = 0;
    }

    return WeekDay[tomorrow_wday];
}

string WhatDay() {
    return WeekDay[NumberDayOfWeek()];
}