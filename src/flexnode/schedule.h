// Distributed under version 3 of the Gnu Affero GPL software license, 
// see the accompanying file COPYING for details.


#ifndef FLEX_SCHEDULE
#define FLEX_SCHEDULE

#define COMPLAINT_WAIT_TIME (12 * 1000 * 1000) // 12 seconds

#include "database/data.h"
#include <boost/thread.hpp>

#include "log.h"
#define LOG_CATEGORY "schedule.h"

class ScheduledTask
{
public:
    string_t task_type;
    void (*task_function)(uint160);
    CLocationIterator<> schedule_scanner;

    ScheduledTask(const char* task_type, void (*task_function)(uint160)):
        task_type(task_type),
        task_function(task_function)
    {
        schedule_scanner = scheduledata.LocationIterator(task_type);
        ClearTasks();
    }

    void DoTask(uint160 task_hash)
    {
        task_function(task_hash);
    }

    void ClearTasks()
    {
        uint64_t scheduled_time = 0;
        uint160 task_hash;
        schedule_scanner.SeekEnd();
        
        while (schedule_scanner.GetPreviousObjectAndLocation(task_hash,
                                                             scheduled_time))
        {
            scheduledata.RemoveFromLocation(task_type, scheduled_time);
            schedule_scanner = scheduledata.LocationIterator(task_type);
            schedule_scanner.SeekEnd();
        }
    }

    void DoTasksScheduledForExecutionBeforeNow()
    {
        uint64_t now = GetTimeMicros();
        uint64_t scheduled_time = 0;
        uint160 task_hash;
        schedule_scanner = scheduledata.LocationIterator(task_type);
        schedule_scanner.SeekStart();
        
        while (schedule_scanner.GetNextObjectAndLocation(task_hash,
                                                         scheduled_time))
        {
            if (scheduled_time > now)
                break;
            log_ << "time is now " << now << "\n"
                 << "doing " << task_type 
                 << " scheduled for " << scheduled_time << "\n";
            
            scheduledata.RemoveFromLocation(task_type, scheduled_time);
            schedule_scanner = scheduledata.LocationIterator(task_type);
            schedule_scanner.SeekStart();
            try
            {
                DoTask(task_hash);
            }
            catch (const std::runtime_error& e)
            {
                log_ << "encountered runtime error: " << e.what()
                     << " during scheduled execution of " << task_type
                     << " with task " << task_hash << "\n";
            }
            catch (...)
            {
                log_ << "encountered unknown error "
                     << " during scheduled execution of " << task_type
                     << " with task " << task_hash << "\n";
            }
        }
    }
};

class Scheduler
{
public:
    std::vector<ScheduledTask> tasks;
    bool running;

    Scheduler()
    {
        boost::thread t(&Scheduler::DoTasks, this);
        running = true;
    }

    ~Scheduler()
    {
        running = false;
    }

    void DoTasks()
    {
        while (running)
        {
            foreach_(ScheduledTask& task, tasks)
                task.DoTasksScheduledForExecutionBeforeNow();
            boost::this_thread::interruption_point();
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
    }

    void AddTask(ScheduledTask task)
    {
        tasks.push_back(task);
    }

    void Schedule(string_t task_type, uint160 task_hash, uint64_t when)
    {
        scheduledata[task_hash].Location(task_type) = when;
        log_ << "scheduled " << task_type << " with task "
             << task_hash << " at " << when << "\n";
    }
};


#endif