#pragma once
#include "Time.h"
#include <string>
#include <vector>
#include <sstream>

// Разделение строки по символу
std::vector<std::string> split(std::string string, const char separator) {
	const int n = std::count(string.begin(), string.end(), separator);
	std::vector<std::string> strs(n + 1);

	int pos = 0;
	for (int i = 0; i <= n; i++) {
		pos = string.find(separator);
		strs[i] = string.substr(0, pos);
		string = string.substr(pos + 1);
	}

	return strs;
}

// Структура для хранения данных о группе и подгруппе
struct Group {
    std::string group = "";

    // Конструктор для установки имени группы
    explicit Group(const std::string& group) : group(group) {}

    // Устанавливает имя группы. Если входная строка содержит пробелы,
    // они будут сохранены, так как нет необходимости разделять на подгруппы.
    void setGroup(const std::string& group_name) {
        this->group = group_name;
    }
};


std::string ScheduleForDayForStudent(std::string group, std::string day) {
    setlocale(LC_ALL, ".UTF8");

    // Обработка специальной команды 'today'
    if (day == "today") {
        day = WhatDay(); // Получение текущего дня недели
    } else if (day == "tomorrow") {
        day = GetTomorrow(); // Получение завтрашнего дня недели
    }

    // Логирование подключения к базе данных
    mongocxx::client connection{uri};
    std::cout << "Подключение к MongoDB установлено." << std::endl;

    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];

    // Логирование запроса к базе данных
    std::cout << "Запрос к коллекции Schedule отправлен." << std::endl;

    auto find_one_result = Collection.find_one({});
    if (!find_one_result) {
        std::cout << u8"Документ не найден" << std::endl;
        return u8"Ошибка: Документ не найден";
    }

    // Логирование полученных данных
    std::string Json = bsoncxx::to_json((*find_one_result).view());
    std::cout << "Полученные данные: " << Json << std::endl;

    json Doc = json::parse(Json);

    // Проверка наличия группы в данных
    std::cout << "Поиск информации для группы " << group << "." << std::endl;
    if (!Doc.contains(group)) {
        std::cout << u8"Информация для группы " << group << u8" не найдена" << std::endl;
        return u8"Ошибка: Информация для группы не найдена";
    }

    // Определение четности недели (это должна быть ваша реализованная функция)
    std::string weekType = WhatWeek();

    // Получение расписания для конкретной недели (четной/нечетной)
    if (!Doc[group].contains(weekType)) {
        std::cout << u8"Информация для недели " << weekType << u8" не найдена" << std::endl;
        return u8"Ошибка: Информация для недели не найдена";
    }
    json weekSchedule = Doc[group][weekType];

    // Поиск информации для дня
    if (!weekSchedule.contains(day)) {
        std::cout << u8"Информация для дня " << day << u8" не найдена" << std::endl;
        return u8"Ошибка: Информация для дня не найдена";
    }

    json daySchedule = weekSchedule[day];
    std::string FormatedLesson = "";

    // Проверка пустоты расписания
    if (daySchedule.empty()) {
        std::cout << "Расписание на день " << day << " пустое." << std::endl;
        return u8"Выходной";
    }

    // Формирование расписания на день
    int lessonNumber = 1; // Переменная для нумерации пар
    for (json::iterator it = daySchedule.begin(); it != daySchedule.end(); ++it) {
        json Lesson = it.value();

        // Проверка на пустое название пары
    if (Lesson["Название пары"].get<std::string>().empty()) {
        lessonNumber++;
        continue; // Пропускаем пустые пары
    }
        // Отображение деталей урока
        std::cout << "Обработка урока номер: " << lessonNumber << std::endl;
        std::cout << "Название пары: " << Lesson["Название пары"].get<std::string>() << std::endl;
        std::cout << "Тип: " << Lesson["Тип"].get<std::string>() << std::endl;
        std::cout << "Преподаватель: " << Lesson["Преподаватель"].get<std::string>() << std::endl;
        std::cout << "Помещение: " << Lesson["Помещение"].get<std::string>() << std::endl;
        std::cout << "Комментарий: " << Lesson.value("Комментарий", "Нет комментария") << std::endl;

        FormatedLesson += std::to_string(lessonNumber) + u8". Название пары: " + Lesson["Название пары"].get<std::string>() +
                        u8"\nТип: " + Lesson["Тип"].get<std::string>() +
                        u8"\nПреподаватель: " + Lesson["Преподаватель"].get<std::string>() +
                        u8"\nПомещение: " + Lesson["Помещение"].get<std::string>() +
                        u8"\nКомментарий: " + Lesson.value("Комментарий", "Нет комментария") + u8"\n\n";

        lessonNumber++; // Увеличение номера пары
}

    // Логирование финального результата
    std::cout << "Формирование итогового расписания." << std::endl;
    return FormatedLesson.empty() ? u8"Выходной" : FormatedLesson;
}


