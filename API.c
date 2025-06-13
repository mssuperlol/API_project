#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#define STRING_MAX_LENGTH 256
#define DIM_RECETTEAR ('Z' - 'A' + 1) * 2 + 1    //chars from 'a' to 'z' + slot for '_' and numbers

/**
 * Ingredient struct
 * name: string of the name of the ingredient
 * quantity: amount of the ingredient for the recipe
 * next: pointer to the next ingredient in the recipe
 */
typedef struct ING {
    char *name;
    int quantity;
    struct ING *next;
} ingredient;

/**
 * Recipe struct
 * name: string of the name of the recipe
 * ing: list of the ingredients, in alphabetic order
 * father: pointer to the father of the current recipe in the tree
 * left: pointer to the recipe that comes before in alphabetic order
 * right: pointer to the recipe that comes after in alphabetic order
 */
typedef struct REC {
    char *name;
    ingredient *ing;
    struct REC *father;
    struct REC *left;
    struct REC *right;
} recipe;

/**
 * Lot struct
 * quantity: amount of ingredients in the lot
 * t_expiration: time of expiration of the ingredients
 * next: pointer to the next lot of the same ingredient, in order of expiration date
 */
typedef struct LOT {
    int quantity;
    int t_expiration;
    struct LOT *next;
} lot;

/**
 * Tag lot struct, that contains all the lots of the same ingredient
 * name: name of the ingredient
 * stock: list of the lots with the same ingredient
 * tot_quantity: total quantity of all the lots
 * father: pointer to the father of the current tag in the tree
 * left: pointer to the tag that comes before in alphabetic order
 * right: pointer to the tag that comes after in alphabetic order
 */
typedef struct TAG {
    char *name;
    lot *stock;
    int tot_quantity;
    struct TAG *father;
    struct TAG *left;
    struct TAG *right;
} tag_lot;

/**
 * Order struct
 * time: time when the order is received
 * name: name of the ordered product
 * quantity: amount of ordered products
 * weight: weight of the order
 * this_recipe: pointer to the recipe of the ordered product
 * is_waiting: whether the order is prepared or not
 */
typedef struct ORD {
    int time;
    char *name;
    int quantity;
    int weight;
    ingredient *this_recipe;
    bool is_waiting;
} order;

/**
 * Alt order struct, to store the orders while they are being taken by the courier
 * time: time when the order is received
 * name: name of the ordered product
 * quantity: amount of the ordered products
 * weight: weight of the order
 * next: pointer to the next alt_order, ordered by descending weight
 */
typedef struct ALT {
    int time;
    char *name;
    int quantity;
    int weight;
    struct ALT *next;
} alt_order;

void expiration(int, tag_lot *[]);

void expiration_iterative(int, tag_lot *);

void load_order(order *, int, int);

void add_recipe(char [], recipe *[]);

void remove_recipe(char [], recipe *[], order *, int);

void restock(char [], tag_lot *[], int);

void recheck_orders(tag_lot *[], order *, int);

int new_order(char [], recipe *[], tag_lot *[], int, order **, int);

int next_word(char [], int, char []);

int find_index(char);

int calculate_quantity(lot *);

bool check_ingredients_for_order(tag_lot *[], order *);

recipe *find_recipe(recipe *, char []);

tag_lot *find_lot(tag_lot *, char []);

void free_recipes(recipe *);

void free_lots(tag_lot *);

