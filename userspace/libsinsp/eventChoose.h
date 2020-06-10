//
// Created by root on 2020/6/2.
//
#pragma once
#ifndef SYSDIG_EVENTCHOOSE_H
#define SYSDIG_EVENTCHOOSE_H

#include <string>
#include <sinsp.h>

struct Tuple
{
    string      srcIp; //src ip
    string      srcPort; // src port
    string      dstIp;  // dst ip
    string      dstPort; // dst port
};

struct RoleType
{
    string      localEndpoint;   	// 本地终端
    string      localService;	 	// 本地服务
    string      remoteEndpoint;	    // 远端终端
    string      remoteService;		// 远端服务
};

struct ProcessMsg
{
    string        pid;  // proccess id
    string        ppid; // parent proccess id
    string        mtid; // main thread id
    string        name; // proccess name
    string        commandLind; //The comand which creat the proccess.
    uint32_t      pfmajor;  // pfmajor error count
    uint32_t      pfminor;  // pfminor error count
};

struct RequestData
{
    string  requestHeaderSign;
    string  requestHeaderJson;
    string  responseHeaderSign;
    string  requestHeaderJson;
};

struct RequestTime
{
    uint    timeNet;
    uint    timeFile;
    uint    timeProcess;
};

struct NetInfo
{
    Tuple         tuple;
    RoleType      roleType;
    uint32_t      ioBytesIn;
    uint32_t      ioBytesOut;
    bool          isConnectionIn;
    ProcessMsg*    processMsg;
};

struct FileInfo
{
    uint32_t        openCount;		//file.open.count
    uint64_t        ioBytesIn;		//file.bytes.in/out
    uint64_t        ioBytesOut;
    uint32_t        iopsIn;
    uint32_t        iopsOut;		// 每秒操作数，file.iops.in/out
    uint32_t        fileTimeIn;
    uint32_t        fileTimeOut;    //file.time.in/out
    ProcessMsg*		processMsg;
};

struct RequestInfo
{
    string           pidAndtid;     //线程id
    uint             parseStat;     // 解析状态
    uint             type;		    // 协议类型
    Tuple            tuple;
    RoleType         roleType;
    uint32_t         ioBytesIn;		// net.response.length
    uint32_t         ioBytesOut;
    uint64_t         reqTs;		    // 请求时间戳
    uint64_t         resTs;			// 响应时间戳
    uint64_t         inteval        // 请求耗时
    RequestData*	 map<string,string>;	// 协议详细数据
    RequestTime*     requestTime;	// 一次请求耗时
    ProcessMsg*	 	 processMsg;
};

struct ErrorInfo
{
    string            type;      //error type
    string            data;		 //error data
    processMsg*		  processMsg;
};

struct ProcessInfo
{
    string        pid;
    string        name; // proccess name
    string        ppid; // parent proccess id
    string        mtid; // main thread id
    time          startTs;	//启动时间戳
    uint32_t      startType;	//启动方式
    time          duration;	//持续时间
    bool          ifExit;		//是否消亡
    time          exitTs;		//消亡时间戳
    string        commandLine; // 进程启动命令  ,proc.commandLine
    string        name;		// 进程名 , proc.name
    double        fdUsed;		// 进程fd使用率
    uint32_t      fdlimit;	// 进程fd 限制数
    uint32_t      pfmajor;  // pfmajor error count
    uint32_t      pfminor;  // pfminor error count
};

enum distributeType
{
    DT_OTHER = 0, // default type
    DT_NET = 1,    // net type event
    DT_FILE = 2,    // file type event
    DT_REQUEST = 3, // request type event
    DT_ERROR = 4,    // error type event
    DT_PROCESS = 5,  // proccess type event
};


std::vector<string> pidAndtids;
std::vector<RequestInfo*> requests;
std::vector<NetInfo*> netInfos;
std::vector<FileInfo*> fileInfos;
std::vector<ErrorInfo*> errorInfos;
std::vector<ProcessInfo*> processInfos;
/**
 **把event 按特定条件分类：net,file,request,error,proccess,
 **每个类有特定的数据结构统计数据 。
*/
class EventChoose{

public:

    bool distributeEvent(sinsp_evt* evt);

};


#endif //SYSDIG_EVENTCHOOSE_H
