import os
import pandas as pd
import numpy as np
from scipy import stats
import os
import re
import sys
from natsort import natsorted
from matplotlib import pyplot as plt
from math import isnan


def get_dataset_results(root_results_directory, keep_minimal_rows=True):
    results = {}
    raw_result_names = os.listdir(root_results_directory)
    for raw_result_name in filter(lambda x: x.find('results_') == 0, raw_result_names):
        dataset_results = []
        dataset_name = raw_result_name.split('_', maxsplit=1)[-1].split('.')[0]
        dataset_results_path = root_results_directory + raw_result_name

        with open(dataset_results_path, 'rt') as dataset_measures:
            measure = dataset_measures.readline()

            while measure:
                for word in measure.split(' '):
                    try:
                        dataset_results.append(float(word))
                    except ValueError:
                        if word == 'NaN':
                            dataset_results.append(word)
                        pass
                measure = dataset_measures.readline()

            results[dataset_name] = dataset_results

    if keep_minimal_rows:
        # truncate columns
        min_size = 1000
        for key, column in results.items():
            if len(column) < min_size:
                min_size = len(column)
        for key, column in results.items():
            if len(column) > min_size:
                results[key] = column[0:min_size]
    else:
        max_size = 0
        for key, column in results.items():
            max_size = max(max_size, len(column))
        for key, column in results.items():
            while len(results[key]) < max_size:
                results[key].append('NaN')
    res = pd.DataFrame(results)
    return res

      
java_path=sys.argv[1]
cpp_path=sys.argv[2]      

java_res = get_dataset_results(f'{java_path}/') 
java_res = java_res.reindex(natsorted(java_res.columns), axis=1)
print('\nRECEIVED THE FOLLOWING JAVA RESULTS:')
print(java_res)

cpp_res = get_dataset_results(f'{cpp_path}/')
cpp_res = cpp_res.reindex(natsorted(cpp_res.columns), axis=1)
print('\nRECEIVED THE FOLLOWING C++ RESULTS:')
print(cpp_res)

def mean_confidence_interval(data, confidence=0.95):
    a = np.array(data).astype(float)
    n = len(a)
    m, se = np.mean(a), stats.sem(a) # среднее и ошибка среднего
    h = se * stats.t.ppf((1 + confidence) / 2., n-1) # n-1 - параметр распределения, количество степеней свободы.
    # Чем оно меньше, тем шире края. Чем больше, тем ближе к нормальному.
    # распределение Стьюдента - как нормальное, только края шире. Используется, когда мало сэмплов. Функция квантилей
    return m, m-h, m+h

# Рисуем график
confidence = 0.95
plt.figure(figsize=(10,10))
marker_width=3
java_names=['iris', 'balance-scale', 'chess', 'abalone', 'nursery', 'breast-cancer-wisconsin',\
  'bridges', 'echocardiogram', 'adult', 'letter', 'ncvoter_1001r_19c', 'hepatitis',\
  'horse', 'fd-reduced-30', 'plista_1k', 'flight_1k', 'uniprot_1001r_223c']
cpp_names=java_names
pretty_names=['iris', 'alance-scale', 'chess', 'abalone', 'nursery', 'breast-cancer',\
  'bridges', 'echocardiogram', 'adult', 'letter', 'ncvoter', 'hepatitis',\
  'horse', 'fd-reduced-30', 'plista', 'flight', 'uniprot']
ax = plt.subplot(111)
for i, dataset in enumerate(java_names):
    m, m_low, m_up = mean_confidence_interval(java_res[dataset], confidence)
    if i == 0:
        label = 'Metanome (Java)'
    else:
        label = None
    dataset_pretty_name = pretty_names[i]
    ax.plot((dataset_pretty_name, dataset_pretty_name), (m_low, m_up), '_-', color='red',
            lw=marker_width, markersize=11, markeredgewidth=marker_width, label=label)
    #plt.plot(dataset, m_low, '_', s=3)

    m, m_low, m_up = mean_confidence_interval(cpp_res[cpp_names[i]], confidence)
    if i == 0:
        label = 'Desbordante (C++)'
    else:
        label = None
    ax.plot((dataset_pretty_name, dataset_pretty_name), (max(0, m_low), m_up), '_-', color='blue',
            lw=marker_width, markersize=11, markeredgewidth=marker_width, label=label)
plt.xticks(rotation=45)
plt.yscale('symlog', basey=10, subsy=range(2,11), linthreshy=1)
plt.ylim(0, 2000000)
ax.set_ylabel('Time, ms')
ax.legend()
#plt.yscale('log')
ax.grid(True, which='both', linestyle='--')
plt.savefig(f'comparison_colorful_highres.png', dpi=300)
#plt.show()
print('\nSAVED THE GRAPH TO "comparison_colorful_highres.png"')

# Переводим в таблицу

def dataframe_to_tex_table(some_dataframe):
    '''
    Prints tex representation, replaces 'plusminus' with \pm

    :param some_dataframe: columns and rows ought to be ordered
    '''
    for index, row in some_dataframe.iterrows():
        tex_row = str(index) + ' & ' + ' & '.join(map(str, list(row))) + r' \\'
        tex_row = re.sub(u"([\d\.\,]*)\u00b1([\d\.\,]*)", r'\1 $\\pm$ \2', tex_row)
        print(tex_row)

def mean_confidence_interval_str(data, confidence=0.95):
    m, low, up = mean_confidence_interval(data, confidence)
    if isnan(m):
        return "NaN"
    return str(int(m)) + u"\u00b1" + str(int(m - low))

java_res = java_res.aggregate(mean_confidence_interval_str).to_frame().T
cpp_res = cpp_res.aggregate(mean_confidence_interval_str).to_frame().T      
both_res = pd.concat([cpp_res, java_res], keys=['Desbordante', 'Metanome'])
both_res.to_csv('comparison_results.csv')
print('\nSAVED THE AGGREGATED CSV TO "comparison_results.csv"')

print('\nPRINTING THE AGGREGATED TABLE AS A TEX TABLE BODY:')
print(dataframe_to_tex_table(both_res))

