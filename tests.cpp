#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>

extern std::unordered_map<std::string, std::unordered_map<std::string, float>> portfolio;
extern std::vector<std::string> history;

void LoadPortfolioFromFile(const std::string& filename);
void balance_change(int summa);
double GetAssetPrice(std::string ticker);
void buy_asset(std::string name, float quantity);
void sell_asset(std::string name, float quantity);
std::vector<double> GetHistoricalPrices(std::string ticker, int days, const std::string& interval = "1d");

void ResetPortfolio() {
    portfolio["Balance"]["amount"] = 10000.0f;
    portfolio["Moneti"] = {
        {"BTC", 0.0f}, {"ETH", 0.0f}, {"BNB", 0.0f}, {"SOL", 0.0f}, {"XRP", 0.0f}
    };
    history.clear();
}

TEST_SUITE("Crypto Portfolio Manager Tests") {

    TEST_CASE("1. Testing LoadPortfolioFromFile") {
        ResetPortfolio();
        std::string test_filename = "test_portfolio.txt";

        std::ofstream test_file(test_filename);
        test_file << "5500.50\n";
        test_file << "BTC 1.25\n";
        test_file << "ETH 4.0\n";
        test_file.close();

        LoadPortfolioFromFile(test_filename);
        CHECK(portfolio["Balance"]["amount"] == doctest::Approx(5500.50f));
        CHECK(portfolio["Moneti"]["BTC"] == doctest::Approx(1.25f));
        CHECK(portfolio["Moneti"]["ETH"] == doctest::Approx(4.0f));

        ResetPortfolio();
        LoadPortfolioFromFile("missing_file.txt");
        CHECK(portfolio["Balance"]["amount"] == 10000.0f);

        std::remove(test_filename.c_str());
    }

    TEST_CASE("2. Testing balance_change") {
        ResetPortfolio();

        balance_change(1500);
        CHECK(portfolio["Balance"]["amount"] == 11500.0f);
        
        balance_change(-50000); 
        CHECK(portfolio["Balance"]["amount"] == 11500.0f); 

        balance_change(0); 
        CHECK(portfolio["Balance"]["amount"] == 11500.0f); 
    }

    // 3. ТЕСТ: GetAssetPrice
    TEST_CASE("3. Testing GetAssetPrice") {
        double price_upper = GetAssetPrice("BTC");
        double price_lower = GetAssetPrice("btc");
        
        if (price_upper > 0.0) {
            CHECK(price_upper > 0.0);
            CHECK(price_lower > 0.0);
            CHECK(price_upper == doctest::Approx(price_lower).epsilon(0.01));
        }

        CHECK(GetAssetPrice("INVALID") == -1.0);
    }

    // 4. ТЕСТ: buy_asset
    TEST_CASE("4. Testing buy_asset") {
        ResetPortfolio();
        double btc_price = GetAssetPrice("BTC");

        if (btc_price > 0.0) {
            buy_asset("BTC", 0.1f);
            CHECK(portfolio["Moneti"]["BTC"] == 0.1f);
        }

        ResetPortfolio();
        buy_asset("BTC", 500.0f); 
        CHECK(portfolio["Moneti"]["BTC"] == 0.0f);
        CHECK(portfolio["Balance"]["amount"] == 10000.0f);

        ResetPortfolio();
        buy_asset("BTC", -1.0f); 
        CHECK(portfolio["Moneti"]["BTC"] == 0.0f);
        CHECK(portfolio["Balance"]["amount"] == 10000.0f);

        ResetPortfolio();
        buy_asset("fdsfsd", -1.0f); 
        CHECK(portfolio["Moneti"]["fdsfsd"] == 0.0f);
        CHECK(portfolio["Balance"]["amount"] == 10000.0f);
    }

    TEST_CASE("5. Testing sell_asset") {
        ResetPortfolio();
        portfolio["Moneti"]["ETH"] = 2.0f; 
        double eth_price = GetAssetPrice("ETH");

        if (eth_price > 0.0) {
            sell_asset("ETH", 1.0f);
            CHECK(portfolio["Moneti"]["ETH"] == 1.0f);
        }

        ResetPortfolio();
        portfolio["Moneti"]["ETH"] = 0.5f;
        sell_asset("ETH", 10.0f);
        CHECK(portfolio["Moneti"]["ETH"] == 0.5f);

        ResetPortfolio();
        portfolio["Moneti"]["ETH"] = 0.5f;
        sell_asset("ETH", -1.0f);
        CHECK(portfolio["Moneti"]["ETH"] == 0.5f);
    }

    TEST_CASE("6. Testing GetHistoricalPrices") {
        std::vector<double> prices = GetHistoricalPrices("BTC", 5);
        
        if (!prices.empty()) {
            CHECK(prices.size() == static_cast<size_t>(5));
        }

        std::vector<double> empty_prices = GetHistoricalPrices("FAKE", 5);
        CHECK(empty_prices.empty() == true);

        std::vector<double> emptyy = GetHistoricalPrices("BTC", 0);
        CHECK(emptyy.empty() == true);
    }
}