#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>


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

void Cell::Set(std::string text, const SheetInterface* sheet) {
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
			std::string value;
			if (c == ESCAPE_SIGN) {
				value = text.substr(1);
			}
			else {
				value = text;
			}
			impl_ = std::make_unique<TextImpl>(text, value);
		}
	}
	impl_->SetSheet(sheet);
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
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

void Cell::Impl::SetSheet(const SheetInterface* sheet) {
	sheet_ptr_ = sheet;
}

// EmptyImpl
Cell::Value Cell::EmptyImpl::GetValue() const {
	return Value("");
}
std::string Cell::EmptyImpl::GetText() const {
	return "";
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
	return {};
}

// TextImpl
Cell::TextImpl::TextImpl(std::string text, std::string value)
	: text_(std::move(text))
	, value_(std::move(value)) {
}

Cell::Value Cell::TextImpl::GetValue() const {
	return value_;
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
	return {};
}

// FormulaImpl
Cell::FormulaImpl::FormulaImpl(std::unique_ptr<FormulaInterface> formula)
	: formula_(std::move(formula)) {
}

Cell::Value Cell::FormulaImpl::GetValue() const {
	if (!HasCach()) {
		auto result = formula_->Evaluate(*sheet_ptr_);
		if (std::holds_alternative<double>(result)) {
			return Value(std::get<double>(result));
		}
		else {
			return Value(std::get<FormulaError>(result));
		}
	}
	return cached_value_;
}

std::string Cell::FormulaImpl::GetText() const {
	std::string result = formula_->GetExpression();
	result.insert(result.begin(), '=');
	return result;
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}