int main() {
    int delta_T = 0, max_load = 0, t = 0, dim_ordini = 0;
    bool exit = false;
    char *instruction = NULL;
    recipe *recipe_book[DIM_RECETTEAR] = {};
    tag_lot *warehouse[DIM_RECETTEAR] = {};
    order *orders = malloc(sizeof(order));

    instruction = malloc(sizeof(char) * STRING_MAX_LENGTH);
    strcpy(instruction, "");

    if (fgets(instruction, STRING_MAX_LENGTH, stdin) != NULL) {
        sscanf(instruction, "%d %d", &delta_T, &max_load);
        free(instruction);
    } else
        return 1;

    while (!exit) {
        expiration(t, warehouse);

        if (t != 0 && t % delta_T == 0) //courier arrives
            load_order(orders, dim_ordini, max_load);

        int buffer_size = STRING_MAX_LENGTH;
        bool input_read = false;
        char *buffer = NULL;
        buffer = malloc(sizeof(char) * buffer_size);
        instruction = malloc(sizeof(char) * buffer_size);
        strcpy(buffer, "");
        strcpy(instruction, "");

        while (!input_read) {
            char c = ' ';
            int j = 0;

            if (fgets(buffer, buffer_size, stdin) != NULL) {
                strcat(instruction, buffer);

                for (j = 0; j < buffer_size && c != '\n' && c != '\0'; j++) {
                    c = buffer[j];
                }

                if (j == buffer_size) {
                    buffer_size = buffer_size * 2;
                    free(buffer);
                    buffer = malloc(sizeof(char) * buffer_size);
                    instruction = realloc(instruction, sizeof(char) * (strlen(instruction) + buffer_size));
                    input_read = false;
                } else {
                    input_read = true;
                }
            } else {
                exit = true;
                input_read = true;
            }
        }

        if (exit != true) {
            char parola[STRING_MAX_LENGTH] = "";
            sscanf(instruction, "%s", parola);

            if (strcmp(parola, "aggiungi_ricetta") == 0) //ADD RECIPES
            {
                add_recipe(instruction, recipe_book);
            } else if (strcmp(parola, "rimuovi_ricetta") == 0) //REMOVE RECIPES
            {
                remove_recipe(instruction, recipe_book, orders, dim_ordini);
            } else if (strcmp(parola, "rifornimento") == 0) //RESTOCK
            {
                restock(instruction, warehouse, t);
                recheck_orders(warehouse, orders, dim_ordini);
            } else if (strcmp(parola, "ordine") == 0) //ORDER
            {
                dim_ordini = new_order(instruction, recipe_book, warehouse, t, &orders, dim_ordini);
            } else //WRONG COMMAND OR INSTRUCTIONS END
            {
                exit = true;
            }

            t++;
        }

        free(instruction);
        free(buffer);
    }

    //clean up of dynamically allocated memories
    for (int i = 0; i < DIM_RECETTEAR; i++) {
        free_recipes(recipe_book[i]);
    }

    for (int i = 0; i < DIM_RECETTEAR; i++) {
        free_lots(warehouse[i]);
    }

    for (int i = 0; i < dim_ordini; i++) {
        if (orders[i].time != -1)
            free(orders[i].name);
    }

    free(orders);

    return 0;
}

/**
 * Parses the entire warehouse and removes expired ingredients
 * @param t current time
 * @param warehouse warehouse to remove expired ingredients from
 */
void expiration(int t, tag_lot *warehouse[]) {
    for (int i = 0; i < DIM_RECETTEAR; i++)
        //NB: warehouse stores lots by letter, and they are sorted in alphabetic order
        expiration_iterative(t, warehouse[i]);

    return;
}

/**
 * Parses a specific warehouse subtree and removes expired ingredients
 * NB: lots are stored as a list
 * @param t current time
 * @param current_node current node of the tree
 */
void expiration_iterative(int t, tag_lot *current_node) {
    if (current_node != NULL) {
        expiration_iterative(t, current_node->left);

        lot *current_lotto = current_node->stock;
        bool any_expired = false;

        while (current_lotto != NULL) {
            if (t >= current_lotto->t_expiration) {
                lot *da_eliminare = current_lotto;
                current_lotto = current_lotto->next;
                current_node->stock = current_lotto;
                free(da_eliminare);

                any_expired = true;
            } else {
                current_lotto = current_lotto->next;
            }
        }

        if (any_expired) {
            current_node->tot_quantity = calculate_quantity(current_node->stock);
        }

        expiration_iterative(t, current_node->right);
    }
}

