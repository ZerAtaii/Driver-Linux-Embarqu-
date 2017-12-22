//////////////////////////////////////////////////////////////////
/*
Driver d'un mini écran LCD de 128 par 160 pixels.
La communication se fait via le SPI. Le pin de reset permet de "redémarrer" l'écran
lorsque cette sortie passe à 1. Le pin de data permet à l'écran de savoir s'il reçoit
des données ou des commandes (0 pour commande et 1 pour donnée).
La séquence d'initialisation de l'écran a été trouvée sur internet.
La fonction set_window permet de choisir une région de l'écran selon ses coordonnées.
La fonction draw_rectangle permet de dessiner un rectangle monochromatique dans la région spécifiée.
Un utilisateur peut afficher une image à l'aide du fichier virtuel créé par le driver.
Il suffit pour cela d'écrire le buffer contenant les couleurs des pixels dans le fichier
virtuel.
*/
//////////////////////////////////////////////////////////////////






#include "LCDGames.h"


#define DEVICE_NAME "device"
#define CLASS_NAME "LCD"
#define SIZE_OF_BUF 3
#define GPIO_DATA 6  //Pin de data/command du SPI
#define GPIO_RESET 1  // Pin de reset du SPI qui doit rester à 0


#define NODELAY 0

//////////////////////////////////////////////////////////////////
//					VARIABLES GLOBALES							//
//////////////////////////////////////////////////////////////////

static struct spi_device *spi_global;

static int major = 1;
static struct class* device_class = NULL;
static struct device* device_lcd = NULL;
dev_t devt;



// Séquence d'initialisation spécifique à l'écran
//Le tableau = en 0: nombre de commandes puis après la suite (commande, nb_args, délai, args)
static unsigned char Init[] = {                  // Initialization commands for 7735B screens
    19,                       // 15 commandes dans la liste:
    ST7735_SWRESET,0,0x96,  //  1: Software reset, 0 args, w/delay

    ST7735_SLPOUT ,0,0xFF,  //  2: Out of sleep mode, 0 args, w/delay
		                   //     500 ms delay
    ST7735_FRMCTR1, 3, NODELAY,//  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3,NODELAY,//  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6,NODELAY,//  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1, NODELAY,//  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3, NODELAY,//  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1, NODELAY,//  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2, NODELAY,//  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2, NODELAY,// 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,
    ST7735_PWCTR5 , 2, NODELAY,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1,NODELAY,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0,NODELAY,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1,NODELAY,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1,NODELAY,// 15: set color mode, 1 arg, no delay:
      0x05 ,

	ST7735_GMCTRP1, 16,NODELAY, //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16,NODELAY, //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,0,0x0A, //  3: Normal display on, no args, w/delay

    ST7735_DISPON ,0,0x64, //  4: Main screen turn on, no args w/delay


};

//////////////////////////////////////////////////////////////////
//				GESTION DE L'AFFICHAGE							//
//////////////////////////////////////////////////////////////////

/*
	Envoie une commande via le SPI (pin data/cmd à 0)
	spi = le device initialisé pendant le probe
	cmd = la commande à envoyer sur un octet
*/
static int send_command(struct spi_device *spi, unsigned char cmd) {
  int status;
	gpio_set_value(GPIO_DATA,0);
	if ((status = spi_write(spi, &cmd, 1)) < 0) {
    printk(KERN_ERR"Erreur d'écriture sur le bus SPI");
  }
	return status;
}


/*
	Envoie un ensemble de données depuis un buffer via le SPI (pin data/cmd à 1)
	spi = le device initialisé pendant le probe
	data = les données à envoyer
	len = la longueur des données à envoyer
*/
static int send_data(struct spi_device *spi, unsigned char *data, size_t len) {
  int status;
	gpio_set_value(GPIO_DATA,1);
	if ((status = spi_write(spi, data , len)) < 0) {
    printk(KERN_ERR"Erreur d'écriture sur le bus SPI");
  }
	return status;
}

/*
	Envoie une seule donnée d'une octet via le SPI (pin data/cmd à 1)
	spi = le device initialisé pendant le probe
	data = la donnée à envoyer sur un octet
*/
static int send_data_char(struct spi_device *spi, unsigned char data) {
  int status;
	gpio_set_value(GPIO_DATA,1);
	if ((status = spi_write(spi, &data , 1)) < 0) {
    printk(KERN_ERR"Erreur d'écriture sur le bus SPI");
  }
	return status;
}

