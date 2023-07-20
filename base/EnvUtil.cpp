/*!
 * \file EnvUtil.cpp
 * \date 2020/10/15 20:26
 *
 * \author Von
 * Contact: fenghezhou1@huya.com
 *
 * \brief
 *
 * TODO: long description
 *
 * \note
*/

#include "EnvUtil.h"

#include <cstdlib>

#include "STLUtil.h"
#ifdef _WIN32
#include <windows.h>
#endif

////////////////////////////////////////////////////////////////////////
namespace cpp
{
  bool SetEnvVar(const char* name, const char* value)
  {
    // putenv要求putenv(env_string), 要在程序的声明周期内都有效.因为这个指针已经成为环境的一部分，直接调整env_string会直接调整环境变量的值.
    // https://man7.org/linux/man-pages/man3/putenv.3.html
    // https://man7.org/linux/man-pages/man3/setenv.3.html
#ifdef _WIN32
    return SetEnvironmentVariableA(name, value) == TRUE;
#else
    return 0 == ((value == nullptr) ? ::unsetenv(name) : ::setenv(name, value, 1));
#endif
  }
  
  std::string GetEnvVar(const char* name)
  {
#ifdef _WIN32
    char value[1024] = {0};
    GetEnvironmentVariableA(name, value, 1024);
#else
    const char* value = getenv(name);
#endif
    if (cpp::IsValidCString(value))
      return value;

    return "";
  }

} // namespace cpp

