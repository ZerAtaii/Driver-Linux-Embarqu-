//////////////////////////////////////////////////////////////////
/*
Application permettant de lancer un diaporama sur le mini écran LCD.
Il faut d'abord s'assurer que l'écran est correctement branché et que
le driver a été préalablement lancé.
Il suffit ensuite de lancer l'application LCDUser en ajoutant le nom du
répertoire dans lequel se trouvent les images au format .BMP en argument
*/
//////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <dirent.h>
#include <unistd.h>

// Ensenble des masques pour les couleurs
#define rmask 0x000000ff
#define gmask 0x0000ff00
#define bmask 0x00ff0000
#define amask 0xff000000


/*
	Ouvre un fichier image de type .BMP et enregistre l'ensemble du buffer correspondant à cette image dans dest
	filename = chemin d'accès du fichier BMP
	dest = buffer à remplir
*/
void open_file(char* filename, uint16_t* dest) {
	SDL_Surface *screen = SDL_CreateRGBSurface(0,128,160,32,rmask,gmask,bmask,amask); //Ecran correspondant à notre type d'architecture
	SDL_Surface *image = IMG_Load(filename);		// Chargement de l'image
	SDL_Rect rect = {
		.x = 0,
		.y = 0,
		.h = 0,
		.w = 0
	};
	SDL_BlitSurface(image,NULL,screen, &rect);
	if (image == NULL) {
		printf("erreur sur le chemin de l'image");
		return;
	}
	int index = 0;			// Index du tableau
	uint8_t r, g, b;		// Un entier sur 8 bit par couleur
	int i,j;
	for (i = 0; i < 160; i++) {				// On parcourt l'image
		for (j=0 ; j < 128 ; j++) {
			uint32_t pix = ((uint32_t *)screen->pixels)[(i*128 + j)];			// On récupère la position du pixel dans l'image
			SDL_GetRGB(pix,screen->format,&r,&g,&b);							// On récupère ses valeurs de rouge, vert et bleu
			r = r * 31/255;													// On remet les valeurs sur 5 bits (au lieu de 8)
			g = g * 31/255;
			b = b * 31/255;
			uint16_t val = htons( ((uint16_t)b)<<11 | ((uint16_t)g)<<5 | r); 						// On les place dans le bon ordre
			dest[index] = val;												// On fait les décalages pour avoir chaque couleur (5 pour vert et 11 pour bleu car LITTLE_ENDIAN)
			index++;
		}
	}
	SDL_FreeSurface(screen);
	SDL_FreeSurface(image);  // On libère les surfaces
	SDL_Quit();				//  On quitte SDL

}



/*
	Affiche un fichier BMP à l'écran
	filename = chemin d'accès du fichier BMP
*/
void display(char *filename) {
	uint16_t *buffer = malloc(sizeof(uint16_t)*128*160);  // On définit un buffer de taille 2*largeur*hauteur
	open_file(filename,buffer); 			// On remplit le buffer
	FILE *f = fopen("/dev/LCDGames","wb");
	int i = fwrite(buffer,128*160*2,1,f); // On écrit dans le fichier virtuel du driver pour afficher l'image à l'écran
	fclose(f); // On ferme le fichier virtuel
	free(buffer); // On libère le buffer
}



/*
	Fonction appelée par l'utilisateur pour lancer le diaporama
*/
int main(int argc, char **argv) {
	DIR *dp;
  	struct dirent *ep;
	char *filename;
	filename = malloc(256*sizeof(char));

  	dp = opendir(argv[1]); // On ouvre le dossier fourni en entrée par l'utilisateur
  	if (dp != NULL) {
		while(1) {
			while (ep = readdir(dp)) { 	// On itère sur les fichiers du dossier
				if (strcmp(ep->d_name,".") && strcmp(ep->d_name,"..")) {	// Sauf sur les "." et ".."
					strcpy(filename, argv[1]);
					strcat(filename, ep->d_name); // On écrit le fichier sous la forme dossier/fichier
					printf("%s\n",filename);
				   	display(filename);	// On affiche l'image à l'écran
					sleep(3);			// On laisse un délai de 3 sec entre chaque image
				}
			}

			rewinddir(dp);	// On retourne au début du dossier pour faire une "boucle infinie"
		}
      	(void) closedir (dp); 	// On ferme le dossier
    }
  	else {
    	perror ("Couldn't open the directory");
	}
	free(filename);

  	return 0;
}
