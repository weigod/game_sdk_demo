/*!
 * \file EnvUtil.h
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

// make sure file included only once
#ifndef ENVUTIL_H_
#define ENVUTIL_H_
#include <string>
namespace cpp
{
  // get用标准库的
  bool SetEnvVar(const char* name, const char* value);
  std::string GetEnvVar(const char* name);
}

#endif  // ENVUTIL_H_
