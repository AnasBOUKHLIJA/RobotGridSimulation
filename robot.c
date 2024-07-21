#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>
#include <string.h> 

#define N 3   // Taille de la grille (N x N)
#define R 7   // Nombre total de robots

typedef struct {
    int x, y;      // Position du robot dans la grille
    int points;    // Points du robot
    int alive;     // État de vie du robot (1 si vivant, 0 sinon)
    int moves;     // Nombre de mouvements effectués
    int last_x, last_y;  // Dernière position du robot
    char last_action[200]; // Dernière action du robot
} Robot;

// Pointeur vers le tableau des robots
Robot *robots;  

void move_robot(int id) {
    int dx[] = {0, 0, -1, 1};  // Déplacements possibles en x (gauche, droite, haut, bas)
    int dy[] = {-1, 1, 0, 0};  // Déplacements possibles en y (haut, bas, gauche, droite)

    while (robots[id].alive) {  // Boucle tant que le robot est vivant
        sleep(1);  // Attendre 1 seconde avant le prochain mouvement

        int dir = rand() % 4;  // Choisir une direction aléatoire
        int nx = robots[id].x + dx[dir];  // Nouvelle position en x après déplacement
        int ny = robots[id].y + dy[dir];  // Nouvelle position en y après déplacement

        // Sauvegarder l'état actuel avant le mouvement
        robots[id].last_x = robots[id].x;
        robots[id].last_y = robots[id].y;

        // Début de la section critique
        if (nx >= 0 && ny >= 0 && nx < N && ny < N) {  // Vérifier si la nouvelle position est dans la grille
            int collision = 0;
            for (int i = 0; i < R; i++) {
                if (i != id && robots[i].alive && robots[i].x == nx && robots[i].y == ny) {
                    collision = 1;  // Détection de collision avec un autre robot
                    break;
                }
            }
            if (!collision) {
                // Si pas de collision, mettre à jour la position du robot
                robots[id].x = nx;
                robots[id].y = ny;
                robots[id].points += 1;  // Augmenter les points du robot
                robots[id].moves += 1;  // Incrémenter le nombre de mouvements
                snprintf(robots[id].last_action, sizeof(robots[id].last_action), "Déplacement réussi vers (%d, %d).", nx, ny);
            } else {
                robots[id].points -= 1;  // Décrémenter les points en cas de collision
                snprintf(robots[id].last_action, sizeof(robots[id].last_action), "Déplacement échoué vers (%d, %d) à cause d'une collision.", nx, ny);
            }
        } else {
            robots[id].points -= 1;  // Décrémenter les points si le robot sort de la grille
            snprintf(robots[id].last_action, sizeof(robots[id].last_action), "Déplacement échoué vers (%d, %d) car il sort de la grille.", nx, ny);
        }
        // Fin de la section critique

        if (robots[id].points <= 0) {  // Vérifier si les points du robot sont épuisés
            robots[id].alive = 0;  // Marquer le robot comme mort
        }
    }
    
    exit(0);  // Terminer le processus du robot
}

int main(int argc, char *argv[]) {
    if (argc != 3) {  // Vérifier le nombre d'arguments passés au programme
        fprintf(stderr, "Usage: %s <robot_id> <shmid>\n", argv[0]);
        return 1;
    }

    int id = atoi(argv[1]);  // Convertir l'ID du robot en entier
    int shmid = atoi(argv[2]);  // Convertir l'ID de la mémoire partagée en entier

    robots = (Robot *)shmat(shmid, NULL, 0);  // Attacher la zone de mémoire partagée au processus

    srand(time(NULL) ^ (getpid()<<16));  // Initialiser le générateur de nombres aléatoires avec une graine unique pour chaque processus

    // Synchronisation de l'initialisation
    sleep(1); 

    move_robot(id);  // Déplacer le robot avec l'ID donné

    shmdt(robots);  // Détacher la zone de mémoire partagée du processus
    return 0;
}
