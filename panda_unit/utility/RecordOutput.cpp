/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file RecordOutput.cpp
 * @brief unit tests of the record_output log copier, the tool the flow uses
 * instead of the system tee (see src/utility/record_output.cpp).
 *
 * The tests exercise the failure modes of the tool that is invoked, i.e., the
 * simulation, the system testbench or any other tool whose output is being
 * logged, and of the environment of the copier:
 * - the logged tool dies by a signal (as a panicking tool or an OOM kill),
 *   hangs, or closes its output before hanging;
 * - the console consuming the live copy disappears mid-stream;
 * - the log file cannot be opened, or cannot be written at all.
 *
 * Every wait, and every read from the copier pipes, uses a deadline: the
 * copier must never block the flow forever, and a blocked copier is a test
 * failure, not a hanging suite. The copier must terminate on its own as soon
 * as its input ends, even when the logged tool is still alive, and it must
 * never be killed by a signal. The children are owned by a scope guard, so
 * they are always collected, even when an assertion fails in the middle of
 * a case. The spawned copier starts with the signal dispositions of the real
 * flow, i.e., with SIGPIPE at the default action, and it must survive the
 * broken pipes with its own handling, not with a disposition inherited from
 * the test process.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include <boost/test/unit_test.hpp>

#include "Parameter.hpp"
#include "fileIO.hpp"
#include "string_manipulation.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef RECORD_OUTPUT_TOOL
#define RECORD_OUTPUT_TOOL "record_output"
#endif

BOOST_AUTO_TEST_SUITE(record_output_tool)

namespace
{
   // Generous deadline: a correct copier terminates in milliseconds, so a hit
   // deadline means that the copier blocked the execution, which is what the
   // flow must never do.
   constexpr int wait_limit_ms = 15000;
   constexpr int timeout_marker = -9999;
   // A non-positive pid is never handed to waitpid or to kill: with -1,
   // waitpid would observe any child, e.g., the hung producer of another
   // case, and kill(-1, SIGKILL) at the deadline would signal every process
   // the test can reach, unrelated ones included.
   constexpr int invalid_pid_marker = -9998;

   using Clock = std::chrono::steady_clock;
   using TimePoint = Clock::time_point;

   TimePoint deadline_after(int limit_ms)
   {
      return Clock::now() + std::chrono::milliseconds(limit_ms);
   }

