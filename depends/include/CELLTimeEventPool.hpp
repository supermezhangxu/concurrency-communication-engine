#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <iostream>

#include "CELLTimeEvent.hpp"

namespace EventPool {
    class EventPool {
    public:
        explicit EventPool(uint64_t max_count) : max_time_event_count_(max_count), stop_(false) {}

        std::shared_ptr<TimeEventHandler> PushTimeEvent(TimeEvent&& te) {
            std::unique_lock<std::mutex> guard(mut_);
            not_fill_cv_.wait(guard, [this]() -> bool { return this->timer_queue_.size() < this->max_time_event_count_; });
            te.handler_.reset(new TimeEventHandler(te.type_));
            auto result = te.handler_;
            timer_queue_.push(std::move(te));
            at_least_one_cv_.notify_all();
            return result;
        }

        void Run() {
            while (true) {
                bool stop = false;
                try {
                    std::vector<TimeEvent> events = GetReady(stop);
                    if (stop == true) {
                        CleanAllEvent(std::move(events));
                        return;
                    }
                    std::vector<TimeEvent> continue_to;
                    for (auto& each : events) {
                        bool more = each.OnExpire(false);
                        // ����DURATION���͵��¼����ԣ�����true���ʾ����ʱ��������������¼����С�
                        if (more && each.GetType() == Type::DURATION) {
                            each.UpdateTimePoint();
                            continue_to.push_back(each);
                        }
                    }
                    // ��̨�߳�Ӧ��ֱ�Ӽ�����push��Ȼ��֪ͨ�ȴ���at_least_one_cv_�ϵ��̣߳����ܵ���PushTimeEvents����������������
                    std::unique_lock<std::mutex> guard(mut_);
                    for (auto& each : continue_to) {
                        timer_queue_.push(each);
                    }
                    at_least_one_cv_.notify_all();
                }
                catch (const std::exception& e) {
                    std::cout << e.what() << std::endl;
                }
            }
        }

        void Stop() {
            std::unique_lock<std::mutex> guard(mut_);
            // ȷ��ֻ����һ��
            if (stop_ == true) {
                return;
            }
            stop_ = true;
            guard.unlock();
            // ����Run����
            at_least_one_cv_.notify_all();
            if (backend_.joinable()) {
                backend_.join();
            }
        }

        ~EventPool() { Stop(); }

        void Start() {
            backend_ = std::thread([this]() { this->Run(); });
        }

    private:
        std::vector<TimeEvent> GetReady(bool& stop) {
            std::vector<TimeEvent> result;
            std::unique_lock<std::mutex> guard(mut_);
            auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
            while (timer_queue_.empty() == false) {
                if (now >= timer_queue_.top().GetTimePoint()) {
                    result.push_back(timer_queue_.top());
                    timer_queue_.pop();
                }
                else {
                    break;
                }
            }
            stop = stop_;
            // �������
            // ���1 �� ���ȡ���ˣ���ֱ�ӷ���
            // ���2 �� ���ûȡ��������timer_queue_���ǿյ�
            // ���3 �� ���ûȡ����timer_queue_���ǿյ�
            if (result.empty() == false) {
                not_fill_cv_.notify_all();
                return result;
            }
            // ���2 �� �ȴ�������һ��time_event����
            if (timer_queue_.empty() == true) {
                at_least_one_cv_.wait(guard, [this]() -> bool { return this->timer_queue_.empty() == false || stop_ == true; });
            }
            else {
                // ���3 �� ���ȴ������Ҫ�����ѵ�time_event��ʱ�䣬���������;�����ѣ�������time_event���ӣ�
                std::chrono::milliseconds dt = timer_queue_.top().GetTimePoint() - now;
                at_least_one_cv_.wait_for(guard, dt);
            }
            stop = stop_;
            return result;
        }

        void CleanAllEvent(std::vector<TimeEvent>&& events) {
            std::unique_lock<std::mutex> guard(mut_);
            for (auto& each : events) {
                each.OnExpire(true);
            }
            while (timer_queue_.empty() == false) {
                auto each = timer_queue_.top();
                each.OnExpire(true);
                timer_queue_.pop();
            }
        }

        std::mutex mut_;
        struct cmp_for_time_event {
            bool operator()(const TimeEvent& t1, const TimeEvent& t2) { return t1.GetTimePoint() > t2.GetTimePoint(); }
        };
        std::priority_queue<TimeEvent, std::vector<TimeEvent>, cmp_for_time_event> timer_queue_;
        uint64_t max_time_event_count_;
        std::condition_variable not_fill_cv_;
        std::condition_variable at_least_one_cv_;
        bool stop_;
        std::thread backend_;
    };
}  // namespace EventPool