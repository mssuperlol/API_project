#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#define STRING_MAX_LENGTH 256
#define DIM_RICETTARIO ('Z' - 'A' + 1) * 2 + 1    //caratteri dalla 'a' alla 'z' + slot per '_' e numeri

typedef struct ING{
    char *nome;
    int quantity;
    struct ING *next;
} ingrediente;

typedef struct RIC{
    char *nome;
    ingrediente *ing;
    struct RIC *padre;
    struct RIC *left;
    struct RIC *right;
} ricetta;

typedef struct LOT{
    int quantity;
    int t_scadenza; 
    struct LOT *next;
} lotto;

typedef struct TAG{
    char *nome;
    lotto *stock;
    int tot_quantity;
    struct TAG *padre;
    struct TAG *left;
    struct TAG *right;
} tag_lotto;

typedef struct ORD{
    int tempo;
    char *nome;
    int quantity;
    int peso;
    ingrediente *this_ricetta;
    bool in_attesa;
} ordine;

typedef struct ALT{
    int tempo;
    char *nome;
    int quantity;
    int peso;
    struct ALT *next;
} alt_ordine;

void scadenze(int, tag_lotto *[]);
void scadenze_iterativo(int , tag_lotto *);
void carica_ordine(ordine *, int, int);
void aggiungi_ricetta(char[], ricetta *[], tag_lotto *[]);
void rimuovi_ricetta(char[], ricetta *[], ordine *, int);
void rifornimento(char[], tag_lotto *[], int);
void ricontrolla_ordini(tag_lotto *[], ordine *, int);
int nuovo_ordine(char[], ricetta *[], tag_lotto *[], int, ordine **, int);
int next_word(char[], int, char[]);
int trova_indice(char);
int calcola_quantity(lotto *);
bool check_ingredients_for_order(tag_lotto *[], ordine *);
ricetta *ricerca_ricetta(ricetta *, char[]);
tag_lotto *ricerca_lotto(tag_lotto *, char []);
void libera_ricetta(ricetta *);
void libera_lotti(tag_lotto *);

