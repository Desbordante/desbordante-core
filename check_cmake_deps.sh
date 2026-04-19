#!/bin/bash
# check_cmake_deps.sh – проверяет, что для используемых в исходниках
# пространств имён в CMakeLists.txt указаны соответствующие зависимости.
# Запуск: ./check_cmake_deps.sh [директория]

set -euo pipefail

TARGET_DIR="${1:-src/core/algorithms}"
PREFIX="${DESBORDANTE_PREFIX:-Desbordante}"

RED='\033[1;31m'
NC='\033[0m'

declare -A DEPS=(
    ["model::ColumnLayoutTypedRelationData"]="${PREFIX}::model::table ${PREFIX}::model"
    ["model::ColumnLayoutRelationData"]="${PREFIX}::model::table ${PREFIX}::model"
    ["model::IDatasetStream"]="${PREFIX}::model ${PREFIX}::model::table"
    ["model::TypeId"]="${PREFIX}::model::types ${PREFIX}::model"
    ["model::MixedType"]="${PREFIX}::model::types ${PREFIX}::model"
    ["algos::Algorithm"]="${PREFIX}::algos"
    ["algos::StdParamsMap"]="${PREFIX}::algos"
    ["algos::CreateAndLoadAlgorithm"]="${PREFIX}::algos"
    ["util::TimedInvoke"]="${PREFIX}::util"
    ["config::InputTable"]="${PREFIX}::config"
    ["config::names::"]="${PREFIX}::config"
    ["config::Option"]="${PREFIX}::config"
    ["parser::CSVParser"]="${PREFIX}::parser::csv"
    ["spdlog::"]="spdlog::spdlog_header_only"
    ["boost::dynamic_bitset"]="Boost::headers"
    ["boost::any"]="Boost::headers"
)

check_dep() {
    local alternatives="$1"
    local cmake_file="$2"
    for dep in $alternatives; do
        # 1) буквальное совпадение (Desbordante::config, Boost::headers)
        if grep -qF "$dep" "$cmake_file"; then
            return 0
        fi
        # 2) форма ${DESBORDANTE_PREFIX}::<suffix>
        local suffix="${dep#${PREFIX}}"   # ::config, ::model::table и т.д.
        if grep -qF '${DESBORDANTE_PREFIX}'"$suffix" "$cmake_file"; then
            return 0
        fi
    done
    return 1
}

format_alternatives() {
    local alternatives="$1"
    local result=""
    local count=0
    for dep in $alternatives; do
        [[ $count -gt 0 ]] && result+=", "
        result+="$dep"
        if [[ "$dep" == "${PREFIX}::"* ]]; then
            local short_name="${dep#${PREFIX}::}"
            result+=" (or \${DESBORDANTE_PREFIX}::${short_name})"
        fi
        ((count++))
    done
    echo "$result"
}

find "$TARGET_DIR" -name CMakeLists.txt -print0 | while IFS= read -r -d '' cmake_file; do
    dir=$(dirname "$cmake_file")

    if ! grep -q 'target_link_libraries' "$cmake_file"; then
        continue
    fi

    src_files=()
    while IFS= read -r -d '' file; do
        src_files+=("$file")
    done < <(find "$dir" \( -name '*.cpp' -o -name '*.h' \) -print0 2>/dev/null || true)

    [ ${#src_files[@]} -eq 0 ] && continue

    echo "Checking $cmake_file"

    for pattern in "${!DEPS[@]}"; do
        alternatives="${DEPS[$pattern]}"
        if grep -l "$pattern" "${src_files[@]}" &>/dev/null; then
            if ! check_dep "$alternatives" "$cmake_file"; then
                formatted=$(format_alternatives "$alternatives")
                echo -e "${RED}WARNING${NC}: $cmake_file uses ${RED}$pattern${NC} but links none of: $formatted"
            fi
        fi
    done
    echo ""
done
