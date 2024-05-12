
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define SEED 0x12345678
#define SEED2 0x87654321

//Tarefa 1: (consulta cidade)
//objetivo: dado o codigo_ibge retorne as informações desta cidade
//descrição: construa um tabela hash com tratamento de colisões utilizando um hash duplo para armazenar todas as cidades brasileiras do link 
//https://github.com/kelvins/municipios-brasileiros/blob/main/json/municipios.json, a chave desta hash deve ser o codigo_ibge. 
//Ao consultar um codigo_ibge seu código deverá retornar todas as informações desta cidade presente no banco de dados. Fiquem a vontade para reutilizar o código utilizado em sala nesta tarefa.

//Tarefa 2 (vizinhos mais próximo de uma cidade dado o seu codigo_ibge)
//objetivo:  Dado o código_ibge,  você deverá retornar o codigo_ibge das N cidades vizinhas mais próximas, onde N é um parâmetro do usuário.
//descrição: Para a busca pela vizinhança construa uma kdtree utilizando uma ABB (sugestão, consulte o chatgpt sobre esta estrutura para esta tarefa). Na KDtree você deve alternar latitude e longitude.

//Tarefa 3 (combinar tarefa 1 e 2).
//objetivo: dado o nome de uma cidade retornar todas as informações das N cidades vizinhas mais próximas. 
//descrição: crie uma outra tabela hash que dado o nome da cidade retorne o codigo_ibge. De posse do codigo_ibge utilize o código da tarefa 2 para retornar os vizinhos mais próximos e o código da tarefa 1
//para retornar as informações das cidades vizinhas. Cidades com nomes iguais vocês devem retornar a lista de nomes e deixar o usuário escolher a cidade desejada.

// Estrutura do hash
typedef struct {
    uintptr_t * table;      // Tabela hash
    int size;               // Tamanho atual da tabela
    int max;                // Tamanho máximo da tabela
    uintptr_t deleted;      // Valor para representar elementos deletados
    char * (*get_key)(void *);  // Função para obter a chave de um elemento
} thash;

// Estrutura do node
typedef struct _tnode{
    void * reg;              // Aponta para os dados do node
    struct _tnode *esq;      // Filho esquerdo do node
    struct _tnode *dir;      // Filho direito do node
} tnode;

// Estrutura da arvore
typedef struct _abb{
    tnode * raiz;                // Raiz da arvore
    int (*cmp)(void* , void *);  // Funcao para comparar dois elementos da arvore
} tarv;

// Estrutura dos dados de cada cidade
typedef struct {
    char codigo_ibge[128];
    char nome[128];
    double latitude;
    double longitude;
    int capital;
    int codigo_uf;
    int siafi_id;
    int ddd;
    char fuso_horario[128];
} tcodigo_ibge;

// Implementacao do hash (igual a utilizada em aula)
uint32_t hashf(const char* str, uint32_t h) {
    for (; *str; ++str) {
        h ^= *str;;                
        h *= 0x5bd1e995;            
        h ^= h >> 15;               
    }
    return h;
}

// Funcao para obter o codigo ibge do bucket do hash
char* get_key(void* reg) {
    return ((tcodigo_ibge*)reg)->codigo_ibge;
}

// Funcao que recebe os dados da cidade e aloca eles na estrutura tcodigo_ibge
void * aloca_codigo_ibge(char * codigo_ibge, char * nome, double latitude, double longitude, int capital, int codigo_uf, int siafi_id, int ddd, char * fuso_horario) {
    tcodigo_ibge *Codigo_ibge = malloc(sizeof(tcodigo_ibge));
    
    strcpy(Codigo_ibge->codigo_ibge, codigo_ibge);
    strcpy(Codigo_ibge->nome, nome);
    Codigo_ibge->latitude = latitude;
    Codigo_ibge->longitude = longitude;
    Codigo_ibge->capital = capital;
    Codigo_ibge->codigo_uf = codigo_uf;
    Codigo_ibge->siafi_id = siafi_id;
    Codigo_ibge->ddd = ddd;
    strcpy(Codigo_ibge->fuso_horario, fuso_horario);
    
    return Codigo_ibge;
}