std::string AddCommentary(const std::string& group, int LessonIndex, const std::string& Commentary, const std::string& day, const std::string& Teacher) {
    setlocale(LC_ALL, ".UTF8");

    std::cout << "Начало функции AddCommentary" << std::endl;

    // Проверка корректности названия дня недели
    const std::vector<std::string> WeekDay = { "Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота" };
    if (std::find(WeekDay.begin(), WeekDay.end(), day) == WeekDay.end()) {
        std::cout << u8"Ошибка: Неверное название дня недели" << std::endl;
        return u8"Ошибка: Неверное название дня недели";
    }

    mongocxx::client connection{uri};
    std::cout << "Создано подключение к MongoDB" << std::endl;

    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];
    std::cout << "Выбрана коллекция Schedule" << std::endl;

    auto find_one_result = Collection.find_one({});
    if (!find_one_result) {
        std::cout << u8"Документ не найден" << std::endl;
        return u8"Ошибка: Документ не найден";
    }

    std::string Json = bsoncxx::to_json((*find_one_result).view());
    json Doc = json::parse(Json);
    std::cout << "Документ найден, преобразование в JSON" << std::endl;

    std::string weekType = WhatWeek();
    std::cout << "Текущая неделя: " << weekType << std::endl;

    // Проверяем, есть ли расписание для данной группы и дня
    if (!Doc[group][weekType][day].is_array()) {
        std::cout << "Ошибка: Нет расписания для указанной группы или дня." << std::endl;
        return "Ошибка: Нет расписания для указанной группы или дня.";
    }

    auto& lessons = Doc[group][weekType][day]; // Получаем массив уроков на день
    if (LessonIndex < 0 || LessonIndex >= lessons.size()) {
        std::cout << u8"Ошибка: Индекс урока вне допустимого диапазона" << std::endl;
        return u8"Ошибка: Индекс урока вне допустимого диапазона";
    }

    auto& lesson = lessons[LessonIndex];
    std::cout << "Полученное имя преподавателя в запросе: '" << Teacher << "'" << std::endl;
    std::cout << "Имя преподавателя в расписании для урока с индексом " << LessonIndex << ": '" << lesson["Преподаватель"].get<std::string>() << "'" << std::endl;

    if (lesson["Преподаватель"].get<std::string>() != Teacher) {
        std::cout << u8"Ошибка: Преподаватель не соответствует указанному уроку" << std::endl;
        return u8"Ошибка: Преподаватель не соответствует указанному уроку";
    }

    lesson["Комментарий"] = Commentary;
    std::cout << "Комментарий добавлен к уроку" << std::endl;

    // Обновляем документ в базе данных
    bsoncxx::document::value NewDoc = bsoncxx::from_json(Doc.dump());
    Collection.update_one(find_one_result->view(), bsoncxx::builder::basic::make_document(kvp("$set", NewDoc.view())));

    std::cout << "Документ в базе данных обновлен" << std::endl;
    return u8"Комментарий успешно добавлен";
}


std::string FindGroupAndLesson(const std::string& group, int LessonIndex) {
    std::string day = WhatDay(); // Функция определения текущего дня недели
    std::string weekType = WhatWeek(); // Функция определения четности недели

    mongocxx::client connection{uri};
    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];
    
    auto find_one_result = Collection.find_one({});
    if (!find_one_result) {
        return u8"Ошибка: Документ не найден";
    }

    json Doc = json::parse(bsoncxx::to_json(find_one_result->view()));

    if (!Doc.contains(group) || !Doc[group].contains(weekType) || !Doc[group][weekType].contains(day)) {
        return u8"Расписание для группы или дня не найдено.";
    }

    auto& lessons = Doc[group][weekType][day];
    if (LessonIndex >= lessons.size()) {
        return u8"Индекс пары вне допустимого диапазона.";
    }

    auto& lesson = lessons[LessonIndex];
    if (lesson.is_null() || lesson["Название пары"].get<std::string>().empty()) {
        return u8"Пара не найдена или не существует.";
    }

    std::ostringstream oss;
    oss << "Название пары: " << lesson["Название пары"].get<std::string>() << "\n"
        << "Помещение: " << lesson["Помещение"].get<std::string>();

    return oss.str();
}


// Вспомогательная функция для объединения элементов вектора в строку с разделителем
std::string join(const std::vector<std::string>& v, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) {
            oss << delimiter;
        }
        oss << v[i];
    }
    return oss.str();
}

