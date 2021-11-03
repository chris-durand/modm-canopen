#ifndef CANOPEN_UTILITY_CONSTEXPR_MAP
#define CANOPEN_UTILITY_CONSTEXPR_MAP

#include <algorithm>
#include <array>
#include <iterator>
#include <utility>

namespace modm_canopen
{

template<typename K, typename V,
    std::size_t C, typename Cmp = std::less<>>
class ConstexprMap
{
public:
    using Key = K;
    using Value = V;
    using Compare = Cmp;
    using Element = std::pair<Key, Value>;

    static constexpr auto Capacity = C;

    using const_iterator = std::array<Element, Capacity>::const_iterator;

    class OptionalValueRef
    {
    public:
        constexpr OptionalValueRef() = default;
        constexpr OptionalValueRef(Value& data) : data_{&data} {}

        constexpr bool valid() const noexcept
        {
            return static_cast<bool>(data_);
        }

        constexpr explicit operator bool() const noexcept
        {
            return valid();
        }

        constexpr Value* operator->() const noexcept
        {
            return data_;
        }

        constexpr Value& operator*() const noexcept
        {
            return *data_;
        }
    private:
        Value* data_ = nullptr;
    };

    class ConstOptionalValueRef
    {
    public:
        constexpr ConstOptionalValueRef() = default;
        constexpr ConstOptionalValueRef(const Value& data) : data_{&data} {}

        constexpr bool valid() const noexcept
        {
            return static_cast<bool>(data_);
        }

        constexpr explicit operator bool() const noexcept
        {
            return valid();
        }

        constexpr const Value* operator->() const noexcept
        {
            return data_;
        }

        constexpr const Value& operator*() const noexcept
        {
            return *data_;
        }
    private:
        const Value* data_ = nullptr;
    };

    constexpr ConstexprMap() {}

    template<std::forward_iterator Iterator>
    constexpr ConstexprMap(Iterator begin, Iterator end) noexcept
    {
        std::size_t inSize = std::distance(begin, end);
        size_ = std::min(inSize, Capacity);
        std::move(begin, begin + size_, std::begin(data_));

        auto compare = [cmp = Compare{}](const auto& elem0, const auto& elem1) {
            const auto& key0 = elem0.first;
            const auto& key1 = elem1.first;
            return cmp(key0, key1);
        };

        // use partial_sort because of gcc bug:
        // std::sort is not always constexpr
        std::partial_sort(std::begin(data_),
                          std::begin(data_) + size_,
                          std::begin(data_) + size_,
                          compare);
    }

    constexpr ConstOptionalValueRef lookup(Key key) const noexcept
    {
        auto keyCompare = Compare{};
        auto elemCompare = [&keyCompare](const auto& elem0, const auto& elem1) {
            const auto& key0 = elem0.first;
            const auto& key1 = elem1;//.first;
            return keyCompare(key0, key1);
        };

        const auto result = std::lower_bound(data_.begin(), data_.begin() + size_, key, elemCompare);
        if (result != data_.end() && !(keyCompare(key, result->first))) {
            return ConstOptionalValueRef(result->second);
        } else {
            return {};
        }
    }

    constexpr OptionalValueRef lookup(Key key) noexcept
    {
        auto keyCompare = Compare{};
        auto elemCompare = [&keyCompare](const auto& elem0, const auto& elem1) {
            const auto& key0 = elem0.first;
            const auto& key1 = elem1;//.first;
            return keyCompare(key0, key1);
        };

        const auto result = std::lower_bound(data_.begin(), data_.begin() + size_, key, elemCompare);
        if (result != data_.end() && !(keyCompare(key, result->first))) {
            return OptionalValueRef(result->second);
        } else {
            return {};
        }
    }

    constexpr std::size_t size() const noexcept { return size_; }

    constexpr const_iterator begin() const noexcept { return data_.cbegin(); }
    constexpr const_iterator end() const noexcept { return data_.cend(); }

private:
    std::array<Element, Capacity> data_;
    std::size_t size_ = 0;
};

template<typename K, typename V,
    std::size_t C, typename Cmp = std::less<>>
class ConstexprMapBuilder
{
public:
    using Map = ConstexprMap<K, V, C, Cmp>;
    constexpr ConstexprMapBuilder() noexcept = default;

    constexpr ConstexprMapBuilder& insert(K key, const V& value) noexcept
    {
        if(full()) { return *this; }
        data_[size_++] = std::pair<K, V>{key, value};
        return *this;
    }

    constexpr ConstexprMapBuilder& insert(K key, V&& value) noexcept
    {
        if(full()) { return *this; }
        data_[size_++] = std::pair<K, V>{key, std::move(value)};
        return *this;
    }

    constexpr std::size_t size() const noexcept { return size_; }
    constexpr bool full() const noexcept { return size_ == C; }

    constexpr Map buildMap() const noexcept
    {
        return Map{std::begin(data_), std::begin(data_) + size_};
    }

private:
    std::array<typename Map::Element, C> data_;
    std::size_t size_ = 0;
};

}

#endif // CANOPEN_UTILITY_CONSTEXPR_MAP