// Função para construir a tabela de hash
int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *)) {
    // Aloca memória para a tabela de hash
    h->table = calloc(sizeof(void *), nbuckets + 1);
    // Verifica se a alocação foi bem sucedida
    if (h->table == NULL){
        return EXIT_FAILURE;    // Retorna código de falha
    }
    // Inicializa os atributos da tabela
    h->max = nbuckets + 1;
    h->size = 0;
    h->deleted = (uintptr_t) & (h->size);
    h->get_key = get_key;
    return EXIT_SUCCESS;    // Retorna código de sucesso
}

// Função para inserir um elemento na tabela de hash
int hash_insere(thash * h, void * bucket){
    // Calcula o hash da chave do bucket a ser inserido
    uint32_t hash = hashf(h->get_key(bucket), SEED);
    uint32_t hash2 = hashf(h->get_key(bucket), SEED2);
    // Calcula a posição na tabela de hash
    int pos = hash % (h->max);
    int pos2 = hash2 % (h->max);
    int cont = 1;
    // Se a tabela estiver cheia, não é possível inserir mais elementos
    if (h->max == (h->size + 1)) {
        free(bucket);   // Libera a memória alocada para o bucket
        return EXIT_FAILURE;   // Retorna código de falha
    }else {
        // Procura por um bucket vazio na tabela
        while(h->table[pos] != 0){
            // Se encontrar um bucket marcado como deletado, pode inserir neste bucket
            if (h->table[pos] == h->deleted)
                break;
            // Incrementa a posição para procurar o próximo bucket, no hash duplo adicionamos a pos do hash2 e não apenas 1
            pos = ((pos + cont * pos2) % h->max);
            cont += 1;
        }
        // Insere o bucket na posição encontrada
        h->table[pos] = (uintptr_t)bucket;
        // Incrementa o tamanho da tabela
        h->size +=1;
    }
    return EXIT_SUCCESS;    // Retorna código de sucesso
}


// Função para buscar um elemento na tabela de hash
void * hash_busca(thash  h, const char * key) {
    // Calcula a posição inicial na tabela de hash
    int pos = hashf(key, SEED) % (h.max);
    int pos2 = hashf(key, SEED2) % (h.max);
    void * ret = NULL;  // Inicializa o retorno como NULL
    // Percorre a tabela de hash a partir da posição calculada
    while(h.table[pos] != 0 && ret == NULL) {
        // Compara a chave do elemento na posição atual com a chave buscada
        if (strcmp(h.get_key((void*)h.table[pos]), key) == 0) {
            // Se as chaves forem iguais, atribui o elemento à variável de retorno
            ret = (void *) h.table[pos];
        }else{
            // Se as chaves forem diferentes, avança para a próxima posição na tabela
            pos = (pos + pos2) % h.max;
        }
    }
    // Retorna o elemento encontrado ou NULL se não foi encontrado
    return ret;
}

// Função para remover um elemento da tabela de hash
int hash_remove(thash * h, const char * key) {
    // Calcula a posição inicial na tabela de hash
    int pos = hashf(key, SEED) % (h->max);
    int pos2 = hashf(key, SEED2) % (h->max);
    // Percorre a tabela de hash a partir da posição calculada
    while(h->table[pos]!=0) {
        // Compara a chave do elemento na posição atual com a chave buscada
        if (strcmp(h->get_key((void*)h->table[pos]),key) == 0) {
            // Libera a memória alocada para o elemento removido
            free((void *)h->table[pos]);
            // Marca o slot como deletado
            h->table[pos] = h->deleted;
            // Decrementa o tamanho da tabela
            h->size -= 1;
            return EXIT_SUCCESS;    // Retorna código de sucesso
        }else{
            // Se as chaves forem diferentes, avança para a próxima posição na tabela
            pos = (pos + pos2) % h->max;
        }
    }
    return EXIT_FAILURE;    // Retorna código de falha se o elemento não foi encontrado
}

// Função para apagar a tabela de hash
void hash_apaga(thash *h){
    // Percorre todos os elementos da tabela de hash
    int pos;
    for (pos = 0; pos < h->max;pos++) {
        // Verifica se o elemento não é nulo
        if (h->table[pos] != 0) {
            // Verifica se o elemento não foi marcado como deletado
            if (h->table[pos]!=h->deleted) {
                // Libera a memória alocada para o elemento
                free((void*) h->table[pos]);
            }
        }
    }
    // Libera a memória alocada para a tabela de hash
    free(h->table);
}

