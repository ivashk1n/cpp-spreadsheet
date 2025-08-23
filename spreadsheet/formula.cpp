#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category) 
    : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!";
        case Category::Value:
            return "#VALUE!";
        case Category::Arithmetic:
            return "#ARITHM!";
        }
    return "";
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression)
        : ast_(ParseFormulaAST(std::move(expression))) {}

    //Value = std::variant<double, FormulaError>;
    //метод превращает содержимое ячеек в корректные аргументы для формулы, обрабатывает возможные ошибки и возвращает Value
    Value Evaluate(const SheetInterface& sheet) const override {
       
        // Лямбда-функция, которая по позиции ячейки возвращает её числовое значение
        // или бросает ошибку, если ячейка невалидная или содержит некорректные данные.
        //pos — это аргумент функции args_func, то есть каждая ячейка, на которую ссылается формула, передаётся сюда как позиция
        std::function<double(Position)> args_func = [&sheet](Position pos) -> double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            // получаем ячейку
            const auto* cell = sheet.GetCell(pos);

            // пустая ячейка
            if (!cell) {
                return 0.0;
            }

            // вычисляем значение ячейки
            const auto& val = cell->GetValue();

            // в ячейке число (возвращаем число double)
            if (auto num = std::get_if<double>(&val)) {
                return *num;
            }

            // в ячейке текст пробуем интерпретировать как число
            if (auto str = std::get_if<std::string>(&val)) {
                if (str->empty()) {
                    return 0.0;
                }
                // Пробуем распарсить строку как число
                std::istringstream in(*str);
                double result;

                if (in >> result && in.eof()) {
                    return result; // строка успешно преобразована в число
                }
                // Если преобразовать строку не удалось выбрасываемм ошибку
                throw FormulaError(FormulaError::Category::Value);
            }
            //Если значение — это уже ошибка, метод выбрасывает ошибку
            throw std::get<FormulaError>(val);
        };
   
        try { // пробуем посчитать значение формулы
            //Когда Execute выполняет дерево формулы, оно встречает ссылки на ячейки (A1, B2, и т.д.).
            //Для каждой такой ссылки AST вызывает переданную функцию args_func, подставляя в неё Position pos ячейки
            return ast_.Execute(args_func);
        } catch (FormulaError& e) {
            // возвращаем ошибку вместо значения. (например, #REF!, #VALUE! или "#ARITHM!")
            return e;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    // // Возвращает список ячеек, которые непосредственно задействованы в вычислении формулы
    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        
        // Собираем список всех ячеек, на которые ссылается формула
        for (auto cell : ast_.GetCells()) {
            if (cell.IsValid())                // проверяем, что позиция корректная 
                cells.push_back(cell);          // добавляем её в список
        }

        // Убираем дубликаты
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());

        return cells;  
    }

private:
    FormulaAST ast_;
};
}  // namespace



std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    // при парсинге формулы (создание объекта Formula) могут быть выброшены ошибки
    catch (...) {
        throw FormulaException("Invalid formul");
    }
}