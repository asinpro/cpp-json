#include <cassert>
#include <chrono>
#include <exception>
#include <sstream>
#include <string_view>
#include <iostream>

#include "json.h"
#include "json_builder.h"

using namespace std;
using namespace std::literals;
using namespace json;

namespace {

// Ниже даны тесты, проверяющие JSON-библиотеку.
// Можете воспользоваться ими, чтобы протестировать свой код.
// Раскомментируйте их по мере работы.

json::Document LoadJSON(const std::string& s) {
    std::istringstream strm(s);
    return json::Load(strm);
}

std::string Print(const Node& node) {
    std::ostringstream out;
    Print(Document{node}, out);
    return out.str();
}

void MustFailToLoad(const std::string& s) {
    try {
        LoadJSON(s);
        std::cerr << "ParsingError exception is expected on '"sv << s << "'"sv << std::endl;
        assert(false);
    } catch (const json::ParsingError&) {
        // ok
    } catch (const std::exception& e) {
        std::cerr << "exception thrown: "sv << e.what() << std::endl;
        assert(false);
    } catch (...) {
        std::cerr << "Unexpected error"sv << std::endl;
        assert(false);
    }
}

template <typename Fn>
void MustThrowLogicError(Fn fn) {
    try {
        fn();
        std::cerr << "logic_error is expected"sv << std::endl;
        assert(false);
    } catch (const std::logic_error&) {
        // ok
    } catch (const std::exception& e) {
        std::cerr << "exception thrown: "sv << e.what() << std::endl;
        assert(false);
    } catch (...) {
        std::cerr << "Unexpected error"sv << std::endl;
        assert(false);
    }
}

void TestNull() {
    Node null_node;
    assert(null_node.IsNull());
    assert(!null_node.IsInt());
    assert(!null_node.IsDouble());
    assert(!null_node.IsPureDouble());
    assert(!null_node.IsString());
    assert(!null_node.IsArray());
    assert(!null_node.IsDict());

    Node null_node1{nullptr};
    assert(null_node1.IsNull());

    assert(Print(null_node) == "null"s);
    assert(null_node == null_node1);
    assert(!(null_node != null_node1));

    const Node node = LoadJSON("null"s).GetRoot();
    assert(node.IsNull());
    assert(node == null_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON(" \t\r\n\n\r null \t\r\n\n\r "s).GetRoot() == null_node);

    assert(Builder{}.Value(nullptr).Build().IsNull());
}

void TestNumbers() {
    const Node int_node{42};
    assert(int_node.IsInt());
    assert(int_node.AsInt() == 42);
    // целые числа являются подмножеством чисел с плавающей запятой
    assert(int_node.IsDouble());
    // Когда узел хранит int, можно получить соответствующее ему double-значение
    assert(int_node.AsDouble() == 42.0);
    assert(!int_node.IsPureDouble());
    assert(int_node == Node{42});
    // int и double - разные типы, поэтому не равны, даже когда хранят
    assert(int_node != Node{42.0});

    const Node dbl_node{123.45};
    assert(dbl_node.IsDouble());
    assert(dbl_node.AsDouble() == 123.45);
    assert(dbl_node.IsPureDouble());  // Значение содержит число с плавающей запятой
    assert(!dbl_node.IsInt());

    assert(Print(int_node) == "42"s);
    assert(Print(dbl_node) == "123.45"s);
    assert(Print(Node{-42}) == "-42"s);
    assert(Print(Node{-3.5}) == "-3.5"s);

    assert(LoadJSON("42"s).GetRoot() == int_node);
    assert(LoadJSON("123.45"s).GetRoot() == dbl_node);
    assert(LoadJSON("0.25"s).GetRoot().AsDouble() == 0.25);
    assert(LoadJSON("3e5"s).GetRoot().AsDouble() == 3e5);
    assert(LoadJSON("1.2e-5"s).GetRoot().AsDouble() == 1.2e-5);
    assert(LoadJSON("1.2e+5"s).GetRoot().AsDouble() == 1.2e5);
    assert(LoadJSON("-123456"s).GetRoot().AsInt() == -123456);
    assert(LoadJSON("0").GetRoot() == Node{0});
    assert(LoadJSON("0.0").GetRoot() == Node{0.0});
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON(" \t\r\n\n\r 0.0 \t\r\n\n\r ").GetRoot() == Node{0.0});

    assert(Builder{}.Value(-100).Build().AsInt() == -100);
    assert(Builder{}.Value(-0.5).Build().AsDouble() == -0.5);
}

void TestStrings() {
    Node str_node{"Hello, \"everybody\""s};
    assert(str_node.IsString());
    assert(str_node.AsString() == "Hello, \"everybody\""s);

    assert(!str_node.IsInt());
    assert(!str_node.IsDouble());

    assert(Print(str_node) == "\"Hello, \\\"everybody\\\"\""s);

    assert(LoadJSON(Print(str_node)).GetRoot() == str_node);
    const std::string escape_chars
        = R"("\r\n\t\"\\")"s;  // При чтении строкового литерала последовательности \r,\n,\t,\\,\"
    // преобразовываться в соответствующие символы.
    // При выводе эти символы должны экранироваться, кроме \t.
    assert(Print(LoadJSON(escape_chars).GetRoot()) == "\"\\r\\n\t\\\"\\\\\""s);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON("\t\r\n\n\r \"Hello\" \t\r\n\n\r ").GetRoot() == Node{"Hello"s});

    assert(Builder{}.Value("test"s).Build().AsString() == "test"s);
}

