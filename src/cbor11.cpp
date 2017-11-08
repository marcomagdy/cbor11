#include "cbor11.h"
#include <cmath>
#include <sstream>

cbor::cbor(unsigned value) : m_type(cbor::TYPE_UNSIGNED), m_unsigned(value) { }

cbor::cbor(int value) : m_type(value < 0 ? cbor::TYPE_NEGATIVE : cbor::TYPE_UNSIGNED),
    m_integer(value < 0 ? -1 - value : value) { }

cbor::cbor(uint64_t value) : m_type(cbor::TYPE_UNSIGNED), m_unsigned(value) { }

cbor::cbor(int64_t value) : m_type(value < 0 ? cbor::TYPE_NEGATIVE : cbor::TYPE_UNSIGNED),
    m_integer(value < 0 ? -1 - value : value) { }

cbor::cbor(const cbor::binary &value) : m_type(cbor::TYPE_BINARY), m_binary(new binary(value)) { }

cbor::cbor(const cbor::string &value) : m_type(cbor::TYPE_STRING), m_string(new string(value)) { }

cbor::cbor(const char *value) : m_type(cbor::TYPE_STRING), m_string(new string(value)) { }

cbor::cbor(const cbor::array &value) : m_type(cbor::TYPE_ARRAY), m_array(new array(value)) { }

cbor::cbor(const cbor::map &value) : m_type(cbor::TYPE_MAP), m_map(new map(value)) { }

cbor cbor::tagged(uint64_t tag, const cbor &value) {
    cbor result;
    result.m_type = cbor::TYPE_TAGGED;
    result.m_unsigned = tag;
    result.m_array = new cbor::array(1, value);
    return result;
}

cbor::cbor(cbor::simple value) : m_type(cbor::TYPE_SIMPLE), m_unsigned(value & 255) { }

cbor::cbor(bool value) : m_type(cbor::TYPE_SIMPLE), m_unsigned(value ? cbor::SIMPLE_TRUE : cbor::SIMPLE_FALSE) { }

cbor::cbor(float value) : m_type(cbor::TYPE_FLOAT), m_float(value) { }

cbor::cbor(double value) : m_type(cbor::TYPE_FLOAT), m_float(value) { }

cbor::cbor(std::nullptr_t) : m_type(cbor::TYPE_SIMPLE), m_unsigned(cbor::SIMPLE_NULL) { }

cbor::cbor(const cbor& other) : m_type(other.m_type), m_unsigned(other.m_unsigned) {
    switch(other.m_type)
    {
        case TYPE_BINARY:
            m_binary = new binary(*other.m_binary);
            break;
        case TYPE_STRING:
            m_string = new string(*other.m_string);
            break;
        case TYPE_TAGGED:
            // fallthrough
        case TYPE_ARRAY:
            m_array = new array(*other.m_array);
            break;
        case TYPE_MAP:
            m_map = new map(*other.m_map);
            break;
        default:
            break;
    }
}

cbor::cbor(cbor&& other) : m_type(other.m_type), m_unsigned(other.m_unsigned), m_binary(other.m_binary) { 
    other.m_binary = nullptr;
}

cbor::~cbor() {
    destroy();
}

cbor& cbor::operator = (const cbor& other) {
    if(this == &other) {
        return *this;
    }
    destroy();
    m_type = other.m_type;
    m_unsigned = other.m_unsigned;
    switch(m_type)
    {
        case TYPE_BINARY:
            m_binary = new binary(*other.m_binary);
            break;
        case TYPE_STRING:
            m_string = new string(*other.m_string);
            break;
        case TYPE_TAGGED:
            // fallthrough
        case TYPE_ARRAY:
            m_array = new array(*other.m_array);
            break;
        case TYPE_MAP:
            m_map = new map(*other.m_map);
            break;
        default:
            return *this;
    }
    return *this;
}