/**
 * Parses orders and selects enough orders to fill the courier. Loaded orders are moved to a list (ordered by decreasing weight) to be printed and freed
 * @param orders vector that contains all the received orders
 * @param orders_dim dimension of orders
 * @param max_load maximum load that the courier can carry
 */
void load_order(order *orders, int orders_dim, int max_load) {
    int current_load = 0;
    alt_order *orders_to_load = NULL, *temp = NULL, *prev = NULL, *current_ord = NULL;
    bool is_full = false;

    for (int i = 0; i < orders_dim && !is_full; i++) {
        if (orders[i].time != -1 && orders[i].is_waiting == false) {
            if (current_load + orders[i].weight <= max_load) {
                current_load += orders[i].weight;

                current_ord = malloc(sizeof(alt_order));
                current_ord->name = malloc(sizeof(char) * (strlen(orders[i].name) + 1));
                strcpy(current_ord->name, orders[i].name);
                current_ord->weight = orders[i].weight;
                current_ord->quantity = orders[i].quantity;
                current_ord->time = orders[i].time;

                if (orders_to_load == NULL) {
                    //orders_to_load is empty
                    orders_to_load = current_ord;
                    current_ord->next = NULL;
                } else {
                    //orders_to_load is not empty => puts the new order in the proper place
                    temp = orders_to_load;
                    prev = NULL;

                    while (temp != NULL && (current_ord->weight < temp->weight || (
                                                current_ord->weight == temp->weight && current_ord->time > temp->
                                                time))) {
                        prev = temp;
                        temp = temp->next;
                    }

                    if (prev == NULL) {
                        current_ord->next = orders_to_load;
                        orders_to_load = current_ord;
                    } else {
                        prev->next = current_ord;
                        current_ord->next = temp;
                    }
                }

                orders[i].time = -1;
                free(orders[i].name);
            } else {
                is_full = true;
            }
        }
    }

    if (orders_to_load == NULL) {
        printf("camioncino vuoto\n");
    } else {
        while (orders_to_load != NULL) {
            printf("%d %s %d\n", orders_to_load->time, orders_to_load->name, orders_to_load->quantity);
            alt_order *da_eliminare = orders_to_load;
            orders_to_load = orders_to_load->next;
            free(da_eliminare->name);
            free(da_eliminare);
        }
    }

    return;
}

/**
 * Parses the alphabetically ordered list associated to the first letter of the new recipe and places it in the correct spot (if it's not already present)
 * @param instruction string that contains the new recipe
 * @param recipe_book vector that contains all recipes list (separated by the initial letter), which contains all the current recipes
 */
