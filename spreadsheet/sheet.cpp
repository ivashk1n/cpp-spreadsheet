#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {

    if (!pos.IsValid()) {
	    throw InvalidPositionException("Invalid position");
    }

    // Если в таблице недостаточно строк – расширяем
    if (cells_.size() <= static_cast<size_t>(pos.row)) {
        cells_.resize(pos.row + 1);
    }
    // Если в строке недостаточно столбцов – расширяем
    if (cells_[pos.row].size() <= static_cast<size_t>(pos.col)) {
        cells_[pos.row].resize(pos.col + 1);
    }

    //Если ячейка ещё не создана – создаём
    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }

    // Записываем текст в ячейку
    cells_[pos.row][pos.col]->Set(std::move(text));

}

const CellInterface* Sheet::GetCell(Position pos) const {

    if (!pos.IsValid()) {
	    throw InvalidPositionException("Invalid position");
    }

    // Проверяем, не выходит ли позиция за границы таблицы
    if (pos.row >= static_cast<int>(cells_.size())) {
        return nullptr;
    }

    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
	    return nullptr;
    }

    const auto& cell = cells_[pos.row][pos.col];
    return cell ? cell.get() : nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {

    if (!pos.IsValid()) {
	    throw InvalidPositionException("Invalid position");
    }

    // Проверяем, не выходит ли позиция за границы таблицы
    if (pos.row >= static_cast<int>(cells_.size())) {
        return nullptr;
    }

    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
	    return nullptr;
    }

    const auto& cell = cells_[pos.row][pos.col];
    return cell ? cell.get() : nullptr;
}

void Sheet::ClearCell(Position pos) {

    if (!pos.IsValid()) {
	    throw InvalidPositionException("Invalid position");
    }

    // Если строки или колонки нет – просто выходим
    if (pos.row >= static_cast<int>(cells_.size())) {
	    return;
    }
    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
	    return;
    }

    // Если ячейка существует – очищаем её содержимое
    if (cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col]->Clear();
    }
}

//получем размер минимальной печатной области таблицы
Size Sheet::GetPrintableSize() const {
    Size result{0, 0};

    // Проходим по всем строкам и столбцам
    for (int row = 0; row < static_cast<int>(cells_.size()); ++row) {
        for (int col = 0; col < static_cast<int>(cells_[row].size()); ++col) {
            const auto& cell = cells_[row][col];
            // Если ячейка непустая – обновляем границы таблицы
            if (cell && !cell->GetText().empty()) {
                result.rows = std::max(result.rows, row + 1);
                result.cols = std::max(result.cols, col + 1);
            }
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();

    // Проходим по всем строкам в пределах PrintableSize
    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) output << "\t"; // Разделитель между ячейками

            // Проверка на выход за границы и наличие ячейки
            if (row < static_cast<int>(cells_.size()) &&
                col < static_cast<int>(cells_[row].size())) {
                const auto& cell = cells_[row][col];
                if (cell) {
                    // Ячейка может хранить текст, число или ошибку
                    auto value = cell->GetValue();
                    std::visit([&output](auto&& arg) { output << arg; }, value);
                }
            }
        }
        output << "\n"; // Конец строки таблицы
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();

    for (int row = 0; row < size.rows; ++row) {
        for (int col = 0; col < size.cols; ++col) {
            if (col > 0) output << "\t";

            if (row < static_cast<int>(cells_.size()) &&
                col < static_cast<int>(cells_[row].size())) {
                const auto& cell = cells_[row][col];
                if (cell) {
                    output << cell->GetText(); // Печатаем именно текст
                }
            }
        }
        output << "\n";
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}