#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "nbt.hpp"

void printOptionalString(const std::optional<std::string>& str) {
    if (str.has_value()) {
        std::cout << "'" << str.value() << "'";
    } else {
        std::cout << "None";
    }
}

void printEntryDescribe(size_t sz) {
    if (sz <= 1) {
        std::cout << "1 entry";
    } else {
        std::cout << sz << " entries";
    }
}

std::string hexByte(char c) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<unsigned>(c);
    return ss.str();
}

template <typename T>
void print(const T* ptr, int level);

template <>
void print(const nbt::TagByte* ptr, int level);

template <>
void print(const nbt::TagString* ptr, int level);

template <typename T, nbt::TagType tt>
void print(const nbt::TagSingle<T, tt>* ptr, int level) {
    std::string levelStr(level, '\t');
    auto val = ptr->getValue();
    auto tagName = nbt::TAGTYPE_TO_NAME.at(tt);
    std::cout << levelStr << tagName << "(";
    printOptionalString(ptr->getName());
    std::cout << "): " << val << "\n";
}

template <>
void print(const nbt::TagString* ptr, int level) {
    std::string levelStr(level, '\t');
    auto val = ptr->getValue();
    std::cout << levelStr << "TAG_String(";
    printOptionalString(ptr->getName());
    std::cout << "): '" << val << "'\n";
}

template <>
void print(const nbt::TagByte* ptr, int level) {
    std::string levelStr(level, '\t');
    auto val = ptr->getValue();
    std::cout << levelStr << "TAG_Byte(";
    printOptionalString(ptr->getName());
    std::cout << "): " << hexByte(val) << "\n";
}

template <>
void print(const nbt::TagCompound* ptr, int level);

template <>
void print(const nbt::TagList* ptr, int level);

template <>
void print(const nbt::Tag* ptr, int level) {
    auto type = ptr->getTagType();
    switch (type) {
    case nbt::TagType::TAG_COMPOUND:
        print(dynamic_cast<const nbt::TagCompound*>(ptr), level);
        break;
    case nbt::TagType::TAG_STRING:
        print(dynamic_cast<const nbt::TagString*>(ptr), level);
        break;
    case nbt::TagType::TAG_DOUBLE:
        print(dynamic_cast<const nbt::TagDouble*>(ptr), level);
        break;
    case nbt::TagType::TAG_FLOAT:
        print(dynamic_cast<const nbt::TagFloat*>(ptr), level);
        break;
    case nbt::TagType::TAG_LONG:
        print(dynamic_cast<const nbt::TagLong*>(ptr), level);
        break;
    case nbt::TagType::TAG_INT:
        print(dynamic_cast<const nbt::TagInt*>(ptr), level);
        break;
    case nbt::TagType::TAG_SHORT:
        print(dynamic_cast<const nbt::TagShort*>(ptr), level);
        break;
    case nbt::TagType::TAG_BYTE:
        print(dynamic_cast<const nbt::TagByte*>(ptr), level);
        break;
    case nbt::TagType::TAG_LIST:
        print(dynamic_cast<const nbt::TagList*>(ptr), level);
        break;
    case nbt::TagType::TAG_BYTE_ARRAY:
        break;
    default:
        throw std::runtime_error(nbt::TAGTYPE_TO_NAME[type]);
        break;
    }
}

template <>
void print(const nbt::TagCompound* ptr, int level) {
    std::string levelStr(level, '\t');
    const auto& val = ptr->getValue();
    std::cout << levelStr << "TAG_Compound(";
    printOptionalString(ptr->getName());
    std::cout << ") ";
    printEntryDescribe(val.size());
    std::cout << "\n";
    std::cout << levelStr << "{\n";
    for (const auto& it : val) {
        print(it.second.get(), level + 1);
    }
    std::cout << levelStr << "}\n";
}

template <>
void print(const nbt::TagList* ptr, int level) {
    std::string levelStr(level, '\t');
    const auto& val = ptr->getValue();
    std::cout << levelStr << "TAG_List(";
    printOptionalString(ptr->getName());
    std::cout << ") ";
    printEntryDescribe(val.size());
    std::cout << "\n";
    std::cout << levelStr << "{\n";
    for (const auto& it : val) {
        print(it.get(), level + 1);
    }
    std::cout << levelStr << "}\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "./a.out [nbt filename]\n";
        return 1;
    }

    std::ifstream buf(argv[1]);
    auto doc = nbt::readDocument(buf);
    print(doc.get(), 0);
    return 0;
}