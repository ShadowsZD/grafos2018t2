#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphviz/cgraph.h>
#include "grafo.h"

#define TAM_ROTULO 1000

/*************************************************/
/* Atributos internos dos vertices               */
typedef struct vertice_s{
  Agrec_t h;
  char rotulo[TAM_ROTULO];
  int estado;
  unsigned int cor;
} atrb_t; // define atributos do vertice
/*************************************************/

/*************************************************/
/* Estrutura de dados auxiliar - Fila modificada */
/*                                               */
/* FIFO implementada sobre                       */
/* uma lista duplamente encadeada                */
/*                                               */
/* Implementada com uma operação extra:          */
/*  pop_maxlabel (remove o elemento de maior     */
/*  rótulo ao invés do primeiro da fila)         */
/*                                               */
/*************************************************/
typedef struct cel_struct{
    void * data;              // ponteiro generico para dados
    struct cel_struct * prev; // ponteiro para anterior da fila
    struct cel_struct * next; // ponteiro para proximo da fila
} tnode;

typedef struct queue{
    tnode * start;            // ponteiro para primeiro elemento da fila
    tnode * end;              // ponteiro para ultimo elemento da fila
    int size;
} tqueue;

//static tqueue * q_init(void);
//Funçao que inicializa a lista duplamente encadeada utilizada como fila
static tqueue * q_init(void){
    tqueue * queue = malloc(sizeof(tqueue));
    queue->start = NULL;
    queue->end = NULL;
    queue->end = queue->start;
    queue->size = 0;
    return queue;
}

//Funçao usada para inserir um elemento na fila
static void q_push(tqueue * queue, void * key){
    if (queue->size == 0){
      queue->start = malloc(sizeof(tnode));
      queue->start->data = key;
      queue->end = queue->start;
      queue->end->prev = NULL;
    }
    else{
      queue->end->next = malloc(sizeof(tnode));
      queue->end->next->prev = queue->end;
      queue->end = queue->end->next;
      queue->end->data = key;
    }
    queue->end->next = NULL;
    queue->size++;
    return;
}

// Funcao que busca o vertice de maior rotulo na fila
static void * q_pop_maxlabel(tqueue * queue){
    if (queue->size == 0 || queue->start == NULL){
      return -1;
    }
    void *key;
    tnode * max_node;
    if (queue->size == 1){
      max_node = queue->start;
      queue->start = NULL;
      queue->end = NULL;
    }
    else{
      int max_label_size = 0;
      int label_size = 0;
      atrb_t * atrib;
      char atrib_str[6]="atrb_t";
      tnode * node = queue->start;
      while (node){
        atrib = (atrb_t *) aggetrec((Agnode_t *)node->data, atrib_str, FALSE);
        label_size = (int)strlen(atrib->rotulo);
        if (label_size >= max_label_size){
          max_node = node;
          max_label_size = label_size;
        }
        node = node->next;
      }
      if (max_node->prev != NULL && max_node->next !=NULL){
        max_node->next->prev = max_node->prev;
        max_node->prev->next = max_node->next;
      }
      // only prev is NULL
      else if (max_node->prev == NULL && max_node->next!=NULL){
        max_node->next->prev = NULL;
      }
      // only next is NULL
      else if (max_node->prev != NULL && max_node->next ==NULL){
        max_node->prev->next=NULL;
      }
    }
    if (max_node == queue->start){
      if (max_node->next != NULL){
        queue->start=queue->start->next;
      }
    }
    if (max_node == queue->end){
      if (max_node->prev != NULL){
        queue->end=queue->end->prev;
      }
    }
    key = max_node->data;
    free(max_node);
    queue->size--;
    return key;
}

// Desaloca fila
static void q_free(tqueue * queue){
    tnode * node = queue->start;
    tnode * prev = NULL; 
    while (node){
      node = node->next;
      if (node){
        prev = node->prev;
        free(prev);
      }
    }
    free(queue);
}
/*************************************************/
/* Fim da estrutura de dados fila modificada     */
/*************************************************/


//------------------------------------------------------------------------------
// (apontador para) estrutura de dados para representar um grafo
// 
// o grafo pode ser direcionado ou não
// 
// o grafo tem um nome, que é uma "string"

typedef grafo * agraph_t;

//------------------------------------------------------------------------------
// (apontador para) estrutura de dados para representar um vertice

typedef struct vertice * agnode_t;

//------------------------------------------------------------------------------
// desaloca toda a memória usada em *g
// 
// devolve 1 em caso de sucesso,
//         ou 
//         0, caso contrário

