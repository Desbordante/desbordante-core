#!/bin/bash


reset_fmt="\e[0m"
er_fmt="\e[1;31m"
s_fmt="\e[32m"

#!!! нужно установить себе: cpufreq; python3 с numpy, pandas, scipy, matplotlib и natsort

# ================================= ФИКСИРУЕМ ЧАСТОТУ ===========================================
# Смотрим минимальную-максимальную возможную частоту
max_freq=$(cpufreq-info | grep -m 1 'hardware limits' | awk '{print $6 $7}')
min_freq=$(cpufreq-info | grep -m 1 'hardware limits' | awk '{print $3 $4}')

# Говорим для всех ядер, что частота=макс.частоте
echo 'for ((i=0;i<$(nproc);i++)); do cpufreq-set -c $i -g performance -d' $max_freq '-u' $max_freq'; done' | sudo bash

# Проверяем, что всё установилось как надо
new_max_freq=$(cpufreq-info | grep -m 1 'hardware limits' | awk '{print $6 $7}')
new_min_freq=$(cpufreq-info | grep -m 1 'hardware limits' | awk '{print $3 $4}')
if [[ $new_max_freq = $max_freq ]] && [[ $new_min_freq = $min_freq ]]
then
  echo -e $s_fmt'SUCCESS: CPU frequency is set at' $max_freq $reset_fmt
else
  echo -e $er_fmt'FAILURE: FAILED TO SET CPU FREQUENCY' $reset_fmt
fi 



# ========================== ОПИСАНИЕ ДАТАСЕТОВ И ГИПЕРПАРАМЕТРЫ ==================================
declare -a datasets=(
  'iris' 'balance-scale' 'chess' 'abalone' 'nursery' 'breast-cancer-wisconsin'\
  'bridges' 'echocardiogram' 'adult' 'letter' 'ncvoter_1001r_19c' 'hepatitis'\
  'horse' 'fd-reduced-30' 'plista_1k' 'flight_1k' 'uniprot_1001r_223c'
)
declare -a separators=(
  ',' ',' ',' ',' ',' ',' \
  ',' ',' ';' ',' ',' ',' \
  ';' ',' ';' ';' ','
)
declare -a header_presence=(
  'false' 'false' 'false' 'false' 'false' 'false'\
  'false' 'false' 'false' 'false' 'true' 'false'\
  'false' 'true' 'false' 'true' 'true'
)

# Меняя массив индексов, можно отбирать датасеты, если на некоторых алгоритм
# считает слишком долго или выходит за пределы по памяти. Всего 17 датасетов. 
declare -a ds_indices=$(seq 0 16)

#!!! Стоит прогнать сначала на num_repetitions=1, чтобы понять картину в целом
# Возможно, на некоторых датасетах алгоритм явно не будет справляться, 
# и на них не надо делать 10 запусков
num_repetitions=1

#!!! этот параметр нужен, чтобы система не зависала из-за съевшего всю память С++, а отключала его.
# Нужно поставить сюда максимальную оперативную память в KB, которую может позволить ваша система. 
mem_lim_kb=8000000

analysis_dir=$(pwd)

# seed ни на что не влияет, если ваш алгоритм не рандомизированный 
# (если он вдруг рандомизированный, нужно написать об этом)
seed=10000



# ================================== ВЫЗЫВАЕМ DESBORDANTE =========================================
#!!! поменять путь
desbordante_path=/home/mstrutov/Study/_CW/Desbordante/build/target
d_expr_name=$(date +'%m.%d').Desbordante

#!!! Этот аргумент используется так: ./fdtester_run --algo pyro [...]. 
#!!! Нужно дописать src/main.cpp, чтобы подобным образом можно было из консоли вызвать ваш алгоритм
algo_name='pyro'

cd $desbordante_path
mkdir -p $d_expr_name/

