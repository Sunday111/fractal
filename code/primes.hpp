#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>

constexpr float my_sqrt(const float x)
{
    int ui = std::bit_cast<int>(x);
    ui = (1 << 29) + (ui >> 1) - (1 << 22);
    float r = std::bit_cast<float>(ui);
    return r;
}

using ui8 = uint8_t;
using ui64 = uint64_t;
using i64 = int64_t;

template <size_t Size, typename Word = ui64>
class StaticBitset
{
public:
    static constexpr size_t word_bits = sizeof(Word) * 8;
    static constexpr size_t full_words = Size / word_bits;
    static constexpr size_t words_count = (Size % word_bits) ? full_words + 1 : full_words;

public:
    constexpr StaticBitset() {}

    constexpr void reset(bool value) noexcept
    {
        const auto k = value ? (~1ui64) : (0ui64);
        for (size_t i = 0; i < words_count; ++i)
        {
            m_data[i] = k;
        }
    }

    constexpr bool get_at(size_t idx) const
    {
        size_t i = idx / 64;
        size_t j = idx % 64;
        auto mask = make_bit_mask(j);
        bool r = (m_data[i] & mask);
        return r;
    }

    constexpr void set_at(size_t idx, bool new_value)
    {
        size_t i = idx / 64;
        size_t j = idx % 64;
        auto mask = make_bit_mask(j);
        if (new_value)
        {
            m_data[i] |= mask;
        }
        else
        {
            m_data[i] &= ~mask;
        }
    }

private:
    static constexpr Word make_bit_mask(size_t index)
    {
        return (static_cast<Word>(1) << index);
    }

private:
    Word m_data[words_count]{};
};

class PrimesStorage
{
public:
    constexpr PrimesStorage() noexcept
    {
        make_first_segment();
    }

    PrimesStorage(const PrimesStorage&) = delete;
    PrimesStorage& operator=(const PrimesStorage&) = delete;

    [[nodiscard]] constexpr ui64 get_at(size_t prime_index) noexcept
    {
        [[likely]] if (prime_index < m_primes.size())
        {
            return m_primes[prime_index];
        }

        make_segments(prime_index);
        return get_at(prime_index);
    }

private:
    // fill first segment by usual Eratosthenes sieve
    constexpr void make_first_segment() noexcept
    {
        m_segment.reset(true);
        m_segment.set_at(0, false);
        m_segment.set_at(1, false);

        for (size_t i = 2; i < (segment_size / 2) + 1; ++i)
        {
            if (m_segment.get_at(i))
            {
                for (size_t j = i * 2; j < segment_size; j += i)
                {
                    m_segment.set_at(j, false);
                }
            }
        }

        for (ui64 i = 0; i < segment_size; ++i)
        {
            if (m_segment.get_at(i))
            {
                m_primes.push_back(i);
            }
        }

        m_segments.push_back(m_primes.size());
    }

    constexpr void make_segments(size_t required_prime_index) noexcept
    {
        while (required_prime_index >= m_primes.size())
        {
            make_next_segment();
        }
    }

    constexpr void make_next_segment() noexcept
    {
        const size_t prev_primes = m_primes.size();
        const size_t segment_index = m_segments.size();
        const size_t segment_begin = segment_index * segment_size;
        const size_t segment_end = segment_begin + segment_size;
        const size_t m = segment_end - 1;
        const size_t msq = static_cast<size_t>(my_sqrt(static_cast<float>(m))) + 1;

        m_segment.reset(true);

        size_t prime_idx = 0;
        while (m_primes.size() > prime_idx && m_primes[prime_idx] <= msq)
        {
            const size_t prime = m_primes[prime_idx];
            size_t pos = segment_begin / prime;
            pos += (segment_begin % prime) ? 1 : 0;
            pos *= prime;

            while (pos < segment_end)
            {
                m_segment.set_at(pos % segment_size, false);
                pos += prime;
            }

            ++prime_idx;
        }

        for (size_t i = 0; i < segment_size; ++i)
        {
            if (m_segment.get_at(i))
            {
                m_primes.push_back(segment_begin + i);
            }
        }

        const size_t primes_in_segment = m_primes.size() - prev_primes;
        m_segments.push_back(primes_in_segment);
    }

private:
    static constexpr size_t segment_size = 64 * 8;
    StaticBitset<segment_size> m_segment;
    std::vector<ui64> m_primes;
    std::vector<size_t> m_segments;
};

