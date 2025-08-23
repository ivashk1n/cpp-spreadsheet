#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stdexcept>

// базовый класс Impl для ячеек разных типов 
    class Cell::Impl {
    public:
		virtual ~Impl() = default;
		virtual Value GetValue(const Sheet& sheet) const = 0;
        virtual std::string GetText() const = 0;
		virtual std::vector<Position> GetReferencedCells() const { return {}; }

    protected:
        Value value_;
        std::string text_;
    };

    // тип для пустой ячейки
    class Cell::EmptyImpl : public Impl {
    public:
        EmptyImpl() {
            value_ = "";
	        text_ = "";
        }

        Value GetValue(const Sheet&) const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }
    };

    // тип ячейки с текстом
    class Cell::TextImpl : public Impl {
    public:
        TextImpl(std::string text) {
            text_ = text;
            if (text[0] == '\''){ // нужно для того что бы получить формулу в виде текста
	 	        text = text.substr(1);
	        }
            value_ = text;
        }

        Value GetValue(const Sheet&) const override {
            return value_;
        }

        std::string GetText() const override {
            return text_;
        }
    };

    //тип ячейки с формулой
    class Cell::FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string formula) {
            formula = formula.substr(1); // без равно
            value_ = formula;
            formula_int_ = ParseFormula(std::move(formula));
            text_ = "=" + formula_int_->GetExpression();
        }

        Value GetValue(const Sheet& sheet) const override {
            auto value = formula_int_->Evaluate(sheet);

            if (std::holds_alternative<double>(value)) { // проверка на тип
                return std::get<double>(value);
            }
            return std::get<FormulaError>(value); // возврат ошибки
        }

        std::string GetText() const override {
            return text_;
        }

		// Получаем список ячеек, от которых зависит эта формула
		std::vector<Position> GetReferencedCells() const override {
			return formula_int_->GetReferencedCells();
		}

    private:
        std::unique_ptr<FormulaInterface> formula_int_;
    };


// ====== Методы класса Cell ======

// По умолчанию ячейка пустая
Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> impl_;

    if (text.size() == 0) { //пустая ячейка
	    impl_ = std::make_unique<EmptyImpl>();
	}
	else if (text.size() > 1 && text[0] == '=') { // в ячейке формула
	    impl_ = std::make_unique<FormulaImpl>(std::move(text));
	}
	else { // в ячейке текст
	    impl_ = std::make_unique<TextImpl>(std::move(text));
	}

	// Проверка на циклические зависимости
    auto* new_formula = dynamic_cast<FormulaImpl*>(impl_.get());
    if (new_formula) {
        auto refs = new_formula->GetReferencedCells();
        std::unordered_set<const Cell*> visited; // При первом вызове HasCircularDependency множесто пустое.
        //Затем в него добавляются все ячейки, которые были просмотрены в процессе рекурсивной проверки зависимостей
        for (auto& pos : refs) {
            // Получаем ячейки, от которых зависит формула
            const Cell* ref_cell = dynamic_cast<const Cell*>(sheet_.GetCell(pos));
            // Проверяем, не приводит ли зависимость к циклу
            if (ref_cell && ref_cell->HasCircularDependency(this, visited)) {
                throw CircularDependencyException("Circular reference detected");
            }
        }
    }

    SetImpl(std::move(impl_));
}

void Cell::Clear() {
	SetImpl(std::make_unique<EmptyImpl>());
}

// Получение вычисленного значения (с кэшированием)
CellInterface::Value Cell::GetValue() const {
    if (!cache_) {
        cache_ = impl_->GetValue(sheet_);
    }
    return *cache_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

// Установка реализации Impl
void Cell::SetImpl(std::unique_ptr<Impl> impl) {
    impl_ = std::move(impl);
    InvalidateCache(); // нужно сбросить кэш
}

// Инвалидация кэша (очистка)
void Cell::InvalidateCache() {
    cache_.reset();
}

// Проверка на циклическую зависимость
bool Cell::HasCircularDependency(const Cell* target, std::unordered_set<const Cell*>& visited) const {
    // Если дошли до искомой ячейки — цикл найден
    if (this == target) { // ячейка прямо или косвенно ссылается сама на себя.
        return true;
    }

    // Если уже посещали эту ячейку — дальше нет смысла идти
    // если текущей ячейки ещё не было в множестве visited, она туда добавится, и .second == true;
    if (!visited.insert(this).second) {
        return false; //повторное посещение этой ячейки — это просто тупик (ветка, которая не ведёт к циклу).
    }

    // Обходим все зависимости текущей ячейки
    auto refs = impl_->GetReferencedCells(); // все ячейки формулы, на которые ссылается текущая формула
    for (auto& pos : refs) {
        const Cell* ref = dynamic_cast<const Cell*>(sheet_.GetCell(pos)); // получаем ячейку по позиции
        // Если ячейка существует и через неё можно дойти до this == target цикл найден
        if (ref && ref->HasCircularDependency(target, visited)) {
            return true;
        }
    }
    return false; // циклическая зависимость не найдена
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}