int destroi_grafo(grafo g) {
    return agclose((Agraph_t *)g);
}
//------------------------------------------------------------------------------
// devolve o número de vértices de g
int n_vertices(grafo g){
  Agraph_t * graph = (Agraph_t *) g;
  Agnode_t * v;
/*
  void aginit(graph, int kind, char *rec_name,
              int rec_size, int move_to_front);
*/
  int i = 0;
  for (v = agfstnode(graph); v; v = agnxtnode(graph,v)){
    i++;
  }
  return i;
}

// devolve o vértice de nome 'nome' em g
vertice vertice_de_nome(char *nome, grafo g){
  Agraph_t * graph = (Agraph_t *) g;
  return (vertice) agnode(graph, nome, FALSE);
}

//------------------------------------------------------------------------------
// lê um grafo no formato dot de input
// 
// devolve o grafo lido,
//         ou 
//         NULL, em caso de erro 

grafo le_grafo(FILE *input) {

    Agraph_t *g;
    if ((g = agread(input, NULL))){

        return (grafo)g;
    }
    else return NULL;
}

//------------------------------------------------------------------------------
// escreve o grafo g em output usando o formato dot.
//
// devolve o grafo escrito,
//         ou 
//         NULL, em caso de erro 

grafo escreve_grafo(FILE *output, grafo g) {
    Agraph_t * graph = (Agraph_t *) g;
    if (agwrite(graph, output)){
        return (grafo) graph;
    }
    else return NULL;
}
//------------------------------------------------------------------------------
// devolve um número entre 0 e o número de vertices de g

// Nao utilizamos o ponteiro para o grafo
unsigned int cor(vertice v, grafo g){
  char atrbstr[6] = "atrb_t";
  Agnode_t * u = (Agnode_t *) v;
  atrb_t *  atrb = (atrb_t *) aggetrec(u, atrbstr, FALSE);
  return atrb->cor;
}

// retorna vertice w da aresta {u, w} em G
static Agnode_t * vizinho(Agraph_t * g, Agnode_t * u, Agedge_t * e){
    if (!strcmp(agnameof(u), agnameof(aghead(e))))
        return agnode(g, agnameof(agtail(e)), FALSE);
    else
        return agnode(g, agnameof(aghead(e)), FALSE);
}

// retorna tamanho da vizinhanca de u
static int tam_vizinhanca(Agraph_t * g, Agnode_t * u, Agedge_t * e){
  int tam = 0;
  for (e = agfstedge(g,u); e; e = agnxtedge(g,e,u)){
    tam++;
  }
  return tam;
}


//------------------------------------------------------------------------------
// preenche o vetor v (presumidamente um vetor com n_vertices(g)
// posições) com os vértices de g ordenados de acordo com uma busca em
// largura lexicográfica sobre g a partir de r e devolve v
vertice * busca_lexicografica(vertice r, grafo g, vertice *v){
  Agraph_t * graph = (Agraph_t *)g;
  Agnode_t * u; 
  Agnode_t * w; 
  Agnode_t * raiz = (agnode_t *) r; 
  Agedge_t * e; 
  atrb_t * atributos_u;
  atrb_t * atributos_w;
  char atrbstr[6]= "atrb_t";
  tqueue * V = q_init();
  int num_vertices_g = n_vertices(g);
  int i = 0;
  Agnode_t ** ordem_lex = malloc(sizeof(agnode_t) * num_vertices_g);
  char tam_V[16];

  // Inicializa vertices, monta conjunto inicial com todos os vertices
  for(u = agfstnode(graph); u; u = agnxtnode(graph, u)){
    atributos_u = (atrb_t *) agbindrec(u, atrbstr, sizeof(atrb_t), FALSE);
    atributos_u->estado = 0;
    strcpy(atributos_u->rotulo, "");
    q_push(V, (void *) u);
  }
  
  // Define a raiz
  u = agnode(graph, agnameof(raiz), FALSE);
  atributos_u = (atrb_t *) agbindrec(u, atrbstr, sizeof(atrb_t), FALSE);
  sprintf(tam_V, "%d", V->size);
  strcpy(atributos_u->rotulo,tam_V);

  // Inicia a busca
  while ((u = (Agnode_t * ) q_pop_maxlabel(V)) != -1){
    atributos_u = aggetrec(u, atrbstr, FALSE);
//    printf("%s %d\n", agnameof(u), atributos_u->estado);
    if (atributos_u->estado != 2){
      // registra quantos vertices ainda estao em V
      sprintf(tam_V, "%d", V->size);
      // Para cada w E vizinhanca(u)
      for (e = agfstedge(graph,u); e; e = agnxtedge(graph,e,u)){
          w = vizinho(graph, u, e);
          atributos_w = (atrb_t *) agbindrec(w, atrbstr, sizeof(atrb_t), FALSE);
          if (atributos_w->estado != 2){
            strcat(atributos_w->rotulo, tam_V);
            // marca como visitado (irrelevante aparentemente)
            atributos_w->estado = 1;
          }
      }
    }
    // marca u como buscado
    atributos_u->estado = 2;
    ordem_lex[i]=(Agnode_t *) u;
    i++;
  }
  // preenche o vetor 'v' com o reverso da ordem lexografica
  int j = num_vertices_g -1;
  for (i = 0; i < num_vertices_g; i++){
    u = ordem_lex[j];
    atributos_u = (atrb_t *) aggetrec(u, atrbstr, FALSE);
//    printf("Vertice: %s (%s)\n", agnameof(u), atributos_u->rotulo);
    v[i] = (vertice) u;
    j--;
  }
  q_free(V);
  return (vertice *) v;
}


