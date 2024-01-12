#include <iostream>
#include <string>
#include <vector>


#include <codecvt>
#include <cstdint>
#include <locale>


#include <jwt.h>
#include <chrono>

#define CPPHTTPLIB_FORM_URL_ENCODED_PAYLOAD_MAX_LENGTH 1024 * 1024 * 255
#include <nlohmann/json.hpp>
#include <httplib.h>

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using namespace httplib;
using namespace std;
using json = nlohmann::json;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

const string SECRET = "123"; // У Алексея уточнить, что здесь
const auto uri = mongocxx::uri{ "mongodb+srv://temp:19282031@schedulecfu.yyfmhpu.mongodb.net/" };

#include "ForMongoDB.h"


struct JWT_token_p {
    int expires_at;
    std::string name;
    std::string role; // Одна роль: teacher, student или admin
};

void getScheduleForTheUser(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    string action_code = req.has_param("actionCode") ? req.get_param_value("actionCode") : "";
    string JWT_token = req.has_param("JWTtoken") ? req.get_param_value("JWTtoken") : "";
    string group = req.has_param("group") ? req.get_param_value("group") : "";
    string day = req.has_param("day") ? req.get_param_value("day") : "";

    if (action_code.empty() || JWT_token.empty() || group.empty() || day.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        jwt_t* jwt = nullptr;
        int ret = jwt_decode(&jwt, JWT_token.c_str(), (const unsigned char*)SECRET.c_str(), SECRET.length());
        if (ret != 0) {
            throw runtime_error("JWT decoding failed");
        }

        JWT_token_p token;
        token.expires_at = jwt_get_grant_int(jwt, "expires_at");
        token.name = jwt_get_grant(jwt, "name") ? jwt_get_grant(jwt, "name") : "";
        token.role = jwt_get_grant(jwt, "role") ? jwt_get_grant(jwt, "role") : "";

        jwt_free(jwt); // Очистка ресурсов JWT

        if (token.expires_at < chrono::seconds(time(NULL)).count()) {
            cout << "Время жизни токена истекло!" << endl;
            res.set_content("Error401.", "text/plain");
            return;
        }

        if (token.role != "student" && token.role != "teacher" && token.role != "admin") {
            cout << "У " << token.name << " не хватает прав!" << endl;
            res.set_content("У вас не хватает прав!", "text/plain");
            return;
        }

        // Вызов функции getData без переменной parameters
        string response = getData(action_code, token.name, group, day);
        res.set_content(response, "text/plain");
    }
    catch (const exception& e) {
        cerr << "Error in getScheduleForTheUser: " << e.what() << endl;
        res.set_content("Error processing request", "text/plain");
    }
}


void addCommentaryToLesson(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    std::string JWT_token = req.has_param("JWTtoken") ? req.get_param_value("JWTtoken") : "";
    std::string group = req.has_param("group") ? req.get_param_value("group") : "";
    std::string strLessonIndex = req.has_param("LessonIndex") ? req.get_param_value("LessonIndex") : "";
    std::string Commentary = req.has_param("Commentary") ? req.get_param_value("Commentary") : "";
    std::string Day = req.has_param("Day") ? req.get_param_value("Day") : "";
    std::string Teacher = req.has_param("Teacher") ? req.get_param_value("Teacher") : "";
    int LessonIndex;

    if (JWT_token.empty() || group.empty() || strLessonIndex.empty() || Commentary.empty() || Day.empty() || Teacher.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        LessonIndex = std::stoi(strLessonIndex) - 1; // Пользователи вводят номера начиная с 1
        if (LessonIndex < 0 || LessonIndex >= 7) { // Предполагаем, что в день может быть до 7 уроков
            res.set_content(u8"Ошибка: Индекс урока вне допустимого диапазона", "text/plain");
            return;
        }
    } catch (const std::exception& e) {
        res.set_content(u8"Ошибка: Неверный формат индекса урока", "text/plain");
        return;
    }

    try {
        // Раскодирование JWT и проверка валидности токена
        jwt_t* jwt = nullptr;
        int ret = jwt_decode(&jwt, JWT_token.c_str(), (const unsigned char*)SECRET.c_str(), SECRET.length());
        if (ret != 0) {
            throw std::runtime_error("JWT decoding failed");
        }

        // Проверка истечения срока действия токена
        auto token_expires_at = jwt_get_grant_int(jwt, "expires_at");
        if (token_expires_at < std::chrono::seconds(time(nullptr)).count()) {
            jwt_free(jwt);
            res.set_content("Error401: Token expired.", "text/plain");
            return;
        }

        // Проверка роли пользователя
        string role = jwt_get_grant(jwt, "role") ? jwt_get_grant(jwt, "role") : "";
        jwt_free(jwt);
        if (role != "teacher" && role != "admin") {
            res.set_content("Error403: Insufficient permissions.", "text/plain");
            return;
        }

        // Вызов функции AddCommentary
        std::string response = AddCommentary(group, LessonIndex, Commentary, Day, Teacher);
        res.set_content(response, "text/plain");
    } catch (const std::exception& e) {
        std::cerr << "Exception in addCommentaryToLesson: " << e.what() << std::endl;
        res.set_content("Error processing request", "text/plain");
    }
}


