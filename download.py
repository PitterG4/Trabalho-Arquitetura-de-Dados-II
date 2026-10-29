import csv
import glob
import os
import random
import sys

dependencies_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dependencies")
if os.path.exists(dependencies_path):
    sys.path.insert(0, dependencies_path)

DATA_DIR = "data"


def generate_sample(limit=10000):
    access_path = os.path.join(DATA_DIR, "acessos.csv")
    product_path = os.path.join(DATA_DIR, "produtos.csv")

    # Se a amostra ja existe, apenas mantem e usa ela
    if os.path.exists(access_path) and os.path.exists(product_path):
        print(f"Usando a amostra já presente em '{DATA_DIR}/'.")
        return

    categories = [
        "electronics.smartphone", "computers.notebook", "electronics.audio.headphone",
        "appliances.kitchen.refrigerators", "electronics.video.tv", "appliances.environment.air_conditioner"
    ]
    brands = ["samsung", "apple", "xiaomi", "lg", "sony", "dell", "motorola", "panasonic", "philips"]
    event_types = ["view", "cart", "purchase"]

    print(f"Gerando amostra em '{DATA_DIR}/'...")

    products = {}

    with open(access_path, "w", encoding="utf-8", newline="\n") as access_file:
        for i in range(1, limit + 1):
            product_id = 1000000 + (i % 250) + 1
            user_id = 500000000 + (i % 100) + 1
            event_type = random.choices(event_types, weights=[80, 15, 5])[0]
            event_time = f"2019-10-01 00:{i % 60:02d}:00 UTC"

            if product_id not in products:
                cat = random.choice(categories)
                brand = random.choice(brands)
                price = round(random.uniform(50.0, 1500.0), 2)
                products[product_id] = (cat, brand, price)

            access_file.write(f"{i},{product_id},{user_id},{event_time},{event_type}\n")

    with open(product_path, "w", encoding="utf-8", newline="\n") as product_file:
        for product_id, (category, brand, price) in sorted(products.items()):
            product_file.write(f"{product_id},{category},{brand},{price:.2f}\n")

    print(f"Sucesso! Gerados '{access_path}' ({limit} acessos) e '{product_path}' ({len(products)} produtos únicos).")


def download_from_kaggle():
    import kagglehub

    print("\nAviso: O dataset tem cerca de 4 GB.")
    confirm = input("Deseja realmente baixar o dataset? (s/n): ").strip().lower()
    if confirm != "s":
        print("Download cancelado.")
        return

    print("Baixando dataset...")
    path = kagglehub.dataset_download("mkechinov/ecommerce-behavior-data-from-multi-category-store")
    files = sorted(glob.glob(os.path.join(path, "*.csv")))

    if not files:
        print("Nenhum CSV encontrado no dataset baixado.")
        return

    raw_limit = input("Quantas linhas deseja manter do arquivo baixado? (0 = tudo): ").strip()
    limit = int(raw_limit) if raw_limit.isdigit() else 10000

    products = {}
    count = 0

    access_path = os.path.join(DATA_DIR, "acessos.csv")
    product_path = os.path.join(DATA_DIR, "produtos.csv")

    with open(access_path, "w", encoding="utf-8", newline="\n") as access_file:
        for file_path in files:
            with open(file_path, "r", encoding="utf-8", errors="ignore") as input_file:
                reader = csv.DictReader(input_file)
                for row in reader:
                    count += 1
                    product_id = int(row["product_id"])
                    user_id = int(row["user_id"])
                    category = row.get("category_code") or "sem_categoria"
                    brand = row.get("brand") or "sem_marca"
                    price = float(row.get("price") or 0.0)
                    event_time = row.get("event_time") or "sem_data"
                    event_type = row.get("event_type") or "sem_tipo"

                    if product_id not in products:
                        products[product_id] = (category, brand, price)

                    access_file.write(f"{count},{product_id},{user_id},{event_time},{event_type}\n")

                    if limit > 0 and count >= limit:
                        break
            if limit > 0 and count >= limit:
                break

    with open(product_path, "w", encoding="utf-8", newline="\n") as product_file:
        for product_id, (category, brand, price) in sorted(products.items()):
            product_file.write(f"{product_id},{category},{brand},{price:.2f}\n")

    print(f"Sucesso! Gerados '{access_path}' ({count} registros) e '{product_path}' ({len(products)} produtos).")


def main():
    if not os.path.exists(DATA_DIR):
        os.makedirs(DATA_DIR)

    print("\nDOWNLOAD DOS DADOS:")
    print("[1] - Usar a amostra")
    print("[2] - Baixar dataset (4 GB) e escolher amostra")
    option = input("Escolha a opção: ").strip()

    if option == "2":
        download_from_kaggle()
    else:
        generate_sample()


if __name__ == "__main__":
    main()
