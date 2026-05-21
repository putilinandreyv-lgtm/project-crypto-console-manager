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

// Коллбэк для записи ответа от cURL в строку
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
	size_t totalSize = size * nmemb;
	userp->append((char*)contents, totalSize);
	return totalSize;
}

// Вспомогательная функция для HTTP GET запросов
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


double GetAssetPrice(std::string ticker, const std::string& finnhubApiKey) {
	// Приводим тикер к верхнему регистру (например, btc -> BTC)
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);


	static const std::unordered_map<std::string, std::string> cryptoMap = {
		{"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"}, {"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}
	};
	static const std::unordered_map<std::string, std::string> stockMap = {
		{"AAPL", "AAPL"}, {"MSFT", "MSFT"}, {"GOOGL", "GOOGL"}, {"AMZN", "AMZN"}, {"NVDA", "NVDA"}
	};

	if (cryptoMap.find(ticker) != cryptoMap.end()) {
		std::string symbol = cryptoMap.at(ticker);
		std::string url = "https://api.binance.com/api/v3/ticker/price?symbol=" + symbol;
		std::string res = NetworkHttpGet(url);

		if (!res.empty()) {
			try {
				auto js = json::parse(res);
				// Binance возвращает цену как строку "65230.10", переводим в double
				return std::stod(js["price"].get<std::string>());
			}
			catch (...) {
				std::cerr << "Ошибка парсинга данных Binance для " << ticker << std::endl;
			}
		}
	}
	else if (stockMap.find(ticker) != stockMap.end()) {
		if (finnhubApiKey.empty() || finnhubApiKey == "YOUR_API_KEY") {
			std::cerr << "Ошибка: Для получения акций нужен корректный Finnhub API Key!" << std::endl;
			return -1.0;
		}

		std::string symbol = stockMap.at(ticker);
		std::string url = "https://finnhub.io/api/v1/quote?symbol=" + symbol + "&token=" + finnhubApiKey;
		std::string res = NetworkHttpGet(url);

		if (!res.empty()) {
			try {
				auto js = json::parse(res);
				// Поле "c" в Finnhub — это Current Price (текущая цена) в формате числа
				return js["c"].get<double>();
			}
			catch (...) {
				std::cerr << "Ошибка парсинга данных Finnhub для " << ticker << std::endl;
			}
		}
	}
	else {
		std::cerr << "Запрошенный актив '" << ticker << "' не поддерживается топ-5 списком." << std::endl;
	}

	return -1.0;
}





//Создаем портфолио
std::unordered_map<std::string, std::unordered_map<std::string, float>> portfolio = {
	{"Balance", {{"amount", 10000.0f}}},
	{"Moneti", {
		{"BTC", 0.0f},
		{"ETH", 0.0f},
		{"BNB", 0.0f},
		{"SOL", 0.0f},
		{"XRP", 0.0f},
		{"AAPL", 0.0f},
		{"MSFT", 0.0f},
		{"GOOGL", 0.0f},
		{"AMZN", 0.0f},
		{"NVDA", 0.0f},
	}}
};


std::vector<std::string> history;
std::string entry;

std::string get_real_time() {
	time_t now = time(0);
	struct tm buf;
	char buffer[80];

	localtime_s(&buf, &now);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &buf);

	return std::string(buffer);
}

void print_history() {

	std::cout << "\n=== ИСТОРИЯ ОПЕРАЦИЙ ===\n";
	std::cout << "Всего записей: " << history.size() << "\n\n";

	for (const auto& entry : history) {
		std::cout << "• " << entry << "\n";
	}

	std::cout << "\n=== КОНЕЦ ИСТОРИИ ===\n";
}




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


float total_cost = 0;