cbor& cbor::operator = (cbor&& other) {
    if(this == &other) {
        return *this;
    }
    swap(other);
    return *this;
}

void cbor::swap(cbor& other) {
    std::swap(m_type, other.m_type);
    std::swap(m_unsigned, other.m_unsigned);
    std::swap(m_binary, other.m_binary);
}

bool cbor::is_unsigned() const {
    return this->m_type == cbor::TYPE_UNSIGNED;
}

bool cbor::is_signed() const {
    return (this->m_type == cbor::TYPE_UNSIGNED || this->m_type == cbor::TYPE_NEGATIVE) && (this->m_unsigned >> 63) == 0;
}

bool cbor::is_int() const {
    return this->m_type == cbor::TYPE_UNSIGNED || this->m_type == cbor::TYPE_NEGATIVE;
}

bool cbor::is_binary() const {
    return this->m_type == cbor::TYPE_BINARY;
}

bool cbor::is_string() const {
    return this->m_type == cbor::TYPE_STRING;
}

bool cbor::is_array() const {
    return this->m_type == cbor::TYPE_ARRAY;
}

bool cbor::is_map() const {
    return this->m_type == cbor::TYPE_MAP;
}

bool cbor::is_tagged() const {
    return this->m_type == cbor::TYPE_TAGGED;
}

bool cbor::is_simple() const {
    return this->m_type == cbor::TYPE_SIMPLE;
}

bool cbor::is_bool() const {
    return this->m_type == cbor::TYPE_SIMPLE && (this->m_unsigned == cbor::SIMPLE_FALSE || this->m_unsigned == cbor::SIMPLE_TRUE);
}

bool cbor::is_null() const {
    return this->m_type == cbor::TYPE_SIMPLE && this->m_unsigned == cbor::SIMPLE_NULL;
}

bool cbor::is_undefined() const {
    return this->m_type == cbor::TYPE_SIMPLE && this->m_unsigned == cbor::SIMPLE_UNDEFINED;
}

bool cbor::is_float() const {
    return this->m_type == cbor::TYPE_FLOAT;
}

bool cbor::is_number() const {
    return this->m_type == cbor::TYPE_UNSIGNED || this->m_type == cbor::TYPE_NEGATIVE || this->m_type == cbor::TYPE_FLOAT;
}

uint64_t cbor::to_unsigned() const {
    switch (m_type) {
    case cbor::TYPE_UNSIGNED:
        return m_unsigned;
    case cbor::TYPE_NEGATIVE:
        return m_integer;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_unsigned();
    case cbor::TYPE_FLOAT:
        return m_float;
    default:
        return 0;
    }
}

int64_t cbor::to_signed() const {
    switch (m_type) {
    case cbor::TYPE_UNSIGNED:
        return m_integer;
    case cbor::TYPE_NEGATIVE:
        return -1 - m_integer;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_signed();
    case cbor::TYPE_FLOAT:
        return m_float;
    default:
        return 0;
    }
}

cbor::binary cbor::to_binary() const {
    switch (m_type) {
    case cbor::TYPE_BINARY:
        return *m_binary;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_binary();
    default:
        return cbor::binary();
    }
}

cbor::string cbor::to_string() const {
    switch (m_type) {
    case cbor::TYPE_STRING:
        return *m_string;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_string();
    default:
        return cbor::string();
    }
}

cbor::array cbor::to_array() const {
    switch (m_type) {
    case cbor::TYPE_ARRAY:
        return *m_array;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_array();
    default:
        return cbor::array();
    }
}

cbor::map cbor::to_map() const {
    switch (m_type) {
    case cbor::TYPE_MAP:
        return *m_map;
    case cbor::TYPE_TAGGED:
        return m_array->front().to_map();
    default:
        return cbor::map();
    }
}

