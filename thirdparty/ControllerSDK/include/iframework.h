#pragma once

#ifndef WIN32 // or something like that...
#define __stdcall
#endif

//接口基类。对于需要遵循本框架的模块，必须继承此基类。但某些局部使用的、功能性的、与本框架无关的模块，不必继承此接口
class IFramework;
class IInterface
{
public:
	virtual void release() { delete this; }
	virtual bool preInit(IFramework* framework) { return true; }//preInit阶段执行基本的内部初始化逻辑。
};

//模块实例化函数原型标准
typedef void* (__stdcall* PFN_InstanceFunc)();

//接口ID定义
#define INTERFACEID(type)	#type

//框架接口
class IFramework :public IInterface
{
public:
	virtual void* loadInterface(const char* module, const char* instanceFunc) = 0;
	virtual bool registerInterface(const char* iid, void* interfaceObj) = 0;
	virtual void* queryInterface(const char* iid) = 0;
};

#define MODULE_FRAMEWORK	"framework.dll"
#define INSTANCEFUNC_FRAMEWORK	"CreateInstance_Framework"
typedef IFramework* (__stdcall* PFN_CreateInstance_Framework)();