std::string FindTeacherLocation(const std::string& Teacher, int LessonIndex) {
    std::string day = WhatDay(); // Функция определения текущего дня недели
    std::string weekType = WhatWeek(); // Функция определения четности недели

    mongocxx::client connection{uri};
    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];

    std::vector<std::string> groupsWithTeacher; // Список групп, где учитель ведет урок
    std::string lessonName;
    std::string room;

    auto cursor = Collection.find({});
    for (auto&& doc : cursor) {
        json schedule = json::parse(bsoncxx::to_json(doc));

        for (auto& [group, weekSchedule] : schedule.items()) {
            if (weekSchedule.contains(weekType) && weekSchedule[weekType].contains(day)) {
                auto& daySchedule = weekSchedule[weekType][day];

                if (LessonIndex >= 0 && LessonIndex < daySchedule.size()) {
                    auto& lesson = daySchedule[LessonIndex];

                    if (!lesson.is_null() && lesson["Преподаватель"].get<std::string>() == Teacher) {
                        groupsWithTeacher.push_back(group); // Добавляем группу в список
                        if (lessonName.empty()) { // Записываем название урока и аудиторию, если ещё не записали
                            lessonName = lesson["Название пары"].get<std::string>();
                            room = lesson["Помещение"].get<std::string>();
                        }
                    }
                }
            }
        }
    }

    if (!groupsWithTeacher.empty()) {
        std::ostringstream oss;
        oss << "Преподаватель " << Teacher << " ведет урок у групп: " << join(groupsWithTeacher, ", ") << "\n"
            << "Название пары: " << lessonName << "\n"
            << "Помещение: " << room;
        return oss.str();
    }

    return u8"Информация о местоположении учителя не найдена.";
}


std::string FindNextLessonForStudent(const std::string& group, int LessonIndex) {
    std::string day = WhatDay();
    std::string weekType = WhatWeek();

    mongocxx::client connection{uri};
    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];

    auto cursor = Collection.find({});
    for (auto&& doc : cursor) {
        json schedule = json::parse(bsoncxx::to_json(doc));
        
        if (!schedule.contains(group) || !schedule[group].contains(weekType) || !schedule[group][weekType].contains(day)) {
            continue;  // Пропускаем, если расписание для данной группы или дня не найдено
        }

        auto& daySchedule = schedule[group][weekType][day];
        LessonIndex++; // Переход к следующему уроку

        if (LessonIndex >= daySchedule.size()) {
            continue;  // Пропускаем, если следующей пары нет
        }

        auto& lesson = daySchedule[LessonIndex];
        if (lesson.is_null() || lesson["Название пары"].get<std::string>().empty()) {
            continue;  // Пропускаем, если следующая пара пуста
        }

        std::ostringstream oss;
        oss << "Следующая пара для группы " << group << ":\n"
            << "Название пары: " << lesson["Название пары"].get<std::string>() << "\n"
            << "Помещение: " << lesson["Помещение"].get<std::string>();

        return oss.str(); // Возвращаем информацию о следующей паре
    }

    return u8"Следующей пары нет или расписание не найдено.";
}


std::string FindNextLessonForTeacher(const std::string& Teacher, int LessonIndex) {
    std::string day = WhatDay();
    std::string weekType = WhatWeek();

    mongocxx::client connection{uri};
    auto DB = connection["Project"];
    auto Collection = DB["Schedule"];

    auto cursor = Collection.find({});
    std::vector<std::string> groupsWithTeacher;
    std::string nextLessonName;
    std::string nextLessonRoom;

    for (auto&& doc : cursor) {
        json schedule = json::parse(bsoncxx::to_json(doc));
        
        for (auto& [group, weekSchedule] : schedule.items()) {
            if (!weekSchedule.contains(weekType) || !weekSchedule[weekType].contains(day)) {
                continue; // Пропускаем, если нет расписания для этой группы в данный день
            }

            auto& daySchedule = weekSchedule[weekType][day];
            if (LessonIndex < daySchedule.size()) {
                auto& lesson = daySchedule[LessonIndex];
                if (!lesson.is_null() && lesson["Преподаватель"].get<std::string>() == Teacher) {
                    groupsWithTeacher.push_back(group); // Добавляем группу в список

                    if (nextLessonName.empty()) { // Запоминаем информацию о следующем уроке
                        nextLessonName = lesson["Название пары"].get<std::string>();
                        nextLessonRoom = lesson["Помещение"].get<std::string>();
                    }
                }
            }
        }
    }

    if (!groupsWithTeacher.empty()) {
        std::ostringstream oss;
        oss << "Следующая пара преподавателя " << Teacher << ":\n"
            << "Название пары: " << nextLessonName << "\n"
            << "Помещение: " << nextLessonRoom << "\n"
            << "Группы: ";
    for (const auto& group : groupsWithTeacher) {
        oss << group << ", ";
    }
    oss.seekp(-2, std::ios_base::end); // Удалить последнюю запятую
    return oss.str();
    } else {
        return u8"Для указанного учителя не нашлось следующей пары.";
        }
}


// Определяем в какую функцию послать обрабатываться данные
std::string getData(const std::string& action_code, const std::string& name, const std::string& group, const std::string& day, const std::string& strLessonIndex = "", const std::string& Commentary = "", const std::string& Teacher = "") {
    setlocale(LC_ALL, "ru");

    if (action_code == "scheduleFor") {
        return ScheduleForDayForStudent(group, day);

    return u8"Произошла ошибка! Попробуйте ещё раз позже, пожалуйста.";
    }
}