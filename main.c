#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define INTERVAL 50

typedef struct {
    int id;
    char category[40];
    char brand[20];
    float price;
    char end_line;
} Product;

typedef struct {
    int id;
    int product_id;
    int user_id;
    char event_time[25];
    char event_type[15];
    char end_line;
} Access;

typedef struct {
    int idx;
    int id;
} Index;

Index *product_index = NULL;
int product_clusters = 0;

Index *access_index = NULL;
int access_clusters = 0;

// Preenche o texto com espacos em branco no final para manter tamanho fixo
void format_text(char *destination, const char *source, int size)
{
    snprintf(destination, size, "%-*s", size - 1, source);
}

// Constroi o indice parcial pulando de INTERVAL em INTERVAL com fseek
Index *build_index(const char *file_name, int record_size, int *clusters)
{
    FILE *binary = fopen(file_name, "rb");
    if (binary == NULL) return NULL;

    fseek(binary, 0, SEEK_END);
    int elements_amount = ftell(binary) / record_size;

    *clusters = (elements_amount + INTERVAL - 1) / INTERVAL;
    Index *index = malloc((*clusters) * sizeof(Index));

    for (int i = 0; i < *clusters; i++) {
        int id;
        fseek(binary, i * INTERVAL * record_size, SEEK_SET);
        fread(&id, sizeof(int), 1, binary);

        index[i].idx = i * INTERVAL;
        index[i].id = id;
    }

    fclose(binary);
    return index;
}

void rebuild_product_index()
{
    free(product_index);
    product_index = build_index("data/produtos.bin", sizeof(Product), &product_clusters);
}

void rebuild_access_index()
{
    free(access_index);
    access_index = build_index("data/acessos.bin", sizeof(Access), &access_clusters);
}

void save_index(const char *file_name, Index *index, int clusters)
{
    FILE *index_file = fopen(file_name, "wb");
    if (index_file != NULL) {
        fwrite(index, sizeof(Index), clusters, index_file);
        fclose(index_file);
    }
}

Index *load_index(const char *index_name, const char *data_name, int record_size, int *clusters)
{
    FILE *index_file = fopen(index_name, "rb");
    if (index_file != NULL) {
        fseek(index_file, 0, SEEK_END);
        *clusters = ftell(index_file) / sizeof(Index);
        rewind(index_file);

        Index *index = malloc((*clusters) * sizeof(Index));
        fread(index, sizeof(Index), *clusters, index_file);
        fclose(index_file);
        return index;
    }

    return build_index(data_name, record_size, clusters);
}

// Converte os CSVs já ordenados pra binario na pasta data
void insert_initial_data()
{
    FILE *product_text = fopen("data/produtos.csv", "r");
    FILE *access_text = fopen("data/acessos.csv", "r");

    // Se dados não existirem, roda download.py
    if (product_text == NULL || access_text == NULL) {
        if (product_text != NULL) fclose(product_text);
        if (access_text != NULL) fclose(access_text);

        system("python download.py");

        product_text = fopen("data/produtos.csv", "r");
        access_text = fopen("data/acessos.csv", "r");
        if (product_text == NULL || access_text == NULL) return;
    }

    FILE *product_binary = fopen("data/produtos.bin", "wb");
    char line[256] = {0};

    while (fgets(line, sizeof(line), product_text) != NULL) {
        Product product;
        char category[40] = {0};
        char brand[20] = {0};

        sscanf(line, "%d,%39[^,],%19[^,],%f", &product.id, category, brand, &product.price);
        format_text(product.category, category, sizeof(product.category));
        format_text(product.brand, brand, sizeof(product.brand));
        product.end_line = '\n';

        fwrite(&product, sizeof(Product), 1, product_binary);
    }

    fclose(product_text);
    fclose(product_binary);

    FILE *access_binary = fopen("data/acessos.bin", "wb");

    while (fgets(line, sizeof(line), access_text) != NULL) {
        Access access;
        char event_time[25] = {0};
        char event_type[15] = {0};

        sscanf(line, "%d,%d,%d,%24[^,],%14[^\r\n]",
               &access.id, &access.product_id, &access.user_id, event_time, event_type);
        format_text(access.event_time, event_time, sizeof(access.event_time));
        format_text(access.event_type, event_type, sizeof(access.event_type));
        access.end_line = '\n';

        fwrite(&access, sizeof(Access), 1, access_binary);
    }

    fclose(access_text);
    fclose(access_binary);

    rebuild_product_index();
    rebuild_access_index();
    save_index("data/produtos_index.bin", product_index, product_clusters);
    save_index("data/acessos_index.bin", access_index, access_clusters);

    printf("Arquivos binarios gerados na pasta data/!\n");
}

