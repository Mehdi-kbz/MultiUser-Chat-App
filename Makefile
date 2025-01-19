# Définitions des variables
CC = gcc                    # Compilateur
CFLAGS = -Wall              # Options de compilation (affiche tous les avertissements)

# Fichiers sources
CLIENT_SRCS = client.c      # Fichiers source du client
SERVER_SRCS = server.c      # Fichiers source du serveur

# Génération des fichiers objets correspondants
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)  # Fichiers objets générés à partir des sources client
SERVER_OBJS = $(SERVER_SRCS:.c=.o)  # Fichiers objets générés à partir des sources serveur

# Règle par défaut : compiler client et serveur
all: client server

# Compilation du programme client
client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $(CLIENT_OBJS) -o client

# Compilation du programme serveur
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $(SERVER_OBJS) -o server

# Compilation de chaque fichier .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage des fichiers générés
clean:
	rm -f client server $(CLIENT_OBJS) $(SERVER_OBJS)

# Règle de phony pour éviter les conflits avec des fichiers portant le même nom
.PHONY: all clean