cbor::simple cbor::to_simple() const {
    switch (m_type) {
    case cbor::TYPE_TAGGED:
        return m_array->front().to_simple();
    case cbor::TYPE_SIMPLE:
        return cbor::simple(m_unsigned);
    default:
        return cbor::SIMPLE_UNDEFINED;
    }
}

bool cbor::to_bool() const {
    switch (m_type) {
    case cbor::TYPE_TAGGED:
        return m_array->front().to_bool();
    case cbor::TYPE_SIMPLE:
        return m_unsigned == cbor::SIMPLE_TRUE;
    default:
        return false;
    }
}

double cbor::to_float() const {
    switch (m_type) {
    case cbor::TYPE_UNSIGNED:
        return double(m_unsigned);
    case cbor::TYPE_NEGATIVE:
        return ldexp(-1 - (m_integer >> 32), 32) + (-1 - (m_integer << 32 >> 32));
    case cbor::TYPE_TAGGED:
        return m_array->front().to_float();
    case cbor::TYPE_FLOAT:
        return m_float;
    default:
        return 0.0;
    }
}

cbor::operator unsigned() const {
    return to_unsigned();
}

cbor::operator int() const {
    return to_signed();
}

cbor::operator cbor::binary() const {
    return to_binary();
}

cbor::operator cbor::string() const {
    return to_string();
}

cbor::operator cbor::array() const {
    return to_array();
}

cbor::operator cbor::map() const {
    return to_map();
}

cbor::operator cbor::simple() const {
    return to_simple();
}

cbor::operator bool() const {
    return to_bool();
}

cbor::operator double() const {
    return to_float();
}

cbor::type_t cbor::type() const {
    return m_type;
}

uint64_t cbor::tag() const {
    switch (m_type) {
    case cbor::TYPE_TAGGED:
        return m_unsigned;
    default:
        return 0;
    }
}

cbor cbor::child() const {
    switch (this->m_type) {
    case cbor::TYPE_TAGGED:
        return m_array->front();
    default:
        return cbor();
    }
}

bool cbor::operator < (const cbor &other) const {
    if (this->m_type < other.m_type) {
        return true;
    }
    if (this->m_type > other.m_type) {
        return false;
    }
    switch (m_type) {
    case cbor::TYPE_BINARY:
        return m_binary < other.m_binary;
    case cbor::TYPE_STRING:
        return m_string < other.m_string;
    case cbor::TYPE_ARRAY:
        return m_array < other.m_array;
    case cbor::TYPE_MAP:
        return m_map < other.m_map;
    case cbor::TYPE_TAGGED:
        if (m_unsigned == other.m_unsigned) {
            return m_array->front() < other.m_array->front();
        }
        // fallthrough
    default:
        return m_unsigned < other.m_unsigned;
    }
}

bool cbor::operator == (const cbor &other) const {
    if (this->m_type != other.m_type) {
        return false;
    }
    switch (m_type) {
    case cbor::TYPE_BINARY:
        return m_binary == other.m_binary;
    case cbor::TYPE_STRING:
        return m_string == other.m_string;
    case cbor::TYPE_ARRAY:
        return m_array == other.m_array;
    case cbor::TYPE_MAP:
        return m_map == other.m_map;
    case cbor::TYPE_TAGGED:
        if (m_unsigned != other.m_unsigned) {
            return false;
        }
        return m_array == other.m_array;
    default:
        return m_unsigned == other.m_unsigned;
    }
}

bool cbor::operator != (const cbor &other) const {
    return !(*this == other);
}

void read_uint(std::istream &in, int &major, int &minor, uint64_t &value) {
    major = (in.peek() >> 5) & 7;
    minor = in.get() & 31;
    value = 0;
    switch (minor) {
    case 27:
        value |= (unsigned long long) in.get() << 56;
        value |= (unsigned long long) in.get() << 48;
        value |= (unsigned long long) in.get() << 40;
        value |= (unsigned long long) in.get() << 32;
    case 26:
        value |= in.get() << 24;
        value |= in.get() << 16;
    case 25:
        value |= in.get() << 8;
    case 24:
        value |= in.get();
        break;
    default:
        value = minor;
        break;
    }
}

