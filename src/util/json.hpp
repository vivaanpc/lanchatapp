#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace nlohmann {

class json {
public:
    enum class value_t {
        null,
        boolean,
        number,
        string,
        array,
        object
    };

private:
    value_t m_type = value_t::null;
    std::string m_string;
    double m_number = 0.0;
    bool m_boolean = false;
    std::vector<json> m_array;
    std::map<std::string, json> m_object;

public:
    // Constructors
    json() = default;
    json(bool value) : m_type(value_t::boolean), m_boolean(value) {}
    json(int value) : m_type(value_t::number), m_number(static_cast<double>(value)) {}
    json(double value) : m_type(value_t::number), m_number(value) {}
    json(const char* value) : m_type(value_t::string), m_string(value) {}
    json(const std::string& value) : m_type(value_t::string), m_string(value) {}

    // Static factories
    static json array() {
        json j;
        j.m_type = value_t::array;
        return j;
    }

    static json object() {
        json j;
        j.m_type = value_t::object;
        return j;
    }

    // Type checks
    bool is_null() const { return m_type == value_t::null; }
    bool is_boolean() const { return m_type == value_t::boolean; }
    bool is_number() const { return m_type == value_t::number; }
    bool is_string() const { return m_type == value_t::string; }
    bool is_array() const { return m_type == value_t::array; }
    bool is_object() const { return m_type == value_t::object; }

    // Value access
    bool get_bool() const { return m_boolean; }
    double get_number() const { return m_number; }
    const std::string& get_string() const { return m_string; }

    // Operators for object access
    json& operator[](const std::string& key) {
        if (m_type != value_t::object) {
            m_type = value_t::object;
            m_object.clear();
        }
        return m_object[key];
    }

    const json& operator[](const std::string& key) const {
        static json null_json;
        if (m_type != value_t::object) return null_json;
        auto it = m_object.find(key);
        return it != m_object.end() ? it->second : null_json;
    }

    // Array access
    json& operator[](size_t index) {
        if (m_type != value_t::array) {
            m_type = value_t::array;
            m_array.clear();
        }
        if (index >= m_array.size()) {
            m_array.resize(index + 1);
        }
        return m_array[index];
    }

    // Contains check
    bool contains(const std::string& key) const {
        if (m_type != value_t::object) return false;
        return m_object.find(key) != m_object.end();
    }

    // Array operations
    void push_back(const json& value) {
        if (m_type != value_t::array) {
            m_type = value_t::array;
            m_array.clear();
        }
        m_array.push_back(value);
    }

    size_t size() const {
        if (m_type == value_t::array) return m_array.size();
        if (m_type == value_t::object) return m_object.size();
        return 0;
    }

    // Assignment operators
    json& operator=(bool value) {
        m_type = value_t::boolean;
        m_boolean = value;
        return *this;
    }

    json& operator=(int value) {
        m_type = value_t::number;
        m_number = static_cast<double>(value);
        return *this;
    }

    json& operator=(double value) {
        m_type = value_t::number;
        m_number = value;
        return *this;
    }

    json& operator=(const char* value) {
        m_type = value_t::string;
        m_string = value;
        return *this;
    }

    json& operator=(const std::string& value) {
        m_type = value_t::string;
        m_string = value;
        return *this;
    }

    // Conversion operators
    operator std::string() const {
        if (m_type == value_t::string) return m_string;
        return "";
    }

    operator bool() const {
        if (m_type == value_t::boolean) return m_boolean;
        return false;
    }

    operator int() const {
        if (m_type == value_t::number) return static_cast<int>(m_number);
        return 0;
    }

    operator double() const {
        if (m_type == value_t::number) return m_number;
        return 0.0;
    }

    // Iterators for arrays
    auto begin() -> decltype(m_array.begin()) {
        if (m_type == value_t::array) return m_array.begin();
        static std::vector<json> empty;
        return empty.begin();
    }

    auto end() -> decltype(m_array.end()) {
        if (m_type == value_t::array) return m_array.end();
        static std::vector<json> empty;
        return empty.end();
    }

    auto begin() const -> decltype(m_array.begin()) {
        if (m_type == value_t::array) return m_array.begin();
        static const std::vector<json> empty;
        return empty.begin();
    }

    auto end() const -> decltype(m_array.end()) {
        if (m_type == value_t::array) return m_array.end();
        static const std::vector<json> empty;
        return empty.end();
    }

    // Serialization
    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        serialize(oss, indent, 0);
        return oss.str();
    }

private:
    void serialize(std::ostringstream& oss, int indent, int current_indent) const {
        switch (m_type) {
            case value_t::null:
                oss << "null";
                break;
            case value_t::boolean:
                oss << (m_boolean ? "true" : "false");
                break;
            case value_t::number:
                if (m_number == static_cast<int>(m_number)) {
                    oss << static_cast<int>(m_number);
                } else {
                    oss << m_number;
                }
                break;
            case value_t::string:
                oss << '"';
                for (char c : m_string) {
                    switch (c) {
                        case '"': oss << "\\\""; break;
                        case '\\': oss << "\\\\"; break;
                        case '\b': oss << "\\b"; break;
                        case '\f': oss << "\\f"; break;
                        case '\n': oss << "\\n"; break;
                        case '\r': oss << "\\r"; break;
                        case '\t': oss << "\\t"; break;
                        default:
                            if (c < 32) {
                                oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                            } else {
                                oss << c;
                            }
                    }
                }
                oss << '"';
                break;
            case value_t::array:
                oss << '[';
                for (size_t i = 0; i < m_array.size(); ++i) {
                    if (i > 0) oss << ',';
                    if (indent >= 0) {
                        oss << '\n' << std::string(current_indent + indent, ' ');
                    }
                    m_array[i].serialize(oss, indent, current_indent + indent);
                }
                if (indent >= 0 && !m_array.empty()) {
                    oss << '\n' << std::string(current_indent, ' ');
                }
                oss << ']';
                break;
            case value_t::object:
                oss << '{';
                bool first = true;
                for (const auto& pair : m_object) {
                    if (!first) oss << ',';
                    first = false;
                    if (indent >= 0) {
                        oss << '\n' << std::string(current_indent + indent, ' ');
                    }
                    oss << '"' << pair.first << "\":";
                    if (indent >= 0) oss << ' ';
                    pair.second.serialize(oss, indent, current_indent + indent);
                }
                if (indent >= 0 && !m_object.empty()) {
                    oss << '\n' << std::string(current_indent, ' ');
                }
                oss << '}';
                break;
        }
    }