int main()
{
    int delta_T = 0, max_load = 0, t = 0, dim_ordini = 0;
    bool uscire = false;
    char *istruzioni = NULL;
    ricetta *ricettario[DIM_RICETTARIO] = {};
    tag_lotto *magazzino[DIM_RICETTARIO] = {};
    ordine *vettore_ordini = malloc(sizeof(ordine));

    istruzioni = malloc(sizeof(char) * STRING_MAX_LENGTH);
    strcpy(istruzioni, "");

    if(fgets(istruzioni, STRING_MAX_LENGTH, stdin) != NULL)
    {
        sscanf(istruzioni, "%d %d", &delta_T, &max_load);
        free(istruzioni);
    }
    else
        return 1;

    while(!uscire)
    {
        scadenze(t, magazzino);

        if(t != 0 && t%delta_T == 0)    //arrivo corriere
            carica_ordine(vettore_ordini, dim_ordini, max_load);

        int buffer_size = STRING_MAX_LENGTH;
        bool letto_tutto = false;
        char *buffer = NULL;
        buffer = malloc(sizeof(char) * buffer_size);
        istruzioni = malloc(sizeof(char) * buffer_size);
        strcpy(buffer, "");
        strcpy(istruzioni, "");

        while(!letto_tutto)
        {
            char c = ' ';
            int j = 0;

            if(fgets(buffer, buffer_size, stdin) != NULL)
            {
                strcat(istruzioni, buffer);

                for(j = 0; j < buffer_size && c != '\n' && c != '\0'; j++)
                {
                    c = buffer[j];
                }

                if(j == buffer_size)
                {
                    buffer_size = buffer_size*2;
                    free(buffer);
                    buffer = malloc(sizeof(char) * buffer_size);
                    istruzioni = realloc(istruzioni, sizeof(char) * (strlen(istruzioni) + buffer_size));
                    letto_tutto = false;
                }
                else
                {
                    letto_tutto = true;
                }
            }
            else
            {
                uscire = true;
                letto_tutto = true;
            }
        }

        if(uscire != true)
        {
            char parola[STRING_MAX_LENGTH] = "";
            sscanf(istruzioni, "%s", parola);

            if(strcmp(parola, "aggiungi_ricetta") == 0)    //AGGIUNGI RICETTE
            {
                aggiungi_ricetta(istruzioni, ricettario, magazzino);
            }
            else if(strcmp(parola, "rimuovi_ricetta") == 0)   //RIMUOVI RICETTE
            {
                rimuovi_ricetta(istruzioni, ricettario, vettore_ordini, dim_ordini);
            }
            else if(strcmp(parola, "rifornimento") == 0)   //RIFORNIMENTO
            {
                rifornimento(istruzioni, magazzino, t);
                ricontrolla_ordini(magazzino, vettore_ordini, dim_ordini);
            }
            else if(strcmp(parola, "ordine") == 0)   //ORDINE
            {
                dim_ordini = nuovo_ordine(istruzioni, ricettario, magazzino, t, &vettore_ordini, dim_ordini);
            }
            else    //COMANDO ERRATO O FINE ISTRUZIONI
            {
                uscire = true;
            }
            
            t++;
            
        }

        free(istruzioni);
        free(buffer);
    }

    //clean up delle memorie allocate dinamicamente
    for(int i = 0; i < DIM_RICETTARIO; i++)
    {
        libera_ricetta(ricettario[i]);
    }

    for(int i = 0; i < DIM_RICETTARIO; i++)
    {
        libera_lotti(magazzino[i]);
    }
    
    for(int i = 0; i < dim_ordini; i ++)
    {
        if(vettore_ordini[i].tempo != -1)
            free(vettore_ordini[i].nome);
    }

    free(vettore_ordini);

    return 0;
}

//scorre l'intero magazzino e toglie gli ingredienti scaduti
void scadenze(int t, tag_lotto *magazzino[])
{
    for(int i = 0; i < DIM_RICETTARIO; i++)     //NB: il magazzino suddivide i lotti per lettera e sono ordinate in ordine alfabetico
        scadenze_iterativo(t, magazzino[i]);
    
    return;
}

//scorre un sottoalbero specifico del magazzino e toglie gli ingredienti scaduti (NB: i lotti veri e propri sono memorizzati come una lista)
void scadenze_iterativo(int t, tag_lotto *current_nodo)
{
    if(current_nodo != NULL)
    {
        scadenze_iterativo(t, current_nodo->left);

        lotto *current_lotto = current_nodo->stock;
        bool any_expired = false;
            
        while(current_lotto != NULL)
        {
            if(t >= current_lotto->t_scadenza)
            {
                lotto *da_eliminare = current_lotto;
                current_lotto = current_lotto->next;
                current_nodo->stock = current_lotto;
                free(da_eliminare);

                any_expired = true;
            }
            else
            {
                current_lotto = current_lotto->next;
            }
        }

        if(any_expired)
        {
            current_nodo->tot_quantity = calcola_quantity(current_nodo->stock);
        }

        scadenze_iterativo(t, current_nodo->right);
    }
}