void add_recipe(char instruction[], recipe *recipe_book[]) {
    //index of the recipe = first letter of the name of the recipe - 'a'
    int cursor = strlen("aggiungi_ricetta "), index = find_index(instruction[strlen("aggiungi_ricetta ")]), temp_int;
    char temp_string[STRING_MAX_LENGTH] = "";

    //TREE-INSERT
    cursor = next_word(instruction, cursor, temp_string);
    recipe *prev_ric = NULL, *curr_ric = recipe_book[index];

    while (curr_ric != NULL) {
        prev_ric = curr_ric;
        temp_int = strcmp(temp_string, curr_ric->name);

        if (temp_int < 0) {
            curr_ric = curr_ric->left;
        } else if (temp_int > 0) {
            curr_ric = curr_ric->right;
        } else {
            //a recipe with the same name is already present
            printf("ignorato\n");
            return;
        }
    }

    recipe *new_recipe = malloc(sizeof(recipe));
    new_recipe->name = malloc(sizeof(char) * (strlen(temp_string) + 1));
    strcpy(new_recipe->name, temp_string);
    new_recipe->father = prev_ric;
    new_recipe->left = NULL;
    new_recipe->right = NULL;

    if (prev_ric == NULL) {
        recipe_book[index] = new_recipe;
    } else {
        temp_int = strcmp(new_recipe->name, prev_ric->name);
        if (temp_int < 0) {
            prev_ric->left = new_recipe;
        } else {
            prev_ric->right = new_recipe;
        }
    }
    //END TREE-INSERT

    ingredient *new_ing = malloc(sizeof(ingredient));
    new_recipe->ing = new_ing;

    strcpy(temp_string, "");
    cursor = next_word(instruction, cursor, temp_string);
    new_ing->name = malloc(sizeof(char) * (strlen(temp_string) + 1));
    strcpy(new_ing->name, temp_string);
    strcpy(temp_string, "");
    cursor = next_word(instruction, cursor, temp_string);
    new_ing->quantity = strtol(temp_string, NULL, 10);
    strcpy(temp_string, "");
    new_ing->next = NULL;

    //puts the rest of the ingredients in alphabetic order
    while (cursor < strlen(instruction)) {
        ingredient *curr_ing = new_recipe->ing, *prev = NULL;
        new_ing = malloc(sizeof(ingredient));

        cursor = next_word(instruction, cursor, temp_string);
        new_ing->name = malloc(sizeof(char) * (strlen(temp_string) + 1));
        strcpy(new_ing->name, temp_string);
        strcpy(temp_string, "");
        cursor = next_word(instruction, cursor, temp_string);
        new_ing->quantity = strtol(temp_string, NULL, 10);
        strcpy(temp_string, "");

        while (curr_ing != NULL && strcmp(curr_ing->name, new_ing->name) < 0) {
            prev = curr_ing;
            curr_ing = curr_ing->next;
        }

        new_ing->next = curr_ing;
        if (prev == NULL) {
            new_recipe->ing = new_ing;
        } else {
            prev->next = new_ing;
        }
    }

    printf("aggiunta\n");
    return;
}

//scorre la lista associata alla prima lettera della ricetta e cerca una ricetta col nome uguale a quella fornita in input, per poi eliminarla
/**
 * Parses the "recipe first letter" list and searches for a recipe with the same name. If found, eliminates it
 * @param instruction string that contains the name of the recipe to remove
 * @param recipe_book vector with all the recipes
 * @param orders vector that contains all the received orders
 * @param orders_dim dimension of orders
 */
void remove_recipe(char instruction[], recipe *recipe_book[], order *orders, int orders_dim) {
    char old_recipe[STRING_MAX_LENGTH] = "";
    int cursor = strlen("rimuovi_ricetta "), index = find_index(instruction[strlen("rimuovi_ricetta ")]), temp_int = -1;

    cursor = next_word(instruction, cursor, old_recipe);
    recipe *temp_ric = recipe_book[index], *y = NULL, *x = NULL;

    //TREE-DELETE
    while (temp_ric != NULL && temp_int != 0) {
        temp_int = strcmp(old_recipe, temp_ric->name);
        if (temp_int < 0)
            temp_ric = temp_ric->left;
        else if (temp_int > 0)
            temp_ric = temp_ric->right;
    }

    if (temp_ric == NULL) {
        printf("non presente\n");
        return;
    }

    bool is_on_standby = false;
    //parses orders to find if the recipe has on standby orders
    for (int i = 0; i < orders_dim && !is_on_standby; i++) {
        if (orders[i].time != -1 && strcmp(orders[i].name, old_recipe) == 0)
            is_on_standby = true;
    }

    if (is_on_standby) {
        printf("ordini in sospeso\n");
        return;
    }

    if (temp_ric->left == NULL || temp_ric->right == NULL)
        y = temp_ric;
    else {
        //y = TREE-SUCCESSOR(temp_ric)
        if (temp_ric->right != NULL) {
            //y = TREE-MINIMUM(temp_ric->right)
            recipe *x = temp_ric->right;
            while (x->left != NULL)
                x = x->left;
            y = x;
        } else {
            recipe *z = temp_ric->father, *x = temp_ric;
            while (z != NULL && x == z->right) {
                x = z;
                z = z->father;
            }

            y = z;
        }
    }

    if (y->left != NULL)
        x = y->left;
    else
        x = y->right;

    if (x != NULL)
        x->father = y->father;

    if (y->father == NULL)
        recipe_book[index] = x;
    else if (y == y->father->left)
        y->father->left = x;
    else
        y->father->right = x;

    if (y != temp_ric) {
        ingredient *temp_ing = temp_ric->ing;
        strcpy(temp_ric->name, y->name);
        temp_ric->ing = y->ing;
        y->ing = temp_ing;
    }

    while (y->ing != NULL) {
        ingredient *da_eliminare = y->ing;
        y->ing = y->ing->next;
        free(da_eliminare->name);
        free(da_eliminare);
    }

    free(y->name);
    free(y);
    printf("rimossa\n");

    return;
}

