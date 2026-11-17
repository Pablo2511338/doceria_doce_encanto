/*
 * TRABALHO: DOCERIA DOCE ENCANTO - CADASTRO DE PRODUTOS
 */

#include <stdio.h>   // printf, fgets, fopen, fprintf, fclose e outras funcoes de arquivo.
#include <stdlib.h>  // exit: encerra se a entrada do teclado terminar.
#include <string.h>  // strcmp, strstr, strlen, strcpy e strchr.
#include <ctype.h>   // tolower: converte letras comuns para minusculas.
#include <limits.h>  // INT_MAX: maior valor permitido para um int.


#define MAX_PRODUTOS 1000            // Capacidade maxima do vetor de produtos.
#define ARQUIVO_DADOS "produtos.txt" // Arquivo principal com dados separados por |.
#define ARQUIVO_TEMP "produtos.tmp"  // Arquivo temporario para alterar/excluir.
#define ARQUIVO_BACKUP "produtos.bak"// Copia de seguranca durante a substituicao.
#define ARQUIVO_ID "ultimo_id.txt"   // Guarda os IDs ja reservados, mesmo apos exclusoes.
#define ARQUIVO_CSV "produtos.csv"   // Arquivo exportado com virgulas.

// struct agrupa as informacoes de um produto em uma unica variavel.
typedef struct {
    int id;                 // Codigo unico gerado automaticamente.
    char nome[50];          // Nome com ate 49 bytes, mais o caractere \0.
    char categoria[40];     // Categoria com ate 39 bytes, mais \0.
    double preco;           // Preco do produto.
    int quantidade;         // Unidades disponiveis em estoque.
    char fornecedor[50];    // Fornecedor com ate 49 bytes, mais \0.
} Produto;

// Remove a quebra de linha final e espacos extras no comeco e no fim.
void limparEspacos(char *texto) {
    size_t n = strlen(texto);                       // Descobre o tamanho da string.
    while (n > 0 && (texto[n - 1] == '\n' || texto[n - 1] == '\r' || texto[n - 1] == ' ' || texto[n - 1] == '\t')) {
        texto[--n] = '\0';                         // Remove o ultimo caractere indesejado.
    }
    size_t inicio = 0;                             // Guarda onde comeca o texto util.
    while (texto[inicio] == ' ' || texto[inicio] == '\t') inicio++;
    if (inicio > 0) memmove(texto, texto + inicio, strlen(texto + inicio) + 1);
}

// Le uma linha completa do teclado, inclusive nomes com espacos.
void lerTexto(const char *mensagem, char *destino, size_t capacidade) {
    char entrada[256];                             // Buffer maior que os campos do cadastro.
    for (;;) {                                     // Repete ate o usuario digitar um valor valido.
        printf("%s", mensagem);                    // Mostra qual campo deve ser preenchido.
        if (fgets(entrada, sizeof entrada, stdin) == NULL) {
            puts("Entrada encerrada.");
            exit(EXIT_FAILURE);                    // Evita usar dados incompletos.
        }
        if (strchr(entrada, '\n') == NULL && !feof(stdin)) {
            int c;                                  // Caso a linha seja grande demais, descarta o resto.
            while ((c = getchar()) != '\n' && c != EOF) { }
            puts("Texto muito longo. Tente novamente.");
            continue;
        }
        limparEspacos(entrada);                     // Remove espacos e a quebra de linha.
        if (entrada[0] == '\0') {                   // Nao permite campo vazio.
            puts("O campo nao pode ficar vazio.");
            continue;
        }
        if (strchr(entrada, '|') != NULL) {          // | e o separador interno do arquivo.
            puts("Nao use o caractere | nos campos.");
            continue;
        }
        if (strlen(entrada) >= capacidade) {        // Garante espaco para o terminador \0.
            printf("O campo aceita no maximo %zu bytes.\n", capacidade - 1);
            continue;
        }
        strcpy(destino, entrada);                   // Copia somente depois da validacao.
        return;                                     // Termina a funcao com um texto valido.
    }
}

// Le um inteiro usando fgets e sscanf, como nas aulas de leitura de dados.
// O segundo campo detecta se o usuario digitou letras depois do numero.
int lerInteiro(const char *mensagem, int minimo, int maximo) {
    char entrada[100];                           // Texto digitado pelo usuario.
    char sobra;                                  // Guarda um caractere extra, se existir.
    int numero;                                  // Numero convertido.
    for (;;) {                                  // Repete ate obter uma entrada correta.
        printf("%s", mensagem);
        if (fgets(entrada, sizeof entrada, stdin) == NULL) exit(1);
        if (sscanf(entrada, " %d %c", &numero, &sobra) == 1 &&
            numero >= minimo && numero <= maximo) return numero;
        puts("Numero inteiro invalido. Tente novamente.");
    }
}