/*scorre la lista degli ordini pronti, prendendo tot ordini fino a riempire il corriere
questi ordini presi vengono spostati in una nuova lista, ordinata per peso decrescente, che poi viene letta e liberata*/
void carica_ordine(ordine *vettore_ordini, int dim_ordini, int max_load)
{
    int current_load = 0;
    alt_ordine *ordini_da_caricare = NULL, *temp = NULL, *prev = NULL, *current_ord = NULL;
    bool is_full = false;

    for(int i = 0; i < dim_ordini && !is_full; i++)
    {
        if(vettore_ordini[i].tempo != -1 && vettore_ordini[i].in_attesa == false)
        {
            if(current_load + vettore_ordini[i].peso <= max_load)
            {
                current_load += vettore_ordini[i].peso;

                current_ord = malloc(sizeof(alt_ordine));
                current_ord->nome = malloc(sizeof(char) * (strlen(vettore_ordini[i].nome) + 1));
                strcpy(current_ord->nome, vettore_ordini[i].nome);
                current_ord->peso = vettore_ordini[i].peso;
                current_ord->quantity = vettore_ordini[i].quantity;
                current_ord->tempo = vettore_ordini[i].tempo;

                if(ordini_da_caricare == NULL)  //la lista degli ordini da caricare è vuota
                {
                    ordini_da_caricare = current_ord;
                    current_ord->next = NULL;
                }
                else    //la lista degli ordini da caricare non è vuota ==> metto il nuovo ordine ordinato per peso decrescente
                {
                    temp = ordini_da_caricare;
                    prev = NULL;

                    while(temp != NULL && (current_ord->peso < temp->peso || (current_ord->peso == temp->peso && current_ord->tempo > temp->tempo)))
                    {
                        prev = temp;
                        temp = temp->next;
                    }

                    if(prev == NULL)
                    {
                        current_ord->next = ordini_da_caricare;
                        ordini_da_caricare = current_ord;
                    }
                    else
                    {
                        prev->next = current_ord;
                        current_ord->next = temp;
                    }
                }

                vettore_ordini[i].tempo = -1;
                free(vettore_ordini[i].nome);
            }
            else
            {
                is_full = true;
            }
        }
    }

    if(ordini_da_caricare == NULL)
    {
        printf("camioncino vuoto\n");
    }
    else
    {
        while(ordini_da_caricare != NULL)
        {
            printf("%d %s %d\n", ordini_da_caricare->tempo, ordini_da_caricare->nome, ordini_da_caricare->quantity);
            alt_ordine *da_eliminare = ordini_da_caricare;
            ordini_da_caricare = ordini_da_caricare->next;
            free(da_eliminare->nome);
            free(da_eliminare);
        }
    }

    return;
}

//scorre la lista associata alla prima lettera della ricetta e posiziona la nuova ricetta in ordine alfabetico
void aggiungi_ricetta(char istruzioni[], ricetta *ricettario[], tag_lotto *magazzino[])
{
    //l'indice della ricetta nel vettore è pari a quello della prima lettera del nome della ricetta - 'a'
    int cursor = strlen("aggiungi_ricetta "), indice = trova_indice(istruzioni[strlen("aggiungi_ricetta ")]), temp_int;
    char temp_string[STRING_MAX_LENGTH] = "";

    //TREE-INSERT
    cursor = next_word(istruzioni, cursor, temp_string);
    ricetta *prev_ric = NULL, *curr_ric = ricettario[indice];

    while(curr_ric != NULL)
    {
        prev_ric = curr_ric;
        temp_int = strcmp(temp_string, curr_ric->nome);

        if(temp_int < 0)
        {
            curr_ric = curr_ric->left;
        }
        else if(temp_int > 0)
        {
            curr_ric = curr_ric->right;
        }
        else    //una ricetta con lo stesso nome è già presente
        {
            printf("ignorato\n");
            return;
        }
    }
    
    ricetta *new_recipe = malloc(sizeof(ricetta));
    new_recipe->nome = malloc(sizeof(char) * (strlen(temp_string) + 1));
    strcpy(new_recipe->nome, temp_string);
    new_recipe->padre = prev_ric;
    new_recipe->left = NULL;
    new_recipe->right = NULL;
    
    if(prev_ric == NULL)
    {
        ricettario[indice] = new_recipe;
    }
    else 
    {
        temp_int = strcmp(new_recipe->nome, prev_ric->nome);
        if(temp_int < 0)
        {
            prev_ric->left = new_recipe;
        }
        else
        {
            prev_ric->right = new_recipe;
        }
    }
    //FINE TREE-INSERT
    ingrediente *new_ing = malloc(sizeof(ingrediente));
    new_recipe->ing = new_ing;
    
    strcpy(temp_string, "");
    cursor = next_word(istruzioni, cursor, temp_string);
    new_ing->nome = malloc(sizeof(char) * (strlen(temp_string) + 1));
    strcpy(new_ing->nome, temp_string);
    strcpy(temp_string, "");
    cursor = next_word(istruzioni, cursor, temp_string);
    new_ing->quantity = strtol(temp_string, NULL, 10);
    strcpy(temp_string, "");
    new_ing->next = NULL;

    //mette il resto degli ingredienti in ordine alfabetico
    while(cursor < strlen(istruzioni))
    {
        ingrediente *curr_ing = new_recipe->ing, *prev = NULL;
        new_ing = malloc(sizeof(ingrediente));

        cursor = next_word(istruzioni, cursor, temp_string);
        new_ing->nome = malloc(sizeof(char) * (strlen(temp_string) + 1));
        strcpy(new_ing->nome, temp_string);
        strcpy(temp_string, "");
        cursor = next_word(istruzioni, cursor, temp_string);
        new_ing->quantity = strtol(temp_string, NULL, 10);
        strcpy(temp_string, "");

        while(curr_ing != NULL && strcmp(curr_ing->nome, new_ing->nome) < 0)
        {
            prev = curr_ing;
            curr_ing = curr_ing->next;
        }

        new_ing->next = curr_ing;
        if(prev == NULL)
        {
            new_recipe->ing = new_ing;
        }
        else
        {
            prev->next = new_ing;
        }
    }
    
    printf("aggiunta\n");
    return;
}

