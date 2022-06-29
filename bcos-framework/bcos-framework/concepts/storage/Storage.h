#pragma once

#include <bcos-framework/storage/Entry.h>
#include <ranges>
#include <type_traits>

namespace bcos::concepts::storage
{

template <class Impl>
class StorageBase
{
public:
    std::optional<bcos::storage::Entry> getRow(std::string_view table, std::string_view key)
    {
        return impl().impl_getRow(table, key);
    }

    auto getRows(std::string_view table, std::ranges::range auto&& keys)
    {
        return impl().impl_getRows(table, std::forward<decltype(keys)>(keys));
    }

    void setRow(std::string_view table, std::string_view key, bcos::storage::Entry entry)
    {
        impl().impl_setRow(table, key, std::move(entry));
    }

    void createTable(std::string tableName) { impl().impl_createTable(std::move(tableName)); }

private:
    friend Impl;
    StorageBase() = default;
    auto& impl() { return static_cast<Impl&>(*this); }
};

template <class Impl>
concept Storage = std::derived_from<Impl, StorageBase<Impl>>;

}  // namespace bcos::concepts::storage