// Le um preco usando fgets e sscanf; permite 49,90 ou 49.90.
double lerPreco(const char *mensagem) {
    char entrada[100];                           // Texto digitado.
    char sobra;                                  // Detecta caracteres indevidos.
    double numero;                               // Preco convertido.
    for (;;) {
        printf("%s", mensagem);
        if (fgets(entrada, sizeof entrada, stdin) == NULL) exit(1);
        for (int i = 0; entrada[i] != '\0'; i++) {
            if (entrada[i] == ',') entrada[i] = '.'; // Aceita virgula decimal.
        }
        if (sscanf(entrada, " %lf %c", &numero, &sobra) == 1 &&
            numero >= 0.0 && numero <= 99999999.0) return numero;
        puts("Preco invalido. Digite, por exemplo, 49.90 ou 49,90.");
    }
}

// Converte para minusculas sem estragar os acentos mais comuns em UTF-8.
// Ex.: CAMISETA e camiseta; INFORMATICA e informatica; ÓPTICO e óptico.
// Acentos diferentes continuam sendo caracteres diferentes (e != é).
void normalizarTexto(const char *origem, char *destino, size_t capacidade) {
    const unsigned char *p = (const unsigned char *)origem; // Ponteiro para os bytes da string.
    size_t posicao = 0;                          // Proxima posicao livre na string de destino.
    while (*p != '\0' && posicao + 1 < capacidade) {
        if (p[0] == 0xC3 && p[1] != '\0' && posicao + 2 < capacidade) {
            unsigned char segundo = p[1];        // Segundo byte de um caractere acentuado.
            if ((segundo >= 0x80 && segundo <= 0x96) ||
                (segundo >= 0x98 && segundo <= 0x9E)) {
                segundo = (unsigned char)(segundo + 0x20); // Maiuscula acentuada -> minuscula.
            }
            destino[posicao++] = (char)p[0];     // Mantem o primeiro byte UTF-8.
            destino[posicao++] = (char)segundo;   // Guarda o segundo byte normalizado.
            p += 2;                              // Avanca dois bytes.
        } else {
            destino[posicao++] = (char)tolower(*p); // Minuscula normal para letras ASCII.
            p++;                                 // Avanca um byte.
        }
    }
    destino[posicao] = '\0';                     // Finaliza a string corretamente.
}

// Retorna 0 quando as strings sao iguais sem diferenciar maiusculas/minusculas.
int compararTexto(const char *a, const char *b) {
    char textoA[256], textoB[256];               // Buffers suficientes para os campos do cadastro.
    normalizarTexto(a, textoA, sizeof textoA);    // Normaliza o primeiro texto.
    normalizarTexto(b, textoB, sizeof textoB);    // Normaliza o segundo texto.
    return strcmp(textoA, textoB);                // Compara as versoes normalizadas.
}

// Verifica se uma palavra/frase aparece dentro de outro texto, sem diferenciar caixa.
int contemTexto(const char *texto, const char *busca) {
    char textoNormal[256], buscaNormal[256];
    normalizarTexto(texto, textoNormal, sizeof textoNormal);
    normalizarTexto(busca, buscaNormal, sizeof buscaNormal);
    return strstr(textoNormal, buscaNormal) != NULL; // strstr procura uma substring.
}

// Grava uma estrutura Produto como UMA linha de texto no arquivo.
int gravarLinha(FILE *fp, const Produto *p) {
    return fprintf(fp, "%d|%s|%s|%.2f|%d|%s\n",
                   p->id, p->nome, p->categoria, p->preco, p->quantidade, p->fornecedor) >= 0;
}

// Le o arquivo de texto e coloca cada registro em Produto lista[].
// Nao cria arquivo vazio se o cadastro original desaparecer.
int carregarProdutos(Produto lista[], int *quantidade) {
    FILE *fp = fopen(ARQUIVO_DADOS, "r");      // Abre o arquivo existente.
    char linha[512];                            // Guarda uma linha do arquivo.
    *quantidade = 0;                            // Inicialmente o vetor esta vazio.
    if (fp == NULL) {                           // Arquivo principal ausente ou inacessivel.
        puts("ERRO: nao foi possivel abrir produtos.txt. Cadastro nao alterado.");
        return 0;
    }
    while (fgets(linha, sizeof linha, fp) != NULL) {
        Produto atual;                           // Recebe os dados da linha.
        int separadores = 0;                     // O registro deve conter 5 barras |.
        int lidos = 0;                          // Bytes consumidos pelo sscanf.
        int valido = 1;                         // Indica se a linha esta correta.
        if (*quantidade >= MAX_PRODUTOS) {
            puts("ERRO: limite de produtos ultrapassado.");
            fclose(fp);
            return 0;
        }
        // Impede linha maior que o vetor: nao aceita apenas parte de um registro.
        if (strlen(linha) == sizeof linha - 1 && strchr(linha, '\n') == NULL) valido = 0;
        for (int i = 0; linha[i] != '\0'; i++) {
            if (linha[i] == '|') separadores++;  // Conta os separadores presentes.
        }
        if (separadores != 5) valido = 0;        // Exige os seis campos sem extras.
        // %n indica ate onde o sscanf leu, para rejeitar sobras no final.
        if (valido && sscanf(linha, "%d|%49[^|]|%39[^|]|%lf|%d|%49[^\r\n]%n",
                  &atual.id, atual.nome, atual.categoria, &atual.preco,
                  &atual.quantidade, atual.fornecedor, &lidos) != 6) valido = 0;
        // Depois do sexto campo so podem existir quebras de linha (Windows/Linux).
        if (valido) {
            while (linha[lidos] == '\r' || linha[lidos] == '\n') lidos++;
            if (linha[lidos] != '\0') valido = 0; // Rejeita texto extra apos o registro.
        }
        if (valido && (atual.id < 1 || atual.preco < 0.0 ||
            atual.preco > 99999999.0 || atual.quantidade < 0)) valido = 0;
        if (!valido) {
            puts("ERRO: registro invalido em produtos.txt. Nada foi modificado.");
            fclose(fp);
            return 0;
        }
        for (int j = 0; j < *quantidade; j++) {
            if (lista[j].id == atual.id || compararTexto(lista[j].nome, atual.nome) == 0) {
                puts("ERRO: ID ou nome duplicado em produtos.txt.");
                fclose(fp);
                return 0;
            }
        }
        lista[*quantidade] = atual;              // Armazena o produto no vetor.
        (*quantidade)++;                         // Atualiza a contagem via ponteiro.
    }
    if (ferror(fp)) {                            // Verifica erro durante a leitura.
        puts("ERRO: falha ao ler produtos.txt.");
        fclose(fp);
        return 0;
    }
    fclose(fp);                                 // Encerra a leitura.
    return 1;                                   // Arquivo lido corretamente.
}

