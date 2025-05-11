#include <linux/list.h>
#include <linux/slab.h>
#include "list_driver.h"

extern struct list_head list;

int list_pid_exist(const int pid){
    struct process *entry;

    if (list_empty(&list)) {
        return 0;
    }

    list_for_each_entry(entry, &list, link) {
        if (entry->pid == pid) {
            return 1;
        }
    }
    return 0;
}

int list_add_entry(const char *data, int pid){
	struct process *new_node = kmalloc((sizeof(struct process)), GFP_KERNEL);
	
	if (!new_node) {
		printk(KERN_INFO "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
		
		return -1;
	}
	strcpy(new_node->name, data);
    new_node->pid = pid;
	list_add_tail(&(new_node->link), &list);
	
	return 0;
}

void list_show(void)
{
	struct process *entry = NULL;

	printk(KERN_INFO "\nLista de Processos:\n");

	list_for_each_entry(entry, &list, link) {
		printk(KERN_INFO "Message #%d: %s\n", entry->pid, entry->name);
	}
}
