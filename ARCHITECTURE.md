# Structure du projet jsh

Dans ce projet, vous trouverez les fichiers suivants :

- **jsh.c** : Il s'agit du noeud principal du shell. Ce fichier gère l'affichage du prompt et le pré-traitement des commandes entrées par l'utilisateur. Il est responsable de la sauvegarde des flux, de la gestion des redirections, et de l'extraction de la commande à partir de l'entrée utilisateur. Il gère également la lecture de la ligne de commande, l'ajout de celle-ci à l'historique, la gestion des substitutions de processus et des pipelines, et l'exécution de la commande. À la fin de l'exécution, il libère la mémoire allouée pour le prompt et la ligne de commande.

- **commands.c** : Ce fichier gère l'exécution des commandes internes et externes. Pour une commande interne, il exécute la fonction correspondante. Pour une commande externe, il délègue l'exécution à la fonction `execute_external_command`. Il prépare également un tableau d'arguments à passer à la commande, vérifie si la commande est une commande de pipeline ou en arrière-plan, et met à jour le code de sortie de la dernière commande exécutée. À la fin de l'exécution, il libère la mémoire allouée pour la copie de la commande.

- **external_commands.c** : Ce fichier gère l'exécution des commandes externes. Il est responsable de la préparation des arguments, de la création d'un nouveau processus pour exécuter la commande, et de la gestion des erreurs liées à son exécution. Il gère également l'exécution des commandes en arrière-plan et en premier plan, la création de nouveaux jobs pour les commandes en arrière-plan et les processus arrêtés, et le retour du statut de sortie ou du numéro du signal pour les processus terminés.

- **bg.c, cd.c, exit.c, fg.c, jobs.c, kill.c et pwd.c** : Ces fichiers correspondent, comme leur nom le suggère, à toutes les commandes internes du projet, avec un comportement analogue à celui d'un vrai shell Unix.

- **jobs.c** : Ce fichier gère tout ce qui est relatif aux jobs. Il est responsable de la création des jobs, de leur suppression, de la mise à jour de leur état, et de leur stockage. Les jobs sont stockés dans une structure contenant le numéro du job, son pgid, le statut du job (via une énumération), et la ligne de commande associée. Ce fichier fournit également des fonctions pour manipuler et interroger cette structure de jobs, y compris l'ajout de nouveaux jobs, la récupération de jobs par leur ID, la mise à jour de leur ID, leur suppression, et leur affichage avec leurs processus fils. Gère les mises à jour des jobs selon les signaux reçus.

- **signals.c** : Ce fichier contient des fonctions relatives à la gestion des signaux. Il fournit des fonctions pour ignorer ou restaurer tous les signaux conformément au sujet, et pour ignorer ou restaurer spécifiquement le signal `SIGTTOU` lorsqu'une opération telle que `tcsetpgrp` doit être effectuée.

- **redirections.c** : Ce fichier gère toutes les redirections (entrée / sortie, pipelines, substitution de processus) pour une ligne de commande. Il est responsable de l'analyse de ladite ligne pour détecter les redirections et de la modification des descripteurs de fichier en conséquence. Il fournit également des fonctions pour sauvegarder et restaurer les flux d'entrée et de sortie, (essentiel pour le fonctionnement correct des redirections). De plus, il gère les erreurs liées à celles-ci, comme l'ouverture d'un fichier en écriture qui échoue ou la redirection d'une entrée / sortie qui n'existe pas.

> Nous avons fait le choix de séparer au maximum les responsabilités du code, afin que ce dernier soit plus compréhensible et plus modulaire.

---

**Evan JOFFRIN (22102052) et Ali EL JAMAI (22104439), L3 Informatique, Info 4**