import glob
import os
import struct
import pandas as pd

try:
    import truststore
    truststore.inject_into_ssl()
except ImportError:
    pass

import kagglehub

K = 1000
FMT_IDX = "<QQ"
CAMINHO_LOCAL = None
LIMITE_POR_ARQUIVO = 250000

PRODUTOS = {
    "dados": "produtos.dat",
    "indice": "produtos.idx",
    "fmt": "<cQQ64s32sdc",
    "larg": [None, None, 64, 32, None],
    "nomes": ["product_id", "category_id", "category_code", "brand", "price"],
    "tipos": [int, int, str, str, float],
}

EVENTOS = {
    "dados": "eventos.dat",
    "indice": "eventos.idx",
    "fmt": "<cQQQ23s16s36sc",
    "larg": [None, None, None, 23, 16, 36],
    "nomes": ["event_id", "product_id", "user_id", "event_time", "event_type", "user_session"],
    "tipos": [int, int, int, str, str, str],
}


def tam(a):
    return struct.calcsize(a["fmt"])


def total(a):
    return os.path.getsize(a["dados"]) // tam(a)


def texto(valor, largura):
    return str(valor).encode()[:largura].ljust(largura)


def empacotar(a, campos):
    valores = [texto(v, l) if l else v for v, l in zip(campos, a["larg"])]
    return struct.pack(a["fmt"], b"1", *valores, b"\n")


def decodificar(reg):
    return [x.decode(errors="ignore").strip() if isinstance(x, bytes) else x for x in reg[:-1]]


def ler(a, f, n):
    f.seek(n * tam(a))
    return struct.unpack(a["fmt"], f.read(tam(a)))


def limpo(valor):
    return "" if pd.isna(valor) else str(valor)


def construir_indice(a):
    with open(a["dados"], "rb") as d, open(a["indice"], "wb") as idx:
        for n in range(0, total(a), K):
            idx.write(struct.pack(FMT_IDX, ler(a, d, n)[1], n))


def inserir_iniciais(a, registros):
    with open(a["dados"], "wb") as f:
        for campos in registros:
            f.write(empacotar(a, campos))
    construir_indice(a)


def mostrar(a, inicio=0, quantidade=10):
    n = total(a)
    fim = n if quantidade is None else min(n, inicio + quantidade)
    with open(a["dados"], "rb") as f:
        for i in range(inicio, fim):
            print(i, decodificar(ler(a, f, i)))


def posicao(a, chave):
    n = total(a)
    ini, fim = 0, n
    with open(a["dados"], "rb") as f:
        while ini < fim:
            meio = (ini + fim) // 2
            if ler(a, f, meio)[1] < chave:
                ini = meio + 1
            else:
                fim = meio
        reg = ler(a, f, ini) if ini < n else None
    if reg and reg[1] == chave:
        return ini, reg
    return ini, None


def pesquisa_binaria(a, chave):
    pos, reg = posicao(a, chave)
    if reg and reg[0] == b"1":
        return decodificar(reg)
    return None


def consultar_indice(a, chave):
    ti = struct.calcsize(FMT_IDX)
    ini, fim, base = 0, os.path.getsize(a["indice"]) // ti - 1, 0
    with open(a["indice"], "rb") as idx:
        while ini <= fim:
            meio = (ini + fim) // 2
            idx.seek(meio * ti)
            k, p = struct.unpack(FMT_IDX, idx.read(ti))
            if k <= chave:
                base = p
                ini = meio + 1
            else:
                fim = meio - 1
    t = tam(a)
    with open(a["dados"], "rb") as f:
        f.seek(base * t)
        for _ in range(K):
            bruto = f.read(t)
            if not bruto:
                break
            reg = struct.unpack(a["fmt"], bruto)
            if reg[1] == chave:
                return decodificar(reg) if reg[0] == b"1" else None
            if reg[1] > chave:
                break
    return None


def inserir(a, campos):
    pos, existente = posicao(a, campos[0])
    novo = empacotar(a, campos)
    t = tam(a)
    if existente and existente[0] == b"1":
        return False
    with open(a["dados"], "r+b") as f:
        if existente:
            f.seek(pos * t)
            f.write(novo)
            return True
        fim = total(a)
        while fim > pos:
            ini = max(pos, fim - 1000)
            f.seek(ini * t)
            bloco = f.read((fim - ini) * t)
            f.seek((ini + 1) * t)
            f.write(bloco)
            fim = ini
        f.seek(pos * t)
        f.write(novo)
    construir_indice(a)
    return True


def remover(a, chave):
    pos, reg = posicao(a, chave)
    if not reg or reg[0] != b"1":
        return False
    with open(a["dados"], "r+b") as f:
        f.seek(pos * tam(a))
        f.write(b"0")
    return True


def localizar_csvs():
    if CAMINHO_LOCAL:
        path = CAMINHO_LOCAL
    else:
        path = kagglehub.dataset_download("mkechinov/ecommerce-behavior-data-from-multi-category-store")
    print("Path to dataset files:", path)
    return sorted(glob.glob(os.path.join(path, "*.csv")), key=lambda c: "Nov" in os.path.basename(c))


def gerar_eventos(caminhos, limite, produtos):
    n = 0
    for caminho in caminhos:
        for bloco in pd.read_csv(caminho, chunksize=100000, nrows=limite):
            for r in bloco.itertuples(index=False):
                n += 1
                pid = int(r.product_id)
                if pid not in produtos:
                    produtos[pid] = (int(r.category_id), limpo(r.category_code), limpo(r.brand), float(r.price))
                yield (n, pid, int(r.user_id), limpo(r.event_time), limpo(r.event_type), limpo(r.user_session))


def carregar_csv(caminhos, limite):
    produtos = {}
    inserir_iniciais(EVENTOS, gerar_eventos(caminhos, limite, produtos))
    inserir_iniciais(PRODUTOS, ((pid,) + dados for pid, dados in sorted(produtos.items())))


def escolher_arquivo():
    return {"1": PRODUTOS, "2": EVENTOS}.get(input("Arquivo (1-produtos, 2-eventos): "))


def ler_campos(a):
    return [tipo(input(nome + ": ")) for nome, tipo in zip(a["nomes"], a["tipos"])]


def executar(op):
    if op == "1":
        if input("Isso recria os arquivos e apaga alteracoes. Continuar? (s/n): ") == "s":
            carregar_csv(localizar_csvs(), LIMITE_POR_ARQUIVO)
        return
    a = escolher_arquivo()
    if a is None:
        print("Arquivo invalido")
    elif op == "2":
        inicio = int(input("Posicao inicial: "))
        quantidade = int(input("Quantidade (0 = tudo): "))
        mostrar(a, inicio, quantidade or None)
    elif op == "3":
        print(pesquisa_binaria(a, int(input("Chave: "))) or "Nao encontrado")
    elif op == "4":
        print(consultar_indice(a, int(input("Chave: "))) or "Nao encontrado")
    elif op == "5":
        print("Inserido" if inserir(a, ler_campos(a)) else "Chave ja existe")
    elif op == "6":
        print("Removido" if remover(a, int(input("Chave: "))) else "Chave nao encontrada")


def menu():
    while True:
        print("\n1-Carregar CSV  2-Mostrar  3-Pesquisa binaria  4-Consulta por indice  5-Inserir  6-Remover  0-Sair")
        op = input("Opcao: ")
        if op == "0":
            break
        try:
            executar(op)
        except (ValueError, FileNotFoundError):
            print("Entrada invalida ou arquivos ainda nao gerados")


if __name__ == "__main__":
    menu()