// Transforma o numero da cor em uma string rgb
// Grava resultado na string saida
static void gera_rgb(unsigned int num_cor, unsigned int cor_max, char saida[7]){
  
  // se sao poucas colores, colore de forma facil de visualizar
  if (cor_max <= 6){
    const char * cores[7] = {"#000000",
                       "#FF0000", "#00FF00", 
                       "#0000FF", "#FF00FF",
                       "#111444", "#888222"};
    strcpy(saida, cores[num_cor]);
  }
  // se nao, gera cores proximas
  else{
    char string_cor[7];
    char numero[6];
    int tam_num = 0;
    int espacos = 0;
    sprintf(numero, "%u", num_cor);
    tam_num = strlen(numero);
    espacos = 6 - tam_num;
    sprintf(string_cor, "#");
    while (espacos > 0){
      strcat(string_cor, "0");
      espacos --;
    }
    strcat(string_cor, numero);
    strcpy(saida, string_cor);
  }
}

//------------------------------------------------------------------------------
// colore os vértices de g de maneira "gulosa" segundo a ordem dos
// vértices em v e devolve o número de cores utilizado
//
// ao final da execução,
//     1. cor(v,g) > 0 para todo vértice de g
//     2. cor(u,g) != cor(v,g), para toda aresta {u,v} de g
unsigned int colore(grafo g, vertice *v){
  Agnode_t * w;
  Agedge_t * e;
  Agraph_t * graph = (Agraph_t*) g;
  atrb_t * atrb;
  char atrbstr[6]= "atrb_t";
  unsigned int cor_max = 0;
  unsigned int cor_minima = 1;
  unsigned int * cores_ocupadas;
  int tam = n_vertices(g);
  int n_vizinhos;
  int i = 0;
  int j = 0;
  int encontrei_cor = 0;

  /* Pinta todos os vertices com a cor 0 */
  for (i = 0; i < tam; i++){
    atrb = aggetrec(v[i], atrbstr, FALSE);
    atrb->cor = 0;
  }

  // Coloracao gulosa */
  for (i = 0; i < tam; i++){
    n_vizinhos = tam_vizinhanca(graph, v[i], e);
    cores_ocupadas = malloc(sizeof(int) * n_vizinhos); 
    j = 0;
    for (e = agfstedge(graph,v[i]); e; e = agnxtedge(graph,e,v[i])){
        w = vizinho(graph, v[i], e);
        cores_ocupadas[j]=cor(w, graph);
        j++;
    }
    j = 0;
    cor_minima = 1;
    encontrei_cor = 0;
    while (!encontrei_cor){
      encontrei_cor = 1;
      for (j = 0; j < n_vizinhos; j++){
        if (cor_minima == cores_ocupadas[j]){
          cor_minima++;  
          encontrei_cor = 0;
        }
      }
    }
    if (cor_minima > cor_max){
      cor_max = cor_minima;
    }
    free(cores_ocupadas);
    atrb = aggetrec(v[i], atrbstr, FALSE);
    atrb->cor = cor_minima;
  }

  // Transforma atributos internos do grafo em cores externas //
  char string_cor[7]="#000000";
  char symstr[5] = "color";
  Agsym_t * color = agattr(graph, AGNODE, symstr, string_cor);
  for (i = 0; i < tam; i++){
    cor_minima = cor(v[i], (grafo)graph);
    gera_rgb(cor_minima, cor_max, string_cor);
    agxset(v[i], color, string_cor);
  }
  return cor_max;
}

//------------------------------------------------------------------------------