// Larguras fixas para a tabela ficar organizada e caber no terminal.
// A tabela usa larguras fixas para manter todas as colunas alinhadas.
#define COL_ID 5
#define COL_NOME 22
#define COL_CATEGORIA 14
#define COL_PRECO 12
#define COL_QUANTIDADE 10
#define COL_FORNECEDOR 18

// Conta caracteres visiveis, sem contar os bytes extras de acentos UTF-8.
// Exemplo: "Limão" tem 5 caracteres visiveis, apesar dos 6 bytes.
int tamanhoTextoTela(const char *texto) {
    int tamanho = 0;                            // Conta os caracteres na tela.
    const unsigned char *p = (const unsigned char *)texto; // Ponteiro para o texto.
    while (*p != '\0') {                      // Percorre o texto ate seu final.
        if ((*p & 0xC0) != 0x80) tamanho++;     // Ignora bytes de continuacao UTF-8.
        p++;                                    // Avanca para o proximo byte.
    }
    return tamanho;                             // Devolve a largura visual aproximada.
}

// Imprime um texto alinhado a esquerda, preenchendo ate a largura escolhida.
// Se o nome for grande demais, exibe "..." no fim, sem alterar o cadastro.
void imprimirColunaTexto(const char *texto, int largura) {
    int tamanho = tamanhoTextoTela(texto);       // Quantos caracteres aparecem na tela.
    int limite = tamanho > largura ? largura - 3 : largura; // Reserva lugar para "...".
    int impressos = 0;                           // Quantos caracteres foram exibidos.
    const unsigned char *p = (const unsigned char *)texto;
    while (*p != '\0' && impressos < limite) {
        do {                                      // Imprime um caractere UTF-8 inteiro.
            putchar(*p);                          // Escreve o byte do caractere.
            p++;                                  // Avanca no texto.
        } while (*p != '\0' && (*p & 0xC0) == 0x80);
        impressos++;                              // Conta um caractere visivel.
    }
    if (tamanho > largura) {                      // Campo grande demais para sua coluna.
        printf("...");                         // Mostra que o texto foi abreviado.
        impressos += 3;                          // Conta os tres pontos adicionados.
    }
    while (impressos < largura) {                 // Preenche o restante da coluna.
        putchar(' ');                             // Um espaco mantem a coluna alinhada.
        impressos++;
    }
}

// Alinha os valores numericos pela direita (ID, preco e quantidade).
void imprimirColunaNumero(const char *texto, int largura) {
    int tamanho = (int)strlen(texto);            // Numeros usam somente caracteres ASCII.
    for (int i = tamanho; i < largura; i++) putchar(' '); // Espacos antes do numero.
    printf("%s", texto);                        // Imprime o valor a direita.
}

// Imprime divisorias que respeitam exatamente as larguras das colunas.
void linhaTabela(void) {
    const int larguras[] = {                     // Mesmas larguras usadas no cabecalho e dados.
        COL_ID, COL_NOME, COL_CATEGORIA, COL_PRECO, COL_QUANTIDADE, COL_FORNECEDOR
    };
    putchar('+');                                // Borda esquerda da tabela.
    for (int i = 0; i < 6; i++) {                // Percorre as seis colunas.
        for (int j = 0; j < larguras[i] + 2; j++) putchar('-'); // Inclui espacos laterais.
        putchar('+');                            // Separador entre colunas.
    }
    putchar('\n');                             // Finaliza a linha horizontal.
}

// Mostra o cabecalho utilizando as mesmas larguras das linhas de produtos.
void cabecalhoTabela(void) {
    putchar('\n');                           // Comeca a tabela numa nova linha.
    linhaTabela();
    printf("| "); imprimirColunaTexto("ID", COL_ID);
    printf(" | "); imprimirColunaTexto("Produto", COL_NOME);
    printf(" | "); imprimirColunaTexto("Categoria", COL_CATEGORIA);
    printf(" | "); imprimirColunaTexto("Preco (R$)", COL_PRECO);
    printf(" | "); imprimirColunaTexto("Quantidade", COL_QUANTIDADE);
    printf(" | "); imprimirColunaTexto("Fornecedor", COL_FORNECEDOR);
    printf(" |\n");
    linhaTabela();
}

