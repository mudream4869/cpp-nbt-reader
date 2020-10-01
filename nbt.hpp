/**
    NBT Reader
    @file nbt.hpp
    @author Mudream
*/

#pragma once

#include <climits>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


namespace nbt {


namespace endian {


/*
    Return true if host is little endian, false if host is big endian
*/
constexpr bool isHostLittleEndian() {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return false;
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return true;
#else
    static_assert(false, "Unknown endian");
#endif
}

template <typename T>
inline T swapEndian(T u) noexcept;

template <typename T>
inline T swapEndian(T u) noexcept {
    auto u8 = reinterpret_cast<uint8_t *>(&u);
    const size_t sz = sizeof(T);
    for (size_t k = 0; k * 2 < sz; ++k) {
        std::swap(u8[k], u8[sz - k - 1]);
    }
    return u;
}

template <>
inline uint8_t swapEndian(uint8_t u) noexcept {
    return u;
}

template <typename T>
inline T refineBigEndian_(T u, std::false_type) noexcept {
    return u;
}

template <typename T>
inline T refineBigEndian_(T u, std::true_type) noexcept {
    return swapEndian(u);
}

template <typename T>
inline T refineBigEndian(T u) noexcept {
    return refineBigEndian_(
        u, std::integral_constant<bool, isHostLittleEndian()>{});
}


}  // namespace endian


enum class TagType : uint8_t {
    TAG_END,
    TAG_BYTE,
    TAG_SHORT,
    TAG_INT,
    TAG_LONG,
    TAG_FLOAT,
    TAG_DOUBLE,
    TAG_BYTE_ARRAY,
    TAG_STRING,
    TAG_LIST,
    TAG_COMPOUND,
    TAG_INT_ARRAY,
    TAG_LONG_ARRAY
};

std::unordered_map<TagType, std::string> TAGTYPE_TO_NAME = {
    {TagType::TAG_END, "TAG_END"},
    {TagType::TAG_BYTE, "TAG_BYTE"},
    {TagType::TAG_SHORT, "TAG_SHORT"},
    {TagType::TAG_INT, "TAG_INT"},
    {TagType::TAG_LONG, "TAG_LONG"},
    {TagType::TAG_FLOAT, "TAG_FLOAT"},
    {TagType::TAG_DOUBLE, "TAG_DOUBLE"},
    {TagType::TAG_BYTE_ARRAY, "TAG_BYTE_ARRAY"},
    {TagType::TAG_STRING, "TAG_STRING"},
    {TagType::TAG_LIST, "TAG_LIST"},
    {TagType::TAG_COMPOUND, "TAG_COMPOUND"},
    {TagType::TAG_INT_ARRAY, "TAG_INT_ARRAY"},
    {TagType::TAG_LONG_ARRAY, "TAG_LONG_ARRAY"},
};

template <typename T>
T readStream(std::istream &buf);

/*
    Returns the value read from input stream
    @param buf The file stream of input file
    @return the value with type T
*/
template <typename T>
T readStream(std::istream &buf) {
    T tmp;
    buf.read(reinterpret_cast<char *>(&tmp), sizeof(tmp));
    return endian::refineBigEndian<T>(tmp);
}

template <>
std::string readStream(std::istream &buf) {
    auto len = readStream<uint16_t>(buf);

    std::vector<char> tmp(len + 1);
    buf.read(tmp.data(), len);

    return std::string(tmp.begin(), tmp.end());
}

template <>
TagType readStream(std::istream &buf) {
    return static_cast<TagType>(readStream<uint8_t>(buf));
}

class Tag {
public:
    Tag(TagType tt) : type_(tt){};
    Tag(TagType tt, const std::string &name) : type_(tt), name_(name){};
    Tag(TagType tt, std::string &&name) : type_(tt), name_(std::move(name)){};
    virtual ~Tag() = default;

    TagType getTagType() const {
        return type_;
    }

    auto getName() const {
        return name_;
    }

private:
    const TagType type_;
    const std::optional<std::string> name_;
};

template <typename T, TagType tt>
class TagSingle : public Tag {
public:
    TagSingle(std::istream &buf) : Tag(tt) {
        decode(buf);
    }

    TagSingle(const std::string &name, std::istream &buf) : Tag(tt, name) {
        decode(buf);
    }

    TagSingle(std::string &&name, std::istream &buf)
        : Tag(tt, std::move(name)) {
        decode(buf);
    }

    const auto &getValue() const {
        return val_;
    }

private:
    void decode(std::istream &buf) {
        val_ = readStream<T>(buf);
    }

private:
    T val_;
};

using TagByte = TagSingle<int8_t, TagType::TAG_BYTE>;
using TagShort = TagSingle<int16_t, TagType::TAG_SHORT>;
using TagInt = TagSingle<int32_t, TagType::TAG_INT>;
using TagLong = TagSingle<int64_t, TagType::TAG_LONG>;
using TagFloat = TagSingle<float, TagType::TAG_FLOAT>;
using TagDouble = TagSingle<double, TagType::TAG_DOUBLE>;
using TagString = TagSingle<std::string, TagType::TAG_STRING>;

template <typename T, TagType tt>
class TagArray : public Tag {
public:
    TagArray() : Tag(tt) {
    }

