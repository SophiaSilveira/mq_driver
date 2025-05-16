#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "list_driver.h"

extern struct list_head list;

struct process * list_pid_exist(const int pid){
    struct process *entry;

    if (list_empty(&list)) {
        return NULL;
    }

    list_for_each_entry(entry, &list, link) {
        if (entry->pid == pid) {
            return entry;
        }
    }
    return NULL;
}

int list_add_entry(const char *name, int pid, int n_msg){
	struct process *new_node = kmalloc((sizeof(struct process)), GFP_KERNEL);
	
	if (!new_node) {
		printk(KERN_INFO "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
		return -1;
	}
	strcpy(new_node->name, name);
    new_node->pid = pid;
	new_node->n_msg = n_msg;
	new_node->count_msg = 0;

	INIT_LIST_HEAD(&(new_node->list_m));
	
	list_add_tail(&(new_node->link), &list);

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
				if(msg->message == NULL) break;
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

int list_add_msg_entry(const char *name, const char *data, int size){
	struct process *target = NULL;
	struct message_s *new_node = NULL;

	target = list_name_exist(name);


	if (target == NULL) {
        printk(KERN_INFO "MQ_Driver: Target process not found for name: %s\n", name);
        return -1;
    }

	printk(KERN_INFO "\n\n ALVO %s\n", target->name);

	if (data == NULL || strlen(data) == 0) {
   		printk(KERN_ERR "Dados da mensagem são NULL, abortando.\n");
    	return -1;
	}

	new_node = kmalloc((sizeof(struct message_s)), GFP_KERNEL);
		if (!new_node) {
		printk(KERN_INFO "Memory allocation failed, this should never fail due to GFP_KERNEL flag\n");
		kfree(new_node);
		return -1;
	}

	new_node->message = kmalloc(strlen(data) + 1, GFP_KERNEL);
	if (!new_node->message) {
		printk(KERN_INFO "Memory allocation failed for message string\n");
		kfree(new_node);  // libera a struct, já que a string falhou
		return -1;
	}

	strcpy(new_node->message, data);
	new_node->size = size;

	if(target->count_msg == target->n_msg){
		list_delete_head_msg(target);
		INIT_LIST_HEAD(&new_node->link);
		list_add(&new_node->link, &target->list_m);
	}else{
		list_add_tail(&(new_node->link), &target->list_m);
		target->count_msg += 1;
	}


	return 0;
}

int list_add_msg_entry_all(int pid, const char *data, int size){
	int ret = 0;
	struct process *entry;

    if (list_empty(&list)) {
		printk(KERN_INFO "MQ_DRIVER: No process registered to send a message. Try again later\n");
        return -1;
    }

	list_for_each_entry(entry, &list, link) {
        if (entry->pid == pid) continue;
		ret = list_add_msg_entry(entry->name, data, size);
		if(ret == -1){
			printk(KERN_ERR "MQ_DRIVER_ERR: Wasn't possible to delivered the massege for %s process.", entry->name);
		}
	}

	return 0;
}

int list_delete_head_msg(struct process * process){
	struct message_s *entry = NULL;
	
	if (list_empty(&process->list_m)) {
		printk(KERN_INFO "Empty list.\n");
		
		return 1;
	}
	
	entry = list_first_entry(&process->list_m, struct message_s, link);
	
	list_del(&entry->link);
	kfree(&entry->message);
	kfree(entry);
		
	return 0;
}

int list_delete_entry(struct process * process){
	struct message_s *entry = NULL, *tmp;

	list_for_each_entry_safe(entry, tmp, &process->list_m, link) {
		if (entry->message)
			printk(KERN_INFO "Remove message: %s", entry->message);
		else
			printk(KERN_INFO "Remove message: <NULL>");

		list_del(&(entry->link));
		kfree(entry->message); // se você alocou com kmalloc
		kfree(entry);
	}

	printk(KERN_INFO "Remove PROCESS: %s", process->name);
	list_del(&process->link);
	kfree(process);

	return 0;
}
