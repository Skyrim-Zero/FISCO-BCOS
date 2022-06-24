#include <boost/throw_exception.hpp>
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "bcos-ledger/bcos-ledger/LedgerImpl.h"
#include "impl/TarsSerializable.h"
#include <bcos-framework/storage/Entry.h>
#include <bcos-tars-protocol/tars/Block.h>
#include <boost/test/unit_test.hpp>
#include <optional>
#include <ranges>

using namespace bcos::ledger;

struct MockMemoryStorage
{
    std::optional<bcos::storage::Entry> getRow(
        [[maybe_unused]] std::string_view table, [[maybe_unused]] std::string_view key)
    {
        auto entryIt = data.find(std::tuple{table, key});
        if (entryIt != data.end()) { return entryIt->second; }
        return {};
    }

    std::vector<std::optional<bcos::storage::Entry>> getRows(
        [[maybe_unused]] std::string_view table, [[maybe_unused]] std::ranges::range auto const& keys)
    {
        std::vector<std::optional<bcos::storage::Entry>> output;
        output.reserve(std::size(keys));
        for (auto&& it : keys)
        {
            output.emplace_back(getRow(table, it));
        }
        return output;
    }

    void setRow([[maybe_unused]] std::string_view table, [[maybe_unused]] std::string_view key,
        [[maybe_unused]] bcos::storage::Entry entry)
    {}

    void createTable([[maybe_unused]] std::string_view tableName) {}

    std::map<std::tuple<std::string, std::string>, bcos::storage::Entry, std::less<>>& data;
};

struct LedgerImplFixture
{
    LedgerImplFixture() : storage{.data = data}
    {
        // Put some entry into data
        bcostars::BlockHeader header;
        header.data.blockNumber = 10086;
        header.data.gasUsed = "1000";
        header.data.timestamp = 5000;

        bcos::storage::Entry headerEntry;
        auto buffer = bcos::concepts::serialize::encode(header);
        headerEntry.setField(0, std::move(buffer));
        data.emplace(std::tuple{SYS_NUMBER_2_BLOCK_HEADER, "10086"}, std::move(headerEntry));

        bcostars::Block transactionsBlock;
        transactionsBlock.transactionsMetaData.resize(count);
        for (auto i = 0u; i < count; ++i)
        {
            std::string hashStr = "hash_" + boost::lexical_cast<std::string>(i);
            transactionsBlock.transactionsMetaData[i].hash.assign(hashStr.begin(), hashStr.end());

            // transaction
            decltype(transactionsBlock.transactions)::value_type transaction;
            transaction.data.chainID = "chain";
            transaction.data.groupID = "group";
            transaction.importTime = 1000;

            auto txBuffer = bcos::concepts::serialize::encode(transaction);
            bcos::storage::Entry txEntry;
            txEntry.setField(0, std::move(txBuffer));

            data.emplace(std::tuple{SYS_HASH_2_TX, hashStr}, std::move(txEntry));

            // receipt
            decltype(transactionsBlock.receipts)::value_type receipt;
            receipt.data.blockNumber = 10086;
            receipt.data.contractAddress = "contract";

            auto receiptBuffer = bcos::concepts::serialize::encode(receipt);
            bcos::storage::Entry receiptEntry;
            receiptEntry.setField(0, std::move(receiptBuffer));
            data.emplace(std::tuple{SYS_HASH_2_RECEIPT, hashStr}, std::move(receiptEntry));
        }

        auto txsBuffer = bcos::concepts::serialize::encode(transactionsBlock);
        bcos::storage::Entry txsEntry;
        txsEntry.setField(0, std::move(txsBuffer));
        data.emplace(std::tuple{SYS_NUMBER_2_TXS, "10086"}, std::move(txsEntry));
    }

    std::map<std::tuple<std::string, std::string>, bcos::storage::Entry, std::less<>> data;
    MockMemoryStorage storage;

    constexpr static size_t count = 100;
};

BOOST_FIXTURE_TEST_SUITE(LedgerImplTest, LedgerImplFixture)

BOOST_AUTO_TEST_CASE(getBlock)
{
    LedgerImpl<MockMemoryStorage, bcostars::Block> ledger{storage};

    auto [header, transactions, receipts] = ledger.getBlock<BLOCK_HEADER, BLOCK_TRANSACTIONS, BLOCK_RECEIPTS>(10086);
    BOOST_CHECK_EQUAL(header.data.blockNumber, 10086);
    BOOST_CHECK_EQUAL(header.data.gasUsed, "1000");
    BOOST_CHECK_EQUAL(header.data.timestamp, 5000);

    BOOST_CHECK_EQUAL(transactions.size(), count);
    BOOST_CHECK_EQUAL(receipts.size(), count);

    for (auto i = 0u; i < count; ++i)
    {
        BOOST_CHECK_EQUAL(transactions[i].data.chainID, "chain");
        BOOST_CHECK_EQUAL(transactions[i].data.groupID, "group");
        BOOST_CHECK_EQUAL(transactions[i].importTime, 1000);

        BOOST_CHECK_EQUAL(receipts[i].data.blockNumber, 10086);
        BOOST_CHECK_EQUAL(receipts[i].data.contractAddress, "contract");
    }

    auto [block] = ledger.getBlock<BLOCK>(10086);
    BOOST_CHECK_EQUAL(block.blockHeader.data.blockNumber, 10086);
    BOOST_CHECK_EQUAL(block.blockHeader.data.gasUsed, "1000");
    BOOST_CHECK_EQUAL(block.blockHeader.data.timestamp, 5000);

    BOOST_CHECK_EQUAL(block.transactions.size(), count);
    BOOST_CHECK_EQUAL(block.receipts.size(), count);

    for (auto i = 0u; i < count; ++i)
    {
        BOOST_CHECK_EQUAL(block.transactions[i].data.chainID, "chain");
        BOOST_CHECK_EQUAL(block.transactions[i].data.groupID, "group");
        BOOST_CHECK_EQUAL(block.transactions[i].importTime, 1000);

        BOOST_CHECK_EQUAL(block.receipts[i].data.blockNumber, 10086);
        BOOST_CHECK_EQUAL(block.receipts[i].data.contractAddress, "contract");
    }

    BOOST_CHECK_THROW(ledger.getBlock<BLOCK_HEADER>(10087), std::runtime_error);
    BOOST_CHECK_THROW(ledger.getBlock<BLOCK_TRANSACTIONS>(10087), std::runtime_error);
    BOOST_CHECK_THROW(ledger.getBlock<BLOCK_RECEIPTS>(10087), std::runtime_error);
    BOOST_CHECK_THROW(ledger.getBlock<BLOCK>(10087), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()