// Mostra os seis campos de UM produto, sem modificar os dados do arquivo.
void linhaProduto(const Produto *p) {
    char valor[40];                              // Buffer auxiliar para numeros formatados.
    snprintf(valor, sizeof valor, "%d", p->id); // Converte o ID para texto.
    printf("| "); imprimirColunaNumero(valor, COL_ID);
    printf(" | "); imprimirColunaTexto(p->nome, COL_NOME); // Nome alinhado.
    printf(" | "); imprimirColunaTexto(p->categoria, COL_CATEGORIA); // Categoria.
    snprintf(valor, sizeof valor, "%.2f", p->preco); // Preco com duas casas decimais.
    printf(" | "); imprimirColunaNumero(valor, COL_PRECO); // Preco a direita.
    snprintf(valor, sizeof valor, "%d", p->quantidade); // Estoque numerico.
    printf(" | "); imprimirColunaNumero(valor, COL_QUANTIDADE); // Quantidade a direita.
    printf(" | "); imprimirColunaTexto(p->fornecedor, COL_FORNECEDOR); // Fornecedor.
    printf(" |\n");
}

// Exibe o vetor completo em formato de tabela, sem alterar produtos.txt.
void exibirTabela(const Produto lista[], int quantidade) {
    if (quantidade == 0) { puts("Nenhum produto cadastrado."); return; }
    cabecalhoTabela();                           // Imprime o cabecalho alinhado.
    for (int i = 0; i < quantidade; i++) linhaProduto(&lista[i]); // Usa ponteiros.
    linhaTabela();                               // Fecha a borda inferior.
    printf("Total: %d produto(s).\n", quantidade);
}

// Devolve 1 se o ID ja aparece na lista; 0, caso contrario.
int existeID(const Produto lista[], int quantidade, int id) {
    for (int i = 0; i < quantidade; i++) if (lista[i].id == id) return 1;
    return 0;
}

// Procura nome duplicado, ignorando letras maiusculas/minusculas.
// O parametro ignorarID permite que um produto mantenha o proprio nome na alteracao.
int existeNome(const Produto lista[], int quantidade, const char *nome, int ignorarID) {
    for (int i = 0; i < quantidade; i++) {
        if (lista[i].id != ignorarID && compararTexto(lista[i].nome, nome) == 0) return 1;
    }
    return 0;
}

// Busca a posicao do produto pelo ID; devolve -1 quando nao encontra.
int indicePorID(const Produto lista[], int quantidade, int id) {
    for (int i = 0; i < quantidade; i++) if (lista[i].id == id) return i;
    return -1;
}

// Confere se um arquivo existe, sem modifica-lo.
int arquivoExiste(const char *nome) {
    FILE *fp = fopen(nome, "r");                // Apenas leitura: nao apaga o arquivo.
    if (fp == NULL) return 0;                   // Nao encontrou arquivo acessivel.
    fclose(fp);                                 // Fecha apos conferir a existencia.
    return 1;                                   // Encontrou o arquivo.
}

// Recupera o original quando o programa parou durante a troca por backup.
int recuperarBackup(const char *original, const char *backup) {
    if (!arquivoExiste(original) && arquivoExiste(backup)) { // So restaura se falta o original.
        if (rename(backup, original) != 0) {    // Tenta trazer o arquivo antigo de volta.
            perror("Falha ao recuperar arquivo a partir do backup");
            return 0;                           // Interrompe antes de cadastrar novas informacoes.
        }
        printf("Backup recuperado automaticamente: %s\n", original);
    }
    return 1;
}

// Le o ultimo ID guardado. O arquivo pode ter varios numeros, um por linha.
// Usamos o modo "a" para acrescentar IDs sem apagar os ja reservados.
int lerUltimoID(void) {
    FILE *fp = fopen(ARQUIVO_ID, "r");        // Le o controle de IDs.
    char linha[100];                           // Uma linha de numero por vez.
    int ultimo = 0;                            // Ultimo ID valido encontrado.
    int id;                                    // ID lido na linha atual.
    char sobra;                                // Detecta texto extra (ex.: 12xyz).
    if (fp == NULL) return -1;                 // Arquivo nao pode estar ausente.
    while (fgets(linha, sizeof linha, fp) != NULL) {
        if (sscanf(linha, " %d %c", &id, &sobra) != 1 || id <= ultimo) {
            fclose(fp);
            return -1;                         // Rejeita texto e IDs fora de ordem.
        }
        ultimo = id;                           // Atualiza o ultimo ID reservado.
    }
    if (ferror(fp)) { fclose(fp); return -1; } // Falha de leitura.
    fclose(fp);
    return ultimo == 0 ? -1 : ultimo;          // O controle deve conter algum ID.
}

// Reserva o proximo ID sem apagar os anteriores, usando o modo append da aula 08.
int salvarUltimoID(int id) {
    FILE *fp = fopen(ARQUIVO_ID, "a");        // Acrescenta ao final do arquivo.
    if (fp == NULL) return 0;                 // Nao conseguiu abrir.
    int ok = fprintf(fp, "%d\n", id) >= 0; // Reserva o ID mesmo se cadastro falhar.
    if (fclose(fp) != 0) ok = 0;               // Confere se a gravacao terminou.
    return ok;                                 // Informa se conseguiu salvar.
}

