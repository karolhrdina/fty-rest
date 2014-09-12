/*
Copyright (C) 2014 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file subprocess.h
    \brief (sub) process C++ API
    \author Michal Vyskocil <michalvyskocil@eaton.com>
 */

#ifndef _SRC_UTILS_SUBPROCESS_H
#define _SRC_UTILS_SUBPROCESS_H

#include <cxxtools/posix/fork.h>

#include <climits>
#include <unistd.h>

#include <vector>
#include <string>

/* \brief Process abstraction class with stdout/stderr redirection
 *
 * The advantage of this class is easyness of usage, as well as readability as
 * it handles several low-level oddities of POSIX/Linux C-API.
 *
 * Note that SubProcess instance is tied to one process only, so cannot be reused
 * to execute more than one subprocess. This is due "simulate" dynamic nature
 * of a processes. Therefor for a code running unspecified amount of processes, 
 * instances must be heap allocated using new constructor.
 *
 * For that reason copy/move constructor and operator are disallowed.
 *
 * Example:
 * \code
 * SubProcess proc("/bin/true");
 * proc.wait();
 * std::cout "process pid: " << proc.getPid() << std::endl;
 * \endcode
 */

class SubProcess {
    public:
       
        static const int codeRunning = INT_MIN;

        // \brief construct instance
        //
        // @param argv - C-like string of argument, see execvpe(2) for details
        //
        // \todo does not deal with a command line limit
        explicit SubProcess(std::vector<std::string> cxx_argv);

        // \brief close all pipes, waits on process termination
        //
        // \warning destructor calls wait, so can hand your program in a case
        //          child process never ends. Better to call terminate() manually.
        //
        virtual ~SubProcess();
        
        //! \brief return pid of executed command
        pid_t getPid() const { return _fork.getPid(); }
        
        //! \brief get the pipe ends connected to stdout of started program, or -1 if not started
        int getStdout() const { return _outpair[0]; }
        
        //! \brief get the pipe ends connected to stderr of started program, or -1 if not started
        int getStderr() const { return _errpair[0]; }
        
        //! \brief does program run or not
        //
        //  isRunning checks the status code, so wait/poll function calls are necessary
        //  in order to see the real state of a process
        //  \todo - call kill(0) + wait if not running?
        bool isRunning() const { return _state == SubProcessState::RUNNING; }

        //! \brief get the return code, \see wait for meaning
        int getReturnCode() const { return _return_code; }

        //! \brief return core dumped flag
        bool isCoreDumped() const { return _core_dumped; }

        // \brief creates a pipe/pair for stdout/stderr, fork and exec the command. Note this
        // can be started only once, all subsequent calls becames nooop.
        //
        // \throws std::runtime_error if pipe/fork syscalls fails
        //
        //
        void run();

        //! \brief wait on program terminate
        //
        //  @param no_hangup if false (default) wait indefinitelly, otherwise return immediatelly
        //  @return positive return value of a process
        //          negative is a number of a signal which terminates process
        //          or codeRunning constant indicating code is still running
        int wait(bool no_hangup=false);

        //! \brief no hanging varint of /see wait
        int poll() {  return wait(true); }

        //! \brief kill the subprocess with defined signal
        //
        //  @return see kill(2)
        //  \todo - to throw an exception (signal != 0)?
        int kill(int signal=9);

        //! \brief terminate the subprocess with SIGTERM
        //
        //  @return \see kill
        int terminate();
    
    protected:

        enum class SubProcessState {
            NOT_STARTED,
            RUNNING,
            FINISHED
        };
        const char* str_state(SubProcessState state);

        cxxtools::posix::Fork _fork;
        SubProcessState _state;
        std::vector<std::string> _cxx_argv;
        int _return_code;
        bool _core_dumped;
        int _outpair[2];
        int _errpair[2];

        // disallow copy and move constructors
        SubProcess(const SubProcess& p) = delete;
        SubProcess& operator=(SubProcess p) = delete;
        SubProcess(const SubProcess&& p) = delete;
        SubProcess& operator=(SubProcess&& p) = delete;

};

#endif //_SRC_UTILS_SUBPROCESS_H

