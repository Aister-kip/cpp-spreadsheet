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
    if (!pos.IsValid()) {
        throw InvalidPositionException("Wrong position!"s);
    }
    Cell cell;
    cell.Set(text, pos, this);
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
    sheet_[pos.row][pos.col] = std::move(cell);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Wrong position!"s);
    }
    if (pos < Position{ size_.rows, size_.cols }) {
        return &sheet_[pos.row][pos.col];
    }
    else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Wrong position!"s);
    }
    if (pos < Position{ size_.rows, size_.cols }) {
        if (!sheet_[pos.row][pos.col].IsEmpty()) {
            sheet_[pos.row][pos.col].Clear();
            CutSheet();
        }
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

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

