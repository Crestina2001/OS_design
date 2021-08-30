
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#include "time.h"
#include "termio.h"
//#include "bits/signum.h"
#include "signal.h"
#include "sys/time.h"

#define SYSTEM_DELAY_TIME 6000 //延迟时间
#define NULL ((void *)0)

/*======================================================================*
							文件系统
 *======================================================================*/
#define MAX_FILE_PER_LAYER 10
#define MAX_FILE_NAME_LENGTH 20
#define MAX_CONTENT_ 50
#define MAX_FILE_NUM 100

//文件ID计数器
int fileIDCount = 0;
int currentFileID = 0;

struct fileBlock
{
	int fileID;
	char fileName[MAX_FILE_NAME_LENGTH];
	int fileType; //0 for txt, 1 for folder
	char content[MAX_CONTENT_];
	int fatherID;
	int children[MAX_FILE_PER_LAYER];
	int childrenNumber;
};
struct fileBlock blocks[MAX_FILE_NUM];
int IDLog[MAX_FILE_NUM];

//文件管理主函数
void runFileManage(int fd_stdin)
{
	char rdbuf[128];
	char cmd[8];
	char filename[120];
	char buf[1024];
	int m, n;
	char _name[MAX_FILE_NAME_LENGTH];
	FSInit();
	int len = ReadDisk();
	ShowMessage();

	while (1)
	{
		for (int i = 0; i <= 127; i++)
			rdbuf[i] = '\0';
		for (int i = 0; i < MAX_FILE_NAME_LENGTH; i++)
			_name[i] = '\0';
		printf("\n/%s:", blocks[currentFileID].fileName);

		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		assert(fd_stdin == 0);

		char target[10];
		for (int i = 0; i <= 1 && i < r; i++)
		{
			target[i] = rdbuf[i];
		}
		if (rdbuf[0] == 't' && rdbuf[1] == 'o' && rdbuf[2] == 'u' && rdbuf[3] == 'c' && rdbuf[4] == 'h')
		{
			if (rdbuf[5] != ' ')
			{
				printf("You should add the filename, like \"create XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 6];
			}
			CreateFIle(_name, 0);
		}
		else if (rdbuf[0] == 'm' && rdbuf[1] == 'k' && rdbuf[2] == 'd' && rdbuf[3] == 'i' && rdbuf[4] == 'r')
		{
			if (rdbuf[5] != ' ')
			{
				printf("You should add the dirname, like \"mkdir XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			char N[MAX_FILE_NAME_LENGTH];
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 6];
			}
			CreateFIle(_name, 1);
		}
		else if (strcmp(rdbuf, "ls") == 0)
		{
			showFileList();
		}
		else if (strcmp(target, "cd") == 0)
		{
			if (rdbuf[2] == ' ' && rdbuf[3] == '.' && rdbuf[4] == '.')
			{
				ReturnFile(currentFileID);
				continue;
			}
			else if (rdbuf[2] != ' ')
			{
				printf("You should add the dirname, like \"cd XXX\".\n");
				printf("Please input [help] to know more.\n");

				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 3];
			}
			printf("name: %s\n", _name);
			int ID = SearchFile(_name);
			if (ID >= 0)
			{
				if (blocks[ID].fileType == 1)
				{
					currentFileID = ID;
					continue;
				}
				else if (blocks[ID].fileType == 0)
				{
					while (1)
					{
						printf("input the character representing the method you want to operate:"
							   "\nu --- update"
							   "\nd --- detail of the content"
							   "\nq --- quit\n");
						int r = read(fd_stdin, rdbuf, 70);
						rdbuf[r] = 0;
						if (strcmp(rdbuf, "u") == 0)
						{
							printf("input the text you want to write:\n");
							int r = read(fd_stdin, blocks[ID].content, MAX_CONTENT_);
							blocks[ID].content[r] = 0;
						}
						else if (strcmp(rdbuf, "d") == 0)
						{
							printf("--------------------------------------------"
								   "\n%s\n-------------------------------------\n",
								   blocks[ID].content);
						}
						else if (strcmp(rdbuf, "q") == 0)
						{
							printf("would you like to save the change? y/n");
							int r = read(fd_stdin, rdbuf, 70);
							rdbuf[r] = 0;
							if (strcmp(rdbuf, "y") == 0)
							{
								printf("save changes!");
							}
							else
							{
								printf("quit without changing");
							}
							break;
						}
					}
				}
			}
			else
				printf("No such file!");
		}
		else if (strcmp(target, "rm") == 0)
		{
			if (rdbuf[2] != ' ')
			{
				printf("You should add the filename or dirname, like \"rm XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 3];
			}
			int ID = SearchFile(_name);
			if (ID >= 0)
			{
				printf("Delete successfully!\n");
				DeleteFile(ID);
				for (int i = 0; i < blocks[currentFileID].childrenNumber; i++)
				{
					if (ID == blocks[currentFileID].children[i])
					{
						for (int j = i + 1; j < blocks[currentFileID].childrenNumber; j++)
						{
							blocks[currentFileID].children[i] = blocks[currentFileID].children[j];
						}
						blocks[currentFileID].childrenNumber--;
						break;
					}
				}
			}
			else
				printf("No such file!\n");
		}
		else if (strcmp(target, "sv") == 0)
		{
			WriteDisk(1000);
			printf("Save to disk successfully!\n");
		}
		else if (strcmp(rdbuf, "help") == 0)
		{
			printf("\n");
			ShowMessage();
		}
		else if (strcmp(rdbuf, "quit") == 0)
		{
			clear();
			break;
		}
		else if (!strcmp(rdbuf, "clear"))
		{
			clear();
		}
		else
		{
			printf("Sorry, there no such command in the File Manager.\n");
			printf("You can input [help] to know more.\n");
		}
	}
}

void initFileBlock(int fileID, char *fileName, int fileType)
{
	blocks[fileID].fileID = fileID;
	strcpy(blocks[fileID].fileName, fileName);
	blocks[fileID].fileType = fileType;
	blocks[fileID].fatherID = currentFileID;
	blocks[fileID].childrenNumber = 0;
}

void toStr3(char *temp, int i)
{
	if (i / 100 < 0)
		temp[0] = (char)48;
	else
		temp[0] = (char)(i / 100 + 48);
	if ((i % 100) / 10 < 0)
		temp[1] = '0';
	else
		temp[1] = (char)((i % 100) / 10 + 48);
	temp[2] = (char)(i % 10 + 48);
}

void WriteDisk(int len)
{
	char temp[MAX_FILE_NUM * 150 + 103];
	int i = 0;
	temp[i] = '^';
	i++;
	toStr3(temp + i, fileIDCount);
	i = i + 3;
	temp[i] = '^';
	i++;
	for (int j = 0; j < MAX_FILE_NUM; j++)
	{
		if (IDLog[j] == 1)
		{
			toStr3(temp + i, blocks[j].fileID);
			i = i + 3;
			temp[i] = '^';
			i++;
			for (int h = 0; h < strlen(blocks[j].fileName); h++)
			{
				temp[i + h] = blocks[j].fileName[h];
				if (blocks[j].fileName[h] == '^')
					temp[i + h] = (char)1;
			}
			i = i + strlen(blocks[j].fileName);
			temp[i] = '^';
			i++;
			temp[i] = (char)(blocks[j].fileType + 48);
			i++;
			temp[i] = '^';
			i++;
			for (int h = 0; h < strlen(blocks[j].content); h++)
			{
				temp[i + h] = blocks[j].content[h];
				if (blocks[j].content[h] == '^')
					temp[i + h] = (char)1;
			}
			i = i + strlen(blocks[j].content);
			temp[i] = '^';
			i++;
			toStr3(temp + i, blocks[j].fatherID);
			i = i + 3;
			temp[i] = '^';
			i++;
			for (int m = 0; m < MAX_FILE_PER_LAYER; m++)
			{
				toStr3(temp + i, blocks[j].children[m]);
				i = i + 3;
			}
			temp[i] = '^';
			i++;
			toStr3(temp + i, blocks[j].childrenNumber);
			i = i + 3;
			temp[i] = '^';
			i++;
		}
	}
	int fd = 0;
	int n1 = 0;
	fd = open("ss", O_RDWR);
	assert(fd != -1);
	n1 = write(fd, temp, strlen(temp));
	assert(n1 == strlen(temp));
	close(fd);
}

int toInt(char *temp)
{
	int result = 0;
	for (int i = 0; i < 3; i++)
		result = result * 10 + (int)temp[i] - 48;
	return result;
}

int ReadDisk()
{
	char bufr[1000];
	int fd = 0;
	int n1 = 0;
	fd = open("ss", O_RDWR);
	assert(fd != -1);
	n1 = read(fd, bufr, 1000);
	assert(n1 == 1000);
	bufr[n1] = 0;
	int r = 1;
	fileIDCount = toInt(bufr + r);
	r = r + 4;
	for (int i = 0; i < fileIDCount; i++)
	{
		int ID = toInt(bufr + r);
		IDLog[ID] = 1;
		blocks[ID].fileID = ID;
		r = r + 4;
		for (int i = 0; i < MAX_FILE_NAME_LENGTH; i++)
		{
			if (bufr[r] == '^')
				break;
			else if (bufr[r] == (char)1)
				blocks[ID].fileName[i] = '^';
			else
				blocks[ID].fileName[i] = bufr[r];
			r++;
		}
		r++;
		blocks[ID].fileType = (int)bufr[r] - 48;
		r = r + 2;
		for (int j = 0; j < MAX_CONTENT_; j++)
		{
			if (bufr[r] == '^')
				break;
			else if (bufr[r] == (char)1)
				blocks[ID].content[j] = '^';
			else
				blocks[ID].content[j] = bufr[r];
			r++;
		}
		r++;
		blocks[ID].fatherID = toInt(bufr + r);
		r = r + 4;
		for (int j = 0; j < MAX_FILE_PER_LAYER; j++)
		{
			blocks[ID].children[j] = toInt(bufr + r);
			r = r + 3;
		}
		r++;
		blocks[ID].childrenNumber = toInt(bufr + r);
		r = r + 4;
	}
	return n1;
}

void FSInit()
{

	for (int i = 0; i < MAX_FILE_NUM; i++)
	{
		blocks[i].childrenNumber = 0;
		blocks[i].fileID = -2;
		IDLog[i] = '\0';
	}
	IDLog[0] = 1;
	blocks[0].fileID = 0;
	strcpy(blocks[0].fileName, "home");
	strcpy(blocks[0].content, "welcome to use file system!");
	blocks[0].fileType = 2;
	blocks[0].fatherID = 0;
	blocks[0].childrenNumber = 0;
	currentFileID = 0;
	fileIDCount = 1;
}

int CreateFIle(char *fileName, int fileType)
{
	if (blocks[currentFileID].childrenNumber == MAX_FILE_PER_LAYER)
	{
		printf("Sorry you cannot add more files in this layer.\n");
		return 0;
	}
	else
	{
		for (int i = 0; i < blocks[currentFileID].childrenNumber; i++)
		{
			if (strcmp(blocks[blocks[currentFileID].children[i]].fileName, fileName) == 0)
			{
				if (fileType)
				{
					printf("You have a folder of same name!\n");
				}
				else
				{
					printf("You have a file of same name!\n");
				}
				return 0;
			}
		}
		fileIDCount++;
		int target = 0;
		for (int i = 0; i < MAX_FILE_NUM; i++)
		{
			if (IDLog[i] == 0)
			{
				target = i;
				break;
			}
		}
		initFileBlock(target, fileName, fileType);
		blocks[currentFileID].children[blocks[currentFileID].childrenNumber] = target;
		blocks[currentFileID].childrenNumber++;
		if (fileType)
		{
			printf("Create directory %s successful!\n", fileName);
		}
		else
		{
			printf("Create file %s successful!\n", fileName);
		}
		IDLog[target] = 1;
		return 1;
	}
}

void showFileList()
{
	printf("The elements in %s.\n", blocks[currentFileID].fileName); //通过currentFileID获取当前路径s

	printf("-----------------------------------------\n");
	printf("  filename |    type   | id  \n");
	for (int i = 0; i < blocks[currentFileID].childrenNumber; i++)
	{ //遍历每个孩子
		printf("%10s", blocks[blocks[currentFileID].children[i]].fileName);
		if (blocks[blocks[currentFileID].children[i]].fileType == 0)
		{
			printf(" | .txt file |");
		}
		else
		{
			printf(" |   folder  |");
		}
		printf("%3d\n", blocks[blocks[currentFileID].children[i]].fileID);
	}
	printf("-----------------------------------------\n");
}

int SearchFile(char *name)
{
	for (int i = 0; i < blocks[currentFileID].childrenNumber; i++)
	{
		if (strcmp(name, blocks[blocks[currentFileID].children[i]].fileName) == 0)
		{
			return blocks[currentFileID].children[i];
		}
	}
	return -2;
}

void ReturnFile(int ID)
{
	currentFileID = blocks[ID].fatherID;
}

void DeleteFile(int ID)
{
	if (blocks[ID].childrenNumber > 0)
	{
		for (int i = 0; i < blocks[ID].childrenNumber; i++)
		{
			DeleteFile(blocks[blocks[ID].children[i]].fileID);
		}
	}
	IDLog[ID] = 0;
	blocks[ID].fileID = -2;
	blocks[ID].childrenNumber = 0;
	for (int i = 0; i < MAX_CONTENT_; i++)
		blocks[ID].content[i] = '\0';
	for (int i = 0; i < MAX_FILE_NAME_LENGTH; i++)
		blocks[ID].fileName[i] = '\0';
	blocks[ID].fileType = -1;
	for (int i = 0; i < MAX_FILE_PER_LAYER; i++)
		blocks[ID].children[i] = -1;
	blocks[ID].fatherID = -2;
	fileIDCount--;
}

void ShowMessage()
{
	printf("      ====================================================================\n");
	printf("      #                            Welcome to                  ******    #\n");
	printf("      #                      unknownOS ~ File Manager          **        #\n");
	printf("      #                                                        ******    #\n");
	printf("      #                                                        **        #\n");
	printf("      #         [COMMAND]                 [FUNCTION]           **        #\n");
	printf("      #                                                                  #\n");
	printf("      #     $ touch [filename]  |   create a new .txt file               #\n");
	printf("      #     $ mkdir [dirname]   |   create a new folder                  #\n");
	printf("      #     $ ls                |   list the elements in this level      #\n");
	printf("      #     $ cd [dirname]      |   switch work path to this directory   #\n");
	printf("      #     $ cd ..             |   return to the superior directory     #\n");
	printf("      #     $ rm [name]         |   delete a file or directory           #\n");
	printf("      #     $ help              |   show command list of this system     #\n");
	printf("      #     $ clear             |   clear the cmd                        #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #               Powered by doubleZ, budi, flyingfish               #\n");
	printf("      #                       ALL RIGHT REVERSED                         #\n");
	printf("      ====================================================================\n");

	printf("\n\n");
}

/*****************************************************************************
 *                                processManager
 *****************************************************************************/
//进程管理主函数
void runProcessManage(int fd_stdin)
{
	clear();
	char readbuffer[128];
	showProcessWelcome();
	while (1)
	{
		printf("unknownOS ~ process-manager: $ ");

		int end = read(fd_stdin, readbuffer, 70);
		readbuffer[end] = 0;
		int i = 0, j = 0;
		//获得指令
		char cmd[20] = {0};
		while (readbuffer[i] != ' ' && readbuffer[i] != 0)
		{
			cmd[i] = readbuffer[i];
			i++;
		}
		i++;
		//获取目标
		char target[20] = {0};
		while (readbuffer[i] != ' ' && readbuffer[i] != 0)
		{
			target[j] = readbuffer[i];
			i++;
			j++;
		}
		//结束进程
		if (strcmp(cmd, "kill") == 0)
		{
			killProcess(target);
			continue;
		}
		//暂停进程
		if (strcmp(cmd, "pause") == 0)
		{
			pauseProcess(target);
			continue;
		}
		//启动进程
		else if (strcmp(cmd, "start") == 0)
		{
			restartProcess(target);
			continue;
		}
		//帮助
		else if (strcmp(readbuffer, "help") == 0)
		{
			clear();
			showProcessWelcome();
		}
		//打印全部进程
		else if (strcmp(readbuffer, "show") == 0)
		{
			showProcess();
		}
		//退出进程管理
		else if (strcmp(readbuffer, "exit") == 0)
		{
			clear();
			CommandList();
			break;
		}
		else if (!strcmp(readbuffer, "clear"))
		{
			clear();
		}
		//错误命令提示
		else
		{
			printf("Sorry, there is no such command in the Process Manager.\n");
			printf("You can input [help] to know more.\n");
			printf("\n");
		}
	}
}

//打印欢迎界面
void showProcessWelcome()
{
	printf("      ====================================================================\n");
	printf("      #                                  <COMMAND --- process>           #\n");
	printf("      #    procprocpro                                                   #\n");
	printf("      #   procprocprocpr                                                 #\n");
	printf("      #  pro          proc      $ show         |    show all process     #\n");
	printf("      #                 pro                                              #\n");
	printf("      #                 pro     $ kill [id]    |    kill process         #\n");
	printf("      #                pro                                               #\n");
	printf("      #               pro       $ start [id]   |    start process        #\n");
	printf("      #              pro                                                 #\n");
	printf("      #             pro         $ help         |    show command list    #\n");
	printf("      #            pro                                                   #\n");
	printf("      #           pro           $ exit         |    exit system          #\n");
	printf("      #                                                                  #\n");
	printf("      #           pro           $ clear        |    clear  cmd           #\n");
	printf("      #            pro                                                   #\n");
	printf("      #                         $ pause        |    pause process        #\n");
	printf("      #                                                                  #\n");
	printf("      #                   By hms, shenbo, xds, ysx, zby                  #\n");
	printf("      #                      ===  ======  ===  ===  ===                  #\n");
	printf("      ====================================================================\n");

	printf("\n\n");
}

int getMag(int n)
{
	int mag = 1;
	for (int i = 0; i < n; i++)
	{
		mag = mag * 10;
	}
	return mag;
}

//计算进程id
int getPid(char str[])
{
	int length = 0;
	for (; length < MAX_FILENAME_LEN; length++)
	{
		if (str[length] == '\0')
			break;
	}
	int pid = 0;
	for (int i = 0; i < length; i++)
	{
		if (str[i] - '0' > -1 && str[i] - '9' < 1)
		{
			pid = pid + (str[i] + 1 - '1') * getMag(length - 1 - i);
		}
		else
		{
			pid = -1;
			break;
		}
	}
	return pid;
}

//结束进程
void killProcess(char str[])
{
	int pid = getPid(str);
	if (pid >= NR_TASKS + NR_PROCS || pid < 0)
		printf("The pid exceeded the range\n");
	else if (pid >= 0 && pid < NR_TASKS)
		printf("system process %d can not be killed \n", pid);
	else if (pid < NR_TASKS + NR_PROCS)
	{
		if (proc_table[pid].p_flags != 1)
		{
			if (pid == 4 || pid == 6)
			{
				printf("process %d can not be killed \n", pid);
			}
			else
			{
				proc_table[pid].p_flags = 1;
				printf("process %d is killed \n", pid);
			}
		}
		else
			printf("process %d can not be found \n", pid);
	}
	else
		printf("process %d can not be found \n", pid);
	showProcess();
}

//暂停进程
void pauseProcess(char str[])
{
	int pid = getPid(str);
	if (pid >= NR_TASKS + NR_PROCS || pid < 0)
		printf("The pid exceeded the range\n");
	else if (pid >= 0 && pid < NR_TASKS)
		printf("system process %d can not be paused \n", pid);
	else if (pid < NR_TASKS + NR_PROCS)
	{
		if (proc_table[pid].p_flags != 1)
		{
			if (pid == 4 || pid == 6)
			{
				printf("process %d can not be paused \n", pid);
			}
			else
			{
				proc_table[pid].priority = 0;
				printf("process %d is paused \n", pid);
			}
		}
		else
			printf("process %d can not be found \n", pid);
	}
	else
		printf("process %d can not be found \n", pid);
	showProcess();
}

//启动进程
void restartProcess(char str[])
{
	int pid = getPid(str);
	if (pid >= NR_TASKS + NR_PROCS || pid < 0)
		printf("The pid exceeded the range\n");
	else if (pid >= 0 && pid < NR_TASKS)
		printf("system process %d is already running \n", pid);
	else if (pid < NR_TASKS + NR_PROCS)
	{
		if (proc_table[pid].p_flags != 1)
		{
			if (proc_table[pid].priority != 0)
				printf("process %d is already running \n", pid);
			else
			{
				proc_table[pid].priority = 1;
				printf("process %d is running \n", pid);
			}
		}
		else
			printf("process %d can not be found \n", pid);
	}
	else
		printf("process %d can not be found \n", pid);
	showProcess();
}

//打印所有进程
void showProcess()
{
	int i;
	//进程号，进程名，优先级，是否在运行
	printf("===============================================================================\n");
	printf("    ProcessID        $        ProcessName    $    Priority    $    RunningState \n");
	printf("-------------------------------------------------------------------------------\n");
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) //逐个遍历
	{
		if (proc_table[i].p_flags == 1)
			continue;
		printf("        %d                 %15s            %2d", proc_table[i].pid, proc_table[i].name, proc_table[i].priority);
		if (proc_table[i].priority != 0)
			printf("                   yes\n");
		else
			printf("                   no\n");
	}
	printf("===============================================================================\n\n");
}

