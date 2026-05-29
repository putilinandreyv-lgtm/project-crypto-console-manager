#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

// Имитируем структуру портфеля внутри тестов для изоляции бизнес-логики
std::unordered_map<std::string, std::unordered_map<std::string, float>> test_portfolio = {
    {"Balance", {{"amount", 10000.0f}}},
    {"Moneti", {
        {"BTC", 0.0f}, {"ETH", 0.0f}, {"BNB", 0.0f}, {"SOL", 0.0f}, {"XRP", 0.0f}
    }}
};

// ============================================================================
// ТЕСТИРУЕМЫЕ ФУНКЦИИ (6 штук с чистой логикой из твоего ТЗ)
// ============================================================================

// 1. Валидация поддерживаемых тикеров
bool is_supported_ticker(std::string ticker) {
    std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);
    return (ticker == "BTC" || ticker == "ETH" || ticker == "BNB" || ticker == "SOL" || ticker == "XRP");
}

// 2. Расчет стоимости отдельного актива
double calculate_asset_value(double amount, double price) {
    if (amount <= 0 || price <= 0) return 0.0;
    return amount * price;
}

// 3. Валидация диапазона дней для графиков
bool is_valid_days_range(int days) {
    return (days >= 1 && days <= 30);
}

// 4. Логика изменения баланса (без cout/cin)
bool process_balance_change(float current_balance, int summa, float& out_balance) {
    if (current_balance + summa >= 0) {
        out_balance = current_balance + summa;
        return true;
    }
    out_balance = current_balance;
    return false;
}

// 5. Логика списания фиата и начисления монет при покупке
bool process_buy_asset(const std::string& coin, float quantity, double price, float& fiat_balance, float& coin_balance) {
    if (price <= 0 || quantity <= 0) return false;
    double total_cost = price * quantity;
    if (total_cost <= fiat_balance) {
        fiat_balance -= static_cast<float>(total_cost);
        coin_balance += quantity;
        return true;
    }
    return false;
}

// 6. Логика начисления фиата и списания монет при продаже
bool process_sell_asset(const std::string& coin, float quantity, double price, float& fiat_balance, float& coin_balance) {
    if (price <= 0 || quantity <= 0) return false;
    if (quantity <= coin_balance) {
        fiat_balance += static_cast<float>(price * quantity);
        coin_balance -= quantity;
        return true;
    }
    return false;
}

// ============================================================================
// БЛОК ТЕСТОВ (Покрывает И положительные, И отрицательные исходы для 3 баллов)
// ============================================================================

TEST_CASE("1. Функция валидации тикеров криптовалют") {
    SUBCASE("Положительные исходы (Верхний и нижний регистр)") {
        CHECK(is_supported_ticker("BTC") == true);
        CHECK(is_supported_ticker("sol") == true); 
        CHECK(is_supported_ticker("ETH") == true);
    }
    SUBCASE("Отрицательные исходы (Неподдерживаемые или пустые)") {
        CHECK(is_supported_ticker("INVALID") == false);
        CHECK(is_supported_ticker("AAPL") == false);
        CHECK(is_supported_ticker("") == false);
    }
}

TEST_CASE("2. Функция расчета стоимости актива") {
    SUBCASE("Положительные исходы (Корректный расчет стоимости)") {
        CHECK(calculate_asset_value(2.5, 100.0) == doctest::Approx(250.0));
        CHECK(calculate_asset_value(0.1, 50000.0) == doctest::Approx(5000.0));
    }
    SUBCASE("Отрицательные исходы (Невалидные параметры дают 0)") {
        CHECK(calculate_asset_value(-1.0, 100.0) == doctest::Approx(0.0));
        CHECK(calculate_asset_value(5.0, -10.0) == doctest::Approx(0.0));
        CHECK(calculate_asset_value(0.0, 150.0) == doctest::Approx(0.0));
    }
}

TEST_CASE("3. Функция валидации лимита дней (1-30)") {
    SUBCASE("Положительные исходы (Внутри допустимых границ)") {
        CHECK(is_valid_days_range(1) == true);
        CHECK(is_valid_days_range(15) == true);
        CHECK(is_valid_days_range(30) == true);
    }
    SUBCASE("Отрицательные исходы (Выход за границы)") {
        CHECK(is_valid_days_range(0) == false);
        CHECK(is_valid_days_range(31) == false);
        CHECK(is_valid_days_range(-5) == false);
    }
}

TEST_CASE("4. Модуль изменения фиатного баланса") {
    float current_fiat = 10000.0f;
    float updated_fiat = 0.0f;

    SUBCASE("Положительный исход: Успешное пополнение и списание") {
        CHECK(process_balance_change(current_fiat, 500, updated_fiat) == true);
        CHECK(updated_fiat == doctest::Approx(10500.0f));

        CHECK(process_balance_change(current_fiat, -5000, updated_fiat) == true);
        CHECK(updated_fiat == doctest::Approx(5000.0f));
    }
    SUBCASE("Отрицательный исход: Попытка списать больше, чем есть (баланс не меняется)") {
        CHECK(process_balance_change(current_fiat, -20000, updated_fiat) == false);
        CHECK(updated_fiat == doctest::Approx(10000.0f));
    }
}

TEST_CASE("5. Модуль логики покупки активов") {
    float fiat = 10000.0f;
    float btc_amount = 0.0f;

    SUBCASE("Положительный исход: Достаточно средств для покупки") {
        // Покупаем 0.1 BTC по цене 50000$ (Всего 5000$)
        CHECK(process_buy_asset("BTC", 0.1f, 50000.0, fiat, btc_amount) == true);
        CHECK(btc_amount == doctest::Approx(0.1f));
        CHECK(fiat == doctest::Approx(5000.0f));
    }
    SUBCASE("Отрицательный исход: Цена невалидна или баланса не хватает") {
        CHECK(process_buy_asset("BTC", 1.0f, 60000.0, fiat, btc_amount) == false); // Не хватит денег
        CHECK(process_buy_asset("BTC", 0.1f, -1000.0, fiat, btc_amount) == false); // Кривая цена
    }
}

TEST_CASE("6. Модуль логики продажи активов") {
    float fiat = 5000.0f;
    float btc_amount = 0.5f;

    SUBCASE("Положительный исход: Успешная продажа имеющегося объема") {
        // Продаем 0.2 BTC по цене 50000$ (Получаем 10000$)
        CHECK(process_sell_asset("BTC", 0.2f, 50000.0, fiat, btc_amount) == true);
        CHECK(btc_amount == doctest::Approx(0.3f));
        CHECK(fiat == doctest::Approx(15000.0f));
    }
    SUBCASE("Отрицательный исход: Попытка продать больше, чем есть на руках") {
        CHECK(process_sell_asset("BTC", 1.0f, 50000.0, fiat, btc_amount) == false);
        CHECK(btc_amount == doctest::Approx(0.5f)); // Объем не изменился
    }
}