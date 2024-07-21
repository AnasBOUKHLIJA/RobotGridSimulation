#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <string.h> 

#define N 3   // Taille de la grille (N x N)
#define R 7   // Nombre total de robots
#define WINNING_POINTS 20  // Points nécessaires pour gagner

typedef struct {
    int x, y;      // Position du robot dans la grille
    int points;    // Points du robot
    int alive;     // État de vie du robot (1 si vivant, 0 sinon)
    int moves;     // Nombre de mouvements effectués
    int last_x, last_y;  // Dernière position du robot
    char last_action[200]; // Dernière action du robot
} Robot;

int **grid;   // Pointeur vers la grille de jeu
Robot *robots; // Pointeur vers le tableau des robots

// Initialiser la grille avec la valeur -1
void initialize_grid() {
    grid = malloc(N * sizeof(int *));  // Allouer de la mémoire pour les lignes de la grille
    for (int i = 0; i < N; i++) {
        grid[i] = malloc(N * sizeof(int));  // Allouer de la mémoire pour chaque ligne
        for (int j = 0; j < N; j++) {
            grid[i][j] = -1;  // Initialiser chaque cellule de la grille à -1 (cellule vide)
        }
    }
}

// Fonction pour vérifier si une position est déjà occupée
int is_position_occupied(int x, int y) {
    for (int i = 0; i < R; i++) {
        if (robots[i].alive && robots[i].x == x && robots[i].y == y) {
            return 1;  // Position occupée
        }
    }
    return 0;  // Position libre
}

// Fonction pour initialiser les robots
void initialize_robots() {
    int placed = 0;
    while (placed < R) {
        int x = rand() % N;  // Position aléatoire en x
        int y = rand() % N;  // Position aléatoire en y
        if (!is_position_occupied(x, y)) {  // Vérifier si la position est libre
            robots[placed].x = x;
            robots[placed].y = y;
            robots[placed].points = 10;  // Points de départ des robots
            robots[placed].alive = 1;  // Marquer le robot comme vivant
            placed++;
        }
    }
}

void afficheRobotsStatuses() {
    printf("\n\n\nTous les robots sont morts. État final des robots :\n");
    for (int i = 0; i < R; i++) {
        printf("Robot %d : (%d, %d), Points : %d, Mouvements : %d, Dernière position : (%d, %d), Dernière action : %s\n",
        i, robots[i].x, robots[i].y, robots[i].points, robots[i].moves, robots[i].last_x, robots[i].last_y, robots[i].last_action);
    }
    printf("\n\n");
    exit(0);
}

// Fonction chargée d'afficher l'état actuel de la grille
void print_grid() {
    printf("\n\nÉtat de la grille :\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (grid[i][j] == -1)
                printf(". ");  // Afficher un point pour une cellule vide
            else
                printf("%d ", grid[i][j]);  // Afficher l'index du robot si la cellule est occupée
        }
        printf("\n");
    }
}

// Fonction chargée d'afficher l'état des robots
void print_robots() {
    printf("\nÉtat des robots :\n");
    int alive_count = 0;
    int alive_index = -1;
    int winner_found = 0;

    for (int i = 0; i < R; i++) {
        if (robots[i].alive) {
            if (robots[i].points >= WINNING_POINTS) {
                // Le robot a gagné
                printf("Le robot %d a gagné avec %d points et %d mouvements !\n", i, robots[i].points, robots[i].moves);
                winner_found = 1;
                // Déclarer tous les autres robots comme perdants
                for (int j = 0; j < R; j++) {
                    if (j != i && robots[j].alive) {
                        robots[j].alive = 0;
                        snprintf(robots[j].last_action, sizeof(robots[j].last_action), "Le robot %d a atteint %d points et a gagné. Vous êtes maintenant perdant.", i, WINNING_POINTS);
                    }
                }
                break;
            } else {
                // Afficher l'état des robots vivants
                printf("Robot %d : (%d, %d), Points : %d, Mouvements : %d\n", i, robots[i].x, robots[i].y, robots[i].points, robots[i].moves);                
                alive_count++;
                alive_index = i;
            }
            
        }
    }

    if (winner_found) {
        afficheRobotsStatuses();
    } else if (alive_count == 1) {
        printf("Tous les robots sont morts sauf le robot %d qui reste vivant et a gagné avec %d points et %d mouvements !\n", alive_index, robots[alive_index].points, robots[alive_index].moves);
        afficheRobotsStatuses();
    }

    printf("\n");

}

// Fonction chargée de modifier l'état de la grille
void update_grid() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            grid[i][j] = -1;  // Réinitialiser chaque cellule à -1 avant de mettre à jour avec les robots
        }
    }
    for (int i = 0; i < R; i++) {
        if (robots[i].alive) {
            grid[robots[i].x][robots[i].y] = i;  // Mettre à jour la grille avec l'index du robot si le robot est vivant
        }
    }
}

int main() {
    int shmid = shmget(IPC_PRIVATE, R * sizeof(Robot), IPC_CREAT | 0666); // Créer une zone de mémoire partagée pour les robots
    robots = (Robot *)shmat(shmid, NULL, 0);  // Attacher la zone de mémoire partagée au processus

    initialize_grid();  // Initialiser la grille de jeu
    initialize_robots();  // Initialiser les robots avec des positions uniques

    // Crée les processus fils pour gérer les robots
    for (int i = 0; i < R; i++) {
        if (fork() == 0) {  // Créer un processus fils pour chaque robot
            char id_str[10], shmid_str[10];
            sprintf(id_str, "%d", i);  // Convertir l'index du robot en chaîne de caractères
            sprintf(shmid_str, "%d", shmid);  // Convertir l'ID de la mémoire partagée en chaîne de caractères
            execl("./robot", "robot", id_str, shmid_str, NULL);  // Exécuter le programme "robot" avec les arguments nécessaires
        }
    }

    sleep(1);

    // Affiche en continu l'état de la grille et des robots
    while (1) {
        sleep(1);  // Attendre 1 seconde entre chaque mise à jour
        update_grid();  // Mettre à jour la grille avec les positions des robots
        print_grid();  // Afficher l'état actuel de la grille
        print_robots();  // Afficher l'état des robots et vérifier les conditions de victoire
    }

    // Clean up
    for (int i = 0; i < R; i++) {
        wait(NULL);  // Attendre que tous les processus fils se terminent
    }
    shmdt(robots);  // Détacher la zone de mémoire partagée du processus
    shmctl(shmid, IPC_RMID, NULL);  // Supprimer la zone de mémoire partagée

    for (int i = 0; i < N; i++) {
        free(grid[i]);  // Libérer la mémoire allouée pour chaque ligne de la grille
    }
    free(grid);  // Libérer la mémoire allouée pour les lignes de la grille

    return 0;
}