// Funcao para inserir na abb
void abb_insere(tnode ** abb, tcodigo_ibge * codigo_ibge, int profundidade) {
    
    // Caso seja o primeiro elemento a ser inserido
    if ((*abb) == NULL) {
        (*abb) = malloc(sizeof(tnode));
        (*abb)->reg = codigo_ibge;
        (*abb)->dir = (*abb)->esq = NULL;
    // Proximos elementos a serem inseridos
    } else {
        // Profundidade da arvore utilizada para alternar entre inserir comparando a longitude ou latitude
        int nivel = profundidade % 2;
        double pai, filho;
        // Comparando a longitude
        if (nivel == 0) {
            pai = codigo_ibge->longitude;
            filho = ((tcodigo_ibge *)(*abb)->reg)->longitude;
        // Comparando Latitude    
        } else { 
            pai = codigo_ibge->latitude;
            filho = ((tcodigo_ibge *)(*abb)->reg)->latitude;
        }

        // Inserindo na esquerda caso filho < pai ou na direita caso maior
        if (pai > filho) {
            abb_insere(&((*abb)->dir), codigo_ibge, profundidade + 1);
        } else {
            abb_insere(&((*abb)->esq), codigo_ibge, profundidade + 1);
        }
    } 
}

// Calculo da distancia utilizando a formula de Haversine
// Fonte: https://pt.martech.zone/calculate-great-circle-distance/
double calcula_distancia(double latitude1, double longitude1, double latitude2, double longitude2) {
    double diferencaLongitude = longitude1 - longitude2;
    
    double pi = 3.14159;

    double distancia = 60 * 1.1515 * (180 / pi) * acos(
        sin(latitude1 * (pi/180)) * sin(latitude2 * (pi/180)) + 
        cos(latitude1 * (pi/180)) * cos(latitude2 * (pi/180)) * cos(diferencaLongitude * (pi/180))
    );

    return distancia;
}

// Encontra a distancia minima entre uma cidade e as todas as outras recebendo a distancia maxima e minima
double abb_encontra_menor_distancia(tnode * abb, thash h, const char * key, double distancia_menor, double distancia_minima) {
    tcodigo_ibge *cidade = hash_busca(h, key);

    if (abb == NULL) {
        return distancia_menor ;
    }
    
    // calcula a distancia atual
    double distancia_atual = calcula_distancia(cidade->latitude, cidade->longitude, ((tcodigo_ibge *)abb->reg)->latitude, ((tcodigo_ibge *)abb->reg)->longitude);

    // Compara a distanciaa atual para ver se ela é a menor possivel (e ainda maior que a minima para quando setarmos que queremos a proxima distancia)
    if (distancia_atual < distancia_menor && distancia_atual > distancia_minima && strcmp(cidade->codigo_ibge, ((tcodigo_ibge *)abb->reg)->codigo_ibge) != 0) {
        distancia_menor = distancia_atual;
    }

    double menor_distancia_esq = abb_encontra_menor_distancia(abb->esq, h, key, distancia_menor, distancia_minima);
    double menor_distancia_dir = abb_encontra_menor_distancia(abb->dir, h, key, distancia_menor, distancia_minima);

    if (menor_distancia_esq < menor_distancia_dir) {
        return menor_distancia_esq;
    } else {
        return menor_distancia_dir;
    }
}

// Encontra as n cidades mais proximas
void abb_encontra_cidades_proxima(tnode * raiz, tnode * abb, thash h, const char * key, double * distancia_menor, double * distancia_minima, int quantidade_de_cidades, int *cidades_encontradas, char codigos_ibge[quantidade_de_cidades][128]) {
    tcodigo_ibge *cidade = hash_busca(h, key);
    double novenove = 999999;

    if (*cidades_encontradas >= quantidade_de_cidades) {
        return;
    }

    if (abb == NULL) {
        return;
    }

    // Calcula a distancia atual
    double distancia_atual = calcula_distancia(cidade->latitude, cidade->longitude, ((tcodigo_ibge *)abb->reg)->latitude, ((tcodigo_ibge *)abb->reg)->longitude);

    // Caso a distancia atual seja a menor possivel, insere na lista de cidades mais proximas e calcula a proxima menor distancia
    if (distancia_atual <= *distancia_menor && strcmp(cidade->codigo_ibge, ((tcodigo_ibge *)abb->reg)->codigo_ibge) != 0 && *cidades_encontradas <= quantidade_de_cidades) {
        // Insere no array que contem os codigos das cidades proximas e aumenta o contador
        strcpy(codigos_ibge[*cidades_encontradas], ((tcodigo_ibge *)abb->reg)->codigo_ibge);
        (*cidades_encontradas) += 1;
        // A nova menor distancia é a atual
        *distancia_minima = distancia_atual;
        // Calcula a nova menor distancia sendo que agora o novo valor minimo é a distancia atual
        *distancia_menor = abb_encontra_menor_distancia(abb, h, key, novenove, *distancia_minima);
    }

    abb_encontra_cidades_proxima(raiz, abb->esq, h, key, distancia_menor, distancia_minima, quantidade_de_cidades, cidades_encontradas, codigos_ibge); 
    abb_encontra_cidades_proxima(raiz, abb->dir, h, key, distancia_menor, distancia_minima, quantidade_de_cidades, cidades_encontradas, codigos_ibge);
}


