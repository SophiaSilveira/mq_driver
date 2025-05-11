#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
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

int list_add_entry(const char *name, int pid){
	struct process *new_node = kmalloc((sizeof(struct process)), GFP_KERNEL);
	
	if (!new_node) {
		printk(KERN_INFO "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
		
		return -1;
	}
	strcpy(new_node->name, name);
    new_node->pid = pid;
	list_add_tail(&(new_node->link), &list);

	INIT_LIST_HEAD(&new_node->list_m);
	
	return 0;
}

void list_show(void)
{
	struct process *entry = NULL;
	struct message_s *msg = NULL;

	printk(KERN_INFO "\nLista de Processos:\n");

	list_for_each_entry(entry, &list, link) {
		printk(KERN_INFO "Message #%d: %s\n", entry->pid, entry->name);

		if (list_empty(&entry->list_m)) {
            printk(KERN_INFO "  (Sem mensagens)\n");
        } else {
            list_for_each_entry(msg, &entry->list_m, link) {
                printk(KERN_INFO "  Mensagem: %s\n", msg->message);
            }
        }
	}
}

struct process * list_name_exist(const char *name){
    struct process *entry;

    if (list_empty(&list)) {
        return NULL;
    }

    list_for_each_entry(entry, &list, link) {
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    return NULL;
}

int list_add_msg_entry(const char *name, const char *data){
	struct process *target;
	struct message_s *new_node;

	target = list_name_exist(name);

	printk(KERN_INFO "\n\n ALVO %s\n", target->name);

	if (!target) {
        printk(KERN_INFO "MQ_Driver: Target process not found for name: %s\n", name);
        return -1;
    }

	if (!data) {
   		printk(KERN_ERR "Dados da mensagem são NULL, abortando.\n");
    	return -1;
	}

	new_node = kmalloc((sizeof(struct message_s)), GFP_KERNEL);
		if (!new_node) {
		printk(KERN_INFO "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
		
		return -1;
	}

	new_node->message = kmalloc(strlen(data) + 1, GFP_KERNEL);
	if (!new_node->message) {
		printk(KERN_INFO "Memory allocation failed for message string\n");
		kfree(new_node);  // libera a struct, já que a string falhou
		return -1;
	}

	strcpy(new_node->message, data);
	list_add_tail(&(new_node->link), &target->list_m);

	return 0;
}