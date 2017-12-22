/*////////////////////////////////////////////////////////////////////////////////////
Driver permettant de gérer la LED de la carte, ce programme permet de faire
clignoter la LED de 3 couleurs différentes à la suite (rouge, bleu et vert).
Cela est permis grâce à un timer qui à chaque tick allume une couleur et
éteint la précédente, ce qui permet de réaliser une guirlande clignotante
(très) minimaliste. Un bouton est également géré afin de lancer une interruption
lorsqu'on appuie dessus, le timer est alors arrêté et la couleur de la LED reste fixe.
*/////////////////////////////////////////////////////////////////////////////////////


//Includes

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/interrupt.h>

//Defines utilisés dans le code
#define VERT 2
#define BLEU 7
#define ROUGE 4
#define IRQ_BUTTON 6

#define INTERVAL 1

struct hrtimer timerLED; //Timer servant à gérer la synchronisation des couleurs de la LED
//Prototypes des fonctions
static enum hrtimer_restart fonctionTime(struct hrtimer *arg);
irqreturn_t interruption(int irq, void * ident);


//Fonction d'initialisation du driver, lorsqu'on lance le module sur Putty
static int __init fonctionInit(void) {

	ktime_t time;
	time = ktime_set(INTERVAL, 0);//Configuration de l'intervalle du timer (changement toutes les secondes)
	hrtimer_init(&timerLED, CLOCK_MONOTONIC, HRTIMER_MODE_REL);//Initialisation du timer
	timerLED.function = &fonctionTime;//Fonction gérant le comportement de la LED en fonction du timer

	int numero_interruption = gpio_to_irq(IRQ_BUTTON);//Interruption
	if (!gpio_request(IRQ_BUTTON, "INTERRUPTION")) {//Gestion de l'interruption lorsqu'on appuie sur le bouton
		gpio_set_debounce(IRQ_BUTTON, 50);
		gpio_direction_input(IRQ_BUTTON);
		printk(KERN_INFO"Bouton d'interruption initialisé\n");
	}
	else {
		gpio_free(IRQ_BUTTON);
		printk(KERN_INFO"Problème d'allocation du GPIO boutton\n");
	}
	if (!gpio_request(VERT, "VERT")) {//Requête sur le GPIO correspondant à la couleur verte de la LED
		gpio_direction_output(VERT, 0);//allumage LED
		printk(KERN_INFO"Vert OK\n");
	}
	else {
		gpio_free(VERT);//Libération du GPIO correspondant dans le cas d'erreur
		printk(KERN_INFO"Problème d'allocation de la LED verte\n");
	}
	if (!gpio_request(BLEU, "BLEU")) {//Requête sur le GPIO correspondant à la couleur bleue de la LED
		gpio_direction_output(BLEU, 0);
		printk(KERN_INFO"Bleu OK\n");
	}
	else {
		gpio_free(BLEU);
		printk(KERN_INFO"Problème d'allocation de la LED bleue\n");
	}
	if (!gpio_request(ROUGE, "ROUGE")) {//Requête sur le GPIO correspondant à la couleur rouge de la LED
		gpio_direction_output(ROUGE, 0);
		printk(KERN_INFO"Rouge OK\n");
	}
	else {
		gpio_free(ROUGE);
		printk(KERN_INFO"Problème d'allocation de la LED rouge\n");
	}
	hrtimer_start(&timerLED, time, HRTIMER_MODE_REL);//Start du timer
	request_irq(numero_interruption, &interruption, IRQF_TRIGGER_RISING, "BOUTON_INTERRUPTION", "BOUTON_INTERRUPTION");

	return 0;
}

static enum hrtimer_restart fonctionTime(struct hrtimer *arg) {//Fonction pour le restart du timer
	static int index = 0;//entier servant à gérer la couleur de la LED selon sa valeur
	ktime_t currtime, intervalle;
	currtime = ktime_get();//time actuel
	intervalle = ktime_set(INTERVAL, 0);
	if (index == 0) {
		gpio_set_value(VERT, 1);
		gpio_set_value(BLEU, 1);
		gpio_set_value(ROUGE, 0);//On allume rouge
		index++;
	}
	else if (index == 1) {
		gpio_set_value(ROUGE, 1);
		gpio_set_value(BLEU, 1);
		gpio_set_value(VERT, 0);//On allume vert
		index++;
	}
	else if (index == 2) {
		gpio_set_value(ROUGE, 1);
		gpio_set_value(VERT, 1);
		gpio_set_value(BLEU, 0);//On allume bleu
		index = 0;
	}
	hrtimer_forward(arg, currtime, intervalle);//Sert à relancer le timer
	return HRTIMER_RESTART;
}

//Fonction gérant l'interruption matérielle
irqreturn_t interruption(int irq, void * ident) {
	if (gpio_get_value(IRQ_BUTTON)) {//si on appuie sur le bouton (valeur = 1)
		hrtimer_try_to_cancel(&timerLED);//On arrête le timer
	}
	else {
		hrtimer_restart(&timerLED);//Quand on arrête d'appuyer, on restart le timer
	}
	printk(KERN_INFO"Interuption levée!\n");
	return IRQ_HANDLED;
}
//Fonction appelée lors de l'arrêt du module (rmmod)
static void __exit fonctionExit(void) {
	int ret;//entier servant à la gestion d'erreur sur l'arrêt du timer

	//On remet les valeurs des GPIOs à 1 pour éteindre la LED
	gpio_set_value(VERT, 1);
	gpio_set_value(BLEU, 1);
	gpio_set_value(ROUGE, 1);

	//On free les GIOs
	gpio_free(VERT);
	gpio_free(BLEU);
	gpio_free(ROUGE);

	//On arrête le timer
	ret = hrtimer_cancel(&timerLED);
	if (ret) {
		printk(KERN_INFO"Le timer était encore en cours d'utilisation\n");
	}
	printk(KERN_INFO"Module HR Timer correctement désactivé\n");
}

module_init(fonctionInit);
module_exit(fonctionExit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Hell");
MODULE_DESCRIPTION("LED");
