#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    // Получить вычисленное значение (число, строку или ошибку)
    Value GetValue() const override;

    // Получить текст ячейки (в том виде, как ввёл пользователь)
    std::string GetText() const override;

    // Инвалидация кэша (очистка)
    void InvalidateCache();

    // Возвращает список ячеек, которые непосредственно задействованы в вычислении формулы.
    std::vector<Position> GetReferencedCells() const override;

private:
  
    class Impl; // "общая ячейка"
    class EmptyImpl; // пустая ячейка
    class TextImpl; // ячейка с текстом
    class FormulaImpl; // ячейка с формулой

    std::unique_ptr<Impl> impl_; // // Текущая ячейка
    Sheet& sheet_;
    mutable std::optional<Value> cache_; // Кэшированное вычисленное значение (чтобы не пересчитывать повторно)

    // Установить новую реализацию Impl
    void SetImpl(std::unique_ptr<Impl> impl);

    // Проверка на наличие циклической зависимости (поиск по графу)
    bool HasCircularDependency(const Cell* target, std::unordered_set<const Cell*>& visited) const;
};