void print_portf() {
	std::cout << "\n=== ТЕКУЩИЙ ПОРТФЕЛЬ ===\n";
	std::cout << "Баланс: " << portfolio["Balance"]["amount"] << "$\n\n";
	std::cout << "Акции:\n";
	
	for (const auto& pair : portfolio["Moneti"]) {
		std::string coin = pair.first;
		float amount = pair.second;
		if (amount == 0) {
			continue;
		}
		else {
			total_cost += GetAssetPrice(coin, "APIKEY") * amount;
			std::cout << "  " << coin << ": " << amount << " шт.    " << GetAssetPrice(coin, "APIKEY") * amount << "$\n\n";
		}
	}
	total_cost += portfolio["Balance"]["amount"];
	std::cout << "Общая стоимость портфеля: " << total_cost << "$\n";

	std::cout << "========================\n\n";
}



void buy_asset(std::string name, float quantity) {
	try {
		float currentAmount = portfolio["Moneti"].at(name); // Выбросит исключение, если нет ключа
		// Если дошли сюда - монета существует
	}
	catch (const std::out_of_range&) {
		std::cout << "Ошибка: Актив '" << name << "' не поддерживается!\n\n";
		return;
	}

	if (GetAssetPrice(name, "APIKEY") * quantity <= portfolio["Balance"]["amount"]) {
		portfolio["Balance"]["amount"] -= GetAssetPrice(name, "APIKEY") * quantity;
		portfolio["Moneti"][name] += quantity;
		std::cout << "Приобретено " << quantity << " акций " << name << " за " << GetAssetPrice(name, "APIKEY") * quantity << "$" << std::endl;
		std::cout << "Текущий баланс: " << portfolio["Balance"]["amount"] << "$" << std::endl << std::endl;
		entry = "Покупка " + std::to_string(quantity) + " " + name + " за " + std::to_string(GetAssetPrice(name, "APIKEY") * quantity) + "$" + " | " + get_real_time();
		history.push_back(entry);
		entry = "";
	}
	else {
		std::cout << "На балансе недостаточно средств\n\n";
	}
}


void sell_asset(std::string name, float quantity) {

	try {
		float currentAmount = portfolio["Moneti"].at(name); // Выбросит исключение, если нет ключа
		// Если дошли сюда - монета существует
	}
	catch (const std::out_of_range&) {
		std::cout << "Ошибка: Актив '" << name << "' не поддерживается!\n\n";
		return;
	}

	if (quantity <= portfolio["Moneti"][name]) {
		portfolio["Balance"]["amount"] += GetAssetPrice(name, "APIKEY") * quantity;
		portfolio["Moneti"][name] -= quantity;
		std::cout << "Продано " << quantity << " акций " << name << " за " << GetAssetPrice(name, "APIKEY") * quantity << "$" << std::endl;
		std::cout << "Текущий баланс: " << portfolio["Balance"]["amount"] << "$" << std::endl << std::endl;
		entry = "Продажа " + std::to_string(quantity) + " " + name + " за " + std::to_string(GetAssetPrice(name, "APIKEY") * quantity) + "$" + " | " + get_real_time();
		history.push_back(entry);
		entry = "";
	}
	else {
		std::cout << "В портфеле нет столько акций\n\n";
	}

}


