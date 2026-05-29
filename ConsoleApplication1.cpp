#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <any>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <fstream>

using json = nlohmann::json;

/**
 * @brief Функция-колбэк для записи входящих данных от cURL в строку.
 * @param contents Указатель на полученные данные.
 * @param size Размер одного элемента данных.
 * @param nmemb Количество элементов.
 * @param userp Указатель на целевую строку std::string, куда записывается ответ.
 * @return Количество обработанных байт.
 */
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

/**
 * @brief Выполняет сетевой HTTP GET запрос.
 * @param url Целевой URL-адрес для отправки запроса.
 * @return Ответ сервера в виде строки (обычно JSON). При ошибке возвращает пустую строку.
 */
std::string NetworkHttpGet(const std::string& url) {
	CURL* curl = curl_easy_init();
	std::string response;

	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Для тестов

		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "cURL Error: " << curl_easy_strerror(res) << std::endl;
		}
		curl_easy_cleanup(curl);
	}
	return response;
}

/**
 * @brief Получает актуальную цену актива с биржи Binance (для крипты) или Finnhub (для акций).
 * @param ticker Символьный код актива (например, "BTC", "AAPL"). Регистронезависимый.
 * @return Текущая цена актива в USD. Возвращает -1.0 в случае ошибки или если актив не поддерживается.
 */
double GetAssetPrice(std::string ticker) {
	// Приводим тикер к верхнему регистру (например, btc -> BTC)
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);

	static const std::unordered_map<std::string, std::string> cryptoMap = {
		{"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"}, {"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}
	};

	if (cryptoMap.find(ticker) != cryptoMap.end()) {
		std::string symbol = cryptoMap.at(ticker);
		std::string url = "https://api.binance.com/api/v3/ticker/price?symbol=" + symbol;
		std::string res = NetworkHttpGet(url);
		if (!res.empty()) {
			try {
				auto js = json::parse(res);
				return std::stod(js["price"].get<std::string>());
			}
			catch (...) {
				std::cerr << "Ошибка парсинга данных Binance для " << ticker << std::endl;
			}
		}
	}
	else {
		std::cerr << "Запрошенный актив '" << ticker << "' не поддерживается топ-5 списком." << std::endl;
	}

	return -1.0;
}

// Глобальная структура портфеля пользователя
std::unordered_map<std::string, std::unordered_map<std::string, float>> portfolio = {
	{"Balance", {{"amount", 10000.0f}}},
	{"Moneti", {
		{"BTC", 0.0f}, {"ETH", 0.0f}, {"BNB", 0.0f}, {"SOL", 0.0f}, {"XRP", 0.0f}
	}}
};

std::vector<std::string> history; // Вектор для хранения истории операций
std::string entry;               // Буферная строка для формирования записи истории

/**
 * @brief Получает текущее системное время на компьютере.
 * @return Строка с датой и временем в формате "ДД.ММ.ГГГГ ЧЧ:ММ:СС".
 */
std::string get_real_time() {
	time_t now = time(0);
	struct tm buf;
	char buffer[80];

	localtime_s(&buf, &now);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &buf);

	return std::string(buffer);
}

/**
 * @brief Выводит на экран всю историю финансовых операций пользователя.
 */
void print_history() {
	std::cout << "\n=== ИСТОРИЯ ОПЕРАЦИЙ ===\n";
	std::cout << "Всего записей: " << history.size() << "\n\n";

	for (const auto& entry : history) {
		std::cout << "• " << entry << "\n";
	}
	std::cout << "\n=== КОНЕЦ ИСТОРИИ ===\n";
}

/**
 * @brief Изменяет баланс свободных фиатных средств (USD) пользователя.
 * @param summa Сумма изменения (может быть отрицательной для списания средств).
 */
void balance_change(int summa) {
	if (portfolio["Balance"]["amount"] + summa >= 0) {
		portfolio["Balance"]["amount"] += summa;
		std::cout << "Баланс изменен на: " << summa << "$" << std::endl;
		std::cout << "Текущий баланс: " << portfolio["Balance"]["amount"] << "$" << std::endl << std::endl;
		entry = "Баланс изменен на " + std::to_string(summa) + "$  | " + get_real_time();
		history.push_back(entry);
		entry = "";
	}
	else {
		std::cout << "На балансе нет столько средств\n\n";
	}
}