   int wait_with_deadline(pid_t pid, int limit_ms = wait_limit_ms)
   {
      if(pid <= 0)
      {
         return invalid_pid_marker;
      }
      const auto start = std::chrono::steady_clock::now();
      while(true)
      {
         int status = 0;
         const auto done = ::waitpid(pid, &status, WNOHANG);
         if(done == pid)
         {
            if(WIFEXITED(status))
            {
               return WEXITSTATUS(status);
            }
            if(WIFSIGNALED(status))
            {
               // the copier must never die by a signal: report it as a negative
               // status so that the assertions catch it.
               return -WTERMSIG(status);
            }
            return -1;
         }
         const auto elapsed =
             std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
         if(elapsed > limit_ms)
         {
            ::kill(pid, SIGKILL);
            int killed_status = 0;
            ::waitpid(pid, &killed_status, 0);
            return timeout_marker;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
   }

   // RAII file descriptor.
   class Fd
   {
    public:
      Fd() = default;
      explicit Fd(int fd) : fd_(fd)
      {
      }
      Fd(const Fd&) = delete;
      Fd& operator=(const Fd&) = delete;
      Fd(Fd&& other) noexcept : fd_(other.fd_)
      {
         other.fd_ = -1;
      }
      ~Fd()
      {
         close();
      }
      int get() const
      {
         return fd_;
      }
      void close()
      {
         if(fd_ >= 0)
         {
            ::close(fd_);
            fd_ = -1;
         }
      }

    private:
      int fd_ = -1;
   };

   std::string make_payload(size_t size)
   {
      std::string payload;
      payload.reserve(size);
      uint64_t state = 0x12345678abcdefULL;
      while(payload.size() < size)
      {
         state = state * 6364136223846793005ULL + 1442695040888963407ULL;
         payload.push_back(static_cast<char>(state >> 33));
      }
      return payload;
   }

   // A temporary directory, unique and atomically allocated per test process:
   // ctest can run the same suite through several processes in parallel, i.e.,
   // the whole libbambu_unit test together with the record_output_tool one,
   // and fixed names in the shared temporary directory would make the
   // processes delete each other's inputs and logs.
   class ProcessTempDir
   {
    public:
      ProcessTempDir()
      {
         std::error_code ignored;
         const std::filesystem::path parent = std::filesystem::temp_directory_path() / "panda_record_output_tests";
         std::filesystem::create_directories(parent, ignored);
         const std::string pattern = (parent / "proc_XXXXXX").string();
         std::vector<char> buffer(pattern.begin(), pattern.end());
         buffer.push_back('\0');
         const char* made = ::mkdtemp(buffer.data());
         if(made)
         {
            path_ = made;
         }
      }
      ~ProcessTempDir()
      {
         std::error_code ignored;
         if(!path_.empty())
         {
            std::filesystem::remove_all(path_, ignored);
         }
      }
      const std::filesystem::path& path() const
      {
         return path_;
      }

    private:
      std::filesystem::path path_;
   };

   const ProcessTempDir& process_temp_dir()
   {
      static const ProcessTempDir instance;
      return instance;
   }

   // A temporary log file, inside the unique directory of the process,
   // removed at the end of the test.
   class TempPath
   {
    public:
      explicit TempPath(const std::string& name, bool as_directory = false)
      {
         const auto& root = process_temp_dir().path();
         const auto fallback = std::filesystem::temp_directory_path() / ("panda_record_output_" + name);
         path_ = root.empty() ? fallback : root / ("panda_record_output_" + name);
         std::filesystem::remove_all(path_);
         if(as_directory)
         {
            std::filesystem::create_directories(path_);
         }
      }
      ~TempPath()
      {
         std::error_code ignored;
         std::filesystem::remove_all(path_, ignored);
      }
      const std::filesystem::path& path() const
      {
         return path_;
      }

    private:
      std::filesystem::path path_;
   };

   // In a child, right after the fork, close all descriptors except the three
   // standard ones and the explicitly kept ones: inherited copies of the test
   // pipes would keep fake simulations alive and fake consoles readable,
   // preventing the end-of-file and the broken-pipe the copier relies on.
   void close_extra_fds(const std::vector<int>& keep)
   {
      std::vector<int> to_close;
      {
         std::error_code ignored;
         std::filesystem::directory_iterator it{"/proc/self/fd", ignored};
         for(; it != std::filesystem::directory_iterator{}; ++it)
         {
            const std::string name = it->path().filename().string();
            if(name.empty() || name[0] < '0' || name[0] > '9')
            {
               continue;
            }
            const int fd = std::atoi(name.c_str());
            if(fd > 2 && std::find(keep.begin(), keep.end(), fd) == keep.end())
            {
               to_close.push_back(fd);
            }
         }
      }
      for(const int fd : to_close)
      {
         ::close(fd);
      }
   }

   // In a child, right after the fork, restore the signal dispositions of the
   // real flow: the test process ignores SIGPIPE to poke the pipes without
   // dying, but an ignored disposition survives exec. The copier must instead
   // win against the broken pipes with its own std::signal call, which is
   // exactly what these tests check, so it must start from SIG_DFL.
   void reset_signals()
   {
      ::signal(SIGPIPE, SIG_DFL);
   }

   // Launch the copier with the given stdin/stdout redirections: -1 means
   // inherit. Args are the command line arguments of the tool.
   pid_t spawn_tool(const std::vector<std::string>& args, int in_fd, int out_fd)
   {
      std::vector<std::string> full{RECORD_OUTPUT_TOOL};
      full.insert(full.end(), args.begin(), args.end());
      std::vector<char*> argv;
      for(auto& arg : full)
      {
         argv.push_back(const_cast<char*>(arg.c_str()));
      }
      argv.push_back(nullptr);
      const pid_t pid = ::fork();
      if(pid < 0)
      {
         // Never return an error code as if it were a pid: the callers wait
         // on and signal the value they receive.
         BOOST_FAIL("cannot fork the copier: " + std::string(std::strerror(errno)));
      }
      if(pid == 0)
      {
         if(in_fd >= 0)
         {
            ::dup2(in_fd, STDIN_FILENO);
         }
         if(out_fd >= 0)
         {
            ::dup2(out_fd, STDOUT_FILENO);
         }
         close_extra_fds({});
         reset_signals();
         ::execv(RECORD_OUTPUT_TOOL, argv.data());
         ::_exit(127);
      }
      return pid;
   }

   // Launch a child executing body and exiting, i.e., a fake logged tool.
   pid_t spawn_producer(const std::function<void()>& body, const std::vector<int>& keep_fds)
   {
      const pid_t pid = ::fork();
      if(pid < 0)
      {
         BOOST_FAIL("cannot fork the logged tool: " + std::string(std::strerror(errno)));
      }
      if(pid == 0)
      {
         close_extra_fds(keep_fds);
         reset_signals();
         body();
         ::_exit(0);
      }
      return pid;
   }

   // A child owned by the scope: whatever the assertions do, even throwing,
   // the child is killed and collected when the case ends. wait_with_deadline
   // always returns having collected the child, also when it expires, so the
   // ownership is released in both cases.
   class Child
   {
    public:
      Child() = default;
      explicit Child(pid_t pid)
      {
         // A child that was not created must never enter the ownership: the
         // waits and the kills of the class take the pid_ as they stand.
         BOOST_REQUIRE(0 < pid);
         pid_ = pid;
      }
      Child(const Child&) = delete;
      Child& operator=(const Child&) = delete;
      ~Child()
      {
         dismiss();
      }
      int wait(int limit_ms = wait_limit_ms)
      {
         const int status = wait_with_deadline(pid_, limit_ms);
         pid_ = -1;
         return status;
      }
      void dismiss()
      {
         if(pid_ > 0)
         {
            ::kill(pid_, SIGKILL);
            int status = 0;
            ::waitpid(pid_, &status, 0);
            pid_ = -1;
         }
      }

    private:
      pid_t pid_ = -1;
   };

   void write_all_fd(int fd, const std::string& data)
   {
      size_t written = 0;
      while(written < data.size())
      {
         const auto res = ::write(fd, data.data() + written, data.size() - written);
         if(res < 0)
         {
            if(errno == EINTR)
            {
               continue;
            }
            return;
         }
         written += static_cast<size_t>(res);
      }
   }

   // Wait until the fd has data, or reaches the end-of-file or an error,
   // without ever staying past the deadline: a copier blocked while keeping
   // the console open must not keep the test inside a blocking read.
   bool readable_until(int fd, const TimePoint& deadline)
   {
      while(true)
      {
         const auto now = Clock::now();
         if(now >= deadline)
         {
            return false;
         }
         const auto remaining =
             static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count()) + 1;
         struct pollfd watch;
         watch.fd = fd;
         watch.events = POLLIN;
         watch.revents = 0;
         const int ready = ::poll(&watch, 1, std::min(remaining, 100));
         if(ready > 0)
         {
            return true;
         }
         if(ready < 0 && errno != EINTR)
         {
            return false;
         }
      }
   }

   struct ReadAll
   {
      std::string data;
      bool timed_out = false;
   };

   // Read until the end-of-file, with the deadline on the whole read: the
   // returned flag tells whether the read was interrupted, and the caller can
   // collect the children before reporting the failure.
   ReadAll read_all_fd(int fd, int limit_ms = wait_limit_ms)
   {
      ReadAll result;
      const TimePoint deadline = deadline_after(limit_ms);
      char buffer[65536];
      while(true)
      {
         if(!readable_until(fd, deadline))
         {
            result.timed_out = true;
            return result;
         }
         const auto res = ::read(fd, buffer, sizeof(buffer));
         if(res == 0)
         {
            return result;
         }
         if(res < 0)
         {
            if(errno == EINTR)
            {
               continue;
            }
            return result;
         }
         result.data.append(buffer, static_cast<size_t>(res));
      }
   }

   int write_file(const std::filesystem::path& path, const std::string& content)
   {
      std::ofstream out(path, std::ios::binary);
      out << content;
      out.close();
      return static_cast<int>(out.good());
   }

   std::string read_file(const std::filesystem::path& path)
   {
      std::ifstream in(path, std::ios::binary);
      return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
   }

   // The test process must not die of SIGPIPE while poking the pipes: the
   // previous disposition is saved and restored when the case ends, and the
   // spawned children reset it to the default, so that the copier can only
   // survive the broken pipes with its own signal handling.
   struct IgnoreSigpipe
   {
      IgnoreSigpipe() : previous(::signal(SIGPIPE, SIG_IGN))
      {
      }
      ~IgnoreSigpipe()
      {
         ::signal(SIGPIPE, previous);
      }
      void (*previous)(int);
   };

   int open_devnull()
   {
      const int fd = ::open("/dev/null", O_RDWR);
      BOOST_REQUIRE_LE(0, fd);
      return fd;
   }

   class LoggingParameter final : public Parameter
   {
    public:
      LoggingParameter() : Parameter("record_output_test", 0, nullptr)
      {
      }
      void SetDefaults() override
      {
      }
      void PrintProgramName(std::ostream&) const override
      {
      }
      void PrintHelp(std::ostream&) const override
      {
      }
      int Exec() override
      {
         return 0;
      }
      void CheckParameters() override
      {
      }
   };
} // namespace

BOOST_AUTO_TEST_CASE(record_output_generated_paths_are_shell_escaped)
{
   TempPath root("generated_paths", /*as_directory=*/true);
   const std::string special = " ' \" $HOME `printf WRONG` $(printf WRONG) \\";
   const std::string payload = "logged output\n";
   for(const auto level : {0u, 3u})
   {
      for(const auto& directory : {std::string("scripts"), "scripts" + special})
      {
         const auto script_dir = root.path() / directory;
         std::filesystem::create_directories(script_dir);
         auto param = std::make_shared<LoggingParameter>();
         param->setOption(OPT_output_level, level);
         param->setOption(OPT_output_temporary_directory, script_dir.string());
         const auto log = root.path() / ("output" + special + ".log");
         Child command{spawn_producer(
             [&]() {
                const int status = PandaSystem(param, "printf '%s' " + shell_escape_argument(payload), false, log);
                ::_exit(IsError(status) ? 1 : 0);
             },
             {})};
         BOOST_REQUIRE_EQUAL(0, command.wait());
         BOOST_CHECK_EQUAL(read_file(log), payload);
         std::filesystem::remove(log);
      }
   }
}

BOOST_AUTO_TEST_CASE(record_output_crashed_tool_does_not_block)
{
   IgnoreSigpipe guard;
   // The logged tool writes its banner and then dies by a signal, as a
   // panicking tool, a crashing simulator or an OOM kill do.
   const std::string banner = "Sim: starting\nSim: 4711 events\n";
   int producer_pipe[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(producer_pipe));
   Fd producer_read{producer_pipe[0]};
   Fd producer_write{producer_pipe[1]};
   Fd devnull{open_devnull()};
   TempPath log("crashed.log");
   Child producer{spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          ::raise(SIGKILL);
       },
       {producer_pipe[1]})};
   Child copier{spawn_tool({log.path().string()}, producer_read.get(), devnull.get())};
   producer_read.close();
   producer_write.close();
   BOOST_REQUIRE_EQUAL(0, copier.wait());
   BOOST_CHECK(producer.wait() == -SIGKILL);
   BOOST_CHECK_EQUAL(read_file(log.path()), banner);
}