// Cria os arquivos necessarios, sem apagar produtos que ja existem.
void criarArquivo(void) {
    // Se o cadastro sumiu mas o controle de IDs existe, nao criar um vazio.
    if (!arquivoExiste(ARQUIVO_DADOS) && arquivoExiste(ARQUIVO_ID)) {
        puts("ERRO: produtos.txt desapareceu. Verifique seus arquivos.");
        return;
    }
    FILE *fp = fopen(ARQUIVO_DADOS, "a");      // Modo append cria ou preserva o arquivo.
    if (fp == NULL) { perror("Erro ao criar o cadastro"); return; }
    if (fclose(fp) != 0) { perror("Erro ao fechar o cadastro"); return; }
    puts("Cadastro pronto: produtos.txt (dados anteriores preservados).");
}


// Produtos FICTICIOS de uma doceria, seguindo os campos do exemplo do professor.
// Os nomes, categorias, precos, quantidades e fornecedores sao dados didaticos.
// Estes registros sao criados APENAS na primeira execucao, se nao houver cadastro.
int criarProdutosIniciais(void) {
    // Nunca substitui uma lista existente, mesmo que ela esteja vazia.
    if (arquivoExiste(ARQUIVO_DADOS) || arquivoExiste(ARQUIVO_ID)) return 0;

    // Evita apagar um temporario deixado por uma execucao interrompida.
    if (arquivoExiste(ARQUIVO_TEMP)) {
        puts("Existe produtos.tmp. Confira esse arquivo antes de iniciar a loja.");
        return 0;
    }

    // Vetor de estruturas: cada linha corresponde a um produto completo.
    const Produto iniciais[] = {  // Vetor de struct: 12 doces ja cadastrados.
        { 1, "Brigadeiro Tradicional", "Brigadeiros", 4.50,  60, "Doce Cacau"}, // Doce 1.
        { 2, "Beijinho de Coco", "Docinhos", 4.00,  45, "Coco & Sabor"}, // Doce 2.
        { 3, "Bolo de Chocolate", "Bolos", 65.00,   8, "Confeitaria Aurora"}, // Doce 3.
        { 4, "Bolo de Cenoura", "Bolos", 48.90,  10, "Confeitaria Aurora"}, // Doce 4.
        { 5, "Cupcake de Baunilha", "Cupcakes", 12.50,  24, "Casa dos Cupcakes"}, // Doce 5.
        { 6, "Cupcake Red Velvet", "Cupcakes", 14.90,  20, "Casa dos Cupcakes"}, // Doce 6.
        { 7, "Brownie com Nozes", "Brownies", 16.00,  18, "Forno da Vila"}, // Doce 7.
        { 8, "Pudim de Leite", "Sobremesas", 22.00,  12, "Sabor da Vovó"}, // Doce 8.
        { 9, "Torta de Limão", "Tortas", 78.00,   6, "Confeitaria Aurora"}, // Doce 9.
        {10, "Trufa de Maracujá", "Trufas", 6.50,  35, "Doce Cacau"}, // Doce 10.
        {11, "Cookie de Chocolate", "Biscoitos", 9.90,  30, "Forno da Vila"}, // Doce 11.
        {12, "Palha Italiana", "Docinhos", 8.00,  28, "Coco & Sabor"}, // Doce 12.
    };
    int quantidade = (int)(sizeof iniciais / sizeof iniciais[0]); // Quantos produtos ha no vetor.

    // Escreve primeiro em arquivo temporario, sem expor dados incompletos.
    FILE *fp = fopen(ARQUIVO_TEMP, "w");
    if (fp == NULL) { perror("Erro ao preparar produtos iniciais"); return 0; }
    int ok = 1;
    for (int i = 0; i < quantidade; i++) {
        if (!gravarLinha(fp, &iniciais[i])) { ok = 0; break; }
    }
    if (fclose(fp) != 0) ok = 0;
    if (!ok) {
        remove(ARQUIVO_TEMP);
        puts("Erro ao escrever os produtos iniciais.");
        return 0;
    }

    // Registra o ultimo ID dos doces iniciais; o proximo sera 13.
    if (!salvarUltimoID(iniciais[quantidade - 1].id)) {
        remove(ARQUIVO_TEMP);
        puts("Erro ao salvar o contador inicial de IDs.");
        return 0;
    }

    // A lista preparada passa a ser o arquivo principal.
    if (rename(ARQUIVO_TEMP, ARQUIVO_DADOS) != 0) {
        perror("Erro ao criar produtos.txt");
        remove(ARQUIVO_TEMP);
        // Caso tenha ocorrido uma falha, conserva o contador para nao reutilizar IDs.
        return 0;
    }

    printf("Doceria Doce Encanto iniciada com %d produtos de exemplo!\n", quantidade);
    puts("Os dados ficam em produtos.txt e nao serao reiniciados ao reabrir.");
    return 1;
}

