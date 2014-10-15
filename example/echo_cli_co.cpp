/*
 * =====================================================================================
 *
 *       Filename:  echo_cli.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014年05月11日 08时16分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdlib.h>
#include <stdio.h>
#include "conet_all.h"
#include "thirdparty/gflags/gflags.h"

#include "base/incl/ip_list.h"
#include "base/incl/net_tool.h"

//using namespace conet;
DEFINE_string(server_addr, "127.0.0.1:12314", "server address");
DEFINE_int32(task_num, 10, "concurrent task num");
DEFINE_string(data_file, "1.txt", "send data file");

struct task_t
{
    std::string file;
    std::string ip;
    int port;
    conet::coroutine_t *co;
};

int g_finish_task_num=0;

int proc_send(void *arg)
{
    conet::enable_sys_hook();
    task_t *task = (task_t *)(arg);

    int ret = 0;
    int fd = 0;
    fd = conet::connect_to(task->ip.c_str(), task->port);
    conet::set_none_block(fd, false);
    char *line= NULL;
    size_t len = 0;
    char rbuff[1024];
    FILE *fp = fopen(task->file.c_str(), "r");
    if (!fp) {
        fprintf(stderr, "open file:%s failed!", task->file.c_str());
        ++g_finish_task_num;
        return -1;
    }
    while( (ret = getline(&line, &len, fp)) >= 0) {
        if (ret == 0) continue;
        ret = write(fd, line, ret);
        if (ret <= 0) break;
        ret = read(fd, rbuff, 1024);
        if (ret <=0) break;
        //write(1, rbuff, ret);
    }
    ++g_finish_task_num;
    return 0;
}

extern
task_t *tasks = NULL;

int main(int argc, char * argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, false); 

    int num = FLAGS_task_num;

    std::vector<ip_port_t> ip_list;
    parse_ip_list(FLAGS_server_addr, &ip_list);
    if (ip_list.empty()) {
        fprintf(stderr, "server_addr:%s, format error!", FLAGS_server_addr.c_str());
        return 1;
    }

    tasks = new task_t[num];
    for (int i=0; i<num; ++i) {
        tasks[i].ip = ip_list[0].ip;
        tasks[i].port = ip_list[0].port;
        tasks[i].file = FLAGS_data_file;
        tasks[i].co = conet::alloc_coroutine(proc_send, tasks+i);
        resume(tasks[i].co);
    }

    while (g_finish_task_num < FLAGS_task_num) {
        conet::dispatch();
    }

    return 0;
}