/*======================================================================*
							kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task *p_task;
	struct proc *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	u8 privilege;
	u8 rpl;
	int eflags;
	int i, j;
	int prio;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++)
	{
		if (i < NR_TASKS)
		{ /* 任务 */
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio = 15;
		}
		else
		{ /* 用户进程 */
			p_task = user_proc_table + (i - NR_TASKS);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x202; /* IF=1, bit 2 is always 1 */
			prio = 5;
		}

		strcpy(p_proc->name, p_task->name); /* name of the process */
		p_proc->pid = i;					/* pid */

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	restart();

	while (1)
	{
	}
}

/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

void runCalculator(int fd_stdin);  //declaration

/*======================================================================*
							   Terminal
 *======================================================================*/
void Terminal()
{
	int fd;
	int i, n;

	char tty_name[] = "/dev_tty0";

	char rdbuf[128];
	char command3[100], command4[100], command5[100];

	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	//	char filename[MAX_FILENAME_LEN+1] = "zsp01";
	const char bufw[80] = {0};

	clear();

	/*================================= booting pictures ===+++++============================*/
	Booting();
	clear();
	/*================================= system main menu ========+++++=======================*/

	CommandList();
	
	while (1)
	{
		printf("unknown_user@unknownOS: $ ");

		memset(command3, 0, sizeof(command3));
		memset(command4, 0, sizeof(command4));
		memset(command5, 0, sizeof(command5));

		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		mystrncpy(command3, rdbuf, 3);
		mystrncpy(command4, rdbuf, 4);
		mystrncpy(command5, rdbuf, 5);

		if (!strcmp(command4, "menu"))
		{
			clear();
			CommandList();
		}
		else if (!strcmp(command5, "clear"))
		{
			clear();
		}
		else if (!strcmp(command5, "maths"))
		{
			runCalculator(fd_stdin);
		}
		else if (!strcmp(command3, "how"))
		{
			if (strlen(rdbuf) > 4)
			{
				howMain(rdbuf + 4);
			}
			else
			{
				char *str = "NULL";
				howMain(str);
			}

			continue;
		}
		else if (!strcmp(command4, "play"))
		{
			if (strlen(rdbuf) > 5)
			{
				gameMain(rdbuf + 5, fd_stdin, fd_stdout);
			}
			else
			{
				char *str = "NULL";
				gameMain(str, fd_stdin, fd_stdout);
			}
			continue;
		}
		else if (!strcmp(command3, "cal"))
		{
			if (strlen(rdbuf) > 4)
			{
				calMain(rdbuf + 4);
			}
			else
			{
				char *str = "NULL";
				calMain(str);
			}
			continue;
		}
		// else if (!strcmp(rdbuf, "order"))
		// {
		// 	clear();
		// 	runKFC(fd_stdin);
		// }
		else if (!strcmp(rdbuf, "process"))
		{
			clear();
			runProcessManage(fd_stdin);
		}
		else if (!strcmp(rdbuf, "file"))
		{
			clear();
			runFileManage(fd_stdin);
		}
		else if (!strcmp(rdbuf, ""))
		{
			continue;
		}
		else
		{
			clear();
			/*================================= 这里是命令不存在的提示信息 ===============================*/
			NotFound();
		}
	}
}