/**
 * @brief Выводит состав текущего портфеля, баланс и общую ценность всех активов по рыночному курсу.
 */
void print_portf() {
	std::cout << "\n=== ТЕКУЩИЙ ПОРТФЕЛЬ ===\n";
	std::cout << "Баланс: " << portfolio["Balance"]["amount"] << "$\n\n";
	std::cout << "Активы на руках:\n";

	float total_cost = 0; // Локальная переменная (исправлен баг бесконечного суммирования)

	for (const auto& pair : portfolio["Moneti"]) {
		std::string coin = pair.first;
		float amount = pair.second;
		if (amount == 0) {
			continue;
		}
		else {
			double assetPrice = GetAssetPrice(coin);
			if (assetPrice < 0) assetPrice = 0; // Защита, если цена не получена

			total_cost += static_cast<float>(assetPrice * amount);
			std::cout << "  " << coin << ": " << amount << " шт.    " << assetPrice * amount << "$\n";
		}
	}
	total_cost += portfolio["Balance"]["amount"];
	std::cout << "\nОбщая стоимость портфеля: " << total_cost << "$\n";
	std::cout << "========================\n\n";
}

/**
 * @brief Покупка определенного количества акций или криптовалюты.
 * @param name Тикер покупаемого актива.
 * @param quantity Количество единиц к покупке.
 */
void buy_asset(std::string name, float quantity) {
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);
	try {
		float currentAmount = portfolio["Moneti"].at(name);
	}
	catch (const std::out_of_range&) {
		std::cout << "Ошибка: Актив '" << name << "' не поддерживается!\n\n";
		return;
	}

	double price = GetAssetPrice(name);

	if (price <= 0) {
		std::cout << "Не удалось получить цену актива для совершения сделки.\n\n";
		return;
	}

	if (price * quantity <= portfolio["Balance"]["amount"]) {
		portfolio["Balance"]["amount"] -= static_cast<float>(price * quantity);
		portfolio["Moneti"][name] += quantity;
		std::cout << "Приобретено " << quantity << " ед. " << name << " за " << price * quantity << "$" << std::endl;
		std::cout << "Текущий баланс: " << portfolio["Balance"]["amount"] << "$" << std::endl << std::endl;
		entry = "Покупка " + std::to_string(quantity) + " " + name + " за " + std::to_string(price * quantity) + "$" + " | " + get_real_time();
		history.push_back(entry);
		entry = "";
	}
	else {
		std::cout << "На балансе недостаточно средств\n\n";
	}
}

/**
 * @brief Продажа имеющегося актива из портфеля.
 * @param name Тикер продаваемого актива.
 * @param quantity Количество единиц к продаже.
 */
void sell_asset(std::string name, float quantity) {
	std::transform(name.begin(), name.end(), name.begin(), ::toupper);
	try {
		float currentAmount = portfolio["Moneti"].at(name);
	}
	catch (const std::out_of_range&) {
		std::cout << "Ошибка: Актив '" << name << "' не поддерживается!\n\n";
		return;
	}

	if (quantity <= portfolio["Moneti"][name]) {
		double price = GetAssetPrice(name);

		if (price <= 0) {
			std::cout << "Не удалось получить цену актива для совершения сделки.\n\n";
			return;
		}

		portfolio["Balance"]["amount"] += static_cast<float>(price * quantity);
		portfolio["Moneti"][name] -= quantity;
		std::cout << "Продано " << quantity << " ед. " << name << " за " << price * quantity << "$" << std::endl;
		std::cout << "Текущий баланс: " << portfolio["Balance"]["amount"] << "$" << std::endl << std::endl;
		entry = "Продажа " + std::to_string(quantity) + " " + name + " за " + std::to_string(price * quantity) + "$" + " | " + get_real_time();
		history.push_back(entry);
		entry = "";
	}
	else {
		std::cout << "В портфеле нет столько акций\n\n";
	}
}

