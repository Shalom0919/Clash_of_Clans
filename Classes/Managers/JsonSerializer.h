/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     JsonSerializer.h
 * File Function: 统一JSON序列化框架
 * Author:        赵崇治
 * Update Date:   2025/12/24
 * License:       MIT License
 ****************************************************************/
#pragma once

#include "json/document.h"
#include "json/writer.h"
#include "json/stringbuffer.h"
#include "cocos2d.h"

#include <string>
#include <vector>
#include <functional>
#include <type_traits>

/**
 * @class JsonSerializer
 * @brief 统一JSON序列化工具类
 */
class JsonSerializer {
public:
    using Allocator = rapidjson::Document::AllocatorType;
    using Value = rapidjson::Value;
    using Document = rapidjson::Document;

    // ==================== 写入器 ====================
    class Writer {
    public:
        explicit Writer(Allocator& alloc) : _allocator(alloc), _obj(rapidjson::kObjectType) {}

        Writer& write(const char* key, int value) {
            _obj.AddMember(Value(key, _allocator), Value(value), _allocator);
            return *this;
        }

        Writer& write(const char* key, float value) {
            _obj.AddMember(Value(key, _allocator), Value(value), _allocator);
            return *this;
        }

        Writer& write(const char* key, bool value) {
            _obj.AddMember(Value(key, _allocator), Value(value), _allocator);
            return *this;
        }

        Writer& write(const char* key, const std::string& value) {
            Value strVal;
            strVal.SetString(value.c_str(), static_cast<rapidjson::SizeType>(value.length()), _allocator);
            _obj.AddMember(Value(key, _allocator), strVal, _allocator);
            return *this;
        }

        template<typename T>
        Writer& writeArray(const char* key, const std::vector<T>& arr, 
                          std::function<Value(const T&, Allocator&)> itemSerializer) {
            Value arrVal(rapidjson::kArrayType);
            for (const auto& item : arr) {
                arrVal.PushBack(itemSerializer(item, _allocator), _allocator);
            }
            _obj.AddMember(Value(key, _allocator), arrVal, _allocator);
            return *this;
        }

        Value build() { return std::move(_obj); }

    private:
        Allocator& _allocator;
        Value _obj;
    };

    // ==================== 读取器 ====================
    class Reader {
    public:
        explicit Reader(const Value& obj) : _obj(obj) {}

        int readInt(const char* key, int defaultVal = 0) const {
            if (_obj.HasMember(key) && _obj[key].IsInt()) {
                return _obj[key].GetInt();
            }
            return defaultVal;
        }

        float readFloat(const char* key, float defaultVal = 0.0f) const {
            if (_obj.HasMember(key) && _obj[key].IsNumber()) {
                return _obj[key].GetFloat();
            }
            return defaultVal;
        }

        bool readBool(const char* key, bool defaultVal = false) const {
            if (_obj.HasMember(key) && _obj[key].IsBool()) {
                return _obj[key].GetBool();
            }
            return defaultVal;
        }

        std::string readString(const char* key, const std::string& defaultVal = "") const {
            if (_obj.HasMember(key) && _obj[key].IsString()) {
                return _obj[key].GetString();
            }
            return defaultVal;
        }

        template<typename T>
        std::vector<T> readArray(const char* key, std::function<T(const Value&)> itemDeserializer) const {
            std::vector<T> result;
            if (_obj.HasMember(key) && _obj[key].IsArray()) {
                for (const auto& item : _obj[key].GetArray()) {
                    result.push_back(itemDeserializer(item));
                }
            }
            return result;
        }

        bool hasKey(const char* key) const {
            return _obj.HasMember(key);
        }

    private:
        const Value& _obj;
    };

    // ==================== 工具函数 ====================
    static std::string stringify(const Document& doc) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return buffer.GetString();
    }

    static bool parse(const std::string& jsonStr, Document& outDoc) {
        outDoc.Parse(jsonStr.c_str());
        if (outDoc.HasParseError() || !outDoc.IsObject()) {
            CCLOG("JsonSerializer::parse - Parse error");
            return false;
        }
        return true;
    }
};