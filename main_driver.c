#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "list_driver.h"

#define DEVICE_NAME "mq_driver"
#define CLASS_NAME  "mq_class"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sophia Mendes da Silveira & ");
MODULE_DESCRIPTION("A very simple kernel module, with parameters.");
MODULE_VERSION("0.0.1");

//For init
static int majorNumber;
static int number_opens = 0;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

struct list_head list;

static int n_process = -1;
static int n_msg = -1;
static int s_msg = -1;

static int count_n_process = 0;

module_param(n_process, int, 0644);
module_param(n_msg, int, 0644);
module_param(s_msg, int, 0644);

static int	dev_open(struct inode *, struct file *);
static int	dev_release(struct inode *, struct file *);
static ssize_t	dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t	dev_write(struct file *, const char *, size_t, loff_t *);


static struct file_operations fops =
{
	.open = dev_open,
    .read = dev_read,
    .write = dev_write,
	.release = dev_release,
};


static int mq_init(void){

    if (n_process == -1 || n_msg == -1 || s_msg == -1) {
        printk(KERN_ALERT "MQ_Driver not loaded. Pass the numbers of process, messages and message's bytes!\n");
        printk(KERN_ALERT "Example: modprobe mq_driver n_process=1 n_msg=2 s_msg=1234\n");
        return -1;
    }

    printk(KERN_INFO "MQ_Driver: Initializing the LKM\n");

	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT "MQ_Driver failed to register a major number\n");
		return majorNumber;
	}
	
	printk(KERN_INFO "MQ_Driver: registered correctly with major number %d\n", majorNumber);

	// Register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(charClass)) {		// Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "MQ_Driver: failed to register device class\n");
		return PTR_ERR(charClass);	// Correct way to return an error on a pointer
	}
	
	printk(KERN_INFO "MQ_Driver: device class registered correctly\n");

	// Register the device driver
	charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(charDevice)) {		// Clean up if there is an error
		class_destroy(charClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "MQ_Driver: failed to create the device\n");
		return PTR_ERR(charDevice);
	}
	
	printk(KERN_INFO "MQ_Driver: device class created.\n");
	
	INIT_LIST_HEAD(&list);    

    return 0;
}

static void mq_exit(void)
{

    struct process *proc;
    struct process *tmp_proc;
    struct message_s *msg;
    struct message_s *tmp_msg;

    // Itera sobre todos os processos registrados
    list_for_each_entry_safe(proc, tmp_proc, &list, link) {

        // Libera todas as mensagens do processo
        list_for_each_entry_safe(msg, tmp_msg, &proc->list_m, link) {
            list_del(&msg->link);
            kfree(msg->message);  // Libera a string da mensagem
            kfree(msg);           // Libera o struct message_s
        }

        list_del(&proc->link);
        kfree(proc->name);  // Libera nome do processo
        kfree(proc);        // Libera o struct process
    }

    device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "MQ_Driver: mq_driver unloaded.\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	number_opens++;
	printk(KERN_INFO "MQ_Driver: device has been opened %d time(s)\n", number_opens);
	printk("Process id: %d\n", (int) task_pid_nr(current));

	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error = 0;
    int pid = (int) task_pid_nr(current);
    struct process *registered = list_pid_exist(pid);
    
	struct message_s *entry = list_first_entry(&registered->list_m, struct message_s, link);
   
	if (list_empty(&registered->list_m)) {
		printk(KERN_INFO "MQ_Driver: no data.\n");
		
		return 0;
	}	
	
	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
	error = copy_to_user(buffer, entry->message, entry->size);

	if (!error) {				// if true then have success
		printk(KERN_INFO "MQ_Driver: sent %d characters to the user\n", entry->size);
		
        if(list_delete_head_msg(registered) == 0){
            registered->count_msg -= 1;
        }

		return 0;
	} else {
		printk(KERN_INFO "MQ_Driver: failed to send %d characters to the user\n", error);
		
		return -EFAULT;			// Failed -- return a bad address message (i.e. -14)
	}
}


static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    int pid = (int) task_pid_nr(current);
    struct process *registered = list_pid_exist(pid);
    int reg = 0;
    int target_add_message = 0;

    char *buffer_copy = kstrdup(buffer, GFP_KERNEL);
    char *command, *complement;
    int command_c;
    
    if(buffer[0] == '\0') {
        kfree(buffer_copy);
        return s_msg;
    }

    if (buffer_copy == NULL) {
        printk(KERN_ERR "Memory allocation for buffer_copy failed\n");
        return -ENOMEM;
    }

    command = strsep(&buffer_copy, " ");  // Separa a string no primeiro espaço
    complement = buffer_copy;  // O restante é atribuído à 'complement'

    printk("Command: %s, Name: %s\n", command, complement);

    command_c =  strcmp(command, "/reg");
    //verificação do comando reg
    if(command_c != 0 && registered == NULL){ // Caso o usuário tente rodar um comando sem se registrar
        printk(KERN_INFO "MQ_Driver: Process need to be registered first\n");
        kfree(buffer_copy);
        return -1;
        
    }else if(command_c == 0 && registered != NULL){ // Caso o usuário tente se registrar novamente
        printk(KERN_INFO "MQ_Driver: Process alredy registered\n");
        kfree(buffer_copy);
        return -1;
    }else if(command_c == 0 && registered == NULL){ // Usuário deseja se registrar
        if(count_n_process == n_process) { // Verifica se já foi atingido o número máximo de inscrições
            printk(KERN_INFO "MQ_Driver: Process not registered.\nProcess limit reached, try again later!\n");
            kfree(buffer_copy);
            return -1;
        }

        reg = list_add_entry(complement, pid, n_msg);

        if(reg == -1) { // Se ouve algum erro no egistro
            kfree(buffer_copy);
            return -1;
        }

        printk(KERN_INFO "MQ_Driver: Process successfully registered!\n");
        
        list_show();

        count_n_process++; // Contador de processos

        kfree(buffer_copy);
        return 0;
    }

    if(strlen(complement) > s_msg){
        printk(KERN_INFO "MQ_Driver: Message exceeds maximum size of %d bytes!\n", s_msg);
        kfree(buffer_copy);
        return -1;
    }

    command_c =  strcmp(command, "/[ALL]");
    if(command_c == 0){
        target_add_message =  list_add_msg_entry_all(pid, complement, s_msg);
    }
    else{
        memmove(command, command + 1, strlen(command));
        if(strcmp(command, registered->name) != 0){
            target_add_message = list_add_msg_entry(command, complement, s_msg);
        }else{
            target_add_message = -1;
            printk(KERN_INFO "MQ_Driver: A process can't send a message for himself!\n");
        }
       
    }

    list_show();

    kfree(buffer_copy);
	return target_add_message;
	
}

//Tirar da lista
static int dev_release(struct inode *inodep, struct file *filep)
{
    int pid = (int) task_pid_nr(current);
    struct process *registered = list_pid_exist(pid);

    list_delete_entry(registered);

	printk(KERN_INFO "MQ_Driver: device successfully closed\n");
    --count_n_process;
	return 0;
}


module_init(mq_init);
module_exit(mq_exit);