/**
 * For every triple "word-quantity-expiration time" in instruction, and places them in the warehouse in order of expiration time
 * @param instruction string that contains the name of the recipe to remove
 * @param warehouse warehouse to add the ingredients to
 * @param t current time
 */
void restock(char instruction[], tag_lot *warehouse[], int t) {
    char ingrediente[STRING_MAX_LENGTH] = "", temp_int[STRING_MAX_LENGTH] = "";
    int amount = 0, expiration_date = 0, cursor = strlen("rifornimento "), indice;

    while (cursor < strlen(instruction)) {
        strcpy(ingrediente, "");
        cursor = next_word(instruction, cursor, ingrediente);
        strcpy(temp_int, "");
        cursor = next_word(instruction, cursor, temp_int);
        amount = strtol(temp_int, NULL, 10);
        strcpy(temp_int, "");
        cursor = next_word(instruction, cursor, temp_int);
        expiration_date = strtol(temp_int, NULL, 10);

        if (expiration_date > t) {
            indice = find_index(ingrediente[0]);
            tag_lot *current_tag = warehouse[indice], *prev_tag = NULL;
            bool tag_trovato = false;

            //TREE-INSERT
            while (current_tag != NULL && !tag_trovato) {
                prev_tag = current_tag;
                int temp_int = strcmp(ingrediente, current_tag->name);

                if (temp_int < 0)
                    current_tag = current_tag->left;
                else if (temp_int > 0)
                    current_tag = current_tag->right;
                else
                    tag_trovato = true;
            }

            if (!tag_trovato) {
                //a tag with that name doesn't exist => creates a new tag and inserts it
                tag_lot *new = malloc(sizeof(tag_lot));
                new->name = malloc(sizeof(char) * (strlen(ingrediente) + 1));
                strcpy(new->name, ingrediente);
                new->tot_quantity = amount;
                new->stock = malloc(sizeof(lot));
                new->stock->quantity = amount;
                new->stock->t_expiration = expiration_date;
                new->stock->next = NULL;
                new->left = NULL;
                new->right = NULL;
                new->father = prev_tag;

                if (prev_tag == NULL) {
                    warehouse[indice] = new;
                } else if (strcmp(new->name, prev_tag->name) < 0) {
                    prev_tag->left = new;
                } else {
                    prev_tag->right = new;
                }
            } else {
                //a tag with that name already exists => inserts the lot in the list
                lot *current_lot = current_tag->stock, *prev_lot = NULL;

                while (current_lot != NULL && current_lot->t_expiration < expiration_date) {
                    prev_lot = current_lot;
                    current_lot = current_lot->next;
                }

                if (current_lot == NULL || current_lot->t_expiration != expiration_date) {
                    //a lot with the same expiration date doesn't exist => creates a new lot
                    lot *new = malloc(sizeof(lot));
                    new->quantity = amount;
                    new->t_expiration = expiration_date;
                    new->next = current_lot;

                    if (prev_lot == NULL) {
                        current_tag->stock = new;
                    } else {
                        prev_lot->next = new;
                    }
                } else {
                    //a lot with the same expiration date already exists => updates its quantity
                    current_lot->quantity += amount;
                }

                current_tag->tot_quantity += amount;
            }
        }
    }

    printf("rifornito\n");
    return;
}

