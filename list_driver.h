#define NAME_P_SIZE 50

struct process {
	struct list_head link;
	int pid;
    char name[NAME_P_SIZE];
	short size;
};

//int list_add_entry(const char *data);
int list_pid_exist(const int pid);
int list_add_entry(const char *data, int pid);
void list_show(void);