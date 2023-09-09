/*
 * Provides the ability to queue simple member functions
 * of a class to the Windows thread pool.
 *
 */

#pragma once

#include <memory>
#include <Windows.h>

class ThreadPool
{
public:
    template <typename T>
    static void QueueWorkItem(void (T::*function)(void), T *object, ULONG flags = WT_EXECUTELONGFUNCTION)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;
        std::auto_ptr<CallbackType> p(new CallbackType(function, object));
        if (::QueueUserWorkItem(ThreadProc<T>, p.get(), flags))
        {
            p.release();
        }
        else
        {
            throw GetLastError();
        }
    }

private:
    template <typename T>
    static DWORD WINAPI ThreadProc(PVOID context)
    {
        typedef std::pair<void (T::*)(), T *> CallbackType;
        std::auto_ptr<CallbackType> p(static_cast<CallbackType *>(context));
        (p->second->*p->first)();
        return 0;
    }
};