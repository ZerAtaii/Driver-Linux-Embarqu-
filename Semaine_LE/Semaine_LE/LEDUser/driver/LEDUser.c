//////////////////////////////////////////////////////////////////
/*
Driver d'une LED 3 couleurs.
On choisit quelle couleur on veut afficher en écrivant dans le fichier
virtuel du driver.
On envoie 3 bits, chacun correspond à une couleur (rouge/vert/bleu)
Exemple: 100 -> ROUGE
				 101 -> ROUGE et BLEU etc...
On éteint la LED en lisant le fichier virtuel.
*/
//////////////////////////////////////////////////////////////////
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/ktime.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define VERT 2 // Numéro du GPIO de la LED verte
#define BLEU 7 // Numéro du GPIO de la LED bleue
#define ROUGE 4 // Numéro du GPIO de la LED rouge
#define DEVICE_NAME "device"
#define CLASS_NAME "LED"
#define SIZE_OF_BUF 3

// variables globales
static int device_test = 1;

static struct class* device_class = NULL;
static struct device* device_led = NULL;
dev_t devt;
// Prototypes des fonctions
static int device_read(struct file *f, char __user *data, size_t size, loff_t *l);
static int device_write(struct file *f, const char __user *data, size_t size, loff_t *l);
static int device_open(struct inode *i, struct file *f);
static int device_release(struct inode *i, struct file *f);




struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};


//Fonction appelée lorsque l'utilisateur lit le fichier virtuel
static int device_read(struct file *f, char __user *data, size_t size, loff_t *l) {
	printk(KERN_INFO"Remise à zero des LED\n");
	gpio_set_value(ROUGE, 1); // On éteint la LED rouge
	gpio_set_value(VERT, 1); // On éteint la LED verte
	gpio_set_value(BLEU, 1); // On éteint la LED bleue
	return 0;
}

//Fonction appelée lorsque l'utilisateur écrit (avec 'echo') dans le fichier virtuel
static int device_write(struct file *f,const char __user *data, size_t size, loff_t *l) {
	char *buf = kmalloc(SIZE_OF_BUF*sizeof(char) + 1, GFP_KERNEL);
	if (!copy_from_user(buf, data, SIZE_OF_BUF)) { // On récupère les données de l'utilisateur
		buf[SIZE_OF_BUF] = '\0';
		if (buf[0] == '1') { // Si on a 1--
			gpio_set_value(ROUGE, 0); // On allume la LED rouge
		}
		else {
			gpio_set_value(ROUGE, 1);	// On éteint la LED rouge
		}
		if (buf[1] == '1') { // Si on a -1-
			gpio_set_value(VERT, 0); // On allume la LED verte
		}
		else {
			gpio_set_value(VERT, 1); // On éteint la LED verte
		}
		if (buf[2] == '1') { // Si on a --1
			gpio_set_value(BLEU, 0); // On allume la LED bleue
		}
		else {
			gpio_set_value(BLEU, 1); // On éteint la LED bleue
		}

	}
	else {
		printk(KERN_INFO"Problème buffer\n");
	}
	kfree(buf);
	return size;
}
//Fonction appelée lorsque l'utilisateur ouvre le fichier virtuel (avec "insmod")
static int device_open(struct inode *i, struct file *f) {
	printk(KERN_INFO"Ouverture du fichier !");
	return 0;

}
//Fonction appelée lorsque l'utilisateur ferme le fichier virtuel (avec "rmmod")
static int device_release(struct inode *i, struct file *f) {
	printk(KERN_INFO"Fermeture du fichier !");
	return 0;
}

//Fonction d'initialisation du driver
static int __init fonctionInit(void) {
	device_test = register_chrdev(0, DEVICE_NAME, &fops); // On enregistre le device
	if (device_test < 0) {
		printk(KERN_INFO"Erreur enregistrement du device");
	}
	device_class = class_create(THIS_MODULE,CLASS_NAME); // On définit une classe
	devt = MKDEV(device_test,0);
	device_led = device_create(device_class, NULL, devt, NULL, CLASS_NAME "_" DEVICE_NAME);

	if (!gpio_request(VERT, "VERT")) { // On réserve une gpio de sortie pour la LED verte
		gpio_direction_output(VERT, 0);
		printk(KERN_INFO"Vert OK\n");
	}
	else {
		gpio_free(VERT);
		printk(KERN_INFO"Problème d'allocation de la LED verte\n");
	}
	if (!gpio_request(BLEU, "BLEU")) { // On réserve une gpio de sortie pour la LED bleue
		gpio_direction_output(BLEU, 0);
		printk(KERN_INFO"Bleu OK\n");
	}
	else {
		gpio_free(BLEU);
		printk(KERN_INFO"Problème d'allocation de la LED bleue\n");
	}
	if (!gpio_request(ROUGE, "ROUGE")) { // On réserve une gpio de sortie pour la LED rouge
		gpio_direction_output(ROUGE, 0);
		printk(KERN_INFO"Rouge OK\n");
	}
	else {
		gpio_free(ROUGE);
		printk(KERN_INFO"Problème d'allocation de la LED rouge\n");
	}


	return 0;
}


//Fonction appelée à la fermeture du module
static void __exit fonctionExit(void) {
	unregister_chrdev(device_test,DEVICE_NAME); // On libère tout ce qu'on a utilisé
	device_destroy(device_class,devt);
	class_destroy(device_class);
	gpio_set_value(VERT, 1);
	gpio_set_value(BLEU, 1);
	gpio_set_value(ROUGE, 1);
	gpio_free(VERT);
	gpio_free(BLEU);
	gpio_free(ROUGE);
}

module_init(fonctionInit);
module_exit(fonctionExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Hell");
MODULE_DESCRIPTION("LED");