void WhereGroup(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    std::string group = req.has_param("group") ? req.get_param_value("group") : "";
    std::string strLessonIndex = req.has_param("LessonIndex") ? req.get_param_value("LessonIndex") : "";
    int LessonIndex;

    if (group.empty() || strLessonIndex.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        LessonIndex = std::stoi(strLessonIndex) - 1; // Пользователи вводят номера начиная с 1
    } catch (const std::exception&) {
        res.set_content(u8"Ошибка: Неверный формат индекса пары.", "text/plain");
        return;
    }

    if (LessonIndex < 0 || LessonIndex >= 7) {
        res.set_content(u8"Ошибка: Индекс пары вне допустимого диапазона.", "text/plain");
        return;
    }

    // Вызываем функцию поиска группы и пары
    std::string result = FindGroupAndLesson(group, LessonIndex);
    res.set_content(result, "text/plain");
}


void WhereTeacher(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    std::string Teacher = req.has_param("Teacher") ? req.get_param_value("Teacher") : "";
    std::string strLessonIndex = req.has_param("LessonIndex") ? req.get_param_value("LessonIndex") : "";
    int LessonIndex;

    if (Teacher.empty() || strLessonIndex.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        LessonIndex = std::stoi(strLessonIndex) - 1; // Пользователи вводят номера начиная с 1
    } catch (const std::exception&) {
        res.set_content(u8"Ошибка: Неверный формат индекса пары.", "text/plain");
        return;
    }

    // Вызываем функцию поиска информации о местоположении учителя
    std::string result = FindTeacherLocation(Teacher, LessonIndex);
    res.set_content(result, "text/plain");
}


void NextLessonForStudent(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    std::string group = req.has_param("group") ? req.get_param_value("group") : "";
    std::string strLessonIndex = req.has_param("LessonIndex") ? req.get_param_value("LessonIndex") : "";
    int LessonIndex;

    if (group.empty() || strLessonIndex.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        LessonIndex = std::stoi(strLessonIndex) - 1; // Пользователи вводят номера начиная с 1
    } catch (const std::exception&) {
        res.set_content(u8"Ошибка: Неверный формат индекса пары.", "text/plain");
        return;
    }

    std::string result = FindNextLessonForStudent(group, LessonIndex);
    res.set_content(result, "text/plain");
}


void NextLessonForTeacher(const httplib::Request& req, httplib::Response& res) {
    setlocale(LC_ALL, "Russian");

    std::string Teacher = req.has_param("Teacher") ? req.get_param_value("Teacher") : "";
    std::string strLessonIndex = req.has_param("LessonIndex") ? req.get_param_value("LessonIndex") : "";
    int LessonIndex;

    if (Teacher.empty() || strLessonIndex.empty()) {
        res.set_content(u8"Неверные параметры запроса!", "text/plain");
        return;
    }

    try {
        LessonIndex = std::stoi(strLessonIndex) - 1; // Пользователи вводят номера начиная с 1
    } catch (const std::exception&) {
        res.set_content(u8"Ошибка: Неверный формат индекса пары.", "text/plain");
        return;
    }

    std::string result = FindNextLessonForTeacher(Teacher, LessonIndex);
    res.set_content(result, "text/plain");
}


// Сюда поступает запрос на обновление расписания в базе данных
void UpdateSchedule(const httplib::Request& req, httplib::Response& res) {
    if (req.body.empty()) {
        cout << "Empty Schedule\n";
        res.status = 400; // Устанавливаем статус 400 Bad Request
        res.set_content("Empty schedule", "text/plain");
        return;
    }

    try {
        auto doc = bsoncxx::from_json(req.body);

        mongocxx::client connection{uri};
        auto DB = connection["Project"];
        auto Collection = DB["Schedule"];

        auto find_one_result = Collection.find_one({});
        if (!find_one_result) {
            cout << "Document not found\n";
            res.status = 404; // Устанавливаем статус 404 Not Found
            res.set_content("Document not found", "text/plain");
            return;
        }

        Collection.update_one(find_one_result->view(), make_document(kvp("$set", doc.view())));

        res.status = 200; // Устанавливаем статус 200 OK
        res.set_content("Schedule updated", "text/plain");
    } catch (const std::exception& e) {
        cout << "Exception occurred: " << e.what() << endl;
        res.status = 500; // Устанавливаем статус 500 Internal Server Error
        res.set_content("Error processing request", "text/plain");
    }
}


// Запуск сервера
int main() {
	mongocxx::instance inst{};

	Server server;
	server.set_payload_max_length(1000000); // Потому что json расписания жирный

	server.Get("/getSchedule", getScheduleForTheUser); // Для обработки запросов от пользователя (Бота)
	//server.Get("/ScheduleForTomorrowForTeacher", ScheduleForTomorrowForTeacher);
	server.Get("/NextLessonForTeacher", NextLessonForTeacher);
	server.Get("/NextLessonForStudent", NextLessonForStudent);
	server.Get("/WhereGroup", WhereGroup); // Для поиска группы
	server.Get("/WhereTeacher", WhereTeacher); // Для поиска учителя
	server.Get("/AddCommentary", addCommentaryToLesson); // Для обновления комментария
	server.Post("/UpdateSchedule", UpdateSchedule); // Для обновления расписания

	server.listen("0.0.0.0", 8089);
}