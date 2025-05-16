#define NAME_P_SIZE 50

struct process {
	struct list_head link;
	int pid;
    char name[NAME_P_SIZE];
	int n_msg;
	int count_msg;
	struct list_head list_m;
};

struct message_s {
	struct list_head link;
	char *message; // char *msg;
	short size;
};

struct process * list_pid_exist(const int pid);
int list_add_entry(const char *name, int pid, int n_msg);
void list_show(void);
struct process * list_name_exist(const char *name);
int list_add_msg_entry(const char *name, const char *data, int size);
int list_add_msg_entry_all(int pid, const char *data, int size);
int list_delete_head_msg(struct process * process);
int list_delete_entry(struct process * process);
