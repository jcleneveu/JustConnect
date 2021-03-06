#include "game.h"

/* fonction de lancement initial du jeu (appelé UNE fois par lancement du programme) */
int game()
{
    t_game_board* game = NULL;

    game = create_game_board_with_default_value();

    display_game(game);
    return 0;
}

/* fonction de création d'un nouveau jeu aloprs qu'un ancien jeu était déjà lancé */
int new_game(t_game_board* old_game, int rules, int size_x, int size_y)
{
    t_game_board* new_game = NULL;

    new_game = create_game_board (size_x, size_y, rules);

    update_display_game_to_new(old_game, new_game);
    return 0;
}

/* fonction appelée lors d'un click sur une brick */
gboolean on_brick_click_event(GtkWidget *widget, GdkEventExpose *event, gpointer pt)
{
    // 0 déclaration de variables et initialisations
    int i = 0, j = 0, k = 0, l = 0;
    int x = 0, y = 0;
    int nb_color = 0;
    t_color** colors = NULL;
    int is_closed_path = 0;
    int is_temp_closed_path = 0;
    t_brick* temp_brick = NULL;
    t_game_board* game = (t_game_board*)pt;
    int** tab_test = (int**)malloc(game->nb_brick_x*sizeof(int*));
    for(i=0; i<game->nb_brick_x; i++) tab_test[i] = (int*)malloc(game->nb_brick_y*sizeof(int));
    for (i=0; i < game->nb_brick_x; i++)
        for (j=0; j < game->nb_brick_y; j++)
            tab_test[i][j] = 0;

    // 1. recherche de la brick appellée par le click
    for(i=0; i<game->nb_brick_x; i++)
        for(j=0; j<game->nb_brick_y; j++)
            if(game->brick[i][j]->image == widget) { x = i; y = j; }

    // 2. info de debug générique
    if(DEBUG) printf("click sur la brick (%d, %d) : ", x, y);

    // 3. cas n°1 : la brique est vide
    if(is_empty_brick(game->brick[x][y]) && game->remaining_bricks.value > 0)
    {
        if(DEBUG) printf("ajout de la brick sur le plateau\n");

        // 3.1 décrémentation de la pile de pioche
        edit_displayed_int_value(&game->remaining_bricks, game->remaining_bricks.value-1);

        // 3.1 copie de la nouvelle brick sur le plateau
        temp_brick = copy_brick(game->next_brick);
        temp_brick->image = game->brick[x][y]->image;
        game->brick[x][y] = temp_brick;
        game->brick[x][y]->type = ATTACHED_BRICK;

        // 3.2 création d'une nouvelle brick dans la pile
        if(game->remaining_bricks.value > 0 || game->rules == SOLO_GAME_HARD) temp_brick = create_random_brick(game->rules, game->color);
        else temp_brick = create_empty_brick();
        temp_brick->image = game->next_brick->image;
        game->next_brick = temp_brick;

        // 3.3 raffraichssement de l'affichage
        g_signal_connect(game->next_brick->image, "expose-event", G_CALLBACK(on_next_brick_expose_event), game->next_brick);
        on_brick_table_expose_event(game->brick[x][y]->image, NULL, game);
        on_next_brick_expose_event(game->next_brick->image, NULL, game->next_brick);

        return FALSE;
    }

     // 4. cas n°2 : on recherche à savoir si une boucle est bouclée
    if(DEBUG) printf("recherche de boucle fermée\n");

    get_different_colors_from_brick(game->brick[x][y], &colors, &nb_color);

    for(i=0; i<nb_color; i++)
    {
        is_temp_closed_path = detect_looped_brick (TRUE, tab_test, game, x, y, 0, colors[i]);
        is_closed_path = is_closed_path || is_temp_closed_path;

        if(is_temp_closed_path) is_temp_closed_path = i;
        if(is_temp_closed_path) break;
    }

    if(is_closed_path)
    {
        if(DEBUG) printf("boucle fermée\n");
        destroy_game_board_bricks_from_path(game, tab_test, colors[is_temp_closed_path]);
        return FALSE;
    }
    else if(DEBUG) printf("boucle ouverte\n");

    // 5. cas n°3 : il y a fusion possible entre la brique suivante et la brique déjà en place
    if(!is_empty_brick(game->brick[x][y]) && are_superposable_bricks(game->brick[x][y], game->next_brick) && (game->remaining_bricks.value > 0 || game->rules == SOLO_GAME_HARD))
    {
        if(DEBUG) printf("fusion des 2 bricks\n");

        // 5.1 décrémentation de la pile de la pioche
        if(game->remaining_bricks.value > 0) edit_displayed_int_value(&game->remaining_bricks, game->remaining_bricks.value-1);

        // 5.2 fusion des 2 bricks
        fusion_bricks(game->brick[x][y], game->next_brick, &temp_brick, ATTACHED_BRICK);
        temp_brick->image = game->brick[x][y]->image;
        game->brick[x][y] = temp_brick;

        // 5.3 création d'une nouvelle brick
        if(game->remaining_bricks.value > 0 || game->rules == SOLO_GAME_HARD) temp_brick = create_random_brick(game->rules, game->color);
        else temp_brick = create_empty_brick();
        temp_brick->image = game->next_brick->image;
        game->next_brick = temp_brick;

        // 5.4 raffraichissement de l'affichage
        g_signal_connect(game->next_brick->image, "expose-event", G_CALLBACK(on_next_brick_expose_event), game->next_brick);
        on_brick_table_expose_event(game->brick[x][y]->image, NULL, game);
        on_next_brick_expose_event(game->next_brick->image, NULL, game->next_brick);

        return FALSE;
    }

    l = 1;

    // 6. si jamais le nombre de brique qu'il reste dans la pile est nul, on cherche à savoir si le jeu est fini ou pas
    if(game->remaining_bricks.value == 0)
    {
        // 6.1 en niveaux faciles et intermédiaires : si il n'y a plus de brick on ne peut plus rien faire
        if(game->rules == SOLO_GAME_EASY || game->rules == SOLO_GAME_MEDIUM) l = 0;

        // 6.2 en niveau dur, il faut vérifier que le joueur ne puisse plus fusionner sa brick avec l'une du tableau
        if(game->rules == SOLO_GAME_HARD)
        {
            l = 0;
            if(is_turnable_brick(game->next_brick))
            {
                for(k=0; k<MAX_NB_DIRECTION; k++)
                {
                    turn_brick(game->next_brick);
                    for(i=0; i<game->nb_brick_x; i++)
                        for(j=0; j<game->nb_brick_y; j++)
                            if(are_superposable_bricks(game->next_brick, game->brick[i][j]) && !is_empty_brick(game->brick[i][j])) l = 1;
                }
            }
            else
            {
                for(i=0; i<game->nb_brick_x; i++)
                    for(j=0; j<game->nb_brick_y; j++)
                        if(are_superposable_bricks(game->next_brick, game->brick[i][j]) && !is_empty_brick(game->brick[i][j])) l = 1;
            }
        }
    }
    // 7. sinon il faut vérifier que le joueur puisse placer sa brique sur le plateau
    else
    {
        l = 0;
        if(is_turnable_brick(game->next_brick))
        {
            for(k=0; k<MAX_NB_DIRECTION; k++)
            {
                turn_brick(game->next_brick);
                for(i=0; i<game->nb_brick_x; i++)
                    for(j=0; j<game->nb_brick_y; j++)
                        if(are_superposable_bricks(game->next_brick, game->brick[i][j])) l = 1;
            }
        }
        else
        {
            for(i=0; i<game->nb_brick_x; i++)
                for(j=0; j<game->nb_brick_y; j++)
                    if(are_superposable_bricks(game->next_brick, game->brick[i][j])) l = 1;
        }
    }

    if(l == 0)
    {
    	if(game->is_running) game_over(game);
    }
    else if(DEBUG) printf("pas de fin de jeu : des bricks sont encore superposables\n");

    return FALSE;
}