public:
    // Parsing
    static json parse(const std::string& str) {
        size_t pos = 0;
        return parse_value(str, pos);
    }

private:
    static json parse_value(const std::string& str, size_t& pos) {
        skip_whitespace(str, pos);
        
        if (pos >= str.length()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        
        char c = str[pos];
        
        if (c == 'n') return parse_null(str, pos);
        if (c == 't' || c == 'f') return parse_boolean(str, pos);
        if (c == '"') return parse_string(str, pos);
        if (c == '[') return parse_array(str, pos);
        if (c == '{') return parse_object(str, pos);
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number(str, pos);
        
        throw std::runtime_error("Invalid JSON character");
    }

    static void skip_whitespace(const std::string& str, size_t& pos) {
        while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) {
            pos++;
        }
    }

    static json parse_null(const std::string& str, size_t& pos) {
        if (str.substr(pos, 4) == "null") {
            pos += 4;
            return json();
        }
        throw std::runtime_error("Invalid null value");
    }

    static json parse_boolean(const std::string& str, size_t& pos) {
        if (str.substr(pos, 4) == "true") {
            pos += 4;
            return json(true);
        }
        if (str.substr(pos, 5) == "false") {
            pos += 5;
            return json(false);
        }
        throw std::runtime_error("Invalid boolean value");
    }

    static json parse_string(const std::string& str, size_t& pos) {
        if (str[pos] != '"') throw std::runtime_error("Expected '\"'");
        pos++;
        
        std::string result;
        while (pos < str.length() && str[pos] != '"') {
            if (str[pos] == '\\') {
                pos++;
                if (pos >= str.length()) throw std::runtime_error("Incomplete escape sequence");
                
                switch (str[pos]) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: throw std::runtime_error("Invalid escape sequence");
                }
            } else {
                result += str[pos];
            }
            pos++;
        }
        
        if (pos >= str.length() || str[pos] != '"') {
            throw std::runtime_error("Unterminated string");
        }
        pos++;
        
        return json(result);
    }

    static json parse_number(const std::string& str, size_t& pos) {
        size_t start = pos;
        
        if (str[pos] == '-') pos++;
        
        if (pos >= str.length() || str[pos] < '0' || str[pos] > '9') {
            throw std::runtime_error("Invalid number");
        }
        
        while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') {
            pos++;
        }
        
        if (pos < str.length() && str[pos] == '.') {
            pos++;
            if (pos >= str.length() || str[pos] < '0' || str[pos] > '9') {
                throw std::runtime_error("Invalid number");
            }
            while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') {
                pos++;
            }
        }
        
        double value = std::stod(str.substr(start, pos - start));
        return json(value);
    }

    static json parse_array(const std::string& str, size_t& pos) {
        if (str[pos] != '[') throw std::runtime_error("Expected '['");
        pos++;
        
        json result = json::array();
        skip_whitespace(str, pos);
        
        if (pos < str.length() && str[pos] == ']') {
            pos++;
            return result;
        }
        
        while (true) {
            result.push_back(parse_value(str, pos));
            skip_whitespace(str, pos);
            
            if (pos >= str.length()) throw std::runtime_error("Unterminated array");
            
            if (str[pos] == ']') {
                pos++;
                break;
            } else if (str[pos] == ',') {
                pos++;
                skip_whitespace(str, pos);
            } else {
                throw std::runtime_error("Expected ',' or ']'");
            }
        }
        
        return result;
    }

    static json parse_object(const std::string& str, size_t& pos) {
        if (str[pos] != '{') throw std::runtime_error("Expected '{'");
        pos++;
        
        json result = json::object();
        skip_whitespace(str, pos);
        
        if (pos < str.length() && str[pos] == '}') {
            pos++;
            return result;
        }
        
        while (true) {
            skip_whitespace(str, pos);
            json key = parse_string(str, pos);
            skip_whitespace(str, pos);
            
            if (pos >= str.length() || str[pos] != ':') {
                throw std::runtime_error("Expected ':'");
            }
            pos++;
            
            json value = parse_value(str, pos);
            result[key.get_string()] = value;
            skip_whitespace(str, pos);
            
            if (pos >= str.length()) throw std::runtime_error("Unterminated object");
            
            if (str[pos] == '}') {
                pos++;
                break;
            } else if (str[pos] == ',') {
                pos++;
            } else {
                throw std::runtime_error("Expected ',' or '}'");
            }
        }
        
        return result;
    }
};

// Stream operators
inline std::ostream& operator<<(std::ostream& os, const json& j) {
    return os << j.dump();
}

inline std::istream& operator>>(std::istream& is, json& j) {
    std::string str((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
    j = json::parse(str);
    return is;
}

} // namespace nlohmann