    TagArray(std::istream &buf) : Tag(tt) {
        decode(buf);
    }

    TagArray(const std::string &name, std::istream &buf) : Tag(tt, name) {
        decode(buf);
    }

    TagArray(std::string &&name, std::istream &buf) : Tag(tt, std::move(name)) {
        decode(buf);
    }

    const auto &getValue() const {
        return val_;
    }

private:
    void decode(std::istream &buf) {
        auto len = readStream<uint32_t>(buf);
        if (len <= 0) {
            return;
        }

        val_.resize(len);
        for (auto &elem : val_) {
            elem = readStream<T>(buf);
        }
    }

private:
    std::vector<T> val_;
};

using TagByteArray = TagArray<int8_t, TagType::TAG_BYTE_ARRAY>;
using TagIntArray = TagArray<int32_t, TagType::TAG_INT_ARRAY>;
using TagLongArray = TagArray<int64_t, TagType::TAG_LONG_ARRAY>;

class TagCompound;
class TagList;

/*
    Returns a Tag by type and create arguments
    @param type type of tag
    @param args forwarding reference for arguments
    @return a unique pointer of Tag
*/
template <typename... Args>
std::unique_ptr<Tag> makeTag(TagType type, Args &&... args) {
    switch (type) {
    case TagType::TAG_BYTE:
        return std::make_unique<TagByte>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_SHORT:
        return std::make_unique<TagShort>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_INT:
        return std::make_unique<TagInt>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_LONG:
        return std::make_unique<TagLong>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_FLOAT:
        return std::make_unique<TagFloat>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_DOUBLE:
        return std::make_unique<TagDouble>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_BYTE_ARRAY:
        return std::make_unique<TagByteArray>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_STRING:
        return std::make_unique<TagString>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_LIST:
        return std::make_unique<TagList>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_COMPOUND:
        return std::make_unique<TagCompound>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_INT_ARRAY:
        return std::make_unique<TagIntArray>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_LONG_ARRAY:
        return std::make_unique<TagLongArray>(std::forward<Args>(args)...);
        break;
    case TagType::TAG_END:
        throw std::invalid_argument("makeTag: TagType shouldn't be TAG_END");
    default:
        throw std::runtime_error("makeTag: TagType " +
                                 std::to_string(static_cast<int>(type)) +
                                 " not found");
    }
}

class TagList : public Tag {
public:
    TagList() : Tag(TagType::TAG_LIST) {
    }

    TagList(std::istream &buf) : Tag(TagType::TAG_LIST) {
        decode(buf);
    }

    TagList(const std::string &name, std::istream &buf)
        : Tag(TagType::TAG_LIST, name) {
        decode(buf);
    }

    TagList(std::string &&name, std::istream &buf)
        : Tag(TagType::TAG_LIST, std::move(name)) {
        decode(buf);
    }

    const auto &getValue() const {
        return val_;
    }

private:
    void decode(std::istream &buf) {
        elemType_ = readStream<TagType>(buf);
        auto length = readStream<int32_t>(buf);
        if (length <= 0) {
            // TODO
            throw std::runtime_error(
                "TagList::decode: not implement for variant length list");
            return;
        }

        val_.resize(length);
        for (int i = 0; i < length; ++i) {
            val_[i] = makeTag(elemType_, buf);
        }
    }

private:
    TagType elemType_;
    std::vector<std::unique_ptr<Tag>> val_;
};

class TagCompound : public Tag {
public:
    TagCompound() : Tag(TagType::TAG_COMPOUND) {
    }

    TagCompound(std::istream &buf) : Tag(TagType::TAG_COMPOUND) {
        decode(buf);
    }

    TagCompound(const std::string &name, std::istream &buf)
        : Tag(TagType::TAG_COMPOUND, name) {
        decode(buf);
    }

    TagCompound(std::string &&name, std::istream &buf)
        : Tag(TagType::TAG_COMPOUND, std::move(name)) {
        decode(buf);
    }

    const auto &getValue() const {
        return val_;
    }

private:
    void decode(std::istream &buf) {
        for (auto type = readStream<TagType>(buf); type != TagType::TAG_END;
             type = readStream<TagType>(buf)) {
            auto name = readStream<std::string>(buf);
            val_.insert({std::move(name), makeTag(type, name, buf)});
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Tag>> val_;
};

/*
    Returns the root Tag of the document
    @param buf The file stream of input file
    @return the unique pointer of Tag
*/
std::unique_ptr<Tag> readDocument(std::istream &buf) {
    auto tagType = readStream<TagType>(buf);
    if (tagType != TagType::TAG_COMPOUND) {
        throw std::runtime_error(
            "readDocument: document should be a named compound");
    }

    auto name = readStream<std::string>(buf);
    return makeTag(tagType, name, buf);
}


}  // namespace nbt
