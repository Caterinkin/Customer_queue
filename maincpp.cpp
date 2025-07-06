#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <clocale>

// ����� ��� ���������� �������� ��������
class ClientQueue
{
private:
    int client_count;          // ������� ���������� �������� � �������
    int max_clients;           // ������������ ���������� ���������� ��������
    std::mutex mtx;            // ������� ��� ������������� ������� � �������
    std::condition_variable cv; // �������� ���������� ��� �����������
    bool clients_finished;     // ���� ���������� ������ ��������
    bool operator_finished;    // ���� ���������� ������ �������������

public:
    // ����������� �������������� ��� ����������
    ClientQueue(int max) : client_count(0), max_clients(max),
        clients_finished(false), operator_finished(false) {
    }

    // ����� ���������� ������ �������
    void addClient()
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (clients_finished) return;  // ���� ������� �����������, �������

        if (client_count < max_clients)
        {
            client_count++;
            std::cout << "����� ������! ����� ��������: " << client_count << std::endl;
            cv.notify_one();  // ���������� ������������� � ����� �������
        }
        else
        {
            // �������� ��������� ��������
            clients_finished = true;
            cv.notify_one();  // ���������� ������������� � ����������
        }
    }

    // ����� ������������ �������
    void serveClient()
    {
        std::unique_lock<std::mutex> lock(mtx);
        // ���� ���� ��������� ��������, ���� ���������� ������
        cv.wait(lock, [this]() {
            return client_count > 0 || (clients_finished && client_count == 0);
            });

        if (client_count > 0)
        {
            // ����������� ������ �������
            client_count--;
            std::cout << "������ ��������! �������� ��������: " << client_count << std::endl;
        }
        else if (clients_finished)
        {
            // ��� ������� ���������, ����� ��������� ������
            operator_finished = true;
            cv.notify_one();  // ���������� ������� ����� � ����������
        }
    }

    // �������� ���������� ������ ����� �������
    bool isFinished() const
    {
        return clients_finished && operator_finished;
    }

    // ��������, ����� �� ��� ��������� ��������
    bool shouldAddClients() const
    {
        return !clients_finished && client_count < max_clients;
    }

    // ������ ��� ������������� ���������� ��������
    int getMaxClients() const { return max_clients; }
};

// ������� ������ ���������� ��������
void clientThread(ClientQueue& queue)
{
    // ��������� ��������, ���� �� ��������� ���������
    while (queue.shouldAddClients())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // �������� 1 ���
        queue.addClient();
    }
    std::cout << "���������� ������������ ���������� �������� (" << queue.getMaxClients()
        << "). ����� ������� �� �����������." << std::endl;
}

// ������� ������ �������������
void operatorThread(ClientQueue& queue)
{
    // ��������, ���� �� ��������� ��� �������
    while (!queue.isFinished())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // �������� 2 ���
        queue.serveClient();
    }
    std::cout << "��� ������� ���������. ������ ������������� ���������." << std::endl;
}

int main()
{
    // ������������� ������� ������ ��� ����������� ������ ���������
    std::setlocale(LC_ALL, "Russian");

    int max_clients;
    std::cout << "������� ������������ ���������� ��������: ";
    std::cin >> max_clients;

    // ������� ������� �������� � �������� ����������
    ClientQueue queue(max_clients);

    // ��������� ������ �������� � �������������
    std::thread client(clientThread, std::ref(queue));
    std::thread operator_(operatorThread, std::ref(queue));

    // ������� ���������� ����� �������
    client.join();
    operator_.join();

    return 0;
}