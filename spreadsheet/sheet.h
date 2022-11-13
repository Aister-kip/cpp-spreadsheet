#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    Size size_;
    std::vector<std::vector<Cell>> sheet_;

    void DeleteRow(int row);
    void DeleteCol(int col);

    bool RowIsEmpty(int row) const;
    bool ColIsEmpty(int col) const;

    void CutSheet();
};