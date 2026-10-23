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
 * Every test waits with a deadline: the copier must never block the flow
 * forever, and a blocked copier is a test failure, not a hanging suite. The
 * copier must terminate on its own as soon as its input ends, even when the
 * logged tool is still alive, and it must never be killed by a signal.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */

#include <boost/test/unit_test.hpp>

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

   int wait_with_deadline(pid_t pid, int limit_ms = wait_limit_ms)
   {
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
         // The child must not keep copies of the test pipes: a copy of the
         // producer write end would prevent the end-of-file, and a copy of the
         // console read end would prevent the broken-pipe, exactly the blocking
         // situations the flow must avoid.
         close_extra_fds({});
         ::execv(RECORD_OUTPUT_TOOL, argv.data());
         ::_exit(127);
      }
      return pid;
   }

   // Launch a child executing body and exiting, i.e., a fake logged tool.
   pid_t spawn_producer(const std::function<void()>& body, const std::vector<int>& keep_fds)
   {
      const pid_t pid = ::fork();
      if(pid == 0)
      {
         close_extra_fds(keep_fds);
         body();
         ::_exit(0);
      }
      return pid;
   }

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

   std::string read_all_fd(int fd)
   {
      std::string out;
      char buffer[65536];
      while(true)
      {
         const auto res = ::read(fd, buffer, sizeof(buffer));
         if(res == 0)
         {
            return out;
         }
         if(res < 0)
         {
            if(errno == EINTR)
            {
               continue;
            }
            return out;
         }
         out.append(buffer, static_cast<size_t>(res));
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

   // The test process must not die of SIGPIPE while poking the pipes.
   struct IgnoreSigpipe
   {
      IgnoreSigpipe()
      {
         ::signal(SIGPIPE, SIG_IGN);
      }
   };

   int open_devnull()
   {
      const int fd = ::open("/dev/null", O_RDWR);
      BOOST_REQUIRE_LE(0, fd);
      return fd;
   }
} // namespace

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
   const pid_t producer = spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          ::raise(SIGKILL);
       },
       {producer_pipe[1]});
   const pid_t copier = spawn_tool({log.path().string()}, producer_read.get(), devnull.get());
   producer_read.close();
   producer_write.close();
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier));
   const int producer_status = wait_with_deadline(producer);
   BOOST_CHECK(producer_status == -SIGKILL);
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
   const pid_t producer = spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          ::close(producer_pipe[1]);
          while(true)
          {
             ::pause();
          }
       },
       {producer_pipe[1]});
   Fd devnull{open_devnull()};
   const pid_t copier = spawn_tool({log.path().string()}, producer_read.get(), devnull.get());
   producer_read.close();
   producer_write.close();
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier, 5000));
   BOOST_CHECK_EQUAL(read_file(log.path()), banner);
   ::kill(producer, SIGKILL);
   wait_with_deadline(producer, 5000);
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
   const pid_t producer = spawn_producer(
       [&]() {
          write_all_fd(producer_pipe[1], banner);
          while(true)
          {
             ::pause();
          }
       },
       {producer_pipe[1]});
   producer_write.close();
   Fd devnull{open_devnull()};
   const pid_t copier = spawn_tool({log.path().string()}, producer_read.get(), devnull.get());
   producer_read.close();
   // Synchronization: wait until the copier has logged what the producer
   // wrote, so that the kill below is not in race with the write.
   const auto sync_start = std::chrono::steady_clock::now();
   while(read_file(log.path()).empty())
   {
      const auto elapsed =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - sync_start).count();
      BOOST_REQUIRE(elapsed <= 10000);
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
   }
   ::kill(producer, SIGKILL);
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier));
   wait_with_deadline(producer, 5000);
   BOOST_CHECK_EQUAL(read_file(log.path()), banner);
}

BOOST_AUTO_TEST_CASE(record_output_console_death_keeps_the_log_complete)
{
   IgnoreSigpipe guard;
   // The consumer of the live copy reads a few bytes and then closes the
   // pipe: the copier must disable the console copy, keep the log complete,
   // and exit with 0 without being killed by SIGPIPE.
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
   const pid_t copier = spawn_tool({log.path().string()}, input_fd.get(), console_write.get());
   console_write.close();
   char first[64];
   BOOST_REQUIRE_EQUAL(static_cast<ssize_t>(sizeof(first)), ::read(console_read.get(), first, sizeof(first)));
   console_read.close();
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier));
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_large_output_through_live_console_does_not_deadlock)
{
   IgnoreSigpipe guard;
   // A large output must stream end to end: the copier must drain the input
   // and mirror it live without any deadlock between the two sinks.
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
   const pid_t copier = spawn_tool({log.path().string()}, input_fd.get(), console_write.get());
   console_write.close();
   const std::string console_data = read_all_fd(console_read.get());
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier));
   BOOST_CHECK_EQUAL(console_data, payload);
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_unwritable_log_exits_fast)
{
   IgnoreSigpipe guard;
   // A log file that cannot be opened: the copier must report the failure
   // with a non-zero status, without waiting for the logged output.
   TempPath log_dir("log_dir", /*as_directory=*/true);
   Fd devnull{open_devnull()};
   const pid_t copier = spawn_tool({log_dir.path().string()}, devnull.get(), devnull.get());
   BOOST_REQUIRE_EQUAL(1, wait_with_deadline(copier, 5000));
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
   const pid_t copier = spawn_tool({"/dev/full"}, input_fd.get(), console_write.get());
   console_write.close();
   char first[16];
   const ssize_t console_data = ::read(console_read.get(), first, sizeof(first));
   (void)console_data;
   console_read.close();
   BOOST_REQUIRE_EQUAL(1, wait_with_deadline(copier));
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
   const pid_t copier = spawn_tool({"-q", log.path().string()}, input_fd.get(), console_write.get());
   console_write.close();
   BOOST_CHECK_EQUAL(read_all_fd(console_read.get()), std::string());
   BOOST_REQUIRE_EQUAL(0, wait_with_deadline(copier));
   BOOST_CHECK_EQUAL(read_file(log.path()), payload);
}

BOOST_AUTO_TEST_CASE(record_output_usage_errors_terminate)
{
   IgnoreSigpipe guard;
   // Misuse must exit immediately with the usage status, never hang.
   Fd devnull{open_devnull()};
   BOOST_CHECK_EQUAL(2, wait_with_deadline(spawn_tool({}, devnull.get(), devnull.get()), 5000));
   BOOST_CHECK_EQUAL(2, wait_with_deadline(spawn_tool({"a.log", "b.log"}, devnull.get(), devnull.get()), 5000));
   BOOST_CHECK_EQUAL(0, wait_with_deadline(spawn_tool({"-h"}, devnull.get(), devnull.get()), 5000));
}

BOOST_AUTO_TEST_SUITE_END()
