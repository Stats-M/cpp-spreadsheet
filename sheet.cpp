#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet()
{}

void Sheet::SetCell(Position pos, std::string text)
{
    // ���������� ������� �� ������������
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for SetCell()");
    }

    // �������� ������ ��� ������, ���� �����
    Touch(pos);
    // �������� ��������� �� ������ ��� �������� �����
    auto cell = GetCell(pos);

    if (cell)
    {
        // ������ ��� ����������.
        // �������� ������ ���������� �� ������ ����� ������������ �������.
        // �� ������� �� ������ �������� ��������� � ���� ������.
        std::string old_text = cell->GetText();

        // ������������ ��� ������ � ��������� �� ���... 
        InvalidateCell(pos);
        // ... � ������� �����������
        DeleteDependencies(pos);
        // ������� ������ ���������� ������ (�� ��� unique_ptr, � ���������� ������ �� ���������� ������)
        dynamic_cast<Cell*>(cell)->Clear();

        dynamic_cast<Cell*>(cell)->Set(text);
        // ��������� �� ����������� ����������� ����� ���������� cell
        if (dynamic_cast<Cell*>(cell)->IsCyclicDependent(dynamic_cast<Cell*>(cell), pos))
        {
            // ���� ����������� �����������. ����� ���������
            dynamic_cast<Cell*>(cell)->Set(std::move(old_text));
            throw CircularDependencyException("Circular dependency detected!");
        }

        // ��������� �����������
        for (const auto& ref_cell : dynamic_cast<Cell*>(cell)->GetReferencedCells())
        {
            AddDependentCell(ref_cell, pos);
        }
    }
    else
    {
        // ����� ������ (nullptr). ����� �������� ��������� Printable Area � �����
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);

        // ��������� ����������� ������
        if (new_cell.get()->IsCyclicDependent(new_cell.get(), pos))
        {
            throw CircularDependencyException("Circular dependency detected!");
        }

        // � ���������� ������� ���������� �������, ������� � ����������
        // ����������� ������������ ���������.
        // ��������� � ����������� Sheet.

        // �������� �� ������� ����� �� ������� � ���������
        // ��� ������ �� ��� ���� ������ ��� ���������
        for (const auto& ref_cell : new_cell.get()->GetReferencedCells())
        {
            AddDependentCell(ref_cell, pos);
        }

        // �������� unique_ptr � nullptr �� sheet_  �� ����� ���������
        sheet_.at(pos.row).at(pos.col) = std::move(new_cell);
        UpdatePrintableSize();
    }
}

const CellInterface* Sheet::GetCell(Position pos) const
{
    // ���������� ������� �� ������������
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for GetCell()");
    }

    // �.�. �� const GetCell() ������ �� ������ � ������� Sheet,
    // �������� �� � const-������ �������� �������
    // return const_cast<Sheet*>(this)->GetCell(pos);

    // ���� ������ ��� ������ ��������...
    if (CellExists(pos))
    {
        //  ...� �� ��������� �� nullptr...
        if (sheet_.at(pos.row).at(pos.col))
        {
            // ...���������� ���
            return sheet_.at(pos.row).at(pos.col).get();
        }
    }

    // ��� ����� �������������� ����� ���������� ������ nullptr
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos)
{
    // ���������� ������� �� ������������
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for GetCell()");
    }

    // ���� ������ ��� ������ ��������...
    if (CellExists(pos))
    {
        //  ...� �� ��������� �� nullptr...
        if (sheet_.at(pos.row).at(pos.col))
        {
            // ...���������� ���
            return sheet_.at(pos.row).at(pos.col).get();
        }
    }

    // ��� ����� �������������� ����� ���������� ������ nullptr
    return nullptr;
}

void Sheet::ClearCell(Position pos)
{
    // ���������� ������� �� ������������
    if (!pos.IsValid())
    {
        throw InvalidPositionException("Invalid position for ClearCell()");
    }

    if (CellExists(pos))
    {
        //// ������� ����������� ����� ���������� ������, ��������� p.reset(p.get())
        //sheet_.at(pos.row).at(pos.col).reset(
        //    sheet_.at(pos.row).at(pos.col).get()
        //);
        sheet_.at(pos.row).at(pos.col).reset();       // ������� ���������� ������

         // pos.row/col 0-based          max_row/col 1-based
        if ((pos.row + 1 == max_row_) || (pos.col + 1 == max_col_))
        {
            // ��������� ������ ���� �� ������� Printable Area. ����� ����������
            area_is_valid_ = false;
            UpdatePrintableSize();
        }
    }
}

Size Sheet::GetPrintableSize() const
{
    if (area_is_valid_)
    {
        return Size{ max_row_, max_col_ };
    }
    
    // ���� �������� �� ������. ����� ���� ������� {0,0}, ���� ������� ����������

    // �� ���� ������ ������� ���������� {0, 0}
    //return { 0,0 };

    // ������� ����������
    throw InvalidPositionException("The size of printable area has not been updated");
}