// Adiciona um novo registro ao final do arquivo.
void inserirProduto(void) {
    Produto lista[MAX_PRODUTOS];                 // Vetor de structs com os dados atuais.
    Produto novo;                                // Estrutura a ser cadastrada.
    int quantidade;                              // Quantidade atual de produtos.
    int maiorID;                                 // Maior ID encontrado no arquivo.
    if (!carregarProdutos(lista, &quantidade)) return;
    if (quantidade == MAX_PRODUTOS) { puts("Limite de produtos atingido."); return; }
    lerTexto("Nome do produto: ", novo.nome, sizeof novo.nome);
    if (existeNome(lista, quantidade, novo.nome, -1)) {
        puts("ERRO: ja existe um produto com esse nome. Cadastro cancelado.");
        return;
    }
    lerTexto("Categoria: ", novo.categoria, sizeof novo.categoria);
    novo.preco = lerPreco("Preco (ex.: 49,90): ");
    novo.quantidade = lerInteiro("Quantidade: ", 0, INT_MAX);
    lerTexto("Fornecedor: ", novo.fornecedor, sizeof novo.fornecedor);
    maiorID = lerUltimoID();                     // Consulta o controle persistente de IDs.
    if (maiorID < 0) { puts("Erro: ultimo_id.txt invalido. Corrija o arquivo."); return; }
    for (int i = 0; i < quantidade; i++) {
        if (lista[i].id > maiorID) maiorID = lista[i].id; // Sincroniza com o cadastro real.
    }
    if (maiorID == INT_MAX) { puts("Nao ha IDs disponiveis."); return; }
    novo.id = maiorID + 1;                       // Cria o proximo ID incremental.
    if (existeID(lista, quantidade, novo.id)) { // Verificacao adicional, apesar do ID automatico.
        puts("ERRO: ID duplicado detectado. Operacao cancelada.");
        return;
    }
    if (!salvarUltimoID(novo.id)) {             // Reserva o ID antes de gravar o produto.
        puts("Nao foi possivel reservar o novo ID.");
        return;
    }
    FILE *fp = fopen(ARQUIVO_DADOS, "a");      // Modo append conserva registros anteriores.
    if (fp == NULL) { perror("Erro ao abrir produtos.txt"); return; }
    int ok = gravarLinha(fp, &novo);             // Grava o produto no arquivo.
    if (fclose(fp) != 0) ok = 0;                 // Testa se houve falha ao salvar.
    if (ok) printf("Produto cadastrado com sucesso! ID automatico: %d\n", novo.id);
    else puts("Erro ao gravar o produto no arquivo.");
}

// Mostra todos os produtos na ordem original do arquivo.
void listarProdutos(void) {
    Produto lista[MAX_PRODUTOS];
    int quantidade;
    if (!carregarProdutos(lista, &quantidade)) return;
    exibirTabela(lista, quantidade);
}

// Substitui o arquivo original com ajuda de temp e backup.
// A mudanca so acontece depois que a nova versao terminou de ser gravada.
int salvarComTemporario(const Produto lista[], int quantidade) {
    FILE *teste = fopen(ARQUIVO_BACKUP, "r");   // Evita sobrescrever backup de falha anterior.
    if (teste != NULL) {
        fclose(teste);
        puts("Existe produtos.bak. Verifique o backup antes de alterar dados.");
        return 0;
    }
    FILE *fp = fopen(ARQUIVO_TEMP, "w");       // Cria a nova versao separadamente.
    if (fp == NULL) { perror("Erro ao criar temporario"); return 0; }
    int ok = 1;
    for (int i = 0; i < quantidade; i++) {       // Reescreve produtos mantidos/alterados.
        if (!gravarLinha(fp, &lista[i])) { ok = 0; break; }
    }
    if (fclose(fp) != 0) ok = 0;                 // Confere se a gravacao terminou bem.
    if (!ok) { puts("Erro ao gravar temporario."); remove(ARQUIVO_TEMP); return 0; }
    if (rename(ARQUIVO_DADOS, ARQUIVO_BACKUP) != 0) { // Guarda o original em backup.
        perror("Erro ao criar backup");
        remove(ARQUIVO_TEMP);
        return 0;
    }
    if (rename(ARQUIVO_TEMP, ARQUIVO_DADOS) != 0) { // Coloca a nova versao no lugar.
        perror("Erro ao substituir o arquivo");
        if (rename(ARQUIVO_BACKUP, ARQUIVO_DADOS) != 0) {
            puts("ALERTA: restaure manualmente produtos.bak para produtos.txt.");
        }
        remove(ARQUIVO_TEMP);
        return 0;
    }
    if (remove(ARQUIVO_BACKUP) != 0) puts("Aviso: backup anterior nao foi apagado.");
    return 1;
}