/*======================================================================*
							   AnotherFileSys
 *======================================================================*/
void AnotherFileSys()
{
	char tty_name[] = "/dev_tty1";
	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[8];
	char filename[120];
	char buf[1024];
	int m, n;
	char _name[MAX_FILE_NAME_LENGTH];
	FSInit();
	int len = ReadDisk();
	ShowMessage();

	while (1)
	{
		for (int i = 0; i <= 127; i++)
			rdbuf[i] = '\0';
		for (int i = 0; i < MAX_FILE_NAME_LENGTH; i++)
			_name[i] = '\0';
		printf("\n/%s:", blocks[currentFileID].fileName);

		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		assert(fd_stdin == 0);
		char target[10];
		for (int i = 0; i <= 1 && i < r; i++)
		{
			target[i] = rdbuf[i];
		}
		if (rdbuf[0] == 't' && rdbuf[1] == 'o' && rdbuf[2] == 'u' && rdbuf[3] == 'c' && rdbuf[4] == 'h')
		{
			if (rdbuf[5] != ' ')
			{
				printf("You should add the filename, like \"create XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 6];
			}
			CreateFIle(_name, 0);
		}
		else if (rdbuf[0] == 'm' && rdbuf[1] == 'k' && rdbuf[2] == 'd' && rdbuf[3] == 'i' && rdbuf[4] == 'r')
		{
			if (rdbuf[5] != ' ')
			{
				printf("You should add the dirname, like \"mkdir XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			char N[MAX_FILE_NAME_LENGTH];
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 6];
			}
			CreateFIle(_name, 1);
		}
		else if (strcmp(target, "ls") == 0)
		{
			showFileList();
		}
		else if (strcmp(target, "cd") == 0)
		{
			if (rdbuf[2] == ' ' && rdbuf[3] == '.' && rdbuf[4] == '.')
			{
				ReturnFile(currentFileID);
				continue;
			}
			else if (rdbuf[2] != ' ')
			{
				printf("You should add the dirname, like \"cd XXX\".\n");
				printf("Please input [help] to know more.\n");

				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 3];
			}
			printf("name: %s\n", _name);
			int ID = SearchFile(_name);
			if (ID >= 0)
			{
				if (blocks[ID].fileType == 1)
				{
					currentFileID = ID;
					continue;
				}
				else if (blocks[ID].fileType == 0)
				{
					while (1)
					{
						printf("input the character representing the method you want to operate:"
							   "\nu --- update"
							   "\nd --- detail of the content"
							   "\nq --- quit\n");
						int r = read(fd_stdin, rdbuf, 70);
						rdbuf[r] = 0;
						if (strcmp(rdbuf, "u") == 0)
						{
							printf("input the text you want to write:\n");
							int r = read(fd_stdin, blocks[ID].content, MAX_CONTENT_);
							blocks[ID].content[r] = 0;
						}
						else if (strcmp(rdbuf, "d") == 0)
						{
							printf("--------------------------------------------"
								   "\n%s\n-------------------------------------\n",
								   blocks[ID].content);
						}
						else if (strcmp(rdbuf, "q") == 0)
						{
							printf("would you like to save the change? y/n");
							int r = read(fd_stdin, rdbuf, 70);
							rdbuf[r] = 0;
							if (strcmp(rdbuf, "y") == 0)
							{
								printf("save changes!");
							}
							else
							{
								printf("quit without changing");
							}
							break;
						}
					}
				}
			}
			else
				printf("No such file!");
		}
		else if (strcmp(target, "rm") == 0)
		{
			if (rdbuf[2] != ' ')
			{
				printf("You should add the filename or dirname, like \"rm XXX\".\n");
				printf("Please input [help] to know more.\n");
				continue;
			}
			for (int i = 0; i < MAX_FILE_NAME_LENGTH && i < r - 3; i++)
			{
				_name[i] = rdbuf[i + 3];
			}
			int ID = SearchFile(_name);
			if (ID >= 0)
			{
				printf("Delete successfully!\n");
				DeleteFile(ID);
				for (int i = 0; i < blocks[currentFileID].childrenNumber; i++)
				{
					if (ID == blocks[currentFileID].children[i])
					{
						for (int j = i + 1; j < blocks[currentFileID].childrenNumber; j++)
						{
							blocks[currentFileID].children[i] = blocks[currentFileID].children[j];
						}
						blocks[currentFileID].childrenNumber--;
						break;
					}
				}
			}
			else
				printf("No such file!\n");
		}
		else if (strcmp(target, "sv") == 0)
		{
			WriteDisk(1000);
			printf("Save to disk successfully!\n");
		}
		else if (strcmp(rdbuf, "help") == 0)
		{
			printf("\n");
			ShowMessage(fd_stdin);
		}
		else if (strcmp(rdbuf, "quit") == 0)
		{
			printf("You cannot quit File Manager in this mode.\n");
		}
		else if (!strcmp(rdbuf, "clear"))
		{
			for (int i = 0; i < 30; ++i)
			{
				printf("\n");
			}
		}
		else
		{
			printf("Sorry, there no such command in the File Manager.\n");
			printf("You can input [help] to know more.\n");
		}
	}
}

