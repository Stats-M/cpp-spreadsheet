#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

class Sheet : public SheetInterface
{
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    // Очищает unique_ptr вместе с ресурсом (ячейка с содержимым)
    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // Можете дополнить ваш класс нужными полями и методами

    // Производит сброс кэша для указанной ячейки
    void InvalidateCell(const Position& pos);

    // Группа методов для работы с зависимыми ячейками.
    // Вынесены в sheet, т.к.:
    // 1) Они сильно прегружают классы Cell
    // 2) Данные о зависимых ячейках не относятся к Cell, это "внешние" данные

    // Добавляет взаимосвязь "основная ячейка" - "зависящая ячейка".
    // depends_on чаще всего == this
    void AddDependentCell(const Position& depends_on, const Position& dependent_cell);
    // Возвращает перечень ячеек, зависящих от pos
    const std::unordered_set<Position> GetDependentCells(const Position& pos);
    // Удаляет все зависимости для ячейки pos.
    void DeleteDependencies(const Position& pos);

private:
    // Можете дополнить ваш класс нужными полями и методами
    std::unordered_map<Position, std::unordered_set<Position>> cells_dependencies_;

    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;

    int max_row_ = 0;    // Число строк в Printable Area
    int max_col_ = 0;    // Число столбцов в Printable Area
    bool area_is_valid_ = true;    // Флаг валидности текущих значений max_row_/col_

    // Пересчитывет максимальный размер области печати листа
    void UpdatePrintableSize();
    // Проверяет существование ячейки по координатам pos, сама она может быть nullptr
    bool CellExists(Position pos) const;
    // Резервирует память для ячейки, если ячейка с таким адресом не существует.
    // Не производит непосредственное создание элементов и не инвалидирует Printale Area
    void Touch(Position pos);
};