void Sheet::PrintValues(std::ostream& output) const
{
    for (int x = 0; x < max_row_; ++x)
    {
        bool need_separator = false;
        // �������� �� ���� ������ Printable area
        for (int y = 0; y < max_col_; ++y)
        {
            // �������� ������������� ������ �����������
            if (need_separator)
            {
                output << '\t';
            }
            need_separator = true;

            // ���� �� �� ����� �� ������� ������� � ������ �� nullptr
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y))
            {
                // ������ ����������
                auto value = sheet_.at(x).at(y)->GetValue();
                if (std::holds_alternative<std::string>(value))
                {
                    output << std::get<std::string>(value);
                }
                if (std::holds_alternative<double>(value))
                {
                    output << std::get<double>(value);
                }
                if (std::holds_alternative<FormulaError>(value))
                {
                    output << std::get<FormulaError>(value);
                }
            }
        }
        // ���������� �����
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const
{
    for (int x = 0; x < max_row_; ++x)
    {
        bool need_separator = false;
        // �������� �� ���� ������ Printable area
        for (int y = 0; y < max_col_; ++y)
        {
            // �������� ������������� ������ �����������
            if (need_separator)
            {
                output << '\t';
            }
            need_separator = true;

            // ���� �� �� ����� �� ������� ������� � ������ �� nullptr
            if ((y < static_cast<int>(sheet_.at(x).size())) && sheet_.at(x).at(y))
            {
                // ������ ����������
                output << sheet_.at(x).at(y)->GetText();
            }
        }
        // ���������� �����
        output << '\n';
    }
}

void Sheet::InvalidateCell(const Position& pos)
{
    // ��� ���� ��������� ����� ���������� ������������ ���
    for (const auto& dependent_cell : GetDependentCells(pos))
    {
        auto cell = GetCell(dependent_cell);
        // InvalidateCache() ���� ������ � Cell, �������� ���������
        dynamic_cast<Cell*>(cell)->InvalidateCache();
        InvalidateCell(dependent_cell);
    }
}

void Sheet::AddDependentCell(const Position& main_cell, const Position& dependent_cell)
{
    // ��� ���������� ������ ��� main_cell ������� �� ����� []
    cells_dependencies_[main_cell].insert(dependent_cell);
}

const std::set<Position> Sheet::GetDependentCells(const Position& pos)
{
    if (cells_dependencies_.count(pos) != 0)
    {
        // ���� ����� ���� � ������� ������������. ���������� ��������
        return cells_dependencies_.at(pos);
    }

    // ���� �� �����, �� ������ pos ����� �� �������
    return {};
}

void Sheet::DeleteDependencies(const Position& pos)
{
    cells_dependencies_.erase(pos);
}

void Sheet::UpdatePrintableSize()
{
    max_row_ = 0;
    max_col_ = 0;

    // ��������� ������, ��������� nullptr
    for (int x = 0; x < static_cast<int>(sheet_.size()); ++x)
    {
        for (int y = 0; y < static_cast<int>(sheet_.at(x).size()); ++y)
        {
            if (sheet_.at(x).at(y))
            {
//                max_row_ = (max_row_ < (x + 1) ? x + 1 : max_row_);
//                max_col_ = (max_col_ < (y + 1) ? y + 1 : max_col_);
                max_row_ = std::max(max_row_, x + 1);
                max_col_ = std::max(max_col_, y + 1);
            }
        }
    }

    // ���������� ����������
    area_is_valid_ = true;
}

bool Sheet::CellExists(Position pos) const
{
    return (pos.row < static_cast<int>(sheet_.size())) && (pos.col < static_cast<int>(sheet_.at(pos.row).size()));
}

void Sheet::Touch(Position pos)
{
    // ���������� ������� �� ������������
    if (!pos.IsValid())
    {
        return;
    }

    // size() 1-based          pos.row/col 0-based          sheet_[] 0-based

    // ���� ��������� � ������� ����� ������, ��� ����� ������ � pos.row...
    if (static_cast<int>(sheet_.size()) < (pos.row + 1))
    {
        // ... ����������� � �������������� nullptr �������� ������ �� ������ pos.row
        sheet_.reserve(pos.row + 1);
        sheet_.resize(pos.row + 1);
    }

    // ���� ��������� � ������� �������� ������, ��� ����� ������� � pos.col...
    if (static_cast<int>(sheet_.at(pos.row).size()) < (pos.col + 1))
    {
        // ... ����������� � �������������� nullptr �������� ������ �� ������� pos.col
        sheet_.at(pos.row).reserve(pos.col + 1);
        sheet_.at(pos.row).resize(pos.col + 1);
    }
}



// ������ ������� � ������ ������ �������. ���������� � common.h
std::unique_ptr<SheetInterface> CreateSheet()
{
    return std::make_unique<Sheet>();
}