BOOST_AUTO_TEST_CASE(record_output_hung_tool_closing_its_output_does_not_block)
{
   IgnoreSigpipe guard;
   // The logged tool closes its output and then hangs forever. The copier
   // must exit at the input end-of-file, without waiting for the tool to die.
   const std::string banner = "Sim: done writing\n";
   int producer_pipe[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(producer_pipe));
   Fd producer_read{producer_pipe[0]};
   Fd producer_write{producer_pipe[1]};
   TempPath log("hung_closed.log");
   Child producer{spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          ::close(producer_pipe[1]);
          while(true)
          {
             ::pause();
          }
       },
       {producer_pipe[1]})};
   Fd devnull{open_devnull()};
   Child copier{spawn_tool({log.path().string()}, producer_read.get(), devnull.get())};
   producer_read.close();
   producer_write.close();
   BOOST_REQUIRE_EQUAL(0, copier.wait(5000));
   BOOST_CHECK_EQUAL(read_file(log.path()), banner);
   producer.dismiss();
}

BOOST_AUTO_TEST_CASE(record_output_tool_killed_while_hanging_does_not_block)
{
   IgnoreSigpipe guard;
   // The logged tool writes, then hangs keeping its output open. The copier
   // waits on the pipe, but as soon as the hung tool is killed it must exit
   // by itself: nothing blocks the flow forever.
   const std::string banner = "Sim: partial\n";
   int producer_pipe[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(producer_pipe));
   Fd producer_read{producer_pipe[0]};
   Fd producer_write{producer_pipe[1]};
   TempPath log("hung_killed.log");
   Child producer{spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          while(true)
          {
             ::pause();
          }
       },
       {producer_pipe[1]})};
   producer_write.close();
   Fd devnull{open_devnull()};
   Child copier{spawn_tool({log.path().string()}, producer_read.get(), devnull.get())};
   producer_read.close();
   // Synchronization: wait until the copier has logged what the producer
   // wrote, so that the kill below is not in race with the write.
   const TimePoint sync_deadline = deadline_after(10000);
   while(read_file(log.path()).empty())
   {
      BOOST_REQUIRE(Clock::now() < sync_deadline);
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
   }
   producer.dismiss();
   BOOST_REQUIRE_EQUAL(0, copier.wait());
   BOOST_CHECK_EQUAL(read_file(log.path()), banner);
}

