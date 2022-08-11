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

    // ������� unique_ptr ������ � �������� (������ � ����������)
    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    // ������ ��������� ��� ����� ������� ������ � ��������

    // ���������� ����� ���� ��� ��������� ������
    void InvalidateCell(const Position& pos);

    // ������ ������� ��� ������ � ���������� ��������.
    // �������� � sheet, �.�.:
    // 1) ��� ������ ���������� ������ Cell
    // 2) ������ � ��������� ������� �� ��������� � Cell, ��� "�������" ������

    // ��������� ����������� "�������� ������" - "��������� ������".
    // depends_on ���� ����� == this
    void AddDependentCell(const Position& depends_on, const Position& dependent_cell);
    // ���������� �������� �����, ��������� �� pos
    const std::unordered_set<Position> GetDependentCells(const Position& pos);
    // ������� ��� ����������� ��� ������ pos.
    void DeleteDependencies(const Position& pos);

private:
    // ������ ��������� ��� ����� ������� ������ � ��������
    std::unordered_map<Position, std::unordered_set<Position>> cells_dependencies_;

    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;

    int max_row_ = 0;    // ����� ����� � Printable Area
    int max_col_ = 0;    // ����� �������� � Printable Area
    bool area_is_valid_ = true;    // ���� ���������� ������� �������� max_row_/col_

    // ������������ ������������ ������ ������� ������ �����
    void UpdatePrintableSize();
    // ��������� ������������� ������ �� ����������� pos, ���� ��� ����� ���� nullptr
    bool CellExists(Position pos) const;
    // ����������� ������ ��� ������, ���� ������ � ����� ������� �� ����������.
    // �� ���������� ���������������� �������� ��������� � �� ������������ Printale Area
    void Touch(Position pos);
};