#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>
#include <variant>

class ini_parser {
public:

	ini_parser(const std::string& file_name) {
		if (std::filesystem::exists(file_name)) {
			file_.open(file_name, std::ios::in);
			if (!file_.is_open()) {
				std::cerr << "Ошибка открытия файла " << file_name << ".\n";
				throw std::runtime_error("Ошибка открытия файла " + file_name + ".\n");
			}
			else {
				parse_ini_file();
			}
		}
		else {
			std::cerr << "Файл " << file_name << " не найден.\n";
			throw std::runtime_error("Файл " + file_name + " не найден.\n");
		}
	}
	//Заперщаем гененрирование конструктора по-умолчанию, конструктора копирования и operator=
	ini_parser() = delete;
	ini_parser& operator= (const ini_parser& src) = delete;
	ini_parser(const ini_parser& src) = delete;

	template<typename T>
	T get_value(std::string request) {
		std::transform(std::begin(request), std::end(request), std::begin(request), ::tolower);
		if (values.count(request) > 0) {
			return std::get<T>(values[request]);
		}
		else {
			std::cerr << "Ключ " << request << " не найден.\n";
			throw std::runtime_error("Ключ " + request + " не найден.\n");
		}
	}

private:
	std::ifstream file_;
	std::map <std::string, std::variant<int, double, std::string>> values;
	
	std::string& trim(std::string& src) const {
		const char* removed_cymbols = " \t\n\r\f\v";
		src.erase(src.find_last_not_of(removed_cymbols) + 1);
		src.erase(0, src.find_first_not_of(removed_cymbols));
		return src;
	}

	std::pair<bool, bool> get_section(const std::string src, std::string& section) const {
		//Если в исходной строке указано наименование секции, то оно записывается в section, иначе оставляем section без изменения
		//Возвращаемые значения: первое значение пары true - в исходной строке находится наименование секции
		//false - в исходной строке нет наименования секции
		//Второе значение пары true - ошибка в исходной строке, fasle - ошибок нет

		if (src[0] == '[') {
			if (src[src.length() - 1] == ']') {
				std::string tmp = src.substr(1, src.length() - 2);
				tmp = trim(tmp);
				if (tmp.length() > 0) {
					std::transform(std::begin(tmp), std::end(tmp), std::begin(tmp), ::tolower);
					section = tmp;
					return std::make_pair(true, false);
				}
				else {
					return std::make_pair(true, true);
				}
			}
			else {
				return std::make_pair(true, true);
			}
		}
		else {
			return std::make_pair(false, false);
		}
	}

	bool get_key_name(const std::string src, std::string& key_name) const {
		//Если в считанной строке найдено корректное наименование ключа, то оно записывается в key_name, иначе оставляем key_name без изменений
		//возвращаемые значения: true - ошибок нет, false - есть ошибки в исходной строке
		size_t ravno_index = src.find("=");
		if (ravno_index > 0) {
			std::string tmp = src.substr(0, ravno_index);
			tmp = trim(tmp);
			if (tmp.length() > 0) {
				std::transform(std::begin(tmp), std::end(tmp), std::begin(tmp), ::tolower);
				key_name = tmp;
				return true;
			}
		}
		return false;
	}

	bool is_double(const std::string& src) const noexcept{
		if (!src.empty() && src.find_first_not_of("+-0123456789.") == std::string::npos) {
			try {
				double tmp = std::stod(src);
			}
			catch (...) {
				return false;
			}
			return true;
		}
		return false;
	}

	bool is_int(const std::string& src) const noexcept{
		if (!src.empty() && src.find_first_not_of("+-0123456789") == std::string::npos) {
			try {
				int tmp_int = std::stoi(src);
			}
			catch (...) {
				return false;
			}
			return true;
		}
		return false;
	}

	void parse_ini_file() {
		std::string error_string;
		int line_num = 1;
		std::string src_line, section, key_name, value;
		while (!file_.eof() && getline(file_, src_line)) {
			//Убираем комментарии
			size_t comment_index = src_line.find(";");
			if (comment_index >= 0) {
				src_line = src_line.substr(0, comment_index);
			}
			src_line = trim(src_line);
			if (src_line.length() > 0) {
				//Секция
				auto result = get_section(src_line, section);
				if (result.second == true) {
					section = "";
					error_string += "В строке " + std::to_string(line_num) + " указано некорректное наименование секции \n";
				}
				if(result.first == false){
					//Ключ
					if (get_key_name(src_line, key_name)) {
						size_t ravno_index = src_line.find("=");
						value = src_line.substr(ravno_index + 1, src_line.length() - ravno_index);
						value = trim(value);
						if (value.length() > 0) {
							if (!section.empty()) {
								if (is_int(value)) {
									values[section + "." + key_name] = std::stoi(value);
								}
								else if (is_double(value)) {
									values[section + "." + key_name] = std::stod(value);
								}
								else {
									values[section + "." + key_name] = value;
								}
							}
							else {
								error_string += "В строке " + std::to_string(line_num) + " указан ключ " + key_name + ", но не указана секция для него \n";
							}
						}
						else {
							error_string += "В строке " + std::to_string(line_num) + " не указано значение ключа " + key_name + " секции " + section + "\n";
						}
					}
					else {
						error_string += "В строке " + std::to_string(line_num) + " указана некорректная строка ключ=значение\n";
					}
				}
			}
			++line_num;
		}
		file_.close();
		if (error_string != "") {
			std::cerr << error_string;
			throw std::runtime_error(error_string);
		}
	}
};

int main()
{
	setlocale(LC_ALL, "Russian");

	ini_parser parser("ini_file.ini");
	auto string_value = parser.get_value<std::string>("section1.var2");
	std::cout << string_value << std::endl;
	string_value = parser.get_value<std::string>("section2.var2");
	std::cout << string_value << std::endl;
	string_value = parser.get_value<std::string>("section1.var3");
	std::cout << string_value << std::endl;
}
