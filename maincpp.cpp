#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <clocale>

// Класс для управления очередью клиентов
class ClientQueue
{
private:
    int client_count;          // Текущее количество клиентов в очереди
    int max_clients;           // Максимальное допустимое количество клиентов
    std::mutex mtx;            // Мьютекс для синхронизации доступа к очереди
    std::condition_variable cv; // Условная переменная для уведомлений
    bool clients_finished;     // Флаг завершения потока клиентов
    bool operator_finished;    // Флаг завершения потока операциониста

public:
    // Конструктор инициализирует все переменные
    ClientQueue(int max) : client_count(0), max_clients(max),
        clients_finished(false), operator_finished(false) {
    }

    // Метод добавления нового клиента
    void addClient()
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (clients_finished) return;  // Если клиенты закончились, выходим

        if (client_count < max_clients)
        {
            client_count++;
            std::cout << "Новый клиент! Всего клиентов: " << client_count << std::endl;
            cv.notify_one();  // Уведомляем операциониста о новом клиенте
        }
        else
        {
            // Достигли максимума клиентов
            clients_finished = true;
            cv.notify_one();  // Уведомляем операциониста о завершении
        }
    }

    // Метод обслуживания клиента
    void serveClient()
    {
        std::unique_lock<std::mutex> lock(mtx);
        // Ждем либо появления клиентов, либо завершения работы
        cv.wait(lock, [this]() {
            return client_count > 0 || (clients_finished && client_count == 0);
            });

        if (client_count > 0)
        {
            // Обслуживаем одного клиента
            client_count--;
            std::cout << "Клиент обслужен! Осталось клиентов: " << client_count << std::endl;
        }
        else if (clients_finished)
        {
            // Все клиенты обслужены, можно завершать работу
            operator_finished = true;
            cv.notify_one();  // Уведомляем главный поток о завершении
        }
    }

    // Проверка завершения работы обоих потоков
    bool isFinished() const
    {
        return clients_finished && operator_finished;
    }

    // Проверка, нужно ли еще добавлять клиентов
    bool shouldAddClients() const
    {
        return !clients_finished && client_count < max_clients;
    }

    // Геттер для максимального количества клиентов
    int getMaxClients() const { return max_clients; }
};

// Функция потока добавления клиентов
void clientThread(ClientQueue& queue)
{
    // Добавляем клиентов, пока не достигнем максимума
    while (queue.shouldAddClients())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Задержка 1 сек
        queue.addClient();
    }
    std::cout << "Достигнуто максимальное количество клиентов (" << queue.getMaxClients()
        << "). Новые клиенты не принимаются." << std::endl;
}

// Функция потока операциониста
void operatorThread(ClientQueue& queue)
{
    // Работаем, пока не обслужены все клиенты
    while (!queue.isFinished())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // Задержка 2 сек
        queue.serveClient();
    }
    std::cout << "Все клиенты обслужены. Работа операциониста завершена." << std::endl;
}

int main()
{
    // Устанавливаем русскую локаль для корректного вывода сообщений
    std::setlocale(LC_ALL, "Russian");

    int max_clients;
    std::cout << "Введите максимальное количество клиентов: ";
    std::cin >> max_clients;

    // Создаем очередь клиентов с заданным максимумом
    ClientQueue queue(max_clients);

    // Запускаем потоки клиентов и операциониста
    std::thread client(clientThread, std::ref(queue));
    std::thread operator_(operatorThread, std::ref(queue));

    // Ожидаем завершения обоих потоков
    client.join();
    operator_.join();

    return 0;
}