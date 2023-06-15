#include <iostream>
#include <cmath>
#include <unordered_map>

#include "CELLTimeEvent.hpp"
#include "CELLTimeEventPool.hpp"

int main() {
    // ʹ�ö�ʱ����������Ҫ����һ��EventPool���͵ı�������ʼ��ʱ���ܵĲ���Ϊ֧��ͬʱ�洢δ�����Ķ�ʱ�����������
    EventPool::EventPool pool(102400);
    // ����start����������̨�����߳�
    pool.Start();
    // ���岢��ʼ��һ��TimeEvent��ʱ�������õ��ﴥ��ʱ���Ļص�����������exitΪfalse���ʾ��ʱ����������Ϊtrue��ʾ����pool��������Stop����
    EventPool::TimeEvent time_event(std::chrono::seconds(2));
    time_event.SetCallBack([](bool exit) -> bool {
        if (exit == true) {
            std::cout << "pool exit" << std::endl;
            return false;
        }
        std::cout << "hello world" << std::endl;
        return true;
        });

    std::unordered_map<int, size_t> count;

    for (int i = 0; i < 20; ++i) {
        int s = rand() % 20 + 1;
        auto start_tm = std::chrono::system_clock::now();
        EventPool::TimeEvent time_eventi{ std::chrono::milliseconds(s * 1000) };
        time_eventi.SetCallBack([start_tm, i, s, &count](bool exit) -> bool {
            if (exit == true) {
                std::cout << "pool exit, event " << i << " get the notify" << std::endl;
                return false;
            }
            auto expire_tm = std::chrono::system_clock::now();
            int uses = std::chrono::duration_cast<std::chrono::milliseconds>(expire_tm - start_tm).count();
            count[i] += 1;
            double delta = uses / ((double)s * 1000) - count[i];
            if (std::abs(delta) >= 1e-2) {
                std::cerr << "time event " << i << " expired error, " << uses << " " << s << " " << count[i] << std::endl;
            }
            else {
                std::cout << "time event " << i << " expired succ" << std::endl;
            }
            return true;
            });
        pool.PushTimeEvent(std::move(time_eventi));
    }
    // TimeEvent�ĳ�ʼ��������ģʽ����һ��������ʾ������һ��std::chrono::duration�����������Եľ���һ��ʱ��󴥷���
    // ����ģʽ�»ص������ķ���ֵ�������壺�������Ϊtrue�����´���ʱ�䣨����ʱ��ʱ��+��ʼ��ʱ��duration�����ٴμ���ȴ�������
    // �Ӷ����Է���������
    // �ڶ���ģʽ������ʾ������һ��std::chrono::time_point��������ĳ��ʱ��㴥��һ�Σ�
    // ����ģʽ�»ص������ķ���ֵ�����壬����true or false ���ɡ�
    EventPool::TimeEvent time_event2(EventPool::SecondsAfter(5));
    time_event2.SetCallBack([](bool exit) -> bool {
        if (exit == true) {
            std::cout << "pool exit" << std::endl;
            return false;
        }
        std::cout << "hello world2" << std::endl;
        return false;
        });
    // ���岢��ʼ���Ķ�ʱ��ͨ��PushTimeEvent��������pool
    pool.PushTimeEvent(std::move(time_event));
    auto handler = pool.PushTimeEvent(std::move(time_event2));
    // �����Ǳ�ʾ������Ϊ�˹۲쵽��ʱ���ĵ��ö��ȴ���8s���붨ʱ�������޹�
    std::this_thread::sleep_for(std::chrono::seconds(8));
    // PushTimeEvent�����᷵��һ��handler����handler��������ͨ������Stop������ǰ��ֹ��ʱ��
    // Stop����true����ֹ�ɹ���������ֹʧ�ܣ������ڵ�����ֹ��ʱ���̨�߳��Ѿ�ȡ���ö�ʱ����׼��ִ�У�
    // ���Ᵽ֤�������壺ֻҪStop��������true����ö�ʱ��һ�����ᱻִ�У����Ҹú����ĵ���ʼ���ǰ�ȫ�ģ����۶�Ӧ�Ķ�ʱ���Ƿ��Ѿ��������������١�
    // ֻ��time point���͵Ķ�ʱ������ʹ��handler->Stop������ǰ��ֹ
    bool succ = handler->Stop();
    if (succ == true) {
        std::cout << "time event stop succ!" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(20));
    // ����Ҫֹͣ��ʱ��ʱ����Stop�������ú�����֪ͨ��̨�̣߳������л�δ�����Ķ�ʱ���¼��Ļص���������true�����ã�Ȼ����պ�̨�̲߳����ء�
    // �ú��������ֶ����ã������õĻ�����EventPool�����������е��ã����Կ��̵߳Ķ�ε��á�
    pool.Stop();
    return 0;
}