bool cbor::read(std::istream &in) {
    cbor item;
    int major, minor;
    uint64_t value;
    read_uint(in, major, minor, value);
    switch (major) {
    case 0:
        if (minor > 27) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_UNSIGNED;
        item.m_unsigned = value;
        break;
    case 1:
        if (minor > 27) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_NEGATIVE;
        item.m_integer = value;
        break;
    case 2:
        if (minor > 27 && minor < 31) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_BINARY;
        item.m_binary = new binary;
        if (minor == 31) {
            while (in.good() && in.peek() != 255) {
                read_uint(in, major, minor, value);
                if (major != 2 || minor > 27) {
                    in.setstate(std::ios_base::failbit);
                    return false;
                }
                for (uint64_t i = 0; in.good() && i != value; ++i) {
                    item.m_binary->push_back(in.get());
                }
            }
            in.get();
        } else {
            item.m_binary->reserve(value);
            for (uint64_t i = 0; in.good() && i != value; ++i) {
                item.m_binary->push_back(in.get());
            }
        }
        break;
    case 3:
        if (minor > 27 && minor < 31) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_STRING;
        item.m_string = new string;
        if (minor == 31) {
            while (in.good() && in.peek() != 255) {
                read_uint(in, major, minor, value);
                if (major  != 3 || minor > 27) {
                    in.setstate(std::ios_base::failbit);
                    return false;
                }
                for (uint64_t i = 0; in.good() && i != value; ++i) {
                    item.m_string->push_back(in.get());
                }
            }
            in.get();
        } else {
            item.m_string->reserve(value);
            for (uint64_t i = 0; in.good() && i != value; ++i) {
                item.m_string->push_back(in.get());
            }
        }
        break;
    case 4:
        if (minor > 27 && minor < 31) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_ARRAY;
        item.m_array = new array;
        if (minor == 31) {
            while (in.good() && in.peek() != 255) {
                cbor child;
                child.read(in);
                item.m_array->emplace_back(std::move(child));
            }
            in.get();
        } else {
            item.m_array->reserve(value);
            for (uint64_t i = 0; in.good() && i != value; ++i) {
                cbor child;
                child.read(in);
                item.m_array->emplace_back(std::move(child));
            }
        }
        break;
    case 5:
        if (minor > 27 && minor < 31) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_MAP;
        item.m_map = new map;
        if (minor == 31) {
            while (in.good() && in.peek() != 255) {
                cbor key, value;
                key.read(in);
                value.read(in);
                item.m_map->emplace(key, value);
            }
            in.get();
        } else {
            for (uint64_t i = 0; in.good() && i != value; ++i) {
                cbor key, value;
                key.read(in);
                value.read(in);
                item.m_map->emplace(key, value);
            }
        }
        break;
    case 6: {
        if (minor > 27) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        item.m_type = cbor::TYPE_TAGGED;
        item.m_unsigned = value;
        item.m_array = new array;
        cbor child;
        child.read(in);
        item.m_array->emplace_back(std::move(child));
        break;
    }
    case 7:
        if (minor > 27) {
            in.setstate(std::ios_base::failbit);
            return false;
        }
        switch (minor) {
        case 25: {
            item.m_type = cbor::TYPE_FLOAT;
            int sign = value >> 15;
            int exponent = value >> 10 & 31;
            int significand = value & 1023;
            if (exponent == 31) {
                if (significand) {
                    item.m_float = NAN;
                } else if (sign) {
                    item.m_float = -INFINITY;
                } else {
                    item.m_float = INFINITY;
                }
            } else if (exponent == 0) {
                if (sign) {
                    item.m_float = -ldexp(significand, -24);
                } else {
                    item.m_float = ldexp(significand, -24);
                }
            } else {
                if (sign) {
                    item.m_float = -ldexp(1024 | significand, exponent - 25);
                } else {
                    item.m_float = ldexp(1024 | significand, exponent - 25);
                }
            }
            break;
        }
        case 26: {
            union {
                float f;
                uint32_t i;
            };
            i = value;
            item.m_type = cbor::TYPE_FLOAT;
            item.m_float = f;
            break;
        }
        case 27:
            item.m_type = cbor::TYPE_FLOAT;
            item.m_float = value;
            break;
        default:
            item.m_type = cbor::TYPE_SIMPLE;
            item.m_unsigned = value;
        }
        break;
    }
    if (!in.good()) {
        in.setstate(std::ios_base::failbit);
        return false;
    }
    *this = item;
    return true;
}

