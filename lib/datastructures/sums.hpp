#pragma once

template<typename NumberT>
struct BasicAggregation {
    NumberT result{};
    void Add(NumberT x) { result += x; }
};

template<typename NumberT>
struct KahanAggregation {
    NumberT result{};
    NumberT kahan_error{};
    void Add(NumberT x) {
        const auto y = x - kahan_error;
        const auto t = result + y;
        kahan_error = t - result - y;
        result = t;
    }
};