//scorre la lista associata alla prima lettera della ricetta e cerca una ricetta col nome uguale a quella fornita in input, per poi eliminarla
void rimuovi_ricetta(char istruzioni[], ricetta *ricettario[], ordine *vettore_ordini, int dim_ordini)
{
    char old_ricetta [STRING_MAX_LENGTH] = "";
    int cursor = strlen("rimuovi_ricetta "), indice = trova_indice(istruzioni[strlen("rimuovi_ricetta ")]), temp_int = -1;

    cursor = next_word(istruzioni, cursor, old_ricetta);
    ricetta *temp_ric = ricettario[indice], *y = NULL, *x = NULL;

    //TREE-DELETE
    while(temp_ric != NULL && temp_int != 0)
    {
        temp_int = strcmp(old_ricetta, temp_ric->nome);
        if(temp_int < 0)
            temp_ric = temp_ric->left;
        else if(temp_int > 0)
            temp_ric = temp_ric->right;
    }

    if(temp_ric == NULL)
    {
        printf("non presente\n");
        return;
    }

    bool is_on_standby = false;
    for(int i = 0; i < dim_ordini && !is_on_standby; i++)   //scorro la lista di tutti gli ordini
    {
        if(vettore_ordini[i].tempo != -1 && strcmp(vettore_ordini[i].nome, old_ricetta) == 0)
            is_on_standby = true;
    }

    if(is_on_standby)
    {
        printf("ordini in sospeso\n");
        return;
    }

    if(temp_ric->left == NULL || temp_ric->right == NULL)
        y = temp_ric;
    else
    {
        //y = TREE-SUCCESSOR(temp_ric)
        if(temp_ric->right != NULL)
        {
            //y = TREE-MINIMUM(temp_ric->right)
            ricetta *x = temp_ric->right;
            while(x->left != NULL)
                x = x->left;
            y = x;
        }
        else
        {
            ricetta *z = temp_ric->padre, *x = temp_ric;
            while(z != NULL && x == z->right)
            {
                x = z;
                z = z->padre;
            }

            y = z;
        }
    }

    if(y->left != NULL)
        x = y->left;
    else
        x = y->right;

    if(x != NULL)
        x->padre = y->padre;

    if(y->padre == NULL)
        ricettario[indice] = x;
    else if(y == y->padre->left)
        y->padre->left = x;
    else
        y->padre->right = x;

    if(y != temp_ric)
    {
        ingrediente *temp_ing = temp_ric->ing;
        strcpy(temp_ric->nome, y->nome);
        temp_ric->ing = y->ing;
        y->ing = temp_ing;
    }

    while(y->ing != NULL)
    {
        ingrediente *da_eliminare = y->ing;
        y->ing = y->ing->next;
        free(da_eliminare->nome);
        free(da_eliminare);
    }

    free(y->nome);
    free(y);
    printf("rimossa\n");

    return;
}

