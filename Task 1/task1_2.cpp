// Подключение библиотек
#include <iostream>
#include <vector>
#include <tuple>
#include <limits>
#include <unordered_map>
#include <string>
#include <iomanip>
#include <algorithm>

// Структура для хранения информации о таблицах
struct Table {
    // Количество строк в таблице
    double rows;  

    // Кардинальности атрибутов
    std::unordered_map<std::string, double> cardinalities;  
};

// Структура для хранения информации о результате джоина
struct JoinResult {
    // Стоимость джоина
    double cost;  

    // План джоина
    std::string plan;  

    // Количество строк результата джоина
    double resultRows;  
};

// Вектор для хранения всех таблиц
std::vector<Table> tables;

// Предикаты для джоинов
std::vector<std::tuple<int, int, std::string, std::string>> joinPredicates;  

// Предикаты для сканов
std::vector<std::tuple<int, std::string>> scanPredicates;  

// Количество таблиц
int l; 

// Функция для применения предиката скана
double applyScanPredicate(int tableIndex, const std::string& filterAttr) {
    // Получаем кардинальность 
    double cardinality = tables[tableIndex - 1].cardinalities[filterAttr];

    // Получаем количество строк в таблице
    double rows = tables[tableIndex - 1].rows;

    // Возвращает количество строк после применения фильтра
    return rows / cardinality; 
}

// Функция для получения кардинальности джоина
double getCardinalityForJoin(
    int left, 
    int right, 
    const std::string& joinAttr1, 
    const std::string& joinAttr2
) {
    // Произведение количества строк
    double rowsTimes = tables[left - 1].rows * tables[right - 1].rows;  

    // Максимальная кардинальность
    double maxOfAttr = std::max(tables[left - 1].cardinalities[joinAttr1], tables[right - 1].cardinalities[joinAttr2]);  

    // Если максимальная кардинальность равна нулю, 
    // возвращаем большое число, чтобы джоин был недопустимым
    if (maxOfAttr == 0) {
        // Возвращает большое число, если кардинальности равны нулю
        return std::numeric_limits<double>::max(); 
    }

    // Возвращает кардинальность джоина
    return rowsTimes / maxOfAttr;  
}

// Функция для вычисления стоимости джоина
JoinResult computeJoinCost(
    int left, 
    int right, 
    const std::string& joinAttr1, 
    const std::string& joinAttr2,
    const JoinResult&  leftResult,
    const JoinResult&  rightResult
) {
    // Количество строк в левой таблице
    double leftRows = tables[left - 1].rows;  

    // Количество строк в правой таблице
    double rightRows = tables[right - 1].rows;  

    // Получаем кардинальность джоина
    double cardinality = getCardinalityForJoin(left, right, joinAttr1, joinAttr2);  

    // Если кардинальность равна нулю, возвращаем максимальное число
    if (cardinality == std::numeric_limits<double>::max()) {
        // Обработка случая невозможного джоина
        return { std::numeric_limits<double>::max(), "", 0 };
    }

    // Количество строк в результате джоина
    double resultRows = (leftRows * rightRows) / cardinality;  

    // Стоимости для джоинов через хеш-джоин и вложенный цикл алгоритмы
    double hashJoinCost   = leftResult.cost + (leftRows * 3.5) 
    + rightResult.cost + (rightRows * 1.5) + (resultRows * 0.1);

    double nestedLoopCost = leftRows * rightRows + (resultRows * 0.1);

    // Выбираем минимальную стоимость
    double cost = std::min(hashJoinCost, nestedLoopCost);  

    // Формируем план джоина
    std::string plan = "(" + std::to_string(left) + " " 
    + std::to_string(right) + " {" + joinAttr1 + " " 
    + joinAttr2 + "})"; 
    
    // Возвращаем результат джоина
    return { cost, plan, resultRows };  
}

