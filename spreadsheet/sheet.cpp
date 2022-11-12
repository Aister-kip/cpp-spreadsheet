#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <set>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        Cell cell;
        cell.Set(text, this);
        if (!TestCyclicDependencies(pos, cell)) {
            if (size_.rows <= pos.row) {
                sheet_.resize(pos.row + 1);
                sheet_[pos.row].resize(size_.cols);
                size_.rows = pos.row + 1;
            }
            if (size_.cols <= pos.col) {
                for (auto& row : sheet_) {
                    row.resize(pos.col + 1);
                    size_.cols = pos.col + 1;
                }
            }
            for (const auto& cell_pos : cell.GetReferencedCells()) {
                if (Cell* cell = static_cast<Cell*>(GetCell(cell_pos))) {
                    cell->AddParent(pos);
                }
            }
            sheet_[pos.row][pos.col] = std::move(cell);
            InvalidateCache(pos);
        }
        else {
            throw CircularDependencyException("Circular Dependency!");
        }
    }
    else {
        throw InvalidPositionException("Wrong position!"s);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (pos < Position{ size_.rows, size_.cols }) {
            return &sheet_[pos.row][pos.col];
        }
        else {
            return nullptr;
        }
    }
    else {
        throw InvalidPositionException("Wrong position!"s);
    }
}

void Sheet::ClearCell(Position pos) {
    if (pos.IsValid()) {
        if (pos < Position{ size_.rows, size_.cols }) {
            if (!sheet_[pos.row][pos.col].IsEmpty()) {
                InvalidateCache(pos);
                sheet_[pos.row][pos.col].Clear();
                CutSheet();
            }
        }
    }
    else {
        throw InvalidPositionException("Wrong position!"s);
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (const auto& row : sheet_) {
        bool first_col = true;
        for (const auto& col : row) {
            if (!first_col) {
                output << '\t';
            }
            Cell::Value value = col.GetValue();
            if (std::holds_alternative<double>(value)) {
                output << std::get<double>(value);
            }
            if (std::holds_alternative<std::string>(value)) {
                output << std::get<std::string>(value);
            }
            if (std::holds_alternative<FormulaError>(value)) {
                output << std::get<FormulaError>(value);
            }
            first_col = false;
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (const auto& row : sheet_) {
        bool first_col = true;
        for (const auto& col : row) {
            if (!first_col) {
                output << '\t';
            }
            output << col.GetText();
            first_col = false;
        }
        output << '\n';
    }
}

void Sheet::DeleteRow(int row) {
    sheet_.erase(sheet_.begin() + row);
    --size_.rows;
}

void Sheet::DeleteCol(int col) {
    for (auto& row : sheet_) {
        row.erase(row.begin() + col);
    }
    --size_.cols;
}

bool Sheet::RowIsEmpty(int row) const {
    for (const auto& it : sheet_[row]) {
        if (!it.IsEmpty()) {
            return false;
        }
    }
    return true;
}

bool Sheet::ColIsEmpty(int col) const {
    for (const auto& row : sheet_) {
        if (!row[col].IsEmpty()) {
            return false;
        }
    }
    return true;
}

void Sheet::CutSheet() {
    if (size_.cols > 0) {
        for (auto col = size_.cols - 1; col >= 0; --col) {
            if (ColIsEmpty(col)) {
                DeleteCol(col);
            }
            else {
                break;
            }
        }
    }
    if (size_.rows > 0) {
        for (auto row = size_.rows - 1; row >= 0; --row) {
            if (RowIsEmpty(row)) {
                DeleteRow(row);
            }
            else {
                break;
            }
        }
    }
}

void Sheet::InvalidateCache(Position pos) {
    std::set<Position> visited_cells;
    std::queue<Position> queue_;
    queue_.push(pos);
    while (!queue_.empty()) {
        const auto& parent_pos = queue_.front();
        Cell* parent_cell = static_cast<Cell*>(GetCell(parent_pos));
        if (parent_cell) {
            if (parent_cell->HasParents()) {
                for (const auto& parent_pos : parent_cell->GetParents()) {
                    queue_.push(parent_pos);
                }
            }
            parent_cell->ClearCach();
        }
        queue_.pop();
    }
}

bool Sheet::TestCyclicDependencies(Position pos, const Cell& cell) const {
    if (cell.IsReferenced()) {
        std::queue<Position> queue_;
        auto cell_childs = cell.GetReferencedCells();
        for (const auto& cell_pos : cell_childs) {
            queue_.push(cell_pos);
        }
        std::set<Position> visited_cells;
        while (!queue_.empty()) {
            const auto& child_pos = queue_.front();
            if (!visited_cells.count(child_pos)) {
                if (child_pos == pos) {
                    return true;
                }
                const Cell* child_cell = static_cast<const Cell*>(GetCell(child_pos));
                if (child_cell && child_cell->IsReferenced()) {
                    auto ref_cells = child_cell->GetReferencedCells();
                    for (const auto& ref_pos : ref_cells) {
                        queue_.push(ref_pos);
                    }
                }
                visited_cells.insert(child_pos);
            }
            queue_.pop();
        }
    }
    return false;
}

void Sheet::SetParentForRefCells(Position pos) {

}

const Cell* Sheet::GetCellLocal(Position pos) const {
    return nullptr;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

