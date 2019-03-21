#include "LinkerSymbols.h"

AssemblerSymbol::AssemblerSymbol(std::string name, size_t address, size_t width, SymbolClass symbol_class) : name(name), value(address), width(width), symbol_class(symbol_class) {

}

AssemblerSymbol::AssemblerSymbol() {
	this->name = "";
	this->value = 0;
	this->width = 2;
	this->symbol_class = U;
}

AssemblerSymbol::~AssemblerSymbol() {

}

RelocationSymbol::RelocationSymbol(std::string name, size_t value) : name(name), value(value)
{
}

RelocationSymbol::RelocationSymbol()
{
	this->name = "";
	this->value = 0;
}

RelocationSymbol::~RelocationSymbol()
{
}

DataSymbol::DataSymbol(std::string name, std::vector<uint8_t> data) : name(name), data(data)
{
}

DataSymbol::~DataSymbol()
{
}