void TestBool() {
    Node true_node{true};
    assert(true_node.IsBool());
    assert(true_node.AsBool());

    Node false_node{false};
    assert(false_node.IsBool());
    assert(!false_node.AsBool());

    assert(Print(true_node) == "true"s);
    assert(Print(false_node) == "false"s);

    assert(LoadJSON("true"s).GetRoot() == true_node);
    assert(LoadJSON("false"s).GetRoot() == false_node);
    assert(LoadJSON(" \t\r\n\n\r true \r\n"s).GetRoot() == true_node);
    assert(LoadJSON(" \t\r\n\n\r false \t\r\n\n\r "s).GetRoot() == false_node);

    assert(Builder{}.Value(true).Build().AsBool() == true);
}

void TestArray() {
    Node arr_node{Array{1, 1.23, "Hello"s}};
    assert(arr_node.IsArray());
    const Array& arr = arr_node.AsArray();
    assert(arr.size() == 3);
    assert(arr.at(0).AsInt() == 1);

    assert(LoadJSON("[1,1.23,\"Hello\"]"s).GetRoot() == arr_node);
    assert(LoadJSON(Print(arr_node)).GetRoot() == arr_node);
    assert(LoadJSON(R"(  [ 1  ,  1.23,  "Hello"   ]   )"s).GetRoot() == arr_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(LoadJSON("[ 1 \r \n ,  \r\n\t 1.23, \n \n  \t\t  \"Hello\" \t \n  ] \n  "s).GetRoot()
           == arr_node);

    assert(Builder{}.Value(arr).Build().AsArray() == arr);
}

void TestMap() {
    Node dict_node{Dict{{"key1"s, "value1"s}, {"key2"s, 42}}};
    assert(dict_node.IsDict());
    const Dict& dict = dict_node.AsDict();
    assert(dict.size() == 2);
    assert(dict.at("key1"s).AsString() == "value1"s);
    assert(dict.at("key2"s).AsInt() == 42);

    assert(LoadJSON("{ \"key1\": \"value1\", \"key2\": 42 }"s).GetRoot() == dict_node);
    assert(LoadJSON(Print(dict_node)).GetRoot() == dict_node);
    // Пробелы, табуляции и символы перевода строки между токенами JSON файла игнорируются
    assert(
        LoadJSON(
            "\t\r\n\n\r { \t\r\n\n\r \"key1\" \t\r\n\n\r: \t\r\n\n\r \"value1\" \t\r\n\n\r , \t\r\n\n\r \"key2\" \t\r\n\n\r : \t\r\n\n\r 42 \t\r\n\n\r } \t\r\n\n\r"s)
            .GetRoot()
        == dict_node);

    assert(Builder{}.Value(dict).Build().AsDict() == dict);
}

void TestErrorHandling() {
    MustFailToLoad("["s);
    MustFailToLoad("]"s);

    MustFailToLoad("{"s);
    MustFailToLoad("}"s);

    MustFailToLoad("\"hello"s);  // незакрытая кавычка

    MustFailToLoad("tru"s);
    MustFailToLoad("fals"s);
    MustFailToLoad("nul"s);

    Node dbl_node{3.5};
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsInt();
    });
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsString();
    });
    MustThrowLogicError([&dbl_node] {
        dbl_node.AsArray();
    });

    Node array_node{Array{}};
    MustThrowLogicError([&array_node] {
        array_node.AsDict();
    });
    MustThrowLogicError([&array_node] {
        array_node.AsDouble();
    });
    MustThrowLogicError([&array_node] {
        array_node.AsBool();
    });
}

void Benchmark() {
    const auto start = std::chrono::steady_clock::now();
    Array arr;
    arr.reserve(1'000);
    for (int i = 0; i < 1'000; ++i) {
        arr.emplace_back(Dict{
            {"int"s, 42},
            {"double"s, 42.1},
            {"null"s, nullptr},
            {"string"s, "hello"s},
            {"array"s, Array{1, 2, 3}},
            {"bool"s, true},
            {"map"s, Dict{{"key"s, "value"s}}},
        });
    }
    std::stringstream strm;
    json::Print(Document{arr}, strm);
    const auto doc = json::Load(strm);
    assert(doc.GetRoot() == arr);
    const auto duration = std::chrono::steady_clock::now() - start;
    std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms"sv
              << std::endl;
}

}  // namespace

int main() {
    TestNull();
    TestNumbers();
    TestStrings();
    TestBool();
    TestArray();
    TestMap();
    TestErrorHandling();

    Benchmark();

    // json::Builder{}.StartDict().Build();  // правило 3
    // json::Builder{}.StartDict().Key("1"s).Value(1).Value(1);  // правило 2
    // json::Builder{}.StartDict().Key("1"s).Key(""s);  // правило 1
    // json::Builder{}.StartArray().Key("1"s);  // правило 4
    // json::Builder{}.StartArray().EndDict();  // правило 4
    // json::Builder{}.StartArray().Value(1).Value(2).EndDict();  // правило 5

        json::Print(
            json::Document{
                json::Builder{}
                .StartDict()
                    .Key("key1"s).Value(123)
                    .Key("key2"s).Value("value2"s)
                    .Key("key3"s).StartArray()
                        .StartArray().EndArray()
                        .Value(456)
                        .StartDict().EndDict()
                        .StartDict()
                            .Key(""s)
                            .Value(nullptr)
                        .EndDict()
                        .StartDict()
                            .Key("test"s).StartDict().EndDict()
                        .EndDict()
                        .Value(""s)
                    .EndArray()
                .EndDict()
                .Build()
            },
            cout
        );
        cout << endl;

    json::Print(
        json::Document{
            json::Builder{}
            .Value("just a string"s)
            .Build()
        },
        cout
    );
    cout << endl;
}