/*
	Initialise l'écran LCD via une séquence spécifique
	spi = le device initialisé pendant le probe
	tab = la séquence d'initialisation à envoyer
*/
static int ecran_init(struct spi_device *spi, unsigned char *tab) {
	int i, index = 1;
	int numCmds;
	numCmds = tab[0];//Nombre de commandes
	for (i = 0; i<numCmds; i++) { //On itère sur le nombre de commandes
		int delay;
		int numArgs;
		if (send_command(spi, tab[index++]) < 0){goto label_error;} //On envoie la commande correspondante
		numArgs = tab[index++];//On récupère le nombre d'arguments
		delay = tab[index++];// et le délai
		if (numArgs) {//si argument il y a
			if (send_data(spi, tab+index, numArgs) < 0) {goto label_error;};//on envoie les données correspondant aux arguments, tab+index correspond à l'ensemble des caractères hexa à partir de index jusqu'à index+numArgs
		}
		if (delay == 0xFF) {//si délai aussi
			msleep(500);//on attend le délai correspondant
		}
		else if (delay) {
			msleep(delay);
		}
		index += numArgs; //On passe à la commande suivante
	}
	return 0;

  label_error:
  printk(KERN_ERR"Erreur d'initialisation de l'écran");
  return -1;

}

/*
	Définit une région spécifique rectangulaire de l'écran
	spi = le device initialisé pendant le probe
	x0 = début de la ligne / sur 1 octet
	y0 = début de la colonne / sur 1 octet
	x1 = fin de la ligne / sur 1 octet
	y1 = fin de la colonne / sur 1 octet
*/
void set_window(struct spi_device *spi,char x0, char y0, char x1, char y1) {
	send_command(spi,ST7735_CASET); // Choix des lignes
 	send_data_char(spi,0x00);
	send_data_char(spi,x0);     // XSTART
	send_data_char(spi,0x00);
	send_data_char(spi,x1);     // XEND
	send_command(spi,ST7735_RASET); // Choix des colonnes
	send_data_char(spi,0x00);
	send_data_char(spi,y0);     // YSTART
	send_data_char(spi,0x00);
	send_data_char(spi,y1);     // YEND
	send_command(spi,ST7735_RAMWR); 	// Ecrit dans la RAM

}


/*
	Dessine un rectangle monochrome dans la zone spécifiée
	spi = le device initialisé pendant le probe
	x =  ligne / sur 2 octets
	y =  colonne / sur 2 octets
	w = largeur	/ sur 2 octets
	h = hauteur	/ sur 2 octets
	color = Code hexadecimal de la couleur / sur 2 octets
*/
void draw_rectangle(struct spi_device * spi, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
	char *buf;
	int i;
	buf = kmalloc(2*sizeof(char)*h*w, GFP_KERNEL);  // Buffer (d'une taille de largeur*hauteur*2) à remplir avec la valeur de la couleur
	if((x < 0) ||(x >= WIDTH) || (y < 0) || (y >= HEIGHT)) return;//Si on est en dehors de l'écran, on ne fait rien et on sort de la fonction
	set_window(spi, x, y, x+w-1, y+h-1); // On choisit une zone de l'écran entre x et x+w / y et y+h
	for (i=0; i < 2*h*w; i++) {			// On itère sur l'ensemble du buffer qui correspond au nombre de pixel de l'écran
		if(i%2) {
			buf[i]=	color & 0xFF;		// On entre les 8 premiers bits de la couleur
		}
		else {
			buf[i] = color >> 8;		// On entre les 8 derniers bits de la couleur
		}
	}
	if (send_data(spi, buf, 2*w*h)<0){
    kfree(buf);
    return;
  } 		// On envoie le buffer sur le SPI
	msleep(10);	// Pour éviter de free le buffer avant que les données aient été envoyées entierement
	kfree(buf);
}

/*
	Colore l'écran en noir
	spi = le device initialisé pendant le probe
*/
void black_screen(struct spi_device *spi) {
	draw_rectangle(spi, 0, 0, WIDTH, HEIGHT, BLACK);
}

//////////////////////////////////////////////////////////////////
//				GESTION DES ACCES UTILISATEUR					//
//////////////////////////////////////////////////////////////////