// Permite alterar um campo escolhido de um produto existente.
void alterarProduto(void) {
    Produto lista[MAX_PRODUTOS];
    int quantidade;
    if (!carregarProdutos(lista, &quantidade)) return;
    int id = lerInteiro("ID do produto para alterar: ", 1, INT_MAX);
    int posicao = indicePorID(lista, quantidade, id);
    if (posicao == -1) { puts("Produto nao encontrado."); return; }
    puts("Produto encontrado:");
    cabecalhoTabela(); linhaProduto(&lista[posicao]); linhaTabela();
    puts("1 - Nome | 2 - Categoria | 3 - Preco | 4 - Quantidade | 5 - Fornecedor");
    int campo = lerInteiro("Qual campo deseja alterar? ", 1, 5);
    switch (campo) {                              // Aula 12: escolhe a alteracao.
        case 1: {
            char nome[50];
            lerTexto("Novo nome: ", nome, sizeof nome);
            if (existeNome(lista, quantidade, nome, id)) {
                puts("ERRO: esse nome ja pertence a outro produto.");
                return;
            }
            strcpy(lista[posicao].nome, nome);
            break;
        }
        case 2: lerTexto("Nova categoria: ", lista[posicao].categoria,
                         sizeof lista[posicao].categoria); break;
        case 3: lista[posicao].preco = lerPreco("Novo preco: "); break;
        case 4: lista[posicao].quantidade = lerInteiro("Nova quantidade: ", 0, INT_MAX); break;
        case 5: lerTexto("Novo fornecedor: ", lista[posicao].fornecedor,
                         sizeof lista[posicao].fornecedor); break;
    }
    if (salvarComTemporario(lista, quantidade)) puts("Produto alterado com sucesso!");
    else puts("Alteracao nao concluida. Verifique os arquivos.");
}

// Exclui um produto pelo ID, mantendo todos os outros.
void excluirProduto(void) {
    Produto lista[MAX_PRODUTOS];
    int quantidade;
    if (!carregarProdutos(lista, &quantidade)) return;
    int id = lerInteiro("ID do produto para excluir: ", 1, INT_MAX);
    int posicao = indicePorID(lista, quantidade, id);
    if (posicao == -1) { puts("Produto nao encontrado."); return; }
    printf("Produto: %s (ID %d)\n", lista[posicao].nome, id);
    int confirmar = lerInteiro("Confirma exclusao? 1 = Sim, 0 = Nao: ", 0, 1);
    if (!confirmar) { puts("Exclusao cancelada."); return; }
    for (int i = posicao; i < quantidade - 1; i++) {
        lista[i] = lista[i + 1];                // Desloca os registros apos o excluido.
    }
    quantidade--;                                // O vetor agora tem um produto a menos.
    if (salvarComTemporario(lista, quantidade)) puts("Produto excluido com sucesso!");
    else puts("Exclusao nao concluida. Verifique os arquivos.");
}

// Busca por nome OU categoria; aceita partes do texto e ignora caixa.
void buscarProdutos(int porNome) {
    Produto lista[MAX_PRODUTOS];
    int quantidade, encontrados = 0;
    char termo[50];
    if (!carregarProdutos(lista, &quantidade)) return;
    lerTexto(porNome ? "Nome ou parte do nome: " : "Categoria ou parte dela: ",
             termo, sizeof termo);
    for (int i = 0; i < quantidade; i++) {
        const char *campo = porNome ? lista[i].nome : lista[i].categoria;
        if (contemTexto(campo, termo)) {
            if (encontrados == 0) cabecalhoTabela(); // Exibe cabecalho uma unica vez.
            linhaProduto(&lista[i]);               // Exibe apenas os produtos correspondentes.
            encontrados++;
        }
    }
    if (encontrados) linhaTabela();
    printf("Encontrados: %d produto(s).\n", encontrados);
}

// Troca duas estruturas inteiras usando ponteiros (Aula 09).
void trocarProdutos(Produto *a, Produto *b) {
    Produto auxiliar = *a;                        // Guarda o produto apontado por a.
    *a = *b;                                     // Substitui o produto de a pelo de b.
    *b = auxiliar;                               // Coloca o antigo a na posicao de b.
}

// Ordena o vetor com Bubble Sort (Aula 11), conforme modo escolhido.
// Modo 1: nome A-Z | 2: nome Z-A | 3: preco crescente | 4: preco decrescente.
void bubbleSort(Produto lista[], int quantidade, int modo) {
    for (int i = 0; i < quantidade - 1; i++) {   // Cada passagem coloca um elemento no lugar.
        int houveTroca = 0;                       // Ajuda a encerrar quando o vetor ja esta ordenado.
        for (int j = 0; j < quantidade - i - 1; j++) { // Compara produtos vizinhos.
            int trocar = 0;                       // Assume inicialmente que a ordem esta correta.
            if (modo == 1) trocar = compararTexto(lista[j].nome, lista[j + 1].nome) > 0;
            if (modo == 2) trocar = compararTexto(lista[j].nome, lista[j + 1].nome) < 0;
            if (modo == 3) trocar = lista[j].preco > lista[j + 1].preco;
            if (modo == 4) trocar = lista[j].preco < lista[j + 1].preco;
            if (trocar) {                         // Se estiverem fora de ordem, inverte posicoes.
                trocarProdutos(&lista[j], &lista[j + 1]); // Passa enderecos para a funcao.
                houveTroca = 1;
            }
        }
        if (!houveTroca) break;                  // Evita passagens desnecessarias.
    }
}