/*scorre la stringa di istruzioni: per ogni terna parola-quantità-tempo di scadenza, 
poi scorre la lista magazzino e inserisce il nuovo lotto in ordine crescente di tempo di scadenza*/
void rifornimento(char istruzioni[], tag_lotto *magazzino[], int tempo)
{
    char ingrediente[STRING_MAX_LENGTH] = "", temp_int[STRING_MAX_LENGTH] = "";
    int amount = 0, expiration_date = 0, cursor = strlen("rifornimento "), indice;

    while(cursor < strlen(istruzioni))
    {
        strcpy(ingrediente, "");
        cursor = next_word(istruzioni, cursor, ingrediente);
        strcpy(temp_int, "");
        cursor = next_word(istruzioni, cursor, temp_int);
        amount = strtol(temp_int, NULL, 10);
        strcpy(temp_int, "");
        cursor = next_word(istruzioni, cursor, temp_int);
        expiration_date = strtol(temp_int, NULL, 10);
        
        if(expiration_date > tempo)
        {
            indice = trova_indice(ingrediente[0]);
            tag_lotto *current_tag = magazzino[indice], *prev_tag = NULL;
            bool tag_trovato = false;

            //TREE-INSERT
            while(current_tag != NULL && !tag_trovato)
            {
                prev_tag = current_tag;
                int temp_int = strcmp(ingrediente, current_tag->nome);

                if(temp_int < 0)
                    current_tag = current_tag->left;
                else if(temp_int > 0)
                    current_tag = current_tag->right;
                else
                    tag_trovato = true;
            }

            if(!tag_trovato)    //un tag con quel nome non esiste => viene creato e inserito nel BST
            {
                tag_lotto *new = malloc(sizeof(tag_lotto));
                new->nome = malloc(sizeof(char) * (strlen(ingrediente) + 1));
                strcpy(new->nome, ingrediente);
                new->tot_quantity = amount;
                new->stock = malloc(sizeof(lotto));
                new->stock->quantity = amount;
                new->stock->t_scadenza = expiration_date;
                new->stock->next = NULL;
                new->left = NULL;
                new->right = NULL;
                new->padre = prev_tag;

                if(prev_tag == NULL)
                {
                    magazzino[indice] = new;
                }
                else if(strcmp(new->nome, prev_tag->nome) < 0)
                {
                    prev_tag->left = new;
                }
                else
                {
                    prev_tag->right = new;
                }
            }
            else    //un tag con quel nome esiste già => inserisco il lotto nella lista
            {
                lotto *current_lot = current_tag->stock, *prev_lot = NULL;

                while(current_lot != NULL && current_lot->t_scadenza < expiration_date)
                {
                    prev_lot = current_lot;
                    current_lot = current_lot->next;
                }

                if(current_lot == NULL || current_lot->t_scadenza != expiration_date)  //un lotto con la stessa data di scadenza non esiste => creo un nuovo lotto
                {
                    lotto *new = malloc(sizeof(lotto));
                    new->quantity = amount;
                    new->t_scadenza = expiration_date;
                    new->next = current_lot;

                    if(prev_lot == NULL)
                    {
                        current_tag->stock = new;
                    }
                    else
                    {
                        prev_lot->next = new;
                    }
                }
                else    //un lotto con quella data di scadenza esiste già => update della sua quantità
                {
                    current_lot->quantity += amount;
                }

                current_tag->tot_quantity += amount;
            }
        }
    }
    
    printf("rifornito\n");
    return;
}

