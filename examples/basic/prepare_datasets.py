import pandas as pd

# df = pd.read_csv('examples/datasets/ind_datasets/anime.csv')

# df_1 = df[["title", "type", "status", "source", "rating", "premiered_season",
#       "premiered_year", "genres", "themes", "title_english", "title_japanese"]]
# print(df_1)

# df_1.to_csv('examples/datasets/ind_datasets/anime_prep.csv', index=False, header=True)

# df = pd.read_csv('examples/datasets/ind_datasets/manga.csv')
# df_1 = df[["title", "type", "status", "genres", "themes", "title_english", "title_japanese"]]
# print(df_1)

# df_1.to_csv('examples/datasets/ind_datasets/manga_prep.csv', index=False, header=True)



df = pd.read_csv('examples/datasets/ind_datasets/names.csv')

df_1 = df[df.Year > 1990]
print(df_1)

df_1.to_csv('examples/datasets/ind_datasets/names_prep.csv', index=False, header=True)

df = pd.read_csv('examples/datasets/ind_datasets/states.csv')
df_1 = df[df.Year > 1990]
df_2 = df_1[["State", "Sex","Year","Name", "Count",  "Total"]]
print(df_2)

df_2.to_csv('examples/datasets/ind_datasets/states_prep.csv', index=False, header=True)


# df = pd.read_csv('examples/datasets/ind_datasets/names.csv')

# df_1 = df
# print(df_1)

# df_1.to_csv('examples/datasets/ind_datasets/names_prep.csv', index=False, header=True)

# df = pd.read_csv('examples/datasets/ind_datasets/states.csv')
# df_2 = df[["State", "Sex","Year","Name", "Count",  "Total"]]
# print(df_2)

# df_2.to_csv('examples/datasets/ind_datasets/states_prep.csv', index=False, header=True)