/*****************************************************************************
 *                                Test
 *****************************************************************************/
void Test()
{
	spin("Test");
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char *)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void clear()
{
	clear_screen(0, console_table[current_console].cursor);
	console_table[current_console].crtc_start = 0;
	console_table[current_console].cursor = 0;
}

void mystrncpy(char *dest, char *src, int len)
{
	assert(dest != NULL && src != NULL);

	char *temp = dest;
	int i = 0;
	while (i++ < len && (*temp++ = *src++) != '\0')
		;

	if (*(temp) != '\0')
	{
		*temp = '\0';
	}
}

/*开机动画*/
void Booting()
{
	emptyWindow();
	gradualBoot();
}

void emptyWindow()
{
	printf("      ====================================================================\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      ====================================================================\n");

	milli_delay(SYSTEM_DELAY_TIME * 5);
	clear();
}

void gradualBoot()
{

	printf("      ====================================================================\n");
	printf("      #                                                                  #\n");
	printf("      #         _   _     _       _                                      #\n");
	printf("      #        | |_| |__ (_)___  (_)___                                  #\n");
	printf("      #        | __| '_ \\| / __| | / __|                                 #\n");
	printf("      #        | |_| | | | \\__ \\ | \\__ \\                                 #\n");
	printf("      #         \\__|_| |_|_|___/ |_|___/                                 #\n");
	printf("      #              _                   _            _                  #\n");
	printf("      #             / /\\                /\\ \\         / /\\                #\n");
	printf("      #            / /  \\              /  \\ \\       / /  \\               #\n");
	printf("      #           / / /\\ \\___         / /\\ \\ \\     / / /\\ \\__            #\n");
	printf("      #          / / /\\ \\__  /\\      / / /\\ \\ \\   / / /\\ \\___\\           #\n");
	printf("      #         /_/ /  \\__/ / /     / / /  \\ \\_\\  \\ \\ \\ \\/___/           #\n");
	printf("      #         \\ \\ \\    /_/ /     / / /   / / /   \\ \\ \\                 #\n");
	printf("      #          \\_\\/    \\ \\ \\    / / /   / / /_    \\ \\ \\                #\n");
	printf("      #                   \\_\\/_  / / /___/ / //_/\\__/ / /                #\n");
	printf("      #                     /_/\\/ / /____\\/ / \\ \\/___/ /                 #\n");
	printf("      #                     \\_\\/\\/_________/   \\_____\\/                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      ====================================================================\n");

	milli_delay(SYSTEM_DELAY_TIME * 10);
	clear();
}