BOOST_AUTO_TEST_CASE(record_output_console_death_keeps_the_log_complete)
{
   IgnoreSigpipe guard;
   // The consumer of the live copy reads a few bytes and then closes the
   // pipe: the copier must disable the console copy, keep the log complete,
   // and exit with 0 without being killed by SIGPIPE. The copier starts with
   // SIGPIPE at the default action, like in the real flow, so surviving here
   // proves that the tool installs its own handling.
   const auto payload = make_payload(4 * 1024 * 1024);
   TempPath input("payload.bin");
   BOOST_REQUIRE(write_file(input.path(), payload));
   const int in_fd = ::open(input.path().c_str(), O_RDONLY);
   BOOST_REQUIRE_LE(0, in_fd);
   Fd input_fd{in_fd};
   int console[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(console));
   Fd console_read{console[0]};
   Fd console_write{console[1]};
   TempPath log("console_death.log");
   Child copier{spawn_tool({log.path().string()}, input_fd.get(), console_write.get())};
   console_write.close();
   char first[64];
   if(readable_until(console_read.get(), deadline_after(5000)))
   {
      BOOST_REQUIRE_EQUAL(static_cast<ssize_t>(sizeof(first)), ::read(console_read.get(), first, sizeof(first)));
   }
   else
   {
      BOOST_ERROR("the copier did not write the live copy within the deadline");
   }
   console_read.close();
   BOOST_REQUIRE_EQUAL(0, copier.wait());
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_large_output_through_live_console_does_not_deadlock)
{
   IgnoreSigpipe guard;
   // A large output must stream end to end: the copier must drain the input
   // and mirror it live without any deadlock between the two sinks. The read
   // is bounded: a stalled copier fails the case, it does not stall the suite.
   const auto payload = make_payload(8 * 1024 * 1024);
   TempPath input("big_payload.bin");
   BOOST_REQUIRE(write_file(input.path(), payload));
   const int in_fd = ::open(input.path().c_str(), O_RDONLY);
   BOOST_REQUIRE_LE(0, in_fd);
   Fd input_fd{in_fd};
   int console[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(console));
   Fd console_read{console[0]};
   Fd console_write{console[1]};
   TempPath log("large.log");
   Child copier{spawn_tool({log.path().string()}, input_fd.get(), console_write.get())};
   console_write.close();
   const ReadAll console_data = read_all_fd(console_read.get());
   if(console_data.timed_out)
   {
      copier.wait(1000);
      BOOST_FAIL("the console copy stalled: the copier blocked the pipeline");
   }
   BOOST_REQUIRE_EQUAL(0, copier.wait());
   BOOST_CHECK_EQUAL(console_data.data, payload);
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_unwritable_log_exits_fast)
{
   IgnoreSigpipe guard;
   // A log file that cannot be opened: the copier must report the failure
   // with a non-zero status, without waiting for the logged output.
   TempPath log_dir("log_dir", /*as_directory=*/true);
   Fd devnull{open_devnull()};
   Child copier{spawn_tool({log_dir.path().string()}, devnull.get(), devnull.get())};
   BOOST_REQUIRE_EQUAL(1, copier.wait(5000));
}