void write_uint8(std::ostream &out, int major, uint64_t value) {
    if (value < 24) {
        out.put(major << 5 | value);
    } else {
        out.put(major << 5 | 24);
        out.put(value);
    }
}

void write_uint16(std::ostream &out, int major, uint64_t value) {
    out.put(major << 5 | 25);
    out.put(value >> 8);
    out.put(value);
}

void write_uint32(std::ostream &out, int major, uint64_t value) {
    out.put(major << 5 | 26);
    out.put(value >> 24);
    out.put(value >> 16);
    out.put(value >> 8);
    out.put(value);
}

void write_uint64(std::ostream &out, int major, uint64_t value) {
    out.put(major << 5 | 27);
    out.put(value >> 56);
    out.put(value >> 48);
    out.put(value >> 40);
    out.put(value >> 32);
    out.put(value >> 24);
    out.put(value >> 16);
    out.put(value >> 8);
    out.put(value);
}

void write_uint(std::ostream &out, int major, uint64_t value) {
    if ((value >> 8) == 0) {
        write_uint8(out, major, value);
    } else if ((value >> 16) == 0) {
        write_uint16(out, major, value);
    } else if (value >> 32 == 0) {
        write_uint32(out, major, value);
    } else {
        write_uint64(out, major, value);
    }
}

void write_float(std::ostream &out, double value) {
    if (double(float(value)) == value) {
        union {
            float f;
            uint32_t i;
        };
        f = value;
        write_uint32(out, 7, i);
    } else {
        union {
            double f;
            uint64_t i;
        };
        f = value;
        write_uint64(out, 7, i);
    }
}

void cbor::write(std::ostream &out) const {
    switch (this->m_type) {
    case cbor::TYPE_UNSIGNED:
        write_uint(out, 0, m_unsigned);
        break;
    case cbor::TYPE_NEGATIVE:
        write_uint(out, 1, m_integer);
        break;
    case cbor::TYPE_BINARY:
        write_uint(out, 2, m_binary->size());
        out.write(reinterpret_cast<const char *>(m_binary->data()), m_binary->size());
        break;
    case cbor::TYPE_STRING:
        write_uint(out, 3, m_string->size());
        out.write(m_string->c_str(), m_string->size());
        break;
    case cbor::TYPE_ARRAY:
        write_uint(out, 4, m_array->size());
        for(const auto& e : *m_array) {
            e.write(out);
        }
        break;
    case cbor::TYPE_MAP:
        write_uint(out, 5, m_map->size());
        for(auto&& e : *m_map) {
            e.first.write(out);
            e.second.write(out);
        }
        break;
    case cbor::TYPE_TAGGED:
        write_uint(out, 6, m_unsigned);
        m_array->front().write(out);
        break;
    case cbor::TYPE_SIMPLE:
        write_uint8(out, 7, m_unsigned);
        break;
    case cbor::TYPE_FLOAT:
        write_float(out, m_float);
        break;
    }
}

bool cbor::validate(const cbor::binary &in) {
    std::istringstream buf1(std::string(in.begin(), in.end()));
    cbor buf2;
    return buf2.read(buf1) && buf1.peek() == EOF;
}