// Функция для вычисления стоимости дерева джоинов
JoinResult computeJoinTreeCost(const std::vector<int>& tablesToJoin) {
    // Если осталось только одна таблица
    if (tablesToJoin.size() == 1) {  
        // Начальный индекс таблицы
        int tableIndex = tablesToJoin[0];

        // Табличный индекс присваивается плану
        std::string plan = std::to_string(tableIndex);

        // Получаем количество строк в таблице
        double resultRows = tables[tableIndex - 1].rows;

        // Применяем фильтры на сканах
        for (const auto& filter : scanPredicates) { 
            // Количество строк после применения фильтра
            int filterTableNum;

            // Распаковываем предикаты фильтрации
            std::string filterAttr;
            std::tie(filterTableNum, filterAttr) = filter;

            // Если предикат относится к текущей таблице, 
            // добавляем его в план и применяем фильтр
            if (filterTableNum == tableIndex) {
                // К плану добавляется предикат фильтрации
                plan += filterAttr;

                // Применяем фильтр к количеству строк
                resultRows = applyScanPredicate(tableIndex, filterAttr); 
            }
        }

        // Возвращаем результат
        return { 0, plan, resultRows }; 
    }

    // Минимальная стоимость равна максимальному значению
    double minCost = std::numeric_limits<double>::max();

    // Строка для формирования плана
    std::string bestPlan;

    // Количество строк в результате
    double bestResultRows = 0;

    // Пробуем все возможные разбиения списка таблиц
    for (size_t i = 1; i < tablesToJoin.size(); ++i) {
        // Левая таблица 
        std::vector<int> leftTables(
            tablesToJoin.begin(), 
            tablesToJoin.begin() + i
        );

        // Правая таблица
        std::vector<int> rightTables(
            tablesToJoin.begin() + i, 
            tablesToJoin.end()
        );

        // Рекурсивно вычисляем результат для левой части
        JoinResult leftResult = computeJoinTreeCost(leftTables);  

        // Рекурсивно вычисляем результат для правой части
        JoinResult rightResult = computeJoinTreeCost(rightTables);  

        // Флаг, указывающий, найден ли джоин
        bool foundJoin = false;  

        // Проверяем, найден ли джоин для текущих таблиц
        for (const auto& joinPred : joinPredicates) {
            // Распаковываем предикат джоина
            auto [
                table1, 
                table2, 
                attr1, 
                attr2
            ] = joinPred;

            // Проверяем, что таблицы в предикате содержатся в левой или правой части
            if ((std::find(leftTables.begin(), leftTables.end(), table1) != leftTables.end() &&
                 std::find(rightTables.begin(), rightTables.end(), table2) != rightTables.end()) ||
                (std::find(leftTables.begin(), leftTables.end(), table2) != leftTables.end() &&
                 std::find(rightTables.begin(), rightTables.end(), table1) != rightTables.end())) {
                    
                // Вычисляем результат джоина
                JoinResult joinResult = computeJoinCost(table1, table2, attr1, attr2, leftResult, rightResult);  
                
                // Считаем текущую стоимость
                double currentCost = leftResult.cost + rightResult.cost + joinResult.cost;  

                // Если стоимость меньше минимальной
                if (currentCost < minCost) { 
                    // Обновляем минимальную стоимость 
                    minCost = currentCost;

                    // Формируем лучший план
                    bestPlan = "(" + leftResult.plan + " " + rightResult.plan +
                               " {" + std::to_string(table1) + "." + attr1 +
                               " " + std::to_string(table2) + "." + attr2 + "})";  

                    // Запоминаем количество строк
                    bestResultRows = joinResult.resultRows;  

                    // Устанавливаем флаг, что джоин найден
                    foundJoin = true;
                }
            }
        }

        // Если прямой джоин не найден, рассматриваем как cross-join
        if (!foundJoin) {
            // Текущая стоимость для cross-join
            double currentCost = leftResult.cost + rightResult.cost + (leftResult.resultRows * rightResult.resultRows);
            
            // Если текущая стоимость меньше минимальной,
            // то обноваляем минимальную стоимость
            if (currentCost < minCost) {       
                // Минимальная стоимость становится текущей
                minCost = currentCost;

                // Формируем план для cross-join
                bestPlan = "(" + leftResult.plan + " " + rightResult.plan + ")";  

                // Количество строк для cross-join
                bestResultRows = leftResult.resultRows * rightResult.resultRows;  
            }
        }
    }

    // Возвращаем лучший результат
    return { minCost, bestPlan, bestResultRows };  
}

// Функция для чтения входных данных
void readInput() {
    // Читаем количество таблиц
    std::cin >> l;  

    // Резервируем место для таблиц
    tables.resize(l);  

    // Проходимся по всем таблицам и считываем строки
    for (int i = 0; i < l; ++i) {
        // Читаем количество строк для каждой таблицы
        std::cin >> tables[i].rows; 
    }

    // Количество предикатов на фильтры
    int filterCount;  

    // Читаем количество предикатов на фильтры
    std::cin >> filterCount;

    // Идем по всем фильтрам 
    for (int i = 0; i < filterCount; ++i) {
        // Номер таблицы
        int tableNum;

        // Аттрибут фильтра
        std::string filterAttr;

        // Значение кардинальности фильтра
        double cardValue;

        // Читаем атрибуты и их кардинальности
        std::cin >> tableNum >> filterAttr >> cardValue;  

        // Сохраняем кардинальности
        tables[tableNum - 1].cardinalities[filterAttr] = cardValue;  
    }

    // Количество предикатов на сканы
    int scanPredCount;  

    // Считываем предикаты на сканы
    std::cin >> scanPredCount;

    // Идем по всем сканам
    for (int i = 0; i < scanPredCount; ++i) {
        // Номер таблицы
        int tableNum;

        // Аттрибут фильтра
        std::string filterAttr;

        // Читаем предикаты на скан
        std::cin >> tableNum >> filterAttr;

        // Сохраняем предикаты  
        scanPredicates.emplace_back(tableNum, filterAttr);  
    }

    // Количество предикатов на джоини
    int joinPredCount;  

    // Читаем все предикаты на джоины
    std::cin >> joinPredCount;

    // Проходимся по всему количеству
    // предикатов в джоине 
    for (int i = 0; i < joinPredCount; ++i) {  
        // Количество таблиц для джоина
        int table1, table2;

        // Количество аттрибутов для джоина
        std::string attr1, attr2;

        // Читаем предикаты для джоинов
        std::cin >> table1 >> table2 >> attr1 >> attr2; 

        // Сохраняем предикаты 
        joinPredicates.emplace_back(table1, table2, attr1, attr2);  
    }
}

int main() {
    // Читаем входные данные
    readInput();  

    // Создаём вектор индексов для всех таблиц
    std::vector<int> tablesToJoin;

    // Проходим по всем таблицам
    for (int i = 1; i <= l; ++i) {
        // Заполняем вектор номерами таблиц
        tablesToJoin.push_back(i);  
    }

    // Вычисляем результат для дерева джоинов
    JoinResult result = computeJoinTreeCost(tablesToJoin);  

    // Выводим итоговые данные
    // *Точность не менее двух знаков после запятой
    // *Десятичная записать (не экспоненциальная)
    std::cout << result.plan << " "
              << std::fixed  << std::setprecision(2) 
              << result.cost << std::endl;
}