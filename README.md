*This activity has been created as part of the 42 curriculum by romasant.*

# Libft

## Description

Libft c'est ma toute première librairie en C, faite dans le cadre du cursus 42 Paris. Le but est de recoder une partie des fonctions standards de la libc (strlen, memcpy, split...) pour comprendre comment elles marchent vraiment en interne, plutôt que de juste les utiliser sans savoir ce qu'il y a dedans.

Une fois compilée, cette librairie me sert de base pour tous mes futurs projets en C à 42, pas besoin de réécrire ces fonctions à chaque fois.

Le projet est divisé en 3 parties :
- des fonctions qui recodent celles de la libc (strlen, memset, strchr, atoi...)
- des fonctions additionnelles qui n'existent pas dans la libc ou qui existent sous une forme différente (split, itoa, substr...)
- des fonctions pour manipuler des listes chaînées (création, ajout, suppression, parcours)

## Instructions

Pour compiler la librairie :

```
make
```

Ça va générer libft.a à la racine du projet.

Autres règles disponibles :

```
make clean    # supprime les fichiers .o
make fclean   # supprime les .o et la libft.a
make re       # fclean puis recompile tout
```

Pour utiliser la librairie dans un autre projet, il faut inclure libft.h et linker libft.a à la compilation, par exemple :

```
cc main.c -L. -lft -o mon_programme
```

(en supposant que libft.a et libft.h sont dans le même dossier que main.c)

## Ressources

- Les man pages officielles des fonctions originales (man strlen, man memcpy, man ar, etc.) pour comprendre le comportement attendu de chaque fonction avant de la recoder. Et avoir le vrai prototype.
- La documentation de make pour la construction du Makefile.
- Internet (Wikipedia pour les notions (memoire, fonctionnement stack/heap) / stackoverflow / geeksforgeeks.org...)

### Utilisation de l'IA

J'ai utilisé Claude Code comme tuteur pendant ce projet, pas pour avoir des réponses toutes faites mais pour comprendre des concepts que je ne maîtrisais pas encore.

Le code des fonctions a été entierement écrit par moi.

version=v2