std::vector<double> GetHistoricalPrices(std::string ticker, int days, const std::string& interval = "1d") {
	std::transform(ticker.begin(), ticker.end(), ticker.begin(), ::toupper);

	static const std::unordered_map<std::string, std::string> cryptoMap = {
		{"BTC", "BTCUSDT"}, {"ETH", "ETHUSDT"}, {"BNB", "BNBUSDT"},
		{"SOL", "SOLUSDT"}, {"XRP", "XRPUSDT"}
	};

	std::vector<double> prices;

	if (cryptoMap.find(ticker) != cryptoMap.end()) {
		std::string symbol = cryptoMap.at(ticker);

		// Binance API для исторических свечей
		// limit = количество свечей (days)
		std::string url = "https://api.binance.com/api/v3/klines?symbol=" + symbol
			+ "&interval=" + interval + "&limit=" + std::to_string(days);

		std::string res = NetworkHttpGet(url);

		if (!res.empty()) {
			try {
				auto js = json::parse(res);
				for (const auto& candle : js) {
					// Цена закрытия (4-й элемент в свече) как double
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



void SetColor(int color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
}

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
				SetColor(10);  // ОДИН ЦВЕТ ДЛЯ ВСЕХ СТОЛБЦОВ (зеленый)
				std::cout << "#";
			}
			else {
				std::cout << " ";
			}
		}
		std::cout << "|\n";
	}

	// Нижняя граница
	SetColor(7);
	std::cout << "          +";
	for (size_t i = 0; i < prices.size(); i++) {
		std::cout << "-";
	}
	std::cout << "+\n";

	// Подписи дней
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



void SavePortfolioToFile(const std::string& filename) {
	std::ofstream file(filename);
	if (!file) {
		std::cerr << "Ошибка создания файла!\n";
		return;
	}

	// Сохраняем баланс
	file << portfolio["Balance"]["amount"] << "\n";

	// Сохраняем каждую монету
	for (const auto& pair : portfolio["Moneti"]) {
		file << pair.first << " " << pair.second << "\n";
	}

	file.close();
	std::cout << "Сохранено!\n\n";
}

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


int main() {
	setlocale(LC_ALL, "RU");
	short xray = 0;

	double price = GetAssetPrice("btc", "API_key");
	//Цикл чтобы возвращаться в главное меню
	while (xray < 10000) {
		SetColor(14);
		xray++;
		std::cout << "ДОБРО ПОЖАЛОВАТЬ В КОНСОЛЬНЫЙ МЕНЕДЖЕР КРИПТОВАЛЮТЫ\n";
		std::cout << "1. Вывести текущий портфель\n";
		std::cout << "2. Проверить цену акции\n";
		std::cout << "3. Изменить баланс\n";
		std::cout << "4. Купить акцию\n";
		std::cout << "5. Продать акцию\n";
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
		case 2:

			std::cout << "Введите название валюты\n";
			std::cin >> name;
			std::cout << "Цена валюты на данный момент: " << GetAssetPrice(name, "APIKEY") << "$" << std::endl << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 1:
			print_portf();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;
		case 3:
			std::cout << "Введите сумму\n";
			std::cin >> summa;
			balance_change(summa);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 4:
			std::cout << "Введите название акции для покупки\n";
			std::cin >> name;
			std::cout << "Введите количество акций\n";
			std::cin >> quantity;
			buy_asset(name, quantity);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 5:
			std::cout << "Введите название акции для продажи\n";
			std::cin >> name;
			std::cout << "Введите количество акций\n";
			std::cin >> quantity;
			sell_asset(name, quantity);
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		case 6:
			print_history();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			continue;

		
		case 7: {
			std::string name;
			int days;

			SetColor(7);
			std::cout << "Введите тикер (BTC, ETH, AAPL, MSFT, NVDA): ";
			std::cin >> name;

			std::cout << "Введите количество дней (5-30): ";
			std::cin >> days;

			if (days < 1 || days > 30) {
				SetColor(12);
				std::cout << "Ошибка: количество дней должно быть от 1 до 30!\n";
				SetColor(7);
				continue;
			}

			// Загрузка
			SetColor(14);
			std::cout << "\nЗагрузка данных";
			for (int i = 0; i < 3; i++) {
				std::cout << ".";
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}
			std::cout << "\n";

			// Получаем цены
			std::vector<double> prices = GetHistoricalPrices(name, days);

			if (prices.empty()) {
				SetColor(12);
				std::cout << "Не удалось получить данные для " << name << "\n";
				SetColor(7);
				continue;
			}

			// Показываем цветной график
			PrintChart(prices, name);

			SetColor(7);
			std::cout << "Нажмите Enter для продолжения...\n";
			std::cin.ignore();
			std::cin.get();
			continue;
		

		}
		
		case 8:
			std::cout << "Введите название файла\n";
			std::cin >> filename;
			SavePortfolioToFile(filename);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;

		case 9:
			std::cout << "Введите название файла\n";
			std::cin >> filename;
			LoadPortfolioFromFile(filename);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;

		case 10:
			xray = 10000;
			break;
	}
	return 0;
}}