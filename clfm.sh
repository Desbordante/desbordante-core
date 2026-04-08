#!/bin/bash

# Проверяем наличие clang-format
if ! command -v clang-format-21 &> /dev/null; then
    echo "Ошибка: clang-format не установлен"
    exit 1
fi

# Проверяем наличие директории src
if [ ! -d "src" ]; then
    echo "Ошибка: директория src не найдена"
    exit 1
fi

# Форматируем все файлы в src/
find src/ -type f \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \) -exec clang-format-21 -i {} \;

echo "Форматирование завершено"