BOOST_AUTO_TEST_CASE(record_output_unopenable_log_still_drains_the_input)
{
   IgnoreSigpipe guard;
   // The producer must not be killed by SIGPIPE when the log cannot be opened:
   // the copier drains the whole input, keeps the live console copy, and only
   // then reports the failure.
   const auto payload = make_payload(1024 * 1024);
   TempPath input("unopenable_payload.bin");
   BOOST_REQUIRE(write_file(input.path(), payload));
   const int in_fd = ::open(input.path().c_str(), O_RDONLY);
   BOOST_REQUIRE_LE(0, in_fd);
   Fd input_fd{in_fd};
   int console[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(console));
   Fd console_read{console[0]};
   Fd console_write{console[1]};
   TempPath log_dir("unopenable_log_dir", /*as_directory=*/true);
   Child copier{spawn_tool({log_dir.path().string()}, input_fd.get(), console_write.get())};
   console_write.close();
   const ReadAll console_data = read_all_fd(console_read.get());
   if(console_data.timed_out)
   {
      copier.wait(1000);
      BOOST_FAIL("the copier did not drain the input");
   }
   BOOST_CHECK_EQUAL(console_data.data, payload);
   BOOST_REQUIRE_EQUAL(1, copier.wait());
}

BOOST_AUTO_TEST_CASE(record_output_unwritable_data_and_dead_console_still_terminate)
{
   IgnoreSigpipe guard;
   // /dev/full: the log can be opened but every write fails. Together with a
   // console that dies mid-stream, the copier must still terminate, draining
   // the input, and report a non-zero status instead of blocking forever.
   if(!std::filesystem::exists("/dev/full"))
   {
      BOOST_TEST_INFO("/dev/full not available, skipping the log-write failure test");
      return;
   }
   const auto payload = make_payload(4 * 1024 * 1024);
   TempPath input("full_payload.bin");
   BOOST_REQUIRE(write_file(input.path(), payload));
   const int in_fd = ::open(input.path().c_str(), O_RDONLY);
   BOOST_REQUIRE_LE(0, in_fd);
   Fd input_fd{in_fd};
   int console[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(console));
   Fd console_read{console[0]};
   Fd console_write{console[1]};
   Child copier{spawn_tool({"/dev/full"}, input_fd.get(), console_write.get())};
   console_write.close();
   char first[16];
   if(readable_until(console_read.get(), deadline_after(5000)))
   {
      const ssize_t console_data = ::read(console_read.get(), first, sizeof(first));
      (void)console_data;
   }
   console_read.close();
   BOOST_REQUIRE_EQUAL(1, copier.wait());
}

