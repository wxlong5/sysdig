//
// Created by root on 2020/6/2.
//

#include "eventChoose.h"
#include "event.h"

/**
 * get request header  header.
 */
void getRequestHeader(sinsp_evt* evt, sinsp_threadinfo* evThreadInfo, RequestInfo* requestInfo, string headStr)
{
    //根据第一行获得url 和版本号
    int sign1, sign2;
    sign1 = header.find("/");
    sign2 = headStr.find("\r\n");
    requestInfo->requestData["method"] = header.substr(0,sign1);
    requestInfo->requestData["url"] = headStr.substr(sign1,sign2-sign1-8);
    requestInfo->requestData["version"] = headStr.substr(sign2-sign1-8,8);
    //根据json获得header json
    requestInfo->requestData["json"] = headStr.substr(sign2+2,headStr.length()-sign2-2);
    //统计数据大小和时间消耗
    requestInfo->requestTime.timeNet += evThreadInfo->m_latency;
    requestInfo->reqTs = evt->get_ts();
//    requestInfo->ioBytesIn += evt->get_iosize();

}

/**
 * get response header
 * @param evt
 * @param evThreadInfo
 * @param requestInfo
 * @param headStr
 */
void getReponseHeader(sinsp_evt* evt, sinsp_threadinfo* evThreadInfo, RequestInfo* requestInfo, string headStr)
{
    //根据第一行获得状态码
    int sign;
    sign = headStr.find("\r\n");
    if (sign < 0)
    {
        return;
    }
    if(headStr.substr(0,4) != "HTTP")
    {
        return;
    }
    requestInfo->requestData["code"] = headStr.substr(9,3);
    //根据json获得header json
    requestInfo->requestData["json"] = headStr.substr(sign+2,headStr.length()-sign-2);
    //统计数据大小和时间消耗
    requestInfo->requestTime.timeNet += evThreadInfo->m_latency;
    requestInfo->resTs = evt->get_ts();
    requestInfo->inteval = requestInfo->resTs - requestInfo->reqTs + evThreadInfo->m_latency;
//    requestInfo->ioBytesOut += evt->get_iosize()

}

/**
 * 如果是请求，开始把线程加入监控列表
 * evt.is_io = true and evt.buflen > 0 and (fd.sockfamily = ip or fd.sockfamily = unix)
 * @param evt
 */