/*
	Fonction appelée lorsque l'utilisateur lit le fichier virtuel
	INUTILISEE
*/
static int device_read(struct file *f, char __user *data, size_t size, loff_t *l) {
	return 0;
}

/*
	Fonction appelée lorsque l'utilisateur écrit (avec 'echo') dans le fichier virtuel
*/
static int device_write(struct file *f,const char __user *data, size_t size, loff_t *l) {
	char *buf = kmalloc(size, GFP_KERNEL);
	if (!copy_from_user(buf, data, size)) { //On récupère les données de l'utilisateur
		set_window(spi_global, 0, 0, WIDTH, HEIGHT);	// On choisit tout l'écran
		if (send_data(spi_global, buf, size)<0) { // On envoie le buffer sur le SPI
      kfree(buf);
  		return -1;
    }
	}
	else {
		printk(KERN_ERR"Problème de data from user\n");
		kfree(buf);
		return -1;
	}
	kfree(buf);
	return size;
}
/*
	Fonction appelée lorsque l'utilisateur ouvre le fichier virtuel (avec "insmod")
	INUTILISEE
*/
static int device_open(struct inode *i, struct file *f) {
	return 0;

}
/*
	Fonction appelée lorsque l'utilisateur ferme le fichier virtuel (avec "rmmod")
	INUTILISEE
*/
static int device_release(struct inode *i, struct file *f) {
	return 0;
}

//////////////////////////////////////////////////////////////////
//				GESTION DU MODULE								//
//////////////////////////////////////////////////////////////////

/*
	fonction d'initialisation du driver
*/
static int __init fonctionInit(void) {
	int status;

	major = register_chrdev(0, "LCDGames", &fops); // On enregistre le matériel
	if (major < 0) {
		printk(KERN_INFO"Erreur enregistrement du device");
	}
	device_class = class_create(THIS_MODULE,"LCDGames_class"); // On définit sa classe
	devt = MKDEV(major,0);
	device_lcd = device_create(device_class, NULL, devt, NULL, "LCDGames"); // Fichier virtuel



	if (gpio_request(GPIO_RESET, "RESET")) { // On initialise la GPIO de reset de l'écran
		printk(KERN_ERR"Erreur GPIO RESET\n");
		gpio_free(GPIO_RESET);
	}
	else {
		gpio_direction_output(GPIO_RESET, 1);
	}
	if (gpio_request(GPIO_DATA, "Data")) { // On initialise la GPIO de data/commande de l'écran
		printk(KERN_ERR"Erreur GPIO DATA\n");
		gpio_free(GPIO_DATA);
	}
	else {
		gpio_direction_output(GPIO_DATA, 1);
	}

	status = spi_register_driver(&ecran); // On enregistre l'écran en tant que matériel utilisant le bus SPI


	return 0;
}

/*
	fonction appelée à la fermeture du module
*/
static void __exit fonctionExit(void) {
	unregister_chrdev(major,"LCDGamesDevice"); // On libère tout ce qu'on a utilisé
	device_destroy(device_class,devt);
	class_destroy(device_class);
	gpio_direction_output(GPIO_RESET, 0);
	gpio_free(GPIO_DATA);
	gpio_free(GPIO_RESET);
	spi_unregister_driver(&ecran);
}

//////////////////////////////////////////////////////////////////
//				GESTION DU DRIVER SPI							//
//////////////////////////////////////////////////////////////////

/*
	fonction appelée au moment où on enregistre le device sur le spi (spi_register dans la fonction d'initialisation)
	spi = le spi device à initialiser
*/
static int device_probe(struct spi_device *spi) {
	spi->bits_per_word = 8; // On définit le nombre de bit par mot utilisé sur le SPI
	if (spi_setup(spi)) {
		printk(KERN_ERR"Problème setup\n");
	}
	spi_global = spi;
	if (ecran_init(spi, Init)) {
		printk(KERN_ERR"Problème init de l'écran\n");
	}

	black_screen(spi);
	return 0;
}

/*
	fonction appelée au moment où on supprime le device sur le spi
	spi = le spi device à supprimer
	INUTILISEE
*/
static int device_remove(struct spi_device *spi) {
	return 0;
}




module_init(fonctionInit);
module_exit(fonctionExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Hell");
MODULE_DESCRIPTION("LED");