for i in ${ds_indices[@]}	
do
echo ${datasets[i]}
echo -n "" > $d_expr_name/results_${datasets[i]}.txt
for iter in $(seq 1 $num_repetitions)
do
  #!!! Нужно, чтобы среди вывода вашего алгоритма нашлась только одна строка вида 
  # "ELAPSED TIME: <int>" -- время в мс.
  # Скрипт должен скушать любую строку, где будет TIME и одно целое число
  #ulimit ограничивает память для процесса
  if ! (ulimit -v $mem_lim_kb; ./fdtester_run --algo=$algo_name --data="${datasets[i]}".csv \
        --sep="${separators[i]}" --hasHeader="${header_presence[i]}" --seed=$seed | grep 'TIME' \
        >> $d_expr_name/results_${datasets[i]}.txt)
  then
    echo 'TIME: NaN' >> $d_expr_name/results_${datasets[i]}.txt;
  fi
done
done

# Складываем результаты замеров в папку с этим скриптом. Если сегодня скрипт уже был запущен, 
# и там есть папка с таким же названием, она переименуется.  
mv $analysis_dir/$d_expr_name $analysis_dir/$(date +'%m.%d.%H.%M.%S').Desbordante 2> /dev/null
mv $d_expr_name $analysis_dir/



# ===================================== ВЫЗЫВАЕМ METANOME =========================================
#!!! поменять путь на свой
metanome_path=/home/mstrutov/Study/_CW/SkeletoneFresh/MetanomeTestRunner/
m_expr_name=$(date +'%m.%d').Metanome

# !!! Чтобы можно было вызывать Скелетон с нашими датасетами, нужно его модифицировать.
# 1) Поменять MetanomeMock.writeResults, чтобы он складывал результаты в 
#       $metanome_path/target/io/measurements/results_adult.csv 
# 2) На выбор
#    2a) Добавить наши датасеты в Config.setDataset, тогда в командной строке можно будет задавать
#        только один аргумент -- название датасета, как тут.
#    2b) Сделать так, чтобы их вместе с параметрами (isHeader, separator) можно было задавать
#        в командной строке, как в Desbordante.
#
# Таким образом, ваш Скелетон должен обладать таким свойством:
# - из $metanome_path/target/ его можно вызвать вашей строчкой <execute_skeletone>,
#    передав туда либо просто название датасета ($dataset), как здесь, 
#    либо датасет со всеми параметрами, как при вызове Desbordante: 
#         ("${datasets[i]}" "${separators[i]}" "${header_presence[i]}")
# - результаты своей работы он складывает в файлы вида:
#    $metanome_path/target/io/measurements/results_adult.csv 
#    Причём названия датасетов такие же, как в массиве datasets (см. "ОПИСАНИЕ ДАТАСЕТОВ ..." )


cd $metanome_path
 
for i in ${ds_indices[@]}
do
echo ${datasets[i]}
for iter in $(seq 1 $num_repetitions)	
do
  # !!! Возможно, вы запускаете метаном по-другому -- нужно заменить тело цикла.	
  cd target	
  #java -cp MetanomeTestRunner-1.1-SNAPSHOT.jar:dependency/* \
  # de.uni_potsdam.hpi.metanome_test_runner.Main ${datasets[i]} 1> /dev/null
  cd ..
done
done

# Складываем результаты замеров в папку с этим скриптом. Если сегодня скрипт уже был запущен, 
# и там есть папка с таким же названием, она переименуется.  
mv $analysis_dir/$m_expr_name $analysis_dir/$(date +'%m.%d.%H.%M.%S').Metanome 2> /dev/null
mv target/io/measurements $analysis_dir/$m_expr_name



# =================== ВЫЗЫВАЕМ ПИТОНОВСКИЙ СКРИПТ (ТАБЛИЦА С ИНТЕРВАЛАМИИ) ========================

cd $analysis_dir
#!!! Поначалу скрипт может не сработать -- стоит убрать '2> /dev/null', чтобы увидеть ошибки
#Скрипту нужно дать названия папок с замерами для Метанома и Десборданте
python3 aggregate_results.py $m_expr_name $d_expr_name 2> /dev/null


# Возвращаем настройки ядер обратно. Скорее всего, это "powersave" на частотах из [min, max], но, 
# если у вас не так, можно сделать это правильно вручную или ребутнуть ПК
echo 'for ((i=0;i<$(nproc);i++)); do cpufreq-set -c $i -g powersave -d' $min_freq '-u' $max_freq'; done' | sudo bash