[[nodiscard]] constexpr bool is_prime(size_t n) noexcept
{
    // Corner cases
    if (n <= 1) return false;
    if (n <= 3) return true;

    if (n % 2 == 0 || n % 3 == 0) return false;

    for (int i = 5; i * i <= n; i = i + 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;

    return true;
};

void runtime_test_correctness() noexcept
{
    PrimesStorage storage;
    size_t index = 0;
    for (size_t value = 2; value < 10000; ++value)
    {
        if (is_prime(value))
        {
            if (storage.get_at(index) != value)
            {
                std::cout << "at index " << index << "\n";
            }

            ++index;
        }
    }
}

[[nodiscard]] constexpr bool compiletime_test_correctness() noexcept
{
    PrimesStorage storage;
    size_t index = 0;
    for (size_t value = 2; value < 10000; ++value)
    {
        if (is_prime(value))
        {
            if (storage.get_at(index) != value)
            {
                return false;
            }

            ++index;
        }
    }

    return true;
}

struct PrimeAndPower
{
    ui64 power;
    ui64 prime_index;
};

template <typename T, typename Enable = std::enable_if_t<std::is_unsigned_v<T>>>
[[nodiscard]] constexpr size_t bits_required_for_number(T number) noexcept
{
    size_t bits = 0;
    while (number != 0)
    {
        ++bits;
        number >>= 1;
    }

    return bits;
}

template <typename T, typename Enable = std::enable_if_t<std::is_enum_v<T>>>
[[nodiscard]] constexpr size_t bits_required_for_enum(T value) noexcept
{
    using U = std::underlying_type_t<T>;
    const U u_val = static_cast<U>(value);
    return bits_required_for_number(u_val);
}

class FactorizedNumber
{
public:
    constexpr FactorizedNumber() noexcept
    {
        m_negative = false;
    }

    constexpr explicit FactorizedNumber(PrimesStorage& storage, ui64 value) noexcept
    {
        m_negative = false;

        if (value > 0)
        {
            factorize(storage, value);
        }
    }

    [[nodiscard]] constexpr i64 to_number(PrimesStorage& storage) const noexcept
    {
        if (is_zero())
        {
            return 0i64;
        }

        const i64 v = static_cast<i64>(defactorize(storage));
        return m_negative ? -v : v;
    }

    constexpr FactorizedNumber& operator*=(const FactorizedNumber& another) noexcept
    {
        return mul(another);
    }

private:
    constexpr void factorize(PrimesStorage& storage, ui64 v) noexcept
    {
        [[likely]] if (v > 1)
        {
            for (size_t i = 0; v > 1; ++i)
            {
                const auto prime = storage.get_at(i);
                size_t n = 0;
                while ((v % prime) == 0)
                {
                    v /= prime;
                    ++n;
                }

                if (n > 0)
                {
                    PrimeAndPower factor;
                    factor.prime_index = i;
                    factor.power = n;
                    m_factors.push_back(factor);
                }
            }
        }
        else
        {
            PrimeAndPower factor;
            factor.prime_index = 0;
            factor.power = 0;
            m_factors.push_back(factor);
        }
    }

    [[nodiscard]] constexpr bool is_zero() const noexcept
    {
        return m_factors.empty();
    }

    constexpr bool is_one() const noexcept
    {
        return m_factors.size() == 1 && m_factors.back().power == 0;
    }

    constexpr FactorizedNumber& mul(const FactorizedNumber& another) noexcept
    {
        auto& factors_a = m_factors;
        auto& factors_b = another.m_factors;

        if (is_zero() || another.is_zero())
        {
            m_factors.clear();
            m_negative = false;
        }
        else
        {
            auto num_factors_a = factors_a.size();
            auto num_factors_b = factors_b.size();

            size_t i = 0;
            size_t j = 0;
            while (i < num_factors_a && j < num_factors_b)
            {
                auto& factor_a = factors_a[i];
                while (j < num_factors_b && factors_b[j].prime_index < factor_a.prime_index)
                {
                    auto& factor_b = factors_b[j];
                    factors_a.push_back(factor_b);
                    ++j;
                }

                if (j < num_factors_b && factors_b[j].prime_index == factor_a.prime_index)
                {
                    auto& factor_b = factors_b[j];
                    factor_a.power += factor_b.power;
                    ++j;
                }

                ++i;
            }

            factors_a.insert(factors_a.end(), factors_b.begin() + j, factors_b.end());

            auto removed_range = std::ranges::remove(factors_a, 0ui64, &PrimeAndPower::power);
            factors_a.erase(removed_range.begin(), removed_range.end());

            std::ranges::sort(
                factors_a,
                [](auto& a, auto& b)
                {
                    return a.prime_index < b.prime_index;
                });
        }

        return *this;
    }

    [[nodiscard]] constexpr ui64 defactorize(PrimesStorage& storage) const noexcept
    {
        ui64 v = 1;
        for (const auto& factor : m_factors)
        {
            for (size_t i = 0; i < factor.power; ++i)
            {
                v *= storage.get_at(factor.prime_index);
            }
        }

        return v;
    }

private:
    std::vector<PrimeAndPower> m_factors;
    ui8 m_negative : 1;
};

static_assert(compiletime_test_correctness());