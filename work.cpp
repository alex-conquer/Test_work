#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <regex>



std::vector<std::string> Idname = {"db", "g1", "g2", "o1", "o2", "polygon_1", "polygon_2", "polygon_3"};
std::string fileName = "/home/alex/prg/projects/Model_3_ver164.flt";
std::ifstream file;
std::vector<char> buffer;
std::mutex consoleMutex; // Для синхронизации вывода


size_t openFile(std::string filePath = fileName)
{
    file.open(filePath, std::ios::binary);
    if(!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл!" << std::endl;
        return 0;
    }
    buffer = std::vector<char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    std::cout << "Считано " << buffer.size() << " байт." << std::endl;

    file.close();
    return buffer.size();
}

void readingFile(const std::vector<char>& buffer, size_t begin, size_t end)
{
    for (size_t i = begin; i < end;) {
        // Проверка на границы буфера
        if (i + 8 > buffer.size()) {
            break;
        }


        // Считываем ID (7 символов + нулевой байт)
        std::string id(buffer.begin() + i + 4, buffer.begin() + i + 11);
        id = id.substr(0, id.find('\0'));

        // Если ID соответствует одному из искомых
        if (std::find(Idname.begin(), Idname.end(), id) != Idname.end()) {
            // Считываем информацию о материале, если это полигон
            if (id.rfind("poly_", 0) == 0) { // Если начинается с "poly_"
                int16_t colorNameIndex = *(reinterpret_cast<const int16_t*>(&buffer[i + 20]));
                int16_t materialIndex = *(reinterpret_cast<const int16_t*>(&buffer[i + 30]));

                // Вывод информации в консоль
                std::lock_guard<std::mutex> lock(consoleMutex);
                std::cout << "Элемент: " << id
                          << ", Индекс материала: " << materialIndex
                          << ", Индекс цвета: " << colorNameIndex << std::endl;
            } else {
                // Для других элементов, просто вывести их ID
                std::lock_guard<std::mutex> lock(consoleMutex);
                std::cout << "Элемент: " << id << std::endl;
            }
        }

        // Переход к следующей записи
        i += 80;
    }
}


void thredingWork()
{
    std::vector<std::thread> threads;
    size_t numThreads = std::thread::hardware_concurrency(); // Оптимальное число потоков

    size_t fileSize = openFile();
    if (fileSize == 0) {
        return; // Если файл не удалось открыть, выходим
    }

    size_t section = (fileSize + numThreads - 1) / numThreads; // Размер участка для каждого потока

    for (int i = 0; i < numThreads; i++) {
        size_t start = i * section;
        size_t end = std::min(start + section, fileSize);
        threads.emplace_back(readingFile, std::ref(buffer), start, end);
    }
    
    for (auto& t : threads) {
        if(t.joinable()) {
            t.join();
        }
    }

}

int main()
{
   //thredingWork();
    openFile();
    readingFile(buffer,0,buffer.size());
    return 0;
}