#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <queue>

// Cell
Cell::Cell()
	: impl_(std::make_unique<EmptyImpl>()) {
}

Cell::Cell(Cell&& rhs) noexcept
	: impl_(std::move(rhs.impl_)) {
}

Cell& Cell::operator=(Cell&& rhs) noexcept {
	impl_ = std::move(rhs.impl_);
	return *this;
}

void Cell::Set(std::string text, Position pos, SheetInterface* sheet) {
	if (!text.empty()) {
		const char& c = text.front();
		if (c == FORMULA_SIGN && text.size() > 1) {
			try {
				std::unique_ptr<FormulaInterface> formula = ParseFormula(text.substr(1));
				impl_ = std::make_unique<FormulaImpl>(std::move(formula));
			}
			catch (FormulaException& fe) {
				throw fe;
			}
		}
		else {
			impl_ = std::make_unique<TextImpl>(text);
		}
	}
	else {
		impl_ = std::make_unique<EmptyImpl>();
	}
	impl_->SetSheet(sheet);
	if (impl_->TestCyclicDependencies(pos)) {
		throw CircularDependencyException("Circular Dependency!");
	}
	impl_->SetParentForRefCells(pos);
	impl_->InvalidateCache();
}

void Cell::Clear() {
	Set("", impl_->GetPos(), impl_->GetSheet());
}

bool Cell::IsReferenced() const {
	return !GetReferencedCells().empty();
}

void Cell::AddParent(Position pos) {
	impl_->AddParent(pos);
}

bool Cell::HasParents() const {
	return impl_->HasParents();
}

bool Cell::HasCach() const {
	return impl_->HasCach();
}

void Cell::ClearCach() {
	impl_->ClearCach();
}

std::vector<Position> Cell::GetParents() {
	return impl_->GetParents();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}
std::string Cell::GetText() const {
	return impl_->GetText();
}

bool Cell::IsEmpty() const {
	return impl_->GetText().empty();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

// Common Impl
bool Cell::Impl::IsReferenced() const {
	return !GetReferencedCells().empty();
}

std::vector<Position>& Cell::Impl::GetParents() {
	return parent_cells_;
}

void Cell::Impl::AddParent(Position pos) {
	parent_cells_.push_back(pos);
}

bool Cell::Impl::HasParents() const {
	return !parent_cells_.empty();
}

bool Cell::Impl::HasCach() const {
	if (std::holds_alternative<std::string>(cached_value_)) {
		return !std::get<std::string>(cached_value_).empty();
	}
	return true;
}

void Cell::Impl::ClearCach() {
	cached_value_ = Value{};
}

void Cell::Impl::SetSheet(SheetInterface* sheet) {
	sheet_ptr_ = sheet;
}

const Position Cell::Impl::GetPos() const {
	return pos_;
}
SheetInterface* Cell::Impl::GetSheet() {
	return sheet_ptr_;
}


void Cell::Impl::InvalidateCache() {
	std::set<Position> visited_cells;
	std::queue<Position> queue_;
	for (const auto& parent_pos : parent_cells_) {
		queue_.push(parent_pos);
	}
	while (!queue_.empty()) {
		const auto& parent_pos = queue_.front();
		Cell* parent_cell = static_cast<Cell*>(sheet_ptr_->GetCell(parent_pos));
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
	ClearCach();
}

bool Cell::Impl::TestCyclicDependencies(Position pos) const {
	if (IsReferenced()) {
		std::queue<Position> queue_;
		auto cell_childs = GetReferencedCells();
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
				const Cell* child_cell = static_cast<const Cell*>(sheet_ptr_->GetCell(child_pos));
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

void Cell::Impl::SetParentForRefCells(Position pos) {
	for (const auto& cell_pos : GetReferencedCells()) {
		if (Cell* cell = static_cast<Cell*>(sheet_ptr_->GetCell(cell_pos))) {
			cell->AddParent(pos);
		}
	}
}

// EmptyImpl
Cell::Value Cell::EmptyImpl::GetValue() {
	return Value("");
}
std::string Cell::EmptyImpl::GetText() const {
	return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
	return {};
}

// TextImpl
Cell::TextImpl::TextImpl(std::string text)
	: value_(std::move(text)) {
}

Cell::Value Cell::TextImpl::GetValue() {
	char c = value_.front();
	if (c == ESCAPE_SIGN) {
		return value_.substr(1);
	}
	return value_;
}

std::string Cell::TextImpl::GetText() const {
	return value_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
	return {};
}

// FormulaImpl
Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface> formula)
	: formula_(std::move(formula)) {
}

Cell::Value Cell::FormulaImpl::GetValue() {
	if (!HasCach()) {
		auto value = formula_->Evaluate(*sheet_ptr_);
		if (std::holds_alternative<double>(value)) {
			cached_value_ = std::get<double>(value);
		}
		else {
			cached_value_ = std::get<FormulaError>(value);
		}
	}
	return cached_value_;
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}