/* fonction recursive de détéction de boucle fermée */
int detect_looped_brick (int init, int* tab_test[], t_game_board * pt, int pos_x, int pos_y, int direction_of_parent, t_color* color_of_parent)
{
    // 0. déclaration des vartiables temporaires
    int i;
    int reciprocal_result = 1;
    int next_x = pos_x, next_y = pos_y, next_direction = 0;
    int direction_result = 1;

    // 1. on met à 1 la position à laquele on se trouve
    tab_test[pos_x][pos_y] = 1;

    // 2. on regarde le résultat de la recursion seulement si le nombre de stick est supérieur à 0
    if (pt->brick[pos_x][pos_y]->nb_stick > 0)
    {
        // 2.1 on vérifie la réciprocité entre les liens de parenté du parent et de son fils
        if(!init) reciprocal_result = check_relationship_beetween_bricks(pt, pos_x, pos_y, direction_of_parent, color_of_parent);

        // 2.2 puis, pour chaun de ses sticks, on applique la même fonction à ses fils
        for(i=0; i <pt->brick[pos_x][pos_y]->nb_stick; i++)
        {
            if(is_same_color(pt->brick[pos_x][pos_y]->stick[i].color, color_of_parent))
            {
                next_x = pos_x; next_y = pos_y;

                switch(pt->brick[pos_x][pos_y]->stick[i].direction)
                {
                    case BOTTOM:
                        next_direction = TOP;
                        if (pos_y < pt->nb_brick_y-1) next_y = pos_y+1;
                        else next_y = 0;
                    break;

                    case TOP:
                        next_direction = BOTTOM;
                        if (pos_y > 0) next_y = pos_y-1;
                        else next_y = pt->nb_brick_y-1;
                    break;

                    case RIGHT:
                        next_direction = LEFT;
                        if (pos_x < pt->nb_brick_x - 1) next_x = pos_x+1;
                        else next_x = 0;
                    break;

                    case LEFT:
                        next_direction = RIGHT;
                        if (pos_x > 0) next_x = pos_x-1;
                        else next_x = pt->nb_brick_x-1;
                    break;

                    default:
                    next_direction = MAX_NB_DIRECTION+1;
                    printf("problem : unexpeted default on switch");
                    break;
                }

                // 2.3 on applique la fonction à son fils, en faisant attention à boucler les interactions
                if(tab_test[next_x][next_y] == 0) direction_result = direction_result && detect_looped_brick (FALSE, tab_test, pt, next_x, next_y, next_direction, color_of_parent);
                else direction_result = direction_result && check_relationship_beetween_bricks(pt, next_x, next_y, next_direction, color_of_parent);
            }
        }
        return (direction_result && reciprocal_result);
    }
    return 0;
}