void show_products()
{
    int start_position, amount;
    printf("Posicao inicial: ");
    scanf("%d", &start_position);
    printf("Quantidade (0 = tudo): ");
    scanf("%d", &amount);
    getchar();

    FILE *binary = fopen("data/produtos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, start_position * sizeof(Product), SEEK_SET);

    Product buffer;
    int count = 0;

    while (fread(&buffer, sizeof(Product), 1, binary) == 1) {
        printf("ID: %d | Categoria: %s | Marca: %s | Preco: %.2f\n",
               buffer.id, buffer.category, buffer.brand, buffer.price);
        count++;
        if (amount > 0 && count >= amount) break;
    }

    fclose(binary);
}

void show_accesses()
{
    int start_position, amount;
    printf("Posicao inicial: ");
    scanf("%d", &start_position);
    printf("Quantidade (0 = tudo): ");
    scanf("%d", &amount);
    getchar();

    FILE *binary = fopen("data/acessos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, start_position * sizeof(Access), SEEK_SET);

    Access buffer;
    int count = 0;

    while (fread(&buffer, sizeof(Access), 1, binary) == 1) {
        printf("ID: %d | Produto: %d | Usuario: %d | Data: %s | Tipo: %s\n",
               buffer.id, buffer.product_id, buffer.user_id, buffer.event_time, buffer.event_type);
        count++;
        if (amount > 0 && count >= amount) break;
    }

    fclose(binary);
}

void binary_search_product()
{
    int target_id;
    Product buffer;

    printf("Digite o ID do produto para buscar: ");
    scanf("%d", &target_id);
    getchar();

    FILE *binary = fopen("data/produtos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, 0, SEEK_END);
    int elements_amount = ftell(binary) / sizeof(Product);

    int first = 0;
    int last = elements_amount - 1;
    int comparison = 0;
    int found = -1;

    clock_t start = clock();

    while (first <= last) {
        int mid = (first + last) / 2;

        fseek(binary, mid * sizeof(Product), SEEK_SET);
        fread(&buffer, sizeof(Product), 1, binary);

        comparison++;

        if (target_id == buffer.id) {
            found = mid;
            break;
        }
        else if (target_id < buffer.id) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    clock_t finish = clock();
    double time_spent = (double)(finish - start) / CLOCKS_PER_SEC;

    fclose(binary);

    if (found != -1) {
        printf("\nProduto encontrado no byte %d.\n", (int)(found * sizeof(Product)));
        printf("ID: %d | Categoria: %s | Marca: %s | Preco: %.2f\n",
               buffer.id, buffer.category, buffer.brand, buffer.price);
    } else {
        printf("\nProduto nao encontrado.\n");
    }

    printf("Comparacoes: %d\n", comparison);
    printf("Tempo gasto: %f segundos\n", time_spent);
}

void binary_search_access()
{
    int target_id;
    Access buffer;

    printf("Digite o ID do acesso para buscar: ");
    scanf("%d", &target_id);
    getchar();

    FILE *binary = fopen("data/acessos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, 0, SEEK_END);
    int elements_amount = ftell(binary) / sizeof(Access);

    int first = 0;
    int last = elements_amount - 1;
    int comparison = 0;
    int found = -1;

    clock_t start = clock();

    while (first <= last) {
        int mid = (first + last) / 2;

        fseek(binary, mid * sizeof(Access), SEEK_SET);
        fread(&buffer, sizeof(Access), 1, binary);

        comparison++;

        if (target_id == buffer.id) {
            found = mid;
            break;
        }
        else if (target_id < buffer.id) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    clock_t finish = clock();
    double time_spent = (double)(finish - start) / CLOCKS_PER_SEC;

    fclose(binary);

    if (found != -1) {
        printf("\nAcesso encontrado no byte %d.\n", (int)(found * sizeof(Access)));
        printf("ID: %d | Produto: %d | Usuario: %d | Data: %s | Tipo: %s\n",
               buffer.id, buffer.product_id, buffer.user_id, buffer.event_time, buffer.event_type);
    } else {
        printf("\nAcesso nao encontrado.\n");
    }

    printf("Comparacoes: %d\n", comparison);
    printf("Tempo gasto: %f segundos\n", time_spent);
}

void index_search_product()
{
    int target_id;
    printf("Digite o ID do produto para buscar: ");
    scanf("%d", &target_id);
    getchar();

    int first = 0;
    int last = product_clusters - 1;
    int cluster_index = 0;
    int comparisons = 0;

    while (first <= last) {
        int mid = (first + last) / 2;

        comparisons++;

        if (target_id == product_index[mid].id) {
            cluster_index = mid;
            break;
        } else if (target_id < product_index[mid].id) {
            last = mid - 1;
        } else {
            cluster_index = mid;
            first = mid + 1;
        }
    }

    FILE *binary = fopen("data/produtos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, product_index[cluster_index].idx * sizeof(Product), SEEK_SET);

    Product buffer;
    int found = -1;

    for (int i = 0; i < INTERVAL; i++) {
        if (fread(&buffer, sizeof(Product), 1, binary) != 1) break;
        comparisons++;

        if (target_id == buffer.id) {
            found = product_index[cluster_index].idx + i;
            break;
        }
        if (buffer.id > target_id) break;
    }
    fclose(binary);

    if (found != -1) {
        printf("Produto %d encontrado no byte %d! Comparacoes: %d\n",
               target_id, (int)(found * sizeof(Product)), comparisons);
        printf("ID: %d | Categoria: %s | Marca: %s | Preco: %.2f\n",
               buffer.id, buffer.category, buffer.brand, buffer.price);
    } else {
        printf("Produto %d nao encontrado! Comparacoes: %d\n", target_id, comparisons);
    }
}

void index_search_access()
{
    int target_id;
    printf("Digite o ID do acesso para buscar: ");
    scanf("%d", &target_id);
    getchar();

    int first = 0;
    int last = access_clusters - 1;
    int cluster_index = 0;
    int comparisons = 0;

    while (first <= last) {
        int mid = (first + last) / 2;

        comparisons++;

        if (target_id == access_index[mid].id) {
            cluster_index = mid;
            break;
        } else if (target_id < access_index[mid].id) {
            last = mid - 1;
        } else {
            cluster_index = mid;
            first = mid + 1;
        }
    }

    FILE *binary = fopen("data/acessos.bin", "rb");
    if (binary == NULL) return;

    fseek(binary, access_index[cluster_index].idx * sizeof(Access), SEEK_SET);

    Access buffer;
    int found = -1;

    for (int i = 0; i < INTERVAL; i++) {
        if (fread(&buffer, sizeof(Access), 1, binary) != 1) break;
        comparisons++;

        if (target_id == buffer.id) {
            found = access_index[cluster_index].idx + i;
            break;
        }
        if (buffer.id > target_id) break;
    }
    fclose(binary);

    if (found != -1) {
        printf("Acesso %d encontrado no byte %d! Comparacoes: %d\n",
               target_id, (int)(found * sizeof(Access)), comparisons);
        printf("ID: %d | Produto: %d | Usuario: %d | Data: %s | Tipo: %s\n",
               buffer.id, buffer.product_id, buffer.user_id, buffer.event_time, buffer.event_type);
    } else {
        printf("Acesso %d nao encontrado! Comparacoes: %d\n", target_id, comparisons);
    }
}

void add_product()
{
    Product new_product;
    char category[100] = {0};
    char brand[100] = {0};

    printf("ID do produto: ");
    scanf("%d", &new_product.id);
    getchar();

    printf("Categoria: ");
    fgets(category, sizeof(category), stdin);
    category[strcspn(category, "\r\n")] = '\0';

    printf("Marca: ");
    fgets(brand, sizeof(brand), stdin);
    brand[strcspn(brand, "\r\n")] = '\0';

    printf("Preco: ");
    scanf("%f", &new_product.price);
    getchar();

    format_text(new_product.category, category, sizeof(new_product.category));
    format_text(new_product.brand, brand, sizeof(new_product.brand));
    new_product.end_line = '\n';

    FILE *binary = fopen("data/produtos.bin", "rb");
    FILE *aux = fopen("data/temp.bin", "wb");
    if (binary == NULL || aux == NULL) return;

    Product buffer;
    bool inserted = false;

    while (fread(&buffer, sizeof(Product), 1, binary) == 1) {
        if (!inserted && buffer.id > new_product.id) {
            fwrite(&new_product, sizeof(Product), 1, aux);
            inserted = true;
        }
        fwrite(&buffer, sizeof(Product), 1, aux);
    }

    if (!inserted) {
        fwrite(&new_product, sizeof(Product), 1, aux);
    }

    fclose(binary);
    fclose(aux);

    remove("data/produtos.bin");
    rename("data/temp.bin", "data/produtos.bin");

    rebuild_product_index();
    printf("Produto adicionado!\n");
}

void add_access()
{
    Access new_access;
    char event_time[100] = {0};
    char event_type[100] = {0};

    printf("ID do acesso: ");
    scanf("%d", &new_access.id);
    printf("ID do produto: ");
    scanf("%d", &new_access.product_id);
    printf("ID do usuario: ");
    scanf("%d", &new_access.user_id);
    getchar();

    printf("Data/hora: ");
    fgets(event_time, sizeof(event_time), stdin);
    event_time[strcspn(event_time, "\r\n")] = '\0';

    printf("Tipo (view, cart, purchase): ");
    fgets(event_type, sizeof(event_type), stdin);
    event_type[strcspn(event_type, "\r\n")] = '\0';

    format_text(new_access.event_time, event_time, sizeof(new_access.event_time));
    format_text(new_access.event_type, event_type, sizeof(new_access.event_type));
    new_access.end_line = '\n';

    FILE *binary = fopen("data/acessos.bin", "rb");
    FILE *aux = fopen("data/temp.bin", "wb");
    if (binary == NULL || aux == NULL) return;

    Access buffer;
    bool inserted = false;

    while (fread(&buffer, sizeof(Access), 1, binary) == 1) {
        if (!inserted && buffer.id > new_access.id) {
            fwrite(&new_access, sizeof(Access), 1, aux);
            inserted = true;
        }
        fwrite(&buffer, sizeof(Access), 1, aux);
    }

    if (!inserted) {
        fwrite(&new_access, sizeof(Access), 1, aux);
    }

    fclose(binary);
    fclose(aux);

    remove("data/acessos.bin");
    rename("data/temp.bin", "data/acessos.bin");

    rebuild_access_index();
    printf("Acesso adicionado!\n");
}

void remove_product()
{
    int target_id;
    printf("Digite o ID do produto para remover: ");
    scanf("%d", &target_id);
    getchar();

    FILE *binary = fopen("data/produtos.bin", "rb");
    FILE *aux = fopen("data/temp.bin", "wb");
    if (binary == NULL || aux == NULL) return;

    Product buffer;
    int offset = 0;

    while (true) {
        fseek(binary, offset, SEEK_SET);
        if (fread(&buffer, sizeof(Product), 1, binary) != 1) break;

        if (buffer.id != target_id) {
            fwrite(&buffer, sizeof(Product), 1, aux);
        }

        offset += sizeof(Product);
    }

    fclose(binary);
    fclose(aux);

    remove("data/produtos.bin");
    rename("data/temp.bin", "data/produtos.bin");

    rebuild_product_index();
    printf("Produto removido!\n");
}

void remove_access()
{
    int target_id;
    printf("Digite o ID do acesso para remover: ");
    scanf("%d", &target_id);
    getchar();

    FILE *binary = fopen("data/acessos.bin", "rb");
    FILE *aux = fopen("data/temp.bin", "wb");
    if (binary == NULL || aux == NULL) return;

    Access buffer;
    int offset = 0;

    while (true) {
        fseek(binary, offset, SEEK_SET);
        if (fread(&buffer, sizeof(Access), 1, binary) != 1) break;

        if (buffer.id != target_id) {
            fwrite(&buffer, sizeof(Access), 1, aux);
        }

        offset += sizeof(Access);
    }

    fclose(binary);
    fclose(aux);

    remove("data/acessos.bin");
    rename("data/temp.bin", "data/acessos.bin");

    rebuild_access_index();
    printf("Acesso removido!\n");
}

void show_queries()
{
    printf("\nCONSULTAS DO E-COMMERCE:\n");

    FILE *access_file = fopen("data/acessos.bin", "rb");
    if (access_file != NULL) {
        Access access;
        int views = 0, carts = 0, purchases = 0, total = 0;

        while (fread(&access, sizeof(Access), 1, access_file) == 1) {
            total++;
            if (strncmp(access.event_type, "view", 4) == 0) views++;
            else if (strncmp(access.event_type, "cart", 4) == 0) carts++;
            else if (strncmp(access.event_type, "purchase", 8) == 0) purchases++;
        }
        fclose(access_file);

        printf("\n1) Eventos (Total: %d):\n", total);
        printf("   - Visualizacoes (view): %d\n", views);
        printf("   - Carrinho (cart):      %d\n", carts);
        printf("   - Compras (purchase):   %d\n", purchases);
        if (views > 0) {
            printf("   - Conversao (compras / visualizacoes): %.2f%%\n", (purchases * 100.0) / views);
        }
    }

    FILE *product_file = fopen("data/produtos.bin", "rb");
    if (product_file != NULL) {
        Product product, max_product, min_product;
        int count = 0;
        double sum = 0.0;

        while (fread(&product, sizeof(Product), 1, product_file) == 1) {
            if (count == 0 || product.price > max_product.price) max_product = product;
            if (count == 0 || (product.price > 0 && product.price < min_product.price)) min_product = product;
            sum += product.price;
            count++;
        }
        fclose(product_file);

        if (count > 0) {
            printf("\n2) Precos (%d produtos):\n", count);
            printf("   - Preco medio: $%.2f\n", sum / count);
            printf("   - Mais caro: ID %d | Marca: %s -> $%.2f\n", max_product.id, max_product.brand, max_product.price);
            printf("   - Mais barato: ID %d |  Marca: %s -> $%.2f\n", min_product.id, min_product.brand, min_product.price);
        }
    }
}

int main()
{
    system("python download.py");
    insert_initial_data();

    while (true) {
        int user;
        printf("\n[1] - Inserir dados iniciais\n");
        printf("[2] - Mostrar dados\n");
        printf("[3] - Busca binaria\n");
        printf("[4] - Busca por indice parcial\n");
        printf("[5] - Inserir registro\n");
        printf("[6] - Remover registro\n");
        printf("[7] - Consultas do e-commerce\n");
        printf("[8] - Baixar tabela (download.py)\n");
        printf("[9] - Encerrar programa\n");
        printf("\nEscolha o que deseja fazer: ");

        scanf("%1d", &user);
        getchar();

        if (user == 1) {
            insert_initial_data();
        } else if (user == 7) {
            show_queries();
        } else if (user == 8) {
            system("python download.py");
            insert_initial_data();
        } else if (user == 9) {
            save_index("data/produtos_index.bin", product_index, product_clusters);
            save_index("data/acessos_index.bin", access_index, access_clusters);
            printf("\nPrograma concluido.\n");
            return 0;
        } else if (user >= 2 && user <= 6) {
            int file_option;
            printf("Arquivo ([1] - Produtos | [2] - Acessos): ");
            scanf("%1d", &file_option);
            getchar();

            if (file_option == 1) {
                switch (user) {
                    case 2: show_products(); break;
                    case 3: binary_search_product(); break;
                    case 4: index_search_product(); break;
                    case 5: add_product(); break;
                    case 6: remove_product(); break;
                }
            } else if (file_option == 2) {
                switch (user) {
                    case 2: show_accesses(); break;
                    case 3: binary_search_access(); break;
                    case 4: index_search_access(); break;
                    case 5: add_access(); break;
                    case 6: remove_access(); break;
                }
            }
        } else {
            printf("Selecione uma opcao correta.\n");
        }
    }

    return 0;
}