BOOST_AUTO_TEST_CASE(record_output_quiet_writes_only_the_log)
{
   IgnoreSigpipe guard;
   const auto payload = make_payload(256 * 1024);
   TempPath input("quiet_payload.bin");
   BOOST_REQUIRE(write_file(input.path(), payload));
   const int in_fd = ::open(input.path().c_str(), O_RDONLY);
   BOOST_REQUIRE_LE(0, in_fd);
   Fd input_fd{in_fd};
   int console[2];
   BOOST_REQUIRE_EQUAL(0, ::pipe(console));
   Fd console_read{console[0]};
   Fd console_write{console[1]};
   TempPath log("quiet.log");
   Child copier{spawn_tool({"-q", log.path().string()}, input_fd.get(), console_write.get())};
   console_write.close();
   const ReadAll console_data = read_all_fd(console_read.get());
   BOOST_REQUIRE(!console_data.timed_out);
   BOOST_CHECK_EQUAL(console_data.data, std::string());
   BOOST_REQUIRE_EQUAL(0, copier.wait());
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_usage_errors_terminate)
{
   IgnoreSigpipe guard;
   // Misuse must exit immediately with the usage status, never hang.
   Fd devnull{open_devnull()};
   Child no_arguments{spawn_tool({}, devnull.get(), devnull.get())};
   Child two_logs{spawn_tool({"a.log", "b.log"}, devnull.get(), devnull.get())};
   Child help{spawn_tool({"-h"}, devnull.get(), devnull.get())};
   BOOST_CHECK_EQUAL(2, no_arguments.wait(5000));
   BOOST_CHECK_EQUAL(2, two_logs.wait(5000));
   BOOST_CHECK_EQUAL(0, help.wait(5000));
}

BOOST_AUTO_TEST_CASE(record_output_non_positive_pids_are_refused)
{
   // A failed fork must never reach the waits and the kills: with -1, the
   // polling waitpid would observe any child instead of the copier, and the
   // kill at the deadline would signal every process the test can reach.
   // The refusal is immediate: no deadline is even consumed.
   const auto start = Clock::now();
   BOOST_CHECK_EQUAL(invalid_pid_marker, wait_with_deadline(-1));
   BOOST_CHECK_EQUAL(invalid_pid_marker, wait_with_deadline(0, 100));
   BOOST_CHECK_EQUAL(invalid_pid_marker, Child{}.wait(100));
   BOOST_CHECK(Clock::now() < start + std::chrono::milliseconds(100));
}

BOOST_AUTO_TEST_SUITE_END()