/* fonction de rotation de la prochaine brick en cas de clic dessus */
gboolean on_next_brick_click_event(GtkWidget *widget, GdkEventExpose *event, gpointer pt)
{
    // 0. déclaration des variables locales
    t_game_board* game = (t_game_board*)pt;

    // 1. on ré-expose la brick si la rotation de la brick a été faite
    if(turn_brick(game->next_brick)) on_next_brick_expose_event(game->next_brick->image, NULL, game->next_brick);

    // 2. on return faux pour empêcher l'évenement de se re-déclencher
    return FALSE;
}

/* fonction servat à vérifier qu'un fils a bien une realtion avec le aprent qui l'a appelé */
int check_relationship_beetween_bricks(t_game_board* pt, int pos_x, int pos_y, int direction, t_color* color)
{
    // 0. déclaration des variables locales
    int i =0;

    // 1. on regarde si on trouve le stick recherché
    for(i=0; i <pt->brick[pos_x][pos_y]->nb_stick; i++)
        if(pt->brick[pos_x][pos_y]->stick[i].direction == direction && is_same_color(pt->brick[pos_x][pos_y]->stick[i].color, color)) return 1;

    // 2. si on le trouve pas on retourne zero
    return 0;
}

/* fonction appellée à la fin du jeu */
void game_over(t_game_board* game)
{
    // 0. déclaration des variables temporaires
    t_best_score* list = load_best_scores();
    char* pseudo = NULL;

    // 1. print de debug
    if(DEBUG) printf("fin de jeu\n");

    // 2. on arrête le jeu
    game->is_running = FALSE;

    // 3. on demande son pseudo au joueur
    pseudo = display_game_over(game);

    // 4. on insère le score dans la liste des meilleurs scores
    if(strcmp(pseudo,"") != 0) list = insert_in_best_score(list, game->score.value, pseudo);
    if(strcmp(pseudo,"") != 0) save_best_scores(list);

    // 5. on affiche les meilleurs scores
    display_best_score(game->window);
}
