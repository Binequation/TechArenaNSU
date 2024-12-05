import pickle
import argparse
import numpy as np

def load_vectors_in_batches(file_path, batch_size=10000):
    """Построчная загрузка векторов из файла с использованием batch_size для экономии памяти."""
    with open(file_path, 'rb') as f:
        vectors = pickle.load(f)
    for i in range(0, len(vectors), batch_size):
        yield np.array(vectors[i:i + batch_size], dtype=np.float32)

def find_nearest_neighbors_batch(query_vector, dataset_path, k=10, batch_size=10000):
    """Поиск k ближайших соседей с загрузкой данных по частям."""
    all_distances = []
    all_indices = []
    
    for batch_index, vectors_batch in enumerate(load_vectors_in_batches(dataset_path, batch_size)):
        distances = np.linalg.norm(vectors_batch - query_vector, axis=1)
        all_distances.extend(distances)
        all_indices.extend(range(batch_index * batch_size, batch_index * batch_size + len(vectors_batch)))

    # Определяем k ближайших по всем батчам
    sorted_indices = np.argsort(all_distances)[:k]
    nearest_neighbors = [all_indices[i] for i in sorted_indices]
    return nearest_neighbors

def search_top10_nearest(query, dataset_path):
    """Загружает данные и ищет 10 ближайших соседей для запроса."""
    nearest_neighbors = find_nearest_neighbors_batch(query, dataset_path, k=10)
    print(','.join(map(str, nearest_neighbors)))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Поиск 10 ближайших соседей")
    parser.add_argument('--query', required=True, help="Вектор запроса в виде списка чисел с плавающей точкой (например, [1,2,3,...])")
    args = parser.parse_args()

    try:
        query_vector = np.array(list(map(float, args.query.strip('[]').split(','))), dtype=np.float32)
    except ValueError as e:
        print(f"Ошибка преобразования вектора: {e}")
        exit(1)

    dataset_path = '../datasets/gist/gist_base.pkl' if len(query_vector) > 128 else '../datasets/sift/sift_base.pkl'
    search_top10_nearest(query_vector, dataset_path)