/**
 * Parses the orders to check if some stand-by orders can now be prepared
 * @param warehouse warehouse to check the stored ingredients
 * @param orders vector that contains all the received orders
 * @param orders_dim dimension of orders
 */
void recheck_orders(tag_lot *warehouse[], order *orders, int orders_dim) {
    for (int i = 0; i < orders_dim; i++) {
        if (orders[i].time != -1 && orders[i].is_waiting == true && check_ingredients_for_order(
                warehouse, &orders[i])) {
            orders[i].is_waiting = false;
        }
    }

    return;
}

/**
 * Creates a new order (if there's a recipe with the same name), and checks if it can be prepared
 * @param instruction string with the new order
 * @param recipe_book vector with all the recipes
 * @param warehouse warehouse with the ingredients, to check if the new order can be prepared
 * @param t current time
 * @param orders vector that contains all the received orders, to be updated with the new order
 * @param orders_dim dimension of orders
 * @return updated orders_dim
 */
int new_order(char instruction[], recipe *recipe_book[], tag_lot *warehouse[], int t, order **orders, int orders_dim) {
    char nome_ordine[STRING_MAX_LENGTH] = "", num_temp[STRING_MAX_LENGTH] = "";
    int quantity_ricetta = 0, cursor = strlen("ordine "), indice = find_index(instruction[strlen("ordine ")]);
    recipe *current_rec = NULL;

    cursor = next_word(instruction, cursor, nome_ordine);
    cursor = next_word(instruction, cursor, num_temp);
    quantity_ricetta = strtol(num_temp, NULL, 10);

    current_rec = find_recipe(recipe_book[indice], nome_ordine);

    if (current_rec == NULL || strcmp(current_rec->name, nome_ordine) != 0) {
        //the recipe with that name isn't present in the recipe book
        printf("rifiutato\n");
    } else {
        //the recipe is precent in the recipe book => realloc of orders with an extra cell
        printf("accettato\n");

        (*orders) = realloc((*orders), sizeof(order) * (orders_dim + 1));

        (*orders)[orders_dim].name = malloc(sizeof(char) * (strlen(nome_ordine) + 1));
        strcpy((*orders)[orders_dim].name, nome_ordine);
        (*orders)[orders_dim].weight = 0;
        (*orders)[orders_dim].quantity = quantity_ricetta;
        (*orders)[orders_dim].time = t;
        (*orders)[orders_dim].this_recipe = current_rec->ing;
        (*orders)[orders_dim].is_waiting = !check_ingredients_for_order(warehouse, &(*orders)[orders_dim]);

        orders_dim++;
    }

    return orders_dim;
}

/**
 * Checks the ingredients in the warehouse: if there are enough for the order, it's prepared
 * @param warehouse warehouse to check the ingredients
 * @param current_order order to check the ingredients for
 * @return false if the order couldn't be prepared, true otherwise
 */
bool check_ingredients_for_order(tag_lot *warehouse[], order *current_order) {
    ingredient *current_ing = current_order->this_recipe;
    tag_lot *current_tag = NULL;
    int quantity = current_order->quantity;
    bool enough_ingredients = true;

    while (current_ing != NULL && enough_ingredients) {
        //parses the recipe ingredients
        current_tag = find_lot(warehouse[find_index(current_ing->name[0])], current_ing->name);

        if (current_tag == NULL || current_tag->tot_quantity < current_ing->quantity * quantity) {
            enough_ingredients = false;
        } else {
            current_ing = current_ing->next;
        }
    }

    if (enough_ingredients) {
        //if there are enough ingredients, removes used ones from the warehouse
        current_ing = current_order->this_recipe;
        int peso = 0, current_ing_quantity = 0;

        while (current_ing != NULL) {
            current_tag = find_lot(warehouse[find_index(current_ing->name[0])], current_ing->name);
            current_ing_quantity = current_ing->quantity * quantity;
            peso += current_ing_quantity;

            //IN THEORY, it shouldn't be necessary to check if current_tag != NULL
            lot *current_lot = current_tag->stock;

            while (current_lot != NULL && current_ing_quantity > 0) {
                if (current_ing_quantity >= current_lot->quantity) {
                    current_ing_quantity -= current_lot->quantity;

                    lot *da_eliminare = current_lot;
                    current_lot = current_lot->next;
                    current_tag->stock = current_lot;

                    free(da_eliminare);
                } else {
                    current_lot->quantity -= current_ing_quantity;
                    current_ing_quantity = 0;
                }
            }

            current_tag->tot_quantity = calculate_quantity(current_tag->stock);

            current_ing = current_ing->next;
        }

        current_order->weight = peso;
    }

    return (enough_ingredients);
}