void ricontrolla_ordini(tag_lotto *magazzino[], ordine *vettore_ordini, int dim_ordini)
{
    for(int i = 0; i < dim_ordini; i++)  //scorre il vettore per gli ordini in attesa  
    {
        if(vettore_ordini[i].tempo != -1 && vettore_ordini[i].in_attesa == true && check_ingredients_for_order(magazzino, &vettore_ordini[i]))
        {
            vettore_ordini[i].in_attesa = false;
        }
    }

    return;
}

int nuovo_ordine(char istruzioni[], ricetta *ricettario[], tag_lotto *magazzino[], int t, ordine **vettore_ordini, int dim_ordini)
{
    char nome_ordine[STRING_MAX_LENGTH] = "", num_temp[STRING_MAX_LENGTH] = "";
    int quantity_ricetta = 0, cursor = strlen("ordine "), indice = trova_indice(istruzioni[strlen("ordine ")]);
    ricetta *current_rec = NULL;

    cursor = next_word(istruzioni, cursor, nome_ordine);
    cursor = next_word(istruzioni, cursor, num_temp);
    quantity_ricetta = strtol(num_temp, NULL, 10);

    current_rec = ricerca_ricetta(ricettario[indice], nome_ordine);

    if(current_rec == NULL || strcmp(current_rec->nome, nome_ordine) != 0)   //la ricetta non è presente nel ricettario
    {
        printf("rifiutato\n");
    }
    else    //la ricetta è presente nel ricettario => realloc di vettore_ordini con una cella in più
    {
        printf("accettato\n");

        (*vettore_ordini) = realloc((*vettore_ordini), sizeof(ordine) * (dim_ordini + 1));
        
        (*vettore_ordini)[dim_ordini].nome = malloc(sizeof(char) * (strlen(nome_ordine) + 1));
        strcpy((*vettore_ordini)[dim_ordini].nome, nome_ordine);
        (*vettore_ordini)[dim_ordini].peso = 0;
        (*vettore_ordini)[dim_ordini].quantity = quantity_ricetta;
        (*vettore_ordini)[dim_ordini].tempo = t;
        (*vettore_ordini)[dim_ordini].this_ricetta = current_rec->ing;
        (*vettore_ordini)[dim_ordini].in_attesa = !check_ingredients_for_order(magazzino, &(*vettore_ordini)[dim_ordini]);

        dim_ordini++;
    }

    return dim_ordini;
}

//controlla gli ingredienti del magazzino: se sono abbastanza per l'ordine, lo mette direttamente nella lista degli ordini pronti
bool check_ingredients_for_order(tag_lotto *magazzino[], ordine *current_ordine)
{
    ingrediente *current_ing = current_ordine->this_ricetta;
    tag_lotto *current_tag = NULL;
    int quantity = current_ordine->quantity;
    bool enough_ingredients = true;

    while(current_ing != NULL && enough_ingredients)    //scorre gli ingredienti della ricetta
    {
        current_tag = ricerca_lotto(magazzino[trova_indice(current_ing->nome[0])], current_ing->nome);

        if(current_tag == NULL || current_tag->tot_quantity < current_ing->quantity * quantity)
        {
            enough_ingredients = false;
        }
        else
        {
            current_ing = current_ing->next;
        }
    }

    if(enough_ingredients)  //se ci sono abbastanza ingredienti, tolgo quelli usati dal magazzino
    {
        current_ing = current_ordine->this_ricetta;
        int peso = 0, current_ing_quantity = 0;
        
        while(current_ing != NULL)
        {
            current_tag = ricerca_lotto(magazzino[trova_indice(current_ing->nome[0])], current_ing->nome);
            current_ing_quantity = current_ing->quantity * quantity;
            peso += current_ing_quantity;

            //IN TEORIA non serve controllare che current_tag sia != da NULL
            lotto *current_lot = current_tag->stock;

            while(current_lot != NULL && current_ing_quantity > 0)
            {
                if(current_ing_quantity >= current_lot->quantity)
                {
                    current_ing_quantity -= current_lot->quantity;

                    lotto *da_eliminare = current_lot;
                    current_lot = current_lot->next;
                    current_tag->stock = current_lot;
                    
                    free(da_eliminare);
                }
                else
                {
                    current_lot->quantity -= current_ing_quantity;
                    current_ing_quantity = 0;
                }
            }

            current_tag->tot_quantity = calcola_quantity(current_tag->stock);
            
            current_ing = current_ing->next;
        }

        current_ordine->peso = peso;
    }
    
    return(enough_ingredients);
}