// Funcao para abrir o arquivo e inserir as informações no hash
void abre_escreve_arquivo(thash * h) {
    FILE *file = fopen("municipios.txt", "r");
    tcodigo_ibge codigo;
    char linha[512];

    if (file == NULL) {
        printf("Impossível abrir o arquivo.\n");
        return;
    } else {
        // Insere no hash
        while (fgets(linha, sizeof(linha), file)) {
            sscanf(linha, "%[^,],%[^,],%lf,%lf,%d,%d,%d,%d,%[^\n]", 
                   codigo.codigo_ibge, codigo.nome, &codigo.latitude, &codigo.longitude, &codigo.capital, &codigo.codigo_uf, &codigo.siafi_id, &codigo.ddd, codigo.fuso_horario);

            hash_insere(h, aloca_codigo_ibge(codigo.codigo_ibge, codigo.nome, codigo.latitude, codigo.longitude, codigo.capital, codigo.codigo_uf, codigo.siafi_id, codigo.ddd, codigo.fuso_horario));
        /*
            Teste para verificar se está inserindo corretamente
            printf("codigo_ibge: %s\n nome: %s,\n latitude: %f,\n longitude: %f,\n capital: %d,\n codigo_uf: %d,\n siafi_id: %d,\n ddd: %d,\n fuso_horario: %s\n", 
            codigo.codigo_ibge, codigo.nome, codigo.latitude, codigo.longitude, codigo.capital, codigo.codigo_uf, codigo.siafi_id, codigo.ddd, codigo.fuso_horario);
        */        
        }
    }
    fclose(file);
}

// Funcao para construir a abb, ela recebe o hash, uma arvore vazia e o primeiro codigo que sera inserido
int abb_constroi(thash * h, tnode ** abb, const char * key) {
    int latitude_ou_longitude = 1;

    // Insere o codigo da cidade que desejamos conhecer os vizinhos proximos
    tcodigo_ibge *cidade = hash_busca(*h, key);
    abb_insere(abb, aloca_codigo_ibge(cidade->codigo_ibge, cidade->nome, cidade->latitude, cidade->longitude, cidade->capital, cidade->codigo_uf, cidade->siafi_id, cidade->ddd, cidade->fuso_horario), latitude_ou_longitude);  

    // Percorre a tabela hash e insere os outros codigos
    for (int pos = 0; pos < h->max; pos++) {
       if (h->table[pos] != 0 && h->table[pos] != h->deleted) {
            // Obtém o elemento da tabela hash e insere na ABB
            tcodigo_ibge *codigo = (tcodigo_ibge *)h->table[pos];
            abb_insere(abb, aloca_codigo_ibge(codigo->codigo_ibge, codigo->nome, codigo->latitude, codigo->longitude, codigo->capital, codigo->codigo_uf, codigo->siafi_id, codigo->ddd, codigo->fuso_horario), 1);
        }
    }

}