/**
 * @brief Запрашивает массив исторических цен закрытия для криптовалют с Binance.
 * @param ticker Код криптовалюты (например, "BTC").
 * @param days Количество прошедших дней (размер запрашиваемой истории).
 * @param interval Таймфрейм баров (по умолчанию "1d" — 1 день).
 * @return Вектор с историческими ценами. Пустой вектор, если произошла ошибка.
 */
std::vector<double> GetHistoricalPrices(std::string ticker, int days, const std::string& interval = "1d") {
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);

	static const std::unordered_map<std::string, std::string> cryptoMap = {
		{"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"},
		{"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}
	};

	std::vector<double> prices;

	if (cryptoMap.find(ticker) != cryptoMap.end()) {
		std::string symbol = cryptoMap.at(ticker);
		std::string url = "https://api.binance.com/api/v3/klines?symbol=" + symbol
			+ "&interval=" + interval + "&limit=" + std::to_string(days);

		std::string res = NetworkHttpGet(url);

		if (!res.empty()) {
			try {
				auto js = json::parse(res);
				for (const auto& candle : js) {
					double closePrice = std::stod(candle[4].get<std::string>());
					prices.push_back(closePrice);
				}
			}
			catch (...) {
				std::cerr << "Ошибка парсинга исторических данных для " << ticker << std::endl;
			}
		}
	}
	return prices;
}

/**
 * @brief Изменяет цвет шрифта вывода в консоли Windows.
 * @param color Числовой код цвета консоли (0-15).
 */
void SetColor(int color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}

/**
 * @brief Отрисовывает красивый псевдографик (ASCII-art) истории изменения цены в консоли.
 * @param prices Константная ссылка на вектор цен.
 * @param ticker Отображаемый тикер актива на графике.
 */
void PrintChart(const std::vector<double>& prices, const std::string& ticker) {
	if (prices.empty()) {
		std::cout << "Нет данных\n";
		return;
	}

	double maxPrice = *std::max_element(prices.begin(), prices.end());
	double minPrice = *std::min_element(prices.begin(), prices.end());
	double range = maxPrice - minPrice;
	if (range == 0) range = 1;

	SetColor(14);
	std::cout << "\n========== " << ticker << " ==========\n\n";

	const int CHART_HEIGHT = 10;

	for (int row = CHART_HEIGHT; row >= 0; row--) {
		double priceLevel = minPrice + (range * row / CHART_HEIGHT);

		SetColor(7);
		std::cout << std::setw(8) << std::fixed << std::setprecision(0) << priceLevel << " $ |";

		for (size_t i = 0; i < prices.size(); i++) {
			int height = static_cast<int>((prices[i] - minPrice) / range * CHART_HEIGHT);

			if (height >= row) {
				SetColor(10); // Зеленый цвет для графика
				std::cout << "#";
			}
			else {
				std::cout << " ";
			}
		}
		std::cout << "|\n";
	}

	std::cout << "         +";
	for (size_t i = 0; i < prices.size(); i++) {
		std::cout << "-";
	}
	std::cout << "+\n";

	std::cout << "           ";

	int days_count = (int)prices.size();
	int step = days_count / 8;
	if (step < 1) step = 1;

	for (int i = 0; i < days_count; i += step) {
		std::string label = std::to_string(i + 1);
		int next_i = (std::min)(i + step, days_count);
		int width = next_i - i;
		int padding = width / 2;

		for (int p = 0; p < padding; p++) {
			std::cout << " ";
		}
		SetColor(11);
		std::cout << label;
		for (int p = 0; p < width - padding - (int)label.length(); p++) {
			std::cout << " ";
		}
	}

	SetColor(7);
	std::cout << "\n\n";
}

/**
 * @brief Сохраняет текущие данные портфеля (баланс и монеты) в текстовый файл.
 * @param filename Путь или имя файла сохранения.
 */
void SavePortfolioToFile(const std::string& filename) {
	std::ofstream file(filename);
	if (!file) {
		std::cerr << "Ошибка создания файла!\n";
		return;
	}

	file << portfolio["Balance"]["amount"] << "\n";

	for (const auto& pair : portfolio["Moneti"]) {
		file << pair.first << " " << pair.second << "\n";
	}

	file.close();
	std::cout << "Сохранено!\n\n";
}