//funzione che prende come input un vettore frase e copia una parola che inizia da "frase[indice]" in un altro vettore
int next_word(char frase[], int indice, char parola[])
{
    int i = 0, lunghezza = strlen(frase);
    char empty[STRING_MAX_LENGTH] = "";
    strcpy(parola, empty);

    while(indice < lunghezza && frase[indice] != ' ' && frase[indice] != '\0' && frase[indice] != '\n' && frase[indice] != '\r')
    {
        parola[i] = frase[indice];
        i++;
        indice++;
    }

    parola[i] = '\0';
    return ++indice;
}

//dato un carattere, da come output l'indice della sua cella corrispondente
int trova_indice(char c)
{
    if('a' <= c && c <= 'z')
    {
        return(c - 'a' + 'Z' - 'A' + 1);
    }
    else if('A' <= c && c <= 'Z')
    {
        return(c - 'A');
    }
    else
    {
        return(DIM_RICETTARIO - 1);
    }
}

int calcola_quantity(lotto *temp)
{
    lotto *current_lot = temp;
    int tot_quantity = 0;

    while(current_lot != NULL)
    {
        tot_quantity += current_lot->quantity;
        current_lot = current_lot->next;
    }

    return tot_quantity;
}

//TREE-SEARCH per ricetta
ricetta *ricerca_ricetta(ricetta *x, char k[])
{
    if(x == NULL)
        return x;
    
    int temp_int = strcmp(k, x->nome);

    if(temp_int == 0)
        return x;
    else if(temp_int < 0)
        return ricerca_ricetta(x->left, k);
    else
        return ricerca_ricetta(x->right, k);
}

//TREE-SEARCH per tag_lotto
tag_lotto *ricerca_lotto(tag_lotto *x, char k[])
{
    if(x == NULL)
        return x;

    int temp_int = strcmp(k, x->nome);

    if(temp_int == 0)
        return x;
    else if(temp_int < 0)
        return ricerca_lotto(x->left, k);
    else
        return ricerca_lotto(x->right, k);
}

void libera_ricetta(ricetta *temp)
{
    if(temp != NULL)
    {
        libera_ricetta(temp->left);
        libera_ricetta(temp->right);

        while(temp->ing != NULL)
        {
            ingrediente *da_eliminare = temp->ing;
            temp->ing = temp->ing->next;
            free(da_eliminare->nome);
            free(da_eliminare);
        }

        ricetta *da_eliminare = temp;
        free(da_eliminare->nome);
        free(da_eliminare);
    }

    return;
}

void libera_lotti(tag_lotto *temp)
{
    if(temp != NULL)
    {
        libera_lotti(temp->left);
        libera_lotti(temp->right);

        while(temp->stock != NULL)
        {
            lotto *da_eliminare = temp->stock;
            temp->stock = temp->stock->next;
            free(da_eliminare);
        }

        tag_lotto *da_eliminare = temp;
        free(da_eliminare->nome);
        free(da_eliminare);
    }
}