int main (void) {
    thash h;
    int tam = 12000; // 5570 linhas no arquivo csv = tamanho do hash
    int operacao;
    
    // Inicializamos o hash
    hash_constroi(&h, tam, get_key);

    // Lemos o arquivo e inserimos os dados do arquivo no hash
    abre_escreve_arquivo(&h);

    printf("Qual operacao voce deseja realizar? (Digite o numero da operacao) 1.Encontrar as informacoes de uma cidade, 2.Encontrar as cidades mais proximas, 3.Encontrar as informacoes de das cidades mais proximas 4.Sair\n");
    scanf("%d", &operacao);
    printf("\n");

    while(operacao < 4) {
        //Tarefa 1: (consulta cidade)
        if (operacao == 1) {

            char codigo_ibge[128];
            printf("Qual o codigo do IBGE da cidade qual voce deseja ver as informacoes?\n");
            scanf("%s", codigo_ibge);
            printf("\n");
            
            // Procuramos o codigo no hash
            tcodigo_ibge *cidade = hash_busca(h, codigo_ibge);

            if (cidade == NULL) {
                printf("Cidade nao encontrada, verifique se o codigo foi digitado corretamente.");
                printf("\n");
            } else {
                printf("codigo_ibge: %s\nnome: %s,\nlatitude: %f,\nlongitude: %f,\ncapital: %d,\ncodigo_uf: %d,\nsiafi_id: %d,\nddd: %d,\nfuso_horario: %s\n", 
                    cidade->codigo_ibge, cidade->nome, cidade->latitude, cidade->longitude, cidade->capital, cidade->codigo_uf, cidade->siafi_id, cidade->ddd, cidade->fuso_horario);
                printf("\n");
            }
            
            printf("Qual operacao voce deseja realizar? (Digite o numero da operacao) 1.Encontrar as informacoes de uma cidade, 2.Encontrar as cidades mais proximas, 3.Encontrar as informacoes de das cidades mais proximas 4.Sair\n");
            scanf("%d", &operacao);
            printf("\n");

        } else if (operacao == 2 || operacao == 3) {

            int quantidade_de_cidades;
            char codigo_ibge2[128];
            
            printf("Qual o codigo do IBGE da cidade qual voce deseja ver as informacoes?\n");
            scanf("%s", codigo_ibge2);
            printf("\n");

            printf("Quantas cidades mais proximas voce deseja encontrar?\n");
            scanf("%d", &quantidade_de_cidades);
            printf("\n");

            // Procuramos o codigo no hash
            tcodigo_ibge *cidade2 = hash_busca(h, codigo_ibge2);

            if (cidade2 == NULL) {
                printf("Cidade nao encontrada, verifique se o codigo foi digitado corretamente.");
                printf("\n");
            } else {
                int cidades_encontradas = 0;
                double distancia_minima = 0;
                char codigos_ibge[quantidade_de_cidades][128];

                // Constroi a abb para o codigo enviado
                tnode * abb = NULL;
                tnode * raiz = abb;
                int profundidade = 0;
                abb_constroi(&h, &abb, cidade2->codigo_ibge);

                double distancia = abb_encontra_menor_distancia(abb, h, codigo_ibge2, 999999, 0);
                
                // Encontra as cidades mais proximas
                printf("As cidades mais proximas sao:\n");
                abb_encontra_cidades_proxima(raiz, abb, h, codigo_ibge2, &distancia, &distancia_minima, quantidade_de_cidades, &cidades_encontradas, codigos_ibge);
                
                // Caso operacao == 2 imprime apenas os codigos
                if (operacao == 2) {
                    for (int i = 0; i < cidades_encontradas; i++) {
                        printf("%s\n", codigos_ibge[i]);
                    }
                } 

                // Caso operacao == 3 imprime a informacao das cidades
                if (operacao == 3) {
                    for (int i = 0; i < cidades_encontradas; i++) {
                        printf("%s\n", codigos_ibge[i]);
                        tcodigo_ibge *cidades = hash_busca(h, codigos_ibge[i]);
                        printf("codigo_ibge: %s\nnome: %s,\nlatitude: %f,\nlongitude: %f,\ncapital: %d,\ncodigo_uf: %d,\nsiafi_id: %d,\nddd: %d,\nfuso_horario: %s\n", 
                            cidades->codigo_ibge, cidades->nome, cidades->latitude, cidades->longitude, cidades->capital, cidades->codigo_uf, cidades->siafi_id, cidades->ddd, cidades->fuso_horario);
                        printf("\n");
                    }
                }

                printf("Qual operacao voce deseja realizar? (Digite o numero da operacao) 1.Encontrar as informacoes de uma cidade, 2.Encontrar as cidades mais proximas, 3.Encontrar as informacoes de das cidades mais proximas 4.Sair\n");
                scanf("%d", &operacao);
                printf("\n");
            }
        }
    }
    printf("Obrigada por utilizar nosso servico.");
    printf("\n");

    hash_apaga(&h);
    return 0;
}
