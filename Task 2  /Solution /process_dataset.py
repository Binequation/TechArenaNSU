import struct
import pickle
import os
import argparse
import numpy as np

def read_fvecs(file_path, batch_size=1024):
    """Чтение fvecs файла и извлечение векторов с буферизацией."""
    vectors = []
    with open(file_path, 'rb') as f:
        while True:
            batch = []
            for _ in range(batch_size):
                # Читаем размерность вектора
                dim_bytes = f.read(4)
                if not dim_bytes:
                    break
                dim = struct.unpack('i', dim_bytes)[0]
                # Читаем сам вектор
                vector = struct.unpack(f'{dim}f', f.read(4 * dim))
                batch.append(np.array(vector, dtype=np.float32))
            if not batch:
                break
            vectors.extend(batch)
    return vectors

def save_vectors(vectors, output_path):
    """Сохранение векторов в бинарном формате."""
    with open(output_path, 'wb') as f:
        pickle.dump(vectors, f)

def process_dataset(input_file, batch_size=1024):
    """Обрабатывает набор данных и сохраняет его в оптимизированном формате."""
    vectors = read_fvecs(input_file, batch_size)
    
    # Генерация имени выходного файла (замена расширения на .pkl)
    output_file = os.path.splitext(input_file)[0] + '.pkl'
    
    save_vectors(vectors, output_file)
    print(f"Набор данных сохранен в {output_file}")

if __name__ == '__main__':    
    parser = argparse.ArgumentParser(description="Process dataset and save in optimized format")
    parser.add_argument('--data', required=True, help="Path to the input .fvecs file")
    parser.add_argument('--batch_size', type=int, default=1024, help="Batch size for reading data")
    args = parser.parse_args()

    process_dataset(args.data, args.batch_size)