// Carrega, ordena e exibe a lista, mas NAO grava essa ordem em produtos.txt.
void mostrarOrdenado(int modo) {
    Produto lista[MAX_PRODUTOS];
    int quantidade;
    if (!carregarProdutos(lista, &quantidade)) return;
    bubbleSort(lista, quantidade, modo);         // Ordena somente o vetor na memoria RAM.
    exibirTabela(lista, quantidade);             // Exibe o resultado como tabela.
    puts("Observacao: a ordem do arquivo original nao foi modificada.");
}

// Escreve um campo CSV entre aspas, duplicando aspas internas quando houver.
void escreverCampoCSV(FILE *fp, const char *texto) {
    fputc('"', fp);                             // Abre aspas para proteger virgulas no texto.
    for (const char *p = texto; *p != '\0'; p++) {
        if (*p == '"') fputc('"', fp);          // Aspa dentro do valor vira duas aspas.
        fputc(*p, fp);                           // Escreve o caractere atual.
    }
    fputc('"', fp);                             // Fecha aspas do campo.
}

// Exporta os dados separados por VIRGULA, como solicitado pelo professor.
void exportarCSV(void) {
    Produto lista[MAX_PRODUTOS];
    int quantidade;
    if (!carregarProdutos(lista, &quantidade)) return;
    FILE *fp = fopen(ARQUIVO_CSV, "w");         // Cria o CSV (sem alterar produtos.txt).
    if (fp == NULL) { perror("Erro ao criar produtos.csv"); return; }
    fputs("ID,Produto,Categoria,Preco,Quantidade,Fornecedor\n", fp); // Cabecalho separado por virgulas.
    int ok = 1;
    for (int i = 0; i < quantidade; i++) {
        Produto *p = &lista[i];                   // Ponteiro para o produto da vez.
        if (fprintf(fp, "%d,", p->id) < 0) { ok = 0; break; }
        escreverCampoCSV(fp, p->nome); fputc(',', fp);
        escreverCampoCSV(fp, p->categoria); fputc(',', fp);
        if (fprintf(fp, "%.2f,%d,", p->preco, p->quantidade) < 0) { ok = 0; break; }
        escreverCampoCSV(fp, p->fornecedor); fputc('\n', fp);
    }
    if (ferror(fp)) ok = 0;
    if (fclose(fp) != 0) ok = 0;
    if (ok) printf("CSV gerado: %s (%d produtos).\n", ARQUIVO_CSV, quantidade);
    else puts("Falha ao exportar CSV. Verifique o arquivo.");
}

// Exibe o menu principal de opcoes para o usuario.
void mostrarMenu(void) {
    puts("\n========== DOCERIA DOCE ENCANTO ==========");
    puts(" 1 - Criar arquivo de cadastro");
    puts(" 2 - Inserir produto");
    puts(" 3 - Listar todos os produtos");
    puts(" 4 - Alterar um produto");
    puts(" 5 - Excluir um produto");
    puts(" 6 - Buscar produto por nome");
    puts(" 7 - Buscar produto por categoria");
    puts(" 8 - Ordenar por nome (A-Z)");
    puts(" 9 - Ordenar por nome (Z-A)");
    puts("10 - Ordenar por preco (menor -> maior)");
    puts("11 - Ordenar por preco (maior -> menor)");
    puts("12 - Exportar dados para CSV");
    puts(" 0 - Sair");
}

// Ponto de entrada: controla todo o programa pelo menu.
int main(void) {
    int opcao;                                    // Guarda a escolha do usuario.
    // Se o computador parou no meio de uma alteracao, tenta recuperar o original.
    if (!recuperarBackup(ARQUIVO_DADOS, ARQUIVO_BACKUP)) return 1;
    // Somente na primeira execucao, quando NAO ha nenhum dos dois arquivos.
    if (!arquivoExiste(ARQUIVO_DADOS) && !arquivoExiste(ARQUIVO_ID)) {
        if (!criarProdutosIniciais()) return 1;  // Cria os 12 doces iniciais.
    } else if (!arquivoExiste(ARQUIVO_DADOS) || !arquivoExiste(ARQUIVO_ID)) {
        puts("ERRO: produtos.txt ou ultimo_id.txt desapareceu.");
        puts("Verifique os arquivos antes de continuar; o cadastro nao sera zerado.");
        return 1;                               // Impede perder dados silenciosamente.
    }
    do {                                         // Repete ate a opcao zero.
        mostrarMenu();                           // Imprime as opcoes disponiveis.
        opcao = lerInteiro("Escolha uma opcao: ", 0, 12);
        switch (opcao) {                         // Executa uma funcao para cada escolha.
            case 1: criarArquivo(); break;
            case 2: inserirProduto(); break;
            case 3: listarProdutos(); break;
            case 4: alterarProduto(); break;
            case 5: excluirProduto(); break;
            case 6: buscarProdutos(1); break;
            case 7: buscarProdutos(0); break;
            case 8: mostrarOrdenado(1); break;
            case 9: mostrarOrdenado(2); break;
            case 10: mostrarOrdenado(3); break;
            case 11: mostrarOrdenado(4); break;
            case 12: exportarCSV(); break;
            case 0: puts("Encerrando o programa. Ate logo!"); break;
            default: puts("Opcao invalida."); break; // Aula 12: trata opcoes desconhecidas.
        }
    } while (opcao != 0);
    return 0;                                    // Indica encerramento normal.
}