/*所有指令 & help窗口*/
void CommandList()
{
	printf("      ====================================================================\n");
	printf("      #                                          Welcome to                 #\n");
	printf("      #       _                                  unknownOS                  #\n");
	printf("      #      / /\\                                                          #\n");
	printf("      #     / /  \\                            [COMMAND LIST]               #\n");
	printf("      #    / / /\\ \\___              $ menu --- show the command list      #\n");
	printf("      #   / / /\\ \\__  /\\            $ clear --- clear the cmd            #\n");
	printf("      #  /_/ /  \\__/ / /            $ how [command]                        #\n");
	printf("      #  \\ \\ \\    /_/ /                  --- know more about the command #\n");
	printf("      #   \\_\\/    \\ \\ \\             $ play [-option]                   #\n");
	printf("      #            \\_\\/_                 --- play the built-in game       #\n");
	printf("      #              /_/\\           $ cal [YYYY/MM]                        #\n");
	printf("      #              \\_\\/               --- display a calendar            #\n");
	printf("      #                             $ maths --- simplified calculator       #\n");
	printf("      #                             $ process --- process manager           #\n");
	printf("      #                             $ file --- file manager                 #\n");
	printf("      #                                                                     #\n");
	printf("      #                                                                     #\n");
	printf("      #                                                                     #\n");
	printf("      #                                                                     #\n");
	printf("      #                                                                     #\n");
	printf("      ====================================================================\n");

	printf("\n\n");
}

/*没找到该指令窗口*/
void NotFound()
{
	printf("      ====================================================================\n");
	printf("      #                   __                             ____  _____     #\n");
	printf("      #      __  ______  / /______  ____ _      ______  / __ \\/ ___/     #\n");
	printf("      #     / / / / __ \\/ //_/ __ \\/ __ \\ | /| / / __ \\/ / / /\\__ \\      #\n");
	printf("      #    / /_/ / / / / ,< / / / / /_/ / |/ |/ / / / / /_/ /___/ /      #\n");
	printf("      #    \\__,_/_/ /_/_/|_/_/ /_/\\____/|__/|__/_/ /_/\\____//____/       #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                               SORRY                              #\n");
	printf("      #                                                                  #\n");
	printf("      #                                but                               #\n");
	printf("      #                     Your command is UNKNOWN                      #\n");
	printf("      #                 Input [menu] to go back to menu.                 #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                                                                  #\n");
	printf("      #                   By hms, shenbo, xds, ysx, zby                  #\n");
	printf("      #                      ===  ======  ===  ===  ===                  #\n");
	printf("      ====================================================================\n");
	printf("\n\n");
}