/**
 * From a phrase vector, copies a word that starts from "phrase[index]" in the word vector
 * @param phrase
 * @param index
 * @param word
 * @return index of the following word
 */
int next_word(char phrase[], int index, char word[]) {
    int i = 0, lunghezza = strlen(phrase);
    char empty[STRING_MAX_LENGTH] = "";
    strcpy(word, empty);

    while (index < lunghezza && phrase[index] != ' ' && phrase[index] != '\0' && phrase[index] != '\n' && phrase[index]
           != '\r') {
        word[i] = phrase[index];
        i++;
        index++;
    }

    word[i] = '\0';
    return ++index;
}

/**
 * @param c character
 * @return index of the corresponding cell of c in the recipe book
 */
int find_index(char c) {
    if ('a' <= c && c <= 'z') {
        return (c - 'a' + 'Z' - 'A' + 1);
    } else if ('A' <= c && c <= 'Z') {
        return (c - 'A');
    } else {
        return (DIM_RECETTEAR - 1);
    }
}

/**
 * @param temp lot to calculate the quantity of
 * @return the quantity of ingredients in the given lot
 */
int calculate_quantity(lot *temp) {
    lot *current_lot = temp;
    int tot_quantity = 0;

    while (current_lot != NULL) {
        tot_quantity += current_lot->quantity;
        current_lot = current_lot->next;
    }

    return tot_quantity;
}

/**
 * TREE-SEARCH for recipe
 * @param x head of the current subtree
 * @param k name of the recipe to search
 * @return the recipe, or null if not found
 */
recipe *find_recipe(recipe *x, char k[]) {
    if (x == NULL)
        return x;

    int temp_int = strcmp(k, x->name);

    if (temp_int == 0)
        return x;
    else if (temp_int < 0)
        return find_recipe(x->left, k);
    else
        return find_recipe(x->right, k);
}

/**
 * TREE-SEARCH for tag_lot
 * @param x head of the current subtree
 * @param k name of the ingredient to search the lot of
 * @return the lot, or null if not found
 */
tag_lot *find_lot(tag_lot *x, char k[]) {
    if (x == NULL)
        return x;

    int temp_int = strcmp(k, x->name);

    if (temp_int == 0)
        return x;
    else if (temp_int < 0)
        return find_lot(x->left, k);
    else
        return find_lot(x->right, k);
}

/**
 * Recursively deallocates the temp recipe tree
 * @param temp head of the current recipe subtree
 */
void free_recipes(recipe *temp) {
    if (temp != NULL) {
        free_recipes(temp->left);
        free_recipes(temp->right);

        while (temp->ing != NULL) {
            ingredient *da_eliminare = temp->ing;
            temp->ing = temp->ing->next;
            free(da_eliminare->name);
            free(da_eliminare);
        }

        recipe *da_eliminare = temp;
        free(da_eliminare->name);
        free(da_eliminare);
    }

    return;
}

/**
 * Recursively deallocates the temp lot tree
 * @param temp head of the current lot subtree
 */
void free_lots(tag_lot *temp) {
    if (temp != NULL) {
        free_lots(temp->left);
        free_lots(temp->right);

        while (temp->stock != NULL) {
            lot *da_eliminare = temp->stock;
            temp->stock = temp->stock->next;
            free(da_eliminare);
        }

        tag_lot *da_eliminare = temp;
        free(da_eliminare->name);
        free(da_eliminare);
    }
}