RequestInfo* requestCheck(sinsp_fdinfo_t* evfd, sinsp_threadinfo* evThreadInfo, sinsp_evt* evt, string ptid)
{

    sinsp_fdinfo_t* evfd = ev->get_fd_info();
    //第一步过滤，判断是否为socket相关event
    if(!((evt->get_info_flags() & (EF_READS_FROM_FD | EF_WRITES_TO_FD)
        && (evfd->is_ipv4_socket() || evfd->is_unix_socket()))
    {
        return;
    }

    sinsp_evt_param* dataParam;
    dataParam = evt->get_param_value_raw("data");
    //第二步过滤，event 参数是否包含buffer 数据
    if(evt->get_num_params() <= 0 || dataParam == NULL || dataParam->m_len <= 0)
    {
        return;
    }
    //第三步解析判断,buffer length 是否大于0 是否包含 request 标记
    const char *parstr;
    const char *partTuple;
    for (uint32_t i = 0; i < evt->get_num_params(); ++i) {

        if(evt->get_param_name(i) == "data")
        {
            cahr* parstr1 = evt->get_param_as_str(i,&parstr,sinsp_evt::param_fmt = PF_HEX);
//            cout << parstr1 << endl;
        }
        if(evt->get_param_name(i) == "tuple")
        {
            cahr* parTuple1 = evt->get_param_as_str(i,&partTuple,sinsp_evt::param_fmt = PF_HEX);
        }
    }

    string headStr,headSign;
    headStr = parstr;
    int n;
    n = headStr.find("\r\n");
    if ( n > 0)
    {
        headSign = headStr.substr(0,n);
    }else{
        return;
    }

    //确定是http request event,开启线程内 request 监控
    if (headSign.substr(n-8,4) == "HTTP")
    {
        //新建一个requestInfo并初始化，把线程加入到监控队列
        RequestInfo* requestInfo;
        requestInfo->pidAndtid = ptid;
        requestInfo->parseStat = 0;
        pidAndtids.insert(requestInfo);
        //解析request
        getRequestHeader(evt, evThreadInfo, requestInfo, headStr);
    }else{
        return NULL;
    }
    return requestInfo;
}

/**
 * 监控线程，统计请求处理时间
 * @param evt
 */
void requestMonitor(sinsp_threadinfo* evThreadInfo,RequestInfo* requestInfo, sinsp_evt* evt, string ptid)
{
    bool isin = false;

    uint32_t j;
    uint32_t npidAndtid = (uint32_t)pidAndtids.size();

    if(requestInfo == NULL)
    {
        return;
    }
    for(j = 0; j < npidAndtid; j++)
    {
       if(pidAndtids[j].compare(ptid))
       {
           isin = true;
           break;
       }
    }
    if(!isin)
    {
        return;
    }
    //开始记录request解析过程
    distributeType = distributeTypeChoose(evt);
    switch ()
    {
        case DT_REQUEST:
            getResponseHeader();
        case DT_NET:
            if(evt->get_category() == EC_IO_READ )
            {
                requestInfo->ioBytesIn += evt->get_iosize();
            } else if (evt->get_category() == EC_IO_WRITE)
            {
                requestInfo->ioBytesOut += evt->get_iosize();
            }
            requestInfo->requestTime.timeNet += evThreadInfo->m_latency;
            break;
        case DT_FILE:
            requestInfo->requestTime.timeFile += evThreadInfo->m_latency;
            break;
        case DT_PROCESS:
        case DT_OTHER:
            requestInfo->requestTime.timeProcess += evThreadInfo->m_latency;
            break;
        case DT_ERROR:
            break;
        default:
    }

}

/**
 * 统计网络类信息
 */
void monitorNetInfo(sinsp_fdinfo_t* evfd, sinsp_threadinfo* evThreadInfo, sinsp_evt* evt)
{
    bool isin = false;
    for(int i = 0; i < netInfos.size(); i++)
    {
        NetInfo* netInfo = netInfos[i];
        if(netInfo->processMsg.pid == evThreadInfo->m_pid)
        {
            //存在，累加值
            isin = true;
            if(evt->get_category() == EC_IO_READ )
            {
                netInfo->ioBytesIn += evt->get_iosize();
            } else if (evt->get_category() == EC_IO_WRITE)
            {
                fileInfo->ioBytesOut += evt->get_iosize();
            }
            break;
        }
    }
    //不存在，新建
    if(!isin)
    {
        NetInfo* netInfoNew;
        ProcessMsg* processMsg;
        processMsg->pid  = evThreadInfo->m_pid;
        netInfoNew->processMsg = processMsg;
        if(evt->get_category() == EC_IO_READ )
        {
            netInfoNew->ioBytesIn = evt->get_iosize();
        } else if (evt->get_category() == EC_IO_WRITE)
        {
            netInfoNew->ioBytesOut += evt->get_iosize();
        }

        netInfos.add(netInfoNew);
    }

    const char *partTuple;
    for (uint32_t i = 0; i < evt->get_num_params(); ++i)
    {
        if(evt->get_param_name(i) == "tuple")
        {
            cahr* parTuple1 = evt->get_param_as_str(i,&partTuple,sinsp_evt::param_fmt = PF_HEX);
        }
    }
    evfd->is_role_server();
    evfd->m_sockinfo.m_ipv4info.m_fields.m_sip;
}

/**
 *
 * @return
 */
void monitorFileInfo(sinsp_fdinfo_t* evfd, sinsp_threadinfo* evThreadInfo, sinsp_evt* evt)
{
    bool isin = false;
    for(int i = 0; i < fileInfos.size(); i++)
    {
        FileInfo* fileInfo = fileInfos[i];
        if(fileInfo->processMsg.pid == evThreadInfo->m_pid)
        {
            //存在，累加值
            isin = true;
            if(evt->get_category() == EC_IO_READ )
            {
                fileInfo->ioBytesIn += evt->get_iosize();
                fileInfo->fileTimeIn += evThreadInfo->m_latency;
                fileInfo->iopsIn++;
            } else if (evt->get_category() == EC_IO_WRITE)
            {
                fileInfo->ioBytesOut += evt->get_iosize();
                fileInfo->fileTimeOut += evThreadInfo->m_latency;
                fileInfo->iopsOut++;
            }

            if (evt->get_type() == PPME_SYSCALL_OPEN_X)
            {
                fileInfo->openCount++;
            }
            break;
        }
    }
    //不存在，新建
    if(!isin)
    {
        FileInfo* fileInfoNew;
        ProcessMsg* processMsg;
        processMsg->pid  = evThreadInfo->m_pid;
        fileInfoNew->processMsg = processMsg;
        if(evt->get_category() == EC_IO_READ )
        {
            fileInfoNew->ioBytesIn = evt->get_iosize();
            fileInfoNew->fileTimeIn = evThreadInfo->m_latency;
            fileInfoNew->iopsIn = 1;
        } else if (evt->get_category() == EC_IO_WRITE)
        {
            fileInfoNew->ioBytesOut += evt->get_iosize();
            fileInfoNew->fileTimeOut += evThreadInfo->m_latency;
            fileInfoNew->iopsOut = 1;
        }

        if (evt->get_type() == PPME_SYSCALL_OPEN_X)
        {
            fileInfoNew->openCount = 1;
        }

        fileINfos.add(fileInfoNew);
    }
}

/**
 * event 分类处理
 * @param evt
 * @return
 */
bool EventChoose::distributeEvent(sinsp_evt* evt)
{
    uint8_t  distributeType;
    sinsp_threadinfo* evThreadInfo;
    sinsp_fdinfo_t* evfd = ev->get_fd_info();
    RequestInfo* requestInfo;
    string ptid;                  //pid:tid
    char pid[10],tid[10];
    if(evThreadInfo != NULL) {
        //获取进程信息
        evThreadInfo = ev->get_thread_info();
        sprintf(pid, "%d", evThreadInfo->m_pid);
        sprintf(tid, "%d", evThreadInfo->m_tid);
        ptid = (string) pid + ":" + (string) tid;
        //判断是否为request
        requestInfo = requestCheck(evThreadInfo, sinsp_evt * evt, ptid);

        //监控处理request的线程
        if (requestInfo != NULL) {
            requestMonitor(evThreadInfo,requestInfo, sinsp_evt * evt, ptid);
        }
    }
    distributeType = distributeTypeChoose(evt);
    switch ()
    {
        case DT_OTHER:
        case DT_REQUEST:
        case DT_NET:
            monitorNetInfo();
            break;
        case DT_FILE:
            monitorFileInfo();
            break;
        case DT_ERROR:
            monitorErrorInfo();
            break;
        case DT_PROCESS:
            monitorProcessInfo();
            break;
        default:
    }
    return true;
}


/**
 * event分类
 * @param evt
 * @return
 */
uint8_t distributeTypeChoose(sinsp_evt* evt)
{
    // get event`s fd info
    sinsp_fdinfo_t* evfd = ev->get_fd_info();


    if (evt->get_num_params() > 0) {
        for (uint32_t i = 0; i < evt->get_num_params(); ++i) {
            if(ev->get_param_info(i)->type == PT_ERRNO)
            {
                return DT_ERROR;
            }
        }
    }

    if((evt->get_info_flags() & (EF_READS_FROM_FD | EF_WRITES_TO_FD)
        && (evfd->is_ipv4_socket() || evfd->is_unix_socket())
    {
        return  DT_REQUEST;
    }

    if((evt->get_info_flags() & (EF_READS_FROM_FD | EF_WRITES_TO_FD)
        && (evfd->m_type == SCAP_FD_IPV4_SOCK || evfd->m_type == SCAP_FD_IPV6_SOCK))
    {
        return DT_NET;
    }

    if(evt->get_category() == EC_FILE)
    {
        return DT_FILE;
    }

    if(evt->get_category() == EC_PROCESS)
    {
        return DT_PROCESS;
    }

    return DT_OTHER;
}