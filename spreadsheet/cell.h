#pragma once

#include "common.h"
#include "formula.h"

class Cell : public CellInterface {
public:
    Cell();
    Cell(Cell&& rhs) noexcept;

    Cell& operator=(Cell&& rhs) noexcept;

    ~Cell() = default;

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsEmpty() const;
    void Set(std::string text, Position pos, SheetInterface* sheet);
    void Clear();

    bool IsReferenced() const;
    void AddParent(Position pos);
    bool HasParents() const;
    bool HasCach() const;
    void ClearCach();
    std::vector<Position> GetParents();

private:
    class Impl {
    public:
        virtual Value GetValue() = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;

        virtual ~Impl() = default;

        bool IsReferenced() const;
        std::vector<Position>& GetParents();
        void AddParent(Position pos);
        bool HasParents() const;
        bool HasCach() const;
        void ClearCach();

        void SetSheet(SheetInterface* sheet);

        const Position GetPos() const;
        SheetInterface* GetSheet();

        void InvalidateCache();
        bool TestCyclicDependencies(Position pos) const;
        void SetParentForRefCells(Position pos);
    protected:
        SheetInterface* sheet_ptr_ = nullptr;
        Position pos_;
        std::vector<Position> parent_cells_;
        Value cached_value_;
    };

    class EmptyImpl : public Impl {
        Value GetValue() override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string text);

        Value GetValue() override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::string value_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::unique_ptr<FormulaInterface> formula);

        Value GetValue() override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::unique_ptr<FormulaInterface> formula_;
    };

    std::unique_ptr<Impl> impl_;
};