void howMain(char *option)
{
	if (!strcmp(option, "NULL"))
	{
		printf("Sorry, you should add an option.\n");
	}
	else if (!strcmp(option, "whats"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                                                  #\n");
		printf("      #    whawhawhawh                  <COMMAND --- whats>                #\n");
		printf("      #   whawhawhawhawh                                                 #\n");
		printf("      #  wha          what                                               #\n");
		printf("      #                 wha           Type math expression to                                 #\n");
		printf("      #                 wha              calculate values                                #\n");
		printf("      #                wha                                               #\n");
		printf("      #               wha                                                #\n");
		printf("      #              wha                                                 #\n");
		printf("      #             wha                                                  #\n");
		printf("      #            wha                                                   #\n");
		printf("      #           wha                                                    #\n");
		printf("      #                                                                  #\n");
		printf("      #           wha                                                    #\n");
		printf("      #            wha                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "menu"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                                                  #\n");
		printf("      #    mmmmmmmmmmm                                                   #\n");
		printf("      #   mmmmmmmmmmmmmm                                                 #\n");
		printf("      #  mmm          mmmm                                               #\n");
		printf("      #                 mmm                                              #\n");
		printf("      #                 mmm                                              #\n");
		printf("      #                mmm              <COMMAND --- menu>               #\n");
		printf("      #               mmm                   Shows menu                   #\n");
		printf("      #              mmm                                                 #\n");
		printf("      #             mmm                                                  #\n");
		printf("      #            mmm                                                   #\n");
		printf("      #           mmm                                                    #\n");
		printf("      #                                                                  #\n");
		printf("      #           mmm                                                    #\n");
		printf("      #            mmm                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "play"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                  <COMMAND --- play>              #\n");
		printf("      #    ppppppppppp                                                   #\n");
		printf("      #   pppppppppppppp                                                 #\n");
		printf("      #  ppp          pppp              -2048                            #\n");
		printf("      #                 ppp                 --- Play famous 2048 game!   #\n");
		printf("      #                 ppp             -box                             #\n");
		printf("      #                ppp                  --- Play Pushbox game!       #\n");
		printf("      #               ppp                                                #\n");
		printf("      #              ppp                                                 #\n");
		printf("      #             ppp                                                  #\n");
		printf("      #            ppp       For example,                                #\n");
		printf("      #           ppp        $ play -2048                                #\n");
		printf("      #                            can access 2048 game.                 #\n");
		printf("      #           ppp                                                    #\n");
		printf("      #            ppp                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "clear"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                                                  #\n");
		printf("      #    ccccccccccc                                                   #\n");
		printf("      #   cccccccccccccc                                                 #\n");
		printf("      #  ccc          cccc                                               #\n");
		printf("      #                 ccc                                              #\n");
		printf("      #                 ccc                                              #\n");
		printf("      #                ccc              <COMMAND --- clear>              #\n");
		printf("      #               ccc                 Clears console                 #\n");
		printf("      #              ccc                                                 #\n");
		printf("      #             ccc                                                  #\n");
		printf("      #            ccc                                                   #\n");
		printf("      #           ccc                                                    #\n");
		printf("      #                                                                  #\n");
		printf("      #           ccc                                                    #\n");
		printf("      #            ccc                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "cal"))
		{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                                                  #\n");
		printf("      #    calendarcal                                                   #\n");
		printf("      #   calendarcalend                                                 #\n");
		printf("      #  cal          cale                                               #\n");
		printf("      #                 cal                                              #\n");
		printf("      #                 cal                                              #\n");
		printf("      #                cal              <COMMAND --- cal>                #\n");
		printf("      #               cal                Input [YYYY/MM]                 #\n");
		printf("      #              cal            For example, $ cal 2021/08           #\n");
		printf("      #             cal                    (Or 2021/8)                   #\n");
		printf("      #            cal            To get a month view calendar of        #\n");
		printf("      #           cal                     August, 2021                   #\n");
		printf("      #                                                                  #\n");
		printf("      #           cal                                                    #\n");
		printf("      #            cal                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "how"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                      Welcome to                  #\n");
		printf("      #                                      unknownOS                   #\n");
		printf("      #                                                                  #\n");
		printf("      #   ?                                                              #\n");
		printf("      #   ?                                                              #\n");
		printf("      #   ???? ??? ?   ?                 <COMMAND --- how>               #\n");
		printf("      #   ?  ? ? ? ? ? ?                  $ how [command]                #\n");
		printf("      #   ?  ? ? ? ?????           to see manual for the command.        #\n");
		printf("      #   ?  ? ??? ? ? ?                                                 #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                  Input [menu] to go back to menu.                #\n");
		printf("      #                                                                  #\n");
		printf("      #                   By hms, shenbo, xds, ysx, zby                  #\n");
		printf("      #                      ===  ======  ===  ===  ===                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "process"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                  <COMMAND --- process>           #\n");
		printf("      #    procprocpro                                                   #\n");
		printf("      #   procprocprocpr                                                 #\n");
		printf("      #  pro          proc                                               #\n");
		printf("      #                 pro                                              #\n");
		printf("      #                 pro                                              #\n");
		printf("      #                pro                                               #\n");
		printf("      #               pro                                                #\n");
		printf("      #              pro                                                 #\n");
		printf("      #             pro                                                  #\n");
		printf("      #            pro                                                   #\n");
		printf("      #           pro                                                    #\n");
		printf("      #                                                                  #\n");
		printf("      #           pro                                                    #\n");
		printf("      #            pro                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                   By hms, shenbo, xds, ysx, zby                  #\n");
		printf("      #                      ===  ======  ===  ===  ===                  #\n");
		printf("      ====================================================================\n");
	}
	else if (!strcmp(option, "file"))
	{
		clear();
		printf("      ====================================================================\n");
		printf("      #                                  <COMMAND --- file>              #\n");
		printf("      #    filefilefile                                                  #\n");
		printf("      #   filefilefilefi                                                 #\n");
		printf("      #  file          file                                              #\n");
		printf("      #                 file                                             #\n");
		printf("      #                 file                                             #\n");
		printf("      #                file                                              #\n");
		printf("      #               file                                               #\n");
		printf("      #              file                                                #\n");
		printf("      #             file                                                 #\n");
		printf("      #            file                                                  #\n");
		printf("      #           file                                                   #\n");
		printf("      #                                                                  #\n");
		printf("      #           file                                                   #\n");
		printf("      #            file                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                                                                  #\n");
		printf("      #                   By hms, shenbo, xds, ysx, zby                  #\n");
		printf("      #                      ===  ======  ===  ===  ===                  #\n");
		printf("      ====================================================================\n");
	}
	else
	{
		printf("Sorry, there no such option for how.\n");
		printf("Input [menu] to go back to menu.\n");
	}

	printf("\n");
}

/*======================================================================*
								Apps
 *======================================================================*/

/*======================================================================*
							Calendar
 *======================================================================*/
int Isleap(int year)
{
	if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))
		return 1;
	else
		return 0;
}

int Max_day(int year, int month)
{
	int Day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (Isleap(year) == 1)
		Day[1] = 29;
	return Day[month - 1];
}

int Total_day(int year, int month, int day)
{
	int sum = 0;
	int i = 1;
	for (i = 1; i < month; i++)
		sum = sum + Max_day(year, i);
	sum = sum + day;
	return sum;
}

int Weekday(int year, int month, int day)
{
	int count;
	count = (year - 1) + (year - 1) / 4 - (year - 1) / 100 + (year - 1) / 400 + Total_day(year, month, day);
	count = count % 7;
	return count;
}

void display_week(int year, int month, int day)
{
	int count;
	count = Weekday(year, month, day);
	switch (count)
	{
	case 0:
		printf("\n%d-%d-%d is Sunday\n", year, month, day);
		break;
	case 1:
		printf("\n%d-%d-%d is Monday\n", year, month, day);
		break;
	case 2:
		printf("\n%d-%d-%d is Tuesday\n", year, month, day);
		break;
	case 3:
		printf("\n%d-%d-%d is Wednesday\n", year, month, day);
		break;
	case 4:
		printf("\n%d-%d-%d is Thursday\n", year, month, day);
		break;
	case 5:
		printf("\n%d-%d-%d is Friday\n", year, month, day);
		break;
	case 6:
		printf("\n%d-%d-%d is Saturday\n", year, month, day);
		break;
	}
}

void display_month(int year, int month)
{
	int i = 0, j = 1;
	int week, max;
	week = Weekday(year, month, 1);
	max = Max_day(year, month);
	printf("\n                   %d/%d\n", year, month);
	printf("Sun    Mon    Tue    Wed    Thu    Fri    Sat\n");
	for (i = 0; i < week; i++)
		printf("       ");
	for (j = 1; j <= max; j++)
	{
		printf("%d     ", j);
		if (j < 10)
			printf(" ");
		if (i % 7 == 6)
			printf("\n");
		i++;
	}
	printf("\n");
}

void calMain(char *option)
{
	int year, month;
	char year_str[5] = "\0", month_str[3] = "\0";
	if (!strcmp(option, "NULL"))
	{
		printf("Sorry, you should add YYYY/MM.\n");
	}
	else
	{
		if (option[0] < '0' || option[0] > '9')
			printf("Wrong input.\n");
		else
		{
			if (strlen(option) > 0 && strlen(option) < 8)
			{
				char *value = option;
				int i = 0;
				for (int j = 0; i < strlen(value) && value[i] != '/' && value[i] != ' '; ++i, ++j)
				{
					year_str[i] = value[i];
				}
				++i;
				for (int j = 0; i < strlen(value) && value[i] != ' '; ++i, ++j)
				{
					month_str[j] = value[i];
				}
				atoi(year_str, &year);
				atoi(month_str, &month);
				if (year > 0 && month > 0 && month < 13)
				{
					display_month(year, month);
				}
				else
				{
					if (year < 0)
					{
						printf("The [year] you input should greater than 0.\n");
					}
					if (month < 1 || month > 12)
					{
						printf("The [month] you input should between 1 to 12.\n");
					}
					printf("Please input again.\n");
				}
			}
			else
			{
				printf("Sorry,wrong input. You should add [YYYY/MM].\n");
				printf("You can input [how cal] to know more.\n");
			}
		}
	}
}

/*======================================================================*
							Games
 *======================================================================*/

void gameMain(char *option, int fd_stdin, int fd_stdout)
{
	if (!strcmp(option, "NULL"))
	{
		printf("Sorry, you should add an option.\n");
	}
	else if (!strcmp(option, "-2048"))
	{
		Game_2048(fd_stdin, fd_stdout);
	}
	else if (!strcmp(option, "-box"))
	{
		Runpushbox(fd_stdin, fd_stdout);
	}
	else if (!strcmp(option,"-KFC")){
		runKFC(fd_stdin);
	}
	else
	{
		printf("Sorry, there no such option for game.\n");
		printf("You can input [man game] to know more.\n");
	}

	printf("\n");
}

/*======================================================================*
							KFC
 *======================================================================*/

void runKFC(int fd_stdin)
{
	printf("KFC is under construction. Sorry.");
}

/*======================================================================*
							2048
 *======================================================================*/


#define KEY_CODE_UP    0x41
#define KEY_CODE_DOWN  0x42
#define KEY_CODE_LEFT  0x44
#define KEY_CODE_RIGHT 0x43
#define KEY_CODE_QUIT  0x71
struct termios old_config;


static char config_path[4096] = { 0 };
static void loop_game(int fd_stdin);
static void reset_game();
static void left();
static void right();
static void up();
static void down();
static void add_random();
static void win_or_lose();
static int get_blank();
static void refresh();
static int board[4][4];
static int score;
static int if_add;
static int if_over;
static int if_exit;
static char* read_keyboard(int fd_stdin);

void Game_2048(fd_stdin, fd_stdout)
{
	clear();
	reset_game();
	loop_game(fd_stdin);
	clear();
}


void loop_game(int fd_stdin) {
	while (1) {
		char rdbuf[128];
		int r = 0;
		r = read(fd_stdin, rdbuf, 70);
		if (r > 1)
		{
			refresh();
			continue;
		}
		rdbuf[r] = 0;
		char cmd = rdbuf[0];
		if (if_exit) {
			if (cmd == 'y' || cmd == 'Y') {
				clear_screen();
				return;
			}
			else if (cmd == 'n' || cmd == 'N') {
				if_exit = 0;
				refresh();
				continue;
			}
			else {
				continue;
			}
		}

		if (if_over) {
			if (cmd == 'y' || cmd == 'Y') {

				reset_game();
				continue;
			}
			else if (cmd == 'n' || cmd == 'N') {

				clear();
				return;
			}
			else {
				continue;
			}
		}

		if_add = 0;
		switch (cmd) {
		case 'a':
			left();
			break;
		case 's':
			down();
			break;
		case 'w':
			up();
			break;
		case 'd':
			right();
			break;
		case 'q':
			if_exit = 1;
			break;
		default:
			refresh();
			continue;
		}
		if (if_add) {
			add_random();
			refresh();
		}
		else if (if_exit) {
			refresh();
		}
	}
}

void reset_game() {
	score = 0;
	if_add = 1;
	if_over = 0;
	if_exit = 0;
	int n = get_ticks() % 16;
	int i;
	for (i = 0; i < 4; ++i) {
		int j;
		for (j = 0; j < 4; ++j) {
			board[i][j] = (n-- == 0 ? 2 : 0);
		}
	}

	add_random();
	refresh();
}


void add_random() {
	int n = get_ticks() % get_blank();
	int i;
	for (i = 0; i < 4; ++i) {
		int j;
		for (j = 0; j < 4; ++j) {
			if (board[i][j] == 0 && n-- == 0) {
				board[i][j] = (get_ticks() % 10 ? 2 : 4);
				return;
			}
		}
	}
}


int get_blank() {
	int n = 0;
	int i;
	for (i = 0; i < 4; ++i) {
		int j;
		for (j = 0; j < 4; ++j) {
			board[i][j] == 0 ? ++n : 1;
		}
	}
	return n;
}


void win_or_lose() {
	int i;
	for (i = 0; i < 4; ++i) {
		int j;
		for (j = 0; j < 3; ++j) {
			if (board[i][j] == board[i][j + 1] || board[j][i] == board[j + 1][i]) {
				if_over = 0;
				return;
			}
		}
	}
	if_over = 1;
}

void left() {

	int i;
	for (i = 0; i < 4; ++i) {

		int j, k;
		for (j = 1, k = 0; j < 4; ++j) {
			if (board[i][j] > 0)
			{
				if (board[i][k] == board[i][j]) {

					score += board[i][k++] *= 2;
					board[i][j] = 0;
					if_add = 1;
				}
				else if (board[i][k] == 0) {

					board[i][k] = board[i][j];
					board[i][j] = 0;
					if_add = 1;
				}
				else {

					board[i][++k] = board[i][j];
					if (j != k) {

						board[i][j] = 0;
						if_add = 1;
					}
				}
			}
		}
	}
}


void right() {

	int i;
	for (i = 0; i < 4; ++i) {
		int j, k;
		for (j = 2, k = 3; j >= 0; --j) {
			if (board[i][j] > 0) {
				if (board[i][k] == board[i][j]) {
					score += board[i][k--] *= 2;
					board[i][j] = 0;
					if_add = 1;
				}
				else if (board[i][k] == 0) {
					board[i][k] = board[i][j];
					board[i][j] = 0;
					if_add = 1;
				}
				else {
					board[i][--k] = board[i][j];
					if (j != k) {
						board[i][j] = 0;
						if_add = 1;
					}
				}
			}
		}
	}
}


void up() {

	int i;
	for (i = 0; i < 4; ++i) {
		int j, k;
		for (j = 1, k = 0; j < 4; ++j) {
			if (board[j][i] > 0) {
				if (board[k][i] == board[j][i]) {
					score += board[k++][i] *= 2;
					board[j][i] = 0;
					if_add = 1;
				}
				else if (board[k][i] == 0) {
					board[k][i] = board[j][i];
					board[j][i] = 0;
					if_add = 1;
				}
				else {
					board[++k][i] = board[j][i];
					if (j != k) {
						board[j][i] = 0;
						if_add = 1;
					}
				}
			}
		}
	}
}

void down() {

	int i;
	for (i = 0; i < 4; ++i) {
		int j, k;
		for (j = 2, k = 3; j >= 0; --j) {
			if (board[j][i] > 0) {
				if (board[k][i] == board[j][i]) {
					score += board[k--][i] *= 2;
					board[j][i] = 0;
					if_add = 1;
				}
				else if (board[k][i] == 0) {
					board[k][i] = board[j][i];
					board[j][i] = 0;
					if_add = 1;
				}
				else {
					board[--k][i] = board[j][i];
					if (j != k) {
						board[j][i] = 0;
						if_add = 1;
					}
				}
			}
		}
	}
}

void refresh() {
	clear();

	printf("\n\n\n\n");
	printf("                                   SCORE: %05d    \n", score);
	printf("               --------------------------------------------------");
	printf("\n\n                             |****|****|****|****|\n");
	int i;
	for (i = 0; i < 4; ++i) {
		printf("                             |");
		int j;
		for (j = 0; j < 4; ++j) {
			if (board[i][j] != 0) {
				if (board[i][j] < 10) {
					printf("  %d |", board[i][j]);
				}
				else if (board[i][j] < 100) {
					printf(" %d |", board[i][j]);
				}
				else if (board[i][j] < 1000) {
					printf(" %d|", board[i][j]);
				}
				else if (board[i][j] < 10000) {
					printf("%4d|", board[i][j]);
				}
				else {
					int n = board[i][j];
					int k;
					for (k = 1; k < 20; ++k) {
						n = n >> 1;
						if (n == 1) {
							printf("2^%02d|", k);
							break;
						}
					}
				}
			}
			else printf("    |");
		}

		if (i < 3) {
			printf("\n                             |****|****|****|****|\n");
		}
		else {
			printf("\n                             |****|****|****|****|\n");
		}
	}
	printf("\n");
	printf("               --------------------------------------------------\n");
	printf("                  [W]:UP [S]:DOWN [A]:LEFT [D]:RIGHT [Q]:EXIT\n");
	printf("                  Enter your command:");

	if (get_blank() == 0) {
		win_or_lose();
		if (if_over) {
			printf("\r                      \nGAME OVER! TRY AGAIN? [Y/N]:     \b\b\b\b");
		}
	}
	if (if_exit) {
		printf("\r                   \nQUIT THE GAME? [Y/N]:   \b\b");

	}
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						 push box
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int px = 0;
int py = 0;
void draw_map(int map[9][11])
{
	int i;
	int j;
	for (i = 0; i < 9; i++)
	{
		for (int j = 0; j < 11; j++)
		{
			switch (map[i][j])
			{
			case 0:
				printf(" "); //道路
				break;
			case 1:
				printf("#"); //墙壁
				break;
			case 2:
				printf(" "); //游戏边框的空白部分
				break;
			case 3:
				printf("D"); //目的地
				break;
			case 4:
				printf("b"); //箱子
				break;
			case 7:
				printf("!"); //箱子进入目的地
				break;
			case 6:
				printf("p"); //人
				break;
			case 9:
				printf("^"); //人进入目的地
				break;
			}
		}
		printf("\n");
	}
}

void boxMenu()
{
	printf("      ================================================================\n");
		printf("      #                                       Welcome to             #\n");
		printf("      #     boxboxboxbo                      pushBoxGame             #\n");
		printf("      #   boxboxboxboxbox                                            #\n");
		printf("      #  box          boxbo                   Instrcution            #\n");
		printf("      #                 box              set: p:People b:BOX         #\n");
		printf("      #               box                     #:Wall   D:Destination #\n");
		printf("      #             box                  operation:                  #\n");
		printf("      #          box                          s:Down d:Right         #\n");
		printf("      #                                       w:Up   a:Left  q:Quit  #\n");
		printf("      #          box                                                 #\n");
		printf("      #          box                          Enter'q' to quit       #\n");
		printf("      ================================================================\n");
	printf("\n\n");
}

void Runpushbox(fd_stdin, fd_stdout)
{
	char rdbuf[128];
	int r;
	char control;

	int count = 0; //定义记分变量

	int map[9][11] = {
		{2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
		{2, 1, 0, 0, 0, 1, 0, 0, 0, 1, 2},
		{2, 1, 0, 4, 4, 4, 4, 4, 0, 1, 2},
		{2, 1, 0, 4, 0, 4, 0, 4, 0, 0, 1},
		{2, 1, 0, 0, 0, 6, 0, 0, 4, 0, 1},
		{1, 1, 0, 1, 1, 1, 1, 0, 4, 0, 1},
		{1, 0, 3, 3, 3, 3, 3, 1, 0, 0, 1},
		{1, 0, 3, 3, 3, 3, 3, 0, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2},
	};
	while (1)
	{
		clear();
		printf("\n");
		boxMenu();
		draw_map(map);
		printf("Current Score:%d\n", count);
		//找初始位置
		for (px = 0; px < 9; px++)
		{
			for (py = 0; py < 11; py++)
			{
				if (map[px][py] == 6 || map[px][py] == 9)
					break;
			}
			if (map[px][py] == 6 || map[px][py] == 9)
				break;
		}
		printf("CURRENT LOCATION (%d,%d)", px, py);

		printf("\n");
		printf("Please input direction:");

		r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		control = rdbuf[0];

		if (control == 'Q' || control == 'q')
		{
			break;
		}
		switch (control)
		{
		case 'w':
			//如果人前面是空地。
			if (map[px - 1][py] == 0)
			{
				map[px - 1][py] = 6 + 0;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			//如果人前面是目的地。
			else if ((map[px - 1][py] == 3) || (map[px - 1][py] == 9))
			{
				map[px - 1][py] = 6 + 3;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			//如果人前面是箱子。
			else if (map[px - 1][py] == 4)
			{
				if (map[px - 2][py] == 0)
				{
					map[px - 2][py] = 4;
					if (map[px - 1][py] == 7)
						map[px - 1][py] = 9;
					else
						map[px - 1][py] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				//如果人的前面是箱子，而箱子前面是目的地。
				else if (map[px - 2][py] == 3)
				{
					map[px - 2][py] = 7;
					count++;
					if (map[px - 1][py] == 7)
						map[px - 1][py] = 9;
					else
						map[px - 1][py] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			//如果人前面是已经进入某目的地的箱子
			else if (map[px - 1][py] == 7)
			{
				//如果人前面是已经进入某目的地的箱子,箱子前面是空地。
				if (map[px - 2][py] == 0)
				{
					count--;
					map[px - 2][py] = 4;
					map[px - 1][py] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				//如果人前面是已经进入某目的地的箱子，箱子前面是另一目的地。
				if (map[px - 2][py] == 3)
				{
					map[px - 2][py] = 7;
					map[px - 1][py] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			break;
		case 's':
			//如果人前面是空地。
			if (map[px + 1][py] == 0)
			{
				map[px + 1][py] = 6 + 0;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			//如果人前面是目的地。
			else if (map[px + 1][py] == 3)
			{
				map[px + 1][py] = 6 + 3;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			//如果人前面是箱子。
			else if (map[px + 1][py] == 4)
			{
				//如果人前面是箱子，而箱子前面是空地。
				if (map[px + 2][py] == 0)
				{
					map[px + 2][py] = 4;
					if (map[px + 1][py] == 7)
						map[px + 1][py] = 9;
					else
						map[px + 1][py] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				//如果人的前面是箱子，而箱子前面是目的地。
				else if (map[px + 2][py] == 3)
				{
					map[px + 2][py] = 7;
					count++;
					if (map[px + 1][py] == 7)
						map[px + 1][py] = 9;
					else
						map[px + 1][py] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			else if (map[px + 1][py] == 7)
			{
				if (map[px + 2][py] == 0)
				{
					count--;
					map[px + 2][py] = 4;
					map[px + 1][py] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				if (map[px + 2][py] == 3)
				{
					map[px + 2][py] = 7;
					map[px + 1][py] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			break;
		case 'a':
			if (map[px][py - 1] == 0)
			{
				map[px][py - 1] = 6 + 0;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			else if (map[px][py - 1] == 3)
			{
				map[px][py - 1] = 6 + 3;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			else if (map[px][py - 1] == 4)
			{
				if (map[px][py - 2] == 0)
				{
					map[px][py - 2] = 4;
					if (map[px][py - 1] == 7)
						map[px][py - 1] = 9;
					else
						map[px][py - 1] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				else if (map[px][py - 2] == 3)
				{
					count++;
					map[px][py - 2] = 7;
					if (map[px][py - 1] == 7)
						map[px][py - 1] = 9;
					else
						map[px][py - 1] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			else if (map[px][py - 1] == 7)
			{
				if (map[px][py - 2] == 0)
				{
					count--;
					map[px][py - 2] = 4;
					map[px][py - 1] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				if (map[px][py - 2] == 3)
				{
					map[px][py - 2] = 7;
					map[px][py - 1] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			break;
		case 'd':
			if (map[px][py + 1] == 0)
			{
				map[px][py + 1] = 6 + 0;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			else if (map[px][py + 1] == 3)
			{
				map[px][py + 1] = 6 + 3;
				if (map[px][py] == 9)
					map[px][py] = 3;
				else
					map[px][py] = 0;
			}
			else if (map[px][py + 1] == 4)
			{
				if (map[px][py + 2] == 0)
				{
					map[px][py + 2] = 4;
					if (map[px][py + 1] == 7)
						map[px][py + 1] = 9;
					else
						map[px][py + 1] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				else if (map[px][py + 2] == 3)
				{
					count++;
					map[px][py + 2] = 7;
					if (map[px][py + 1] == 7)
						map[px][py + 1] = 9;
					else
						map[px][py + 1] = 6;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			else if (map[px][py + 1] == 7)
			{
				if (map[px][py + 2] == 0)
				{
					count--;
					map[px][py + 2] = 4;
					map[px][py + 1] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
				if (map[px][py + 2] == 3)
				{
					map[px][py + 2] = 7;
					map[px][py + 1] = 9;
					if (map[px][py] == 9)
						map[px][py] = 3;
					else
						map[px][py] = 0;
				}
			}
			break;
		}
		if (count == 8)
		{
			draw_map(map);
			printf("\nCongratulations!!\n");
			break; //退出死循环
		}
	}
}
/*****************************************************************************
 *                                maths
 *****************************************************************************/
void runCalculator(int fd_stdin){
	    clear();
	    int a,b,p,q;
	    char rdbuf[128];
	    char valbuf1[128];
	    char valbuf2[128];
	    char control;     //operator
	    printf("Please choose operators from here : '+ - * /' \n");
	    printf("Input 'Q' or 'q' to exit.\n");
	    while(1)
	    {
	    for (int i = 0; i <= 127; i++)rdbuf[i] = '\0';
	    printf("\nEnter an operator:");
	    p = read(fd_stdin, rdbuf, 70);
		rdbuf[p] = 0;
		control = rdbuf[0];
	    if (control == 'Q' || control == 'q')
		{
			break;
		}
	    else{
		printf("Input two numbers, the first number:");
		q=read(fd_stdin,valbuf1,70);
		valbuf1[q]=0;
		if(valbuf1[0]==45)a=-(valbuf1[1]-48);
		else a=valbuf1[0]-48;
		printf("Input the second number:");
		q=read(fd_stdin,valbuf2,70);
		valbuf2[q]=0;
		if(valbuf2[0]==45)b=-(valbuf2[1]-48);
		else b=valbuf2[0]-48;
	    }
	    switch(control)
	    {
	    case '+' :
		printf("%d+%d=%d\n",a,b,a+b);
		break;
	    case '-' :
		printf("%d-%d=%d\n",a,b,a-b);
		break;
	    case '*' :
		printf("%dx%d=%d\n",a,b,a*b);
		break;
	    case '/' :
		printf("%d/%d=%d\n",a,b,a/b);
		break;
	    default:
		printf("input error\n");
	    }
	    }
}