/**
 * @brief Загружает сохраненные данные баланса и активов из файла в программу.
 * @param filename Путь или имя файла для чтения данных.
 */
void LoadPortfolioFromFile(const std::string& filename) {
	std::ifstream file(filename);
	if (!file) {
		std::cout << "Файл не найден\n";
		return;
	}

	float balance;
	file >> balance;
	portfolio["Balance"]["amount"] = balance;

	std::string coin;
	float amount;
	while (file >> coin >> amount) {
		if (portfolio["Moneti"].find(coin) != portfolio["Moneti"].end()) {
			portfolio["Moneti"][coin] = amount;
		}
	}

	file.close();
	std::cout << "Загружено! Баланс: " << portfolio["Balance"]["amount"] << "$\n\n";
}

/**
 * @brief Главная точка входа в программу. Реализует цикл бесконечного консольного меню.
 */
int main() {
	setlocale(LC_ALL, "RU");
	SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
	short xray = 0;

	// Цикл чтобы возвращаться в главное меню
	while (xray < 10000) {
		SetColor(14);
		xray++;
		std::cout << "ДОБРО ПОЖАЛОВАТЬ В КОНСОЛЬНЫЙ МЕНЕДЖЕР КРИПТОВАЛЮТЫ\n";
		std::cout << "1. Вывести текущий портфель\n";
		std::cout << "2. Проверить цену акции/крипты\n";
		std::cout << "3. Изменить баланс\n";
		std::cout << "4. Купить актив\n";
		std::cout << "5. Продать актив\n";
		std::cout << "6. Вывести историю действий\n";
		std::cout << "7. Вывести график\n";
		std::cout << "8. Сохранить портфель в файл\n";
		std::cout << "9. Загрузить портфель из файла\n";
		std::cout << "10. Выйти из программы\n\n";

		int summa;
		int a;
		float quantity;
		std::string name;
		std::string filename;
		std::cin >> a;

		switch (a) {
		case 1:
			print_portf();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 2:
			std::cout << "Введите название валюты/акции:\n";
			std::cin >> name;
			std::cout << "Цена на данный момент: " << GetAssetPrice(name) << "$" << std::endl << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 3:
			std::cout << "Введите сумму:\n";
			std::cin >> summa;
			balance_change(summa);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 4:
			std::cout << "Введите название актива для покупки:\n";
			std::cin >> name;
			std::cout << "Введите количество:\n";
			std::cin >> quantity;
			buy_asset(name, quantity);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 5:
			std::cout << "Введите название актива для продажи:\n";
			std::cin >> name;
			std::cout << "Введите количество:\n";
			std::cin >> quantity;
			sell_asset(name, quantity);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 6:
			print_history();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 7: {
			int days;
			SetColor(7);
			std::cout << "Введите тикер криптовалюты (BTC, ETH, BNB, SOL, XRP): ";
			std::cin >> name;

			std::cout << "Введите количество дней (5-30): ";
			std::cin >> days;

			if (days < 1 || days > 30) {
				SetColor(12);
				std::cout << "Ошибка: количество дней должно быть от 1 до 30!\n";
				SetColor(7);
				continue;
			}

			SetColor(14);
			std::cout << "\nЗагрузка данных";
			for (int i = 0; i < 3; i++) {
				std::cout << ".";
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			std::cout << "\n";

			std::vector<double> prices = GetHistoricalPrices(name, days);

			if (prices.empty()) {
				SetColor(12);
				std::cout << "Не удалось получить данные для " << name << "\n";
				SetColor(7);
				continue;
			}

			PrintChart(prices, name);

			SetColor(7);
			std::cout << "Нажмите Enter для продолжения...\n";
			std::cin.ignore();
			std::cin.get();
			continue;
		}

		case 8:
			std::cout << "Введите название файла для сохранения:\n";
			std::cin >> filename;
			SavePortfolioToFile(filename);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;

		case 9:
			std::cout << "Введите название файла для загрузки:\n";
			std::cin >> filename;
			LoadPortfolioFromFile(filename);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;

		case 10:
			xray = 10000;
			break;
		}
	}
	return 0;
}