cbor cbor::decode(const cbor::binary &in) {
    std::istringstream buf1(std::string(in.begin(), in.end()));
    cbor buf2;
    if (buf2.read(buf1) && buf1.peek() == EOF) {
        return buf2;
    }
    return cbor();
}

cbor::binary cbor::encode(const cbor &in) {
    std::ostringstream buf1;
    in.write(buf1);
    std::string buf2 = buf1.str();
    return cbor::binary(buf2.begin(), buf2.end());
}

cbor::string cbor::debug(const cbor &in) {
    std::ostringstream out;
    switch (in.m_type) {
    case cbor::TYPE_UNSIGNED:
        out << in.m_unsigned;
        break;
    case cbor::TYPE_NEGATIVE:
        if (1 + in.m_integer == 0) {
            out << "-18446744073709551616";
        } else {
            out << "-" << 1 + in.m_integer;
        }
        break;
    case cbor::TYPE_BINARY:
        out << "h'";
        out << std::hex;
        out.fill('0');
        for(auto e : *in.m_binary) {
            out.width(2);
            out << int(e);
        }
        out << "'";
        break;
    case cbor::TYPE_STRING:
        out << "\"";
        out << std::hex;
        out.fill('0');
        for(auto e : *in.m_string) {
            switch (e) {
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            default:
                if ((unsigned char)e < '\x20') {
                    out << "\\u";
                    out.width(4);
                    out << (int) (unsigned char)e;
                } else {
                    out << e;
                }
                break;
            }
        }
        out << "\"";
        break;
    case cbor::TYPE_ARRAY:
        out << "[";
        for (cbor::array::const_iterator it = (*in.m_array).begin(); it != (*in.m_array).end(); ++it) {
            if (it != in.m_array->begin()) {
                out << ", ";
            }
            out << cbor::debug(*it);
        }
        out << "]";
        break;
    case cbor::TYPE_MAP:
        out << "{";
        for (cbor::map::const_iterator it = (*in.m_map).begin(); it != (*in.m_map).end(); ++it) {
            if (it != in.m_map->begin()) {
                out << ", ";
            }
            out << cbor::debug(it->first) << ": " << cbor::debug(it->second);
        }
        out << "}";
        break;
    case cbor::TYPE_TAGGED:
        out << in.m_unsigned << "(" << cbor::debug(in.m_array->front()) << ")";
        break;
    case cbor::TYPE_SIMPLE:
        switch (in.m_unsigned) {
        case cbor::SIMPLE_FALSE:
            out << "false";
            break;
        case cbor::SIMPLE_TRUE:
            out << "true";
            break;
        case cbor::SIMPLE_NULL:
            out << "null";
            break;
        case cbor::SIMPLE_UNDEFINED:
            out << "undefined";
            break;
        default:
            out << "simple(" << in.m_unsigned << ")";
            break;
        }
        break;
    case cbor::TYPE_FLOAT:
        if (std::isinf(in.m_float)) {
            if (in.m_float < 0) {
                out << "-";
            }
            out << "Infinity";
        } else if (std::isnan(in.m_float)) {
            out << "NaN";
        } else {
            out << std::showpoint << in.m_float;
        }
        break;
    }
    return out.str();
}

void cbor::destroy()
{
    switch(m_type)
    {
        case TYPE_BINARY:
            delete m_binary;
            m_binary = nullptr;
            break;
        case TYPE_STRING:
            delete m_string;
            m_string = nullptr;
            break;
        case TYPE_TAGGED:
            // fallthrough
        case TYPE_ARRAY:
            delete m_array;
            m_array = nullptr;
            break;
        case TYPE_MAP:
            delete m_map;
            m_map = nullptr;
            break;
        default:
            return;
    }
}

void swap(cbor& left, cbor& right) {
    left.swap(right);
}
