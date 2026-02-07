# -*- coding: utf-8 -*-
# snapshottest: v1 - https://goo.gl/zC4yUc
from __future__ import unicode_literals

from snapshottest import Snapshot


snapshots = Snapshot()

snapshots['test_example[advanced/afd_multiple_error_thresholds.py-None-afd_multiple_error_thresholds_output] afd_multiple_error_thresholds_output'] = '''[[1 2 3] -> 4, [0 2 3] -> 4, [0 1 3] -> 4, [0 1 2] -> 4]
[[3] -> 0, [3] -> 1, [3] -> 2, [3] -> 4, [2] -> 0, [2] -> 1, [2] -> 3, [2] -> 4, [1] -> 0, [1] -> 2, [1] -> 3, [1] -> 4, [0] -> 1, [0] -> 2, [0] -> 3, [0] -> 4]
[[3] -> 0, [3] -> 1, [3] -> 2, [3] -> 4, [2] -> 0, [2] -> 1, [2] -> 3, [2] -> 4, [1] -> 0, [1] -> 2, [1] -> 3, [1] -> 4, [0] -> 1, [0] -> 2, [0] -> 3, [0] -> 4]
[[4] -> 1, [4] -> 2, [4] -> 3, [3] -> 0, [3] -> 1, [3] -> 2, [3] -> 4, [2] -> 0, [2] -> 1, [2] -> 3, [2] -> 4, [1] -> 0, [1] -> 2, [1] -> 3, [1] -> 4, [0] -> 1, [0] -> 2, [0] -> 3, [0] -> 4]
'''

snapshots['test_example[advanced/aind_typos.py-None-aind_typos_output] aind_typos_output'] = '''This pipeline demonstrates the process of mining and verifying AINDs
(approximate inclusion dependencies). This pipeline can be used for data
cleaning by identifying typos in the datasets based on the identified AINDs.

It consists of the following steps:
1. Mine all possible AINDs from a set of tables.
2. Filter out exact INDs (which have zero error).
3. Verify the AINDs to identify clusters of data that violate the dependencies.
4. Display detailed information about the dependencies.

Let's find all AINDs with an error threshold less than 0.4.
The datasets under consideration for this scenario are  'orders', 'customers'
and 'products'.

Dataset 'orders':
+----+------+---------------+-----------+
|    |   id |   customer_id | product   |
|----+------+---------------+-----------|
|  0 |    1 |           101 | Laptop    |
|  1 |    2 |           102 | Phone     |
|  2 |    3 |           103 | Tablet    |
|  3 |    4 |           104 | Monitor   |
|  4 |    5 |           108 | Keyboard  |
|  5 |    6 |           201 | Mouse     |
|  6 |    7 |           102 | Charger   |
+----+------+---------------+-----------+

Dataset 'customers':
+----+------+---------+-----------+
|    |   id | name    | country   |
|----+------+---------+-----------|
|  0 |  101 | Alice   | USA       |
|  1 |  102 | Bob     | UK        |
|  2 |  103 | Charlie | Canada    |
|  3 |  104 | David   | Germany   |
|  4 |  105 | Eve     | France    |
+----+------+---------+-----------+

Dataset 'products':
+----+------+----------+-------------+
|    |   id | name     | category    |
|----+------+----------+-------------|
|  0 |    1 | Laptop   | Electronics |
|  1 |    2 | Phone    | Electronics |
|  2 |    3 | Tablet   | Electronics |
|  3 |    4 | Monitor  | Electronics |
|  4 |    5 | Keyboard | Accessories |
|  5 |    6 | Mouse    | Accessories |
|  6 |    7 | Charger  | Accessories |
+----+------+----------+-------------+

Here is the list of exact INDs:
(orders.csv, [id]) -> (products.csv, [id])
(orders.csv, [product]) -> (products.csv, [name])
(products.csv, [id]) -> (orders.csv, [id])
(products.csv, [name]) -> (orders.csv, [product])
(orders.csv, [id, product]) -> (products.csv, [id, name])
(products.csv, [id, name]) -> (orders.csv, [id, product])

Here is the list of AINDs:
AIND: (orders.csv, [customer_id]) -> (customers.csv, [id]) with error threshold = 0.333333
AIND: (customers.csv, [id]) -> (orders.csv, [customer_id]) with error threshold = 0.2

Let's see detailed information about AINDs:
AIND: (orders.csv, [customer_id]) -> (customers.csv, [id]) with error threshold = 0.333333
\x1b[1;41m AIND holds with error = 0.33 \x1b[0m
Number of clusters violating IND: 2
\x1b[1;46m #1 cluster: \x1b[1;49m\x1b[0m
5: 201
\x1b[1;46m #2 cluster: \x1b[1;49m\x1b[0m
4: 108

AIND: (customers.csv, [id]) -> (orders.csv, [customer_id]) with error threshold = 0.2
\x1b[1;41m AIND holds with error = 0.2 \x1b[0m
Number of clusters violating IND: 1
\x1b[1;46m #1 cluster: \x1b[1;49m\x1b[0m
4: 105

Based on the analysis of the AINDs and their errors, we can make the following
conclusions:

  First AIND: The AIND between `orders.customer_id` and `customers.id` holds
with an error threshold of 0.33. The clusters violating the inclusion dependency
indicate possible data issues.
- The `orders.customer_id` value '201' does not match any entry in the
`customers.id` column. This suggests that there might have been a typo where
'201' should have been '101', indicating that the customer who bought the
'Mouse' might actually be Alice.
- Similarly, the `orders.customer_id` value '108' also violates the AIND. This
suggests a missing customer entry in the `customers` table. The customer
corresponding to '108' may not exist in the dataset, pointing to a data
inconsistency.

  Second AIND: The AIND between `customers.id` and `orders.customer_id` with an
error threshold of 0.2 does not indicate any real typo. The issue here is that
this AIND counts customers who have not placed any orders, which is expected
behavior. This dependency does not reflect typos but instead reveals missing
orders for certain customers. Since it's not a typo, this AIND is not useful for
cleaning data.

It's important to take the error threshold into account when working with AINDs.
A higher threshold will reveal more potential errors, but it might also include
non-typo cases, such as customers who have not made any orders yet.
'''

snapshots['test_example[advanced/comparison_mining_fd_approximate.py-None-comparison_mining_fd_approximate_output] comparison_mining_fd_approximate_output'] = '''
=======================================================
This example demonstrates key characteristics of the
approximate functional dependency (FD) discovery
algorithms, AID-FD and EulerFD.
=======================================================


EulerFD is a randomized algorithm, and its results vary based on the seed value. For instance:
With a seed of 2704, EulerFD found 76 FDs.
With a seed of 1321, EulerFD found 78 FDs.
With a seed of 9208, EulerFD found 80 FDs.
An exact FD discovery algorithm, in contrast, consistently identified 78 FDs.

This highlights a key property of EulerFD: it may produce results with both
false positives (extra FDs) and false negatives (missing FDs)
compared to exact methods.


---------------------------------------------------------------------
Let's examine the differences between the results of the exact algorithm and EulerFD.

First, consider the results with a seed of 2704, where EulerFD identified 76 FDs.
Compared to the exact method, EulerFD failed to identify the following 4 FDs:
[1 2 3 5 7 9 11 13] -> 8
[1 2 3 5 7 9 13 14] -> 8
[1 2 4 5 7 9 11 13] -> 8
[1 2 4 5 7 9 13 14] -> 8
Additionally, it incorrectly identified these 2 false FDs:
[1 2 3 5 7 9 13] -> 8
[1 2 4 5 7 9 13] -> 8
Thus, a single run of EulerFD can both miss valid FDs and generate false FDs.

Next, let's analyze the results with a seed of 1321, where EulerFD identified 78 FDs
EulerFD not found 0 FDs.
EulerFD found 0 false FDs.
Therefore, with the seed 1321, EulerFD obtained the exact result.
'''

snapshots['test_example[advanced/comparison_pfd_vs_afd.py-None-comparison_pfd_vs_afd_output] comparison_pfd_vs_afd_output'] = '''pFDs \\ AFDs = OrderedSet(['[DeviceId] -> Data'])
AFDs \\ pFDs = OrderedSet()
AFDs ∩ pFDs = OrderedSet(['[Data] -> Id', '[Data] -> DeviceId', '[Id] -> DeviceId', '[Id] -> Data'])
1 - PerValue([DeviceId] -> Data) = 0.1714285714
e([DeviceId] -> Data) = 0.23076923076923078
In case of PerValue error measure, violations on data from the single "glitchy"
sensor device among many do not prevent dependecy from being found
'''

snapshots['test_example[advanced/comparison_ucc_and_aucc_1.py-None-comparison_ucc_and_aucc_1_output] comparison_ucc_and_aucc_1_output'] = '''\x1b[1m\x1b[36mThis example illustrates the difference between exact and approximate Unique
Column Combinations (UCC and AUCC). Intuitively, a UCC declares that some columns uniquely
identify every tuple in a table. An AUCC allows a certain degree of violation. For more
information on UCC, consult "A Hybrid Approach for Efficient Unique Column Combination Discovery"
by T. Papenbrock and F. Naumann. For more information on AUCC, consult "Efficient Discovery of
Approximate Dependencies" by S. Kruse and F. Naumann.
\x1b[0m
The following table contains records about employees:
\x1b[1m\x1b[36mName     Grade   Salary   Work_experience   
--------------------------------------------
Mark     7       1150     10                
Joyce    5       1100     5                 
Harry    3       1000     7                 
Grace    4       900      12                
Harry    6       1200     1                 
Samuel   1       950      9                 
Nancy    2       800      3                 
\x1b[0mWe'll try to find typos, using UCC mining and AUCC verifying algorithms.

Let's run UCC mining algorithm:
Found UCCs:
\t\x1b[1m\x1b[36m[Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

There is no UCC for \x1b[1m\x1b[36mName\x1b[0m column. Maybe it's an error?
Let's run AUCC verification algorithm for column \x1b[1m\x1b[36mName\x1b[0m:
\x1b[31mUCC does not hold\x1b[0m, but AUCC holds with threshold = 0.048
Threshold is small. It means that, possibly, there is an error in this column.
Let's look at the table again:
\x1b[1m\x1b[36mName     Grade   Salary   Work_experience   
--------------------------------------------
Mark     7       1150     10                
Joyce    5       1100     5                 
Harry    3       1000     7                 
Grace    4       900      12                
Harry    6       1200     1                 
Samuel   1       950      9                 
Nancy    2       800      3                 
\x1b[0m
There are two \x1b[1m\x1b[36mHarrys\x1b[0m. They have different work experience, so they are
two different employees. If they had unique names, AUCC would hold with threshold = 0, and
\x1b[1m\x1b[36mName\x1b[0m could be used as a key:
\x1b[1m\x1b[36mName          Grade   Salary   Work_experience   
-------------------------------------------------
Mark          7       1150     10                
Joyce         5       1100     5                 
Harry Brown   3       1000     7                 
Grace         4       900      12                
Harry Adams   6       1200     1                 
Samuel        1       950      9                 
Nancy         2       800      3                 
\x1b[0m
Let's run UCC mining algorithm:
Found UCCs:
\t\x1b[1m\x1b[36m[Name]\x1b[0m
\t\x1b[1m\x1b[36m[Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

Now we have cleaned the data and \x1b[1m\x1b[36mName\x1b[0m can now be used as a key.
'''

snapshots['test_example[advanced/comparison_ucc_and_aucc_2.py-None-comparison_ucc_and_aucc_2_output] comparison_ucc_and_aucc_2_output'] = '''\x1b[1m\x1b[36mThis example illustrates the difference between exact and approximate Unique
Column Combinations (UCC and AUCC). Intuitively, a UCC declares that some columns uniquely
identify every tuple in a table. An AUCC allows a certain degree of violation. For more
information on UCC, consult "A Hybrid Approach for Efficient Unique Column Combination Discovery"
by T. Papenbrock and F. Naumann. For more information on AUCC, consult "Efficient Discovery of
Approximate Dependencies" by S. Kruse and F. Naumann.
\x1b[0m
The following table contains records about employees:
\x1b[1m\x1b[36mFirst_name   Last_name   Grade   Salary   Work_experience   
------------------------------------------------------------
Mark         Harris      7       1000     12                
Joyce        Harris      6       1000     5                 
Harry        Roberts             1000     7                 
Grace                    4       900      13                
Harry        Walker      4       1000     4                 
Allen                    4       900      9                 
Nancy        Adams       2       1000     3                 
Grace        Weaver      6       1000     6                 
Maria        Clark               1400     10                
Dorothy      Weaver      2                25                
Nancy        Cruz        2       700      8                 
Betty        Howell              950      11                
Grace        Caroll      11      800      16                
Jesse        Mitchell    4       1000     1                 
Dorothy      Weaver      4                2                 
Melissa      Wright      2       1200     15                
Peter        Clark               1500     14                
Margaret     Cooper      6       800      19                
Allen                    4       1000     18                
\x1b[0mWe'll try to find typos, using UCC and AUCC mining algorithms.

Let's run AUCC mining algorithm with threshold = 0.013:
Found AUCCs:
\t\x1b[1m\x1b[36m[First_name Grade]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Last_name]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Last_name Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

Let's run UCC mining algorihm:
Found UCCs:
\t\x1b[1m\x1b[36m[First_name Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

Now let's find AUCCs that are not UCCs -- these columns may contain errors:
ACCs - UCCs =
\t\x1b[1m\x1b[36m[First_name Grade]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Last_name]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Last_name Grade Salary]\x1b[0m

\x1b[1m\x1b[36m[Last_name Grade Salary]\x1b[0m has cardinality of 3 -- it\'s "accidental" UCC.
\x1b[1m\x1b[36m[First_name Grade]\x1b[0m and \x1b[1m\x1b[36m[First_name Salary]\x1b[0m may not hold -- people with the
same first name may have same grade or salary. But \x1b[1m\x1b[36m[First_name Last_name]\x1b[0m must
hold -- even if two employees have same names, their records in our table should be uniquely
identifiable by \x1b[1m\x1b[36m[First_name Last_name]\x1b[0m pair. Let's look at the table again:
\x1b[1m\x1b[36mFirst_name   Last_name   Grade   Salary   Work_experience   
------------------------------------------------------------
Mark         Harris      7       1000     12                
Joyce        Harris      6       1000     5                 
Harry        Roberts             1000     7                 
Grace                    4       900      13                
Harry        Walker      4       1000     4                 
Allen                    4       900      9                 
Nancy        Adams       2       1000     3                 
Grace        Weaver      6       1000     6                 
Maria        Clark               1400     10                
Dorothy      Weaver      2                25                
Nancy        Cruz        2       700      8                 
Betty        Howell              950      11                
Grace        Caroll      11      800      16                
Jesse        Mitchell    4       1000     1                 
Dorothy      Weaver      4                2                 
Melissa      Wright      2       1200     15                
Peter        Clark               1500     14                
Margaret     Cooper      6       800      19                
Allen                    4       1000     18                
\x1b[0m
There are two \x1b[1m\x1b[36mAllens\x1b[0m without the last name and two \x1b[1m\x1b[36mDorothy
Weawer's\x1b[0m. All they have different experience, therefore all of them are different
employees. Thus, it is an oversight or typo in the table. Let's improve the quality of this data:
\x1b[1m\x1b[36mFirst_name   Last_name   Grade   Salary   Work_experience   
------------------------------------------------------------
Mark         Harris      7       1000     12                
Joyce        Harris      6       1000     5                 
Harry        Roberts             1000     7                 
Grace                    4       900      13                
Harry        Walker      4       1000     4                 
Allen        Kelley      4       900      9                 
Nancy        Adams       2       1000     3                 
Grace        Weaver      6       1000     6                 
Maria        Clark               1400     10                
Dorothy      Weaver_1    2                25                
Nancy        Cruz        2       700      8                 
Betty        Howell              950      11                
Grace        Caroll      11      800      16                
Jesse        Mitchell    4       1000     1                 
Dorothy      Weaver_2    4                2                 
Melissa      Wright      2       1200     15                
Peter        Clark               1500     14                
Margaret     Cooper      6       800      19                
Allen        Smith       4       1000     18                
\x1b[0m
Now UCC \x1b[1m\x1b[36m[First_name Last_name]\x1b[0m should hold. Let's run UCC mining algorithm again:
Found UCCs:
\t\x1b[1m\x1b[36m[First_name Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Last_name]\x1b[0m
\t\x1b[1m\x1b[36m[Last_name Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

UCC \x1b[1m\x1b[36m[First_name Last_name]\x1b[0m holds, and we have found and resolved two
inconsistencies in the data.
'''

snapshots['test_example[advanced/md_semantic_checks.py-None-md_semantic_checks_output] md_semantic_checks_output'] = '''In this example we find a meaningful MD and try to use it to enforce data integrity.
We are going to use a dataset of flights between cities.
 id Source             From               To  Distance (km)
  1    ac1 Saint-Petersburg         Helsinki            315
  2    ac2    St-Petersburg         Helsinki            301
  3    ac2           Moscow    St-Petersburg            650
  4    ac2           Moscow    St-Petersburg            638
  5    ac1           Moscow Saint-Petersburg            670
  6    ac1           Moscow    Yekaterinburg           1417
  7    ac2        Trondheim       Copenhagen            877
  8    ac1       Copenhagen        Trondheim            877
  9    ac2          Dobfany         Helsinki           1396
 10    ac2    St-Petersburg         Kostroma            659
 11    ac2    St-Petersburg           Moscow            650
 12    ac1           Varstu         Helsinki            315

Now we are going to define the comparisons:
1) IDs and sources are considered similar if they are equal.
2) Departure ("From" column) and arrival ("To" column) city names are going to be compared to themselves ("From" to "From", "To" to "To") and to each other ("To" to "From", "From" to "To") using the Jaccard metric.
3) Distances are going to be compared to each other using normalized difference: 1 - |dist1 - dist2| / max_distance.
max_distance=1417
Searching for MDs...
Found MDs:
[ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.977417
We find a matching dependency. We assume the dependency is meaningful, i.e. departure and arrival cities can give us a very accurate estimate of flight distance.

Let's test this assumption by adding another carrier's flights and checking whether the MD still holds.
 id Source             From               To  Distance (km)
  1    ac1 Saint-Petersburg         Helsinki            315
  2    ac2    St-Petersburg         Helsinki            301
  3    ac2           Moscow    St-Petersburg            650
  4    ac2           Moscow    St-Petersburg            638
  5    ac1           Moscow Saint-Petersburg            670
  6    ac1           Moscow    Yekaterinburg           1417
  7    ac2        Trondheim       Copenhagen            877
  8    ac1       Copenhagen        Trondheim            877
  9    ac2          Dobfany         Helsinki           1396
 10    ac2    St-Petersburg         Kostroma            659
 11    ac2    St-Petersburg           Moscow            650
 12    ac1           Varstu         Helsinki            315
 13    ac3           Moscow    St-Petersburg            650
 14    ac3           Moscow Saint-Petersburg            637
 15    ac3            Kansk        Salekhard           1914
 16    ac3            Kansk        Salekhard           1912
 17    ac3        Salekhard            Kansk           1913
 18    ac3        Salekhard            Kansk              0

Searching for MDs...
The algorithm threw an exception: Similarity must be in the [0.0, 1.0] range, but is -0.128440

Inspecting the measures, we find that max_distance (1417) is no longer the actual maximum distance. Let's update it.
max_distance=1914
Searching for MDs...
Found MDs:

However, we see that no dependencies were found.

By inspecting the dataset, we find that the row for the flight with ID 18 has an obviously erroneous distance value. Let's fix it.
 id Source             From               To  Distance (km)
  1    ac1 Saint-Petersburg         Helsinki            315
  2    ac2    St-Petersburg         Helsinki            301
  3    ac2           Moscow    St-Petersburg            650
  4    ac2           Moscow    St-Petersburg            638
  5    ac1           Moscow Saint-Petersburg            670
  6    ac1           Moscow    Yekaterinburg           1417
  7    ac2        Trondheim       Copenhagen            877
  8    ac1       Copenhagen        Trondheim            877
  9    ac2          Dobfany         Helsinki           1396
 10    ac2    St-Petersburg         Kostroma            659
 11    ac2    St-Petersburg           Moscow            650
 12    ac1           Varstu         Helsinki            315
 13    ac3           Moscow    St-Petersburg            650
 14    ac3           Moscow Saint-Petersburg            637
 15    ac3            Kansk        Salekhard           1914
 16    ac3            Kansk        Salekhard           1912
 17    ac3        Salekhard            Kansk           1913
 18    ac3        Salekhard            Kansk           1913
Searching for MDs...
Found MDs:
[ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.982759
The dependency is not exactly the same, as we are technically using a different similarity measure, but it means the same thing as before.
Let's add more data from another carrier.
 id Source             From               To  Distance (km)
  1    ac1 Saint-Petersburg         Helsinki            315
  2    ac2    St-Petersburg         Helsinki            301
  3    ac2           Moscow    St-Petersburg            650
  4    ac2           Moscow    St-Petersburg            638
  5    ac1           Moscow Saint-Petersburg            670
  6    ac1           Moscow    Yekaterinburg           1417
  7    ac2        Trondheim       Copenhagen            877
  8    ac1       Copenhagen        Trondheim            877
  9    ac2          Dobfany         Helsinki           1396
 10    ac2    St-Petersburg         Kostroma            659
 11    ac2    St-Petersburg           Moscow            650
 12    ac1           Varstu         Helsinki            315
 13    ac3           Moscow    St-Petersburg            650
 14    ac3           Moscow Saint-Petersburg            637
 15    ac3            Kansk        Salekhard           1914
 16    ac3            Kansk        Salekhard           1912
 17    ac3        Salekhard            Kansk           1913
 18    ac3        Salekhard            Kansk           1913
 19    ac4           Moscow Saint Petersburg           3642
 20    ac4 Saint Petersburg           Moscow           3642
 21    ac4           Moscow          Seattle            411

This time, we remember to update the maximum distance: max_distance=3642
Searching for MDs...
Found MDs:

The dependency is no longer present. We inspect the data and find that "Moscow" and "Saint Petersburg" in the new portion are US cities, unlike before. We can conclude that our initial assumption was incorrect, departure and arrival city names are not enough to determine flight distance accurately.
To resolve this, let us introduce a new attribute indicating the region the city is part of.
 id Source Region             From               To  Distance (km)
  1    ac1 non-US Saint-Petersburg         Helsinki            315
  2    ac2 non-US    St-Petersburg         Helsinki            301
  3    ac2 non-US           Moscow    St-Petersburg            650
  4    ac2 non-US           Moscow    St-Petersburg            638
  5    ac1 non-US           Moscow Saint-Petersburg            670
  6    ac1 non-US           Moscow    Yekaterinburg           1417
  7    ac2 non-US        Trondheim       Copenhagen            877
  8    ac1 non-US       Copenhagen        Trondheim            877
  9    ac2 non-US          Dobfany         Helsinki           1396
 10    ac2 non-US    St-Petersburg         Kostroma            659
 11    ac2 non-US    St-Petersburg           Moscow            650
 12    ac1 non-US           Varstu         Helsinki            315
 13    ac3 non-US           Moscow    St-Petersburg            650
 14    ac3 non-US           Moscow Saint-Petersburg            637
 15    ac3 non-US            Kansk        Salekhard           1914
 16    ac3 non-US            Kansk        Salekhard           1912
 17    ac3 non-US        Salekhard            Kansk           1913
 18    ac3 non-US        Salekhard            Kansk           1913
 19    ac4     US           Moscow Saint Petersburg           3642
 20    ac4     US Saint Petersburg           Moscow           3642
 21    ac4     US           Moscow          Seattle            411
Let us take the new column into account by adding a new column match:
4) Regions are considered similar if they are equal.
Searching for MDs...
Found MDs:
[ normalized_distance(Distance (km), Distance (km))>=0.990939 ] -> equality(Region, Region)>=1
[ equality(Source, Source)>=1 ] -> equality(Region, Region)>=1
[ jaccard(To, To)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.703185 ] -> equality(Region, Region)>=1
[ jaccard(From, From)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.940143 ] -> equality(Region, Region)>=1
[ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 | equality(Region, Region)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.990939
We have now fixed our data.
We can see that there are new dependencies in the result. They are not getting pruned by the support requirement like before, and they also are not meaningful. With larger data quantities they will disappear naturally. We may also try increasing the minimum support threshold, but unfortunately not in this particular case, as the amount of data is small.
'''

snapshots['test_example[basic/data_stats.py-None-data_stats_output] data_stats_output'] = '''Columns with null: []
Columns with all unique values: [0, 1]
Number of columns: 6

Column num: 0
Min: 0008f14d-e2a7-4582-bf5e-89ce32b55606
Max: fff1cd7a-04f9-486c-97de-d5d2c6ddb3cb
Distinct: 945
Min chars in a row: 36
Max chars in a row: 36
Min words in a row: 1
Max words in a row: 1
Char vocabulary: -0123456789abcdef
Word vocabulary: ['0008f14d-e2a7-4582-bf5e-89ce32b55606', '00159fe8-8096-4d0c-8543-5178d5dea8c3', '002a9b64-632b-4ba9-81fa-9dc11219a2fe', '007f9ec6-6947-4928-96f2-9834b5fc46e7', '00821b68-623b-4a17-b293-01f85bfd4fb8', '00a8e460-4a8f-4484-a5a5-0e6d6b8fbccd', '00f79ce0-93d4-48af-b137-3ab9471f6060', '0113bcfe-2059-4f57-b3f1-10c0a585b81e', '014e7c12-c230-4da7-8077-370c61c088da', '0153c8fd-8101-47d4-b679-a7ce6786323e', '016ed458-b167-4354-b1e6-d70b6274c2a1', '01761989-341a-4fea-8cbc-f7789885e36d', '01b5df08-2b72-49e9-bda8-025eaf141d99', '028caf4b-07b3-48a6-98e3-be79bfa927df', '03016fbd-0f89-4034-8dd6-2d00a92a9a81', '032df6e3-238b-4d5a-8de5-90dfb685a5a2', '037da080-4751-4060-9739-adf99b888352', '03a01935-7005-483a-9366-16e733c6c074', '03ab1a63-6b5c-4a67-ab06-6ab0669cbb05', '03c6f860-2a15-4490-a004-581192606ea2', '03e2a948-a4e1-46a4-959b-008665d25f9d', '044670b2-4ac3-4337-99c1-d29454d8e458', '045d099f-8a78-4974-8683-d91e3d911704', '05b4eeb8-8dc2-49c3-a2a6-eeb5e4b26c85', '05c19abe-01a6-4f8e-b63e-e82e0b0fb2b0', '05f06bae-116b-4a4f-af80-93c36b761c7d', '065fe05c-fafe-4d50-b4c3-264424aa928a', '073c7380-7370-467c-ad88-a1fed67c9867', '07c717e4-4666-487c-826d-2c987c35c8eb', '0820d912-9cf2-43ee-9607-a0a8c3d1b705', '085b5f0e-cfb2-4444-9508-ce6cc11f48b3', '085efd34-e8a6-4cb2-acc0-262b31211035', '08742a44-b955-44df-86af-f59f304d7c9a', '088be511-9d6b-4082-a4ef-34a61dd63368', '08c0e2f0-ec96-4fb2-a208-bda04c6f7ac0', '08ed5f60-da44-4780-8eb0-8cec0b7533b3', '08f03ac5-0eff-451a-9294-1cb7c36d811a', '095f51e1-2db7-41c6-855b-1833536ffff3', '099d8c84-8977-472c-8e8a-a114814fc519', '09e38501-3a4f-44c7-809d-a093479403d1', '09ecf006-f471-4124-bccc-475eb525ffb4', '0aa4b8fa-9fda-4a25-84d3-f304f69feab3', '0abb74a8-64fe-445a-993b-fed0a51e34f3', '0ac2fcd4-7257-4a49-b89d-a5ca45551aec', '0ae4c0e8-62be-45ee-b9f2-f3250a37a23e', '0b0c08a2-48c9-4206-9114-ef84c1dec2d0', '0bbcdda5-953e-4252-b50e-77cdcaae65ee', '0c0dd835-2173-4a87-ab62-8d42a4e361b8', '0c32e90f-8c7e-4857-9fee-53dec29ba3da', '0c66d46b-8cb2-48b1-8f91-4efda82662f6', '0c77d203-5188-4b63-b118-352876b70962', '0c804176-0f57-448a-9ad4-b591fd13b6cd', '0d41f1d0-96bf-4f5b-923c-23cbaef9d8b4', '0d446cd6-9566-447d-8b04-386838e94f8f', '0dedbfd1-00d0-42ad-8597-d491fbac28ce', '0e293e8d-d06e-4b1b-b708-7d50b3c71642', '0e345151-cbe5-4977-8334-eaf32b9ec0e2', '0e4faa69-6a0b-4519-bf75-d3668daeba34', '0ebabab8-bcd4-4957-928c-35b119f30c4e', '0ee284fb-3567-48c6-9ddd-2eb04e7e68d0', '0f13468d-bce8-491c-9ddd-a96ca512e2cf', '0f75ec35-0d39-4203-ba9c-d2ffae22c45c', '0f7fec7a-d4ab-484a-b8a7-cded77c9f7b9', '1074be11-3bdf-4338-82f7-2ba0db524882', '10b68abe-b3ac-4870-84d2-6a6f16be806a', '11d4851d-1b65-41a2-b450-10ac0c0a468f', '125e65ca-ebc5-4828-98f2-71becc7acc80', '126e45df-ad9d-418f-800c-f6fcaddac182', '128c0251-92b2-4727-99cc-8f447b351e56', '128c123b-3f44-4397-aebf-40ff29fa1da7', '1311df4b-8ef5-4370-819f-e3964a4d87cb', '13533449-9dc0-4404-aa6c-afa498f126bf', '13833b68-bf58-4e90-99d6-d69fd37352b3', '13abb0de-cccc-472d-80a8-0793b1f4862b', '13c9c0e8-9507-468d-84b6-ea83c311d32a', '13fd94b5-0b39-4f13-a88d-7a330fbcaf50', '14dfc707-626f-4abc-89ed-5d0a9cc6b6e8', '14f84304-5d1a-4d94-aa01-879e403bf3ba', '1504b2a8-728b-49d6-9c49-a43ea76d850d', '1510d8a7-afc3-4f7d-9663-7e61dcf78cb0', '15e72315-b301-480a-bc55-f098c9952d84', '161d97ec-5583-467a-8f3d-db6c8d6e34d5', '16249374-895d-433a-a069-715266b37a73', '165bf2b1-96bb-4e99-b67c-ccb1aa51faa6', '16b828fd-0b1c-45e3-ba71-105810aff995', '16f51b90-593b-4689-9b37-4d88a02b6573', '17886c45-2583-49f0-aa93-5637e6fdcd44', '17ee3ee9-113b-48e0-86af-bbd2e35ab101', '1812f96f-6a86-432d-af9a-84ee72dae94d', '18409110-5fe1-48d0-9e69-7478ac2b9481', '1857d174-2eb4-4199-9e36-33cc3e1114ef', '18a303dd-e90a-4ea8-af3e-51730dcbe197', '18a59941-003e-4805-b79c-35942f30f50d', '18e10b4f-3519-4b10-a3bd-60fdb935d124', '19199e19-51bc-4f52-a1d8-a4045833e7ca', '193d2218-70c2-4c48-bca6-ba0810456583', '193e2857-7f05-47a7-a025-ef43bcb1cd63', '196180b3-fab3-4316-8a24-fb5ea6d325b7', '1967133a-f7e0-48af-af3d-5d7b8b602e9d', '19754bd9-672f-4871-afcf-4f0b3f5834f0', '1a5ea38b-875d-4f4c-be94-6d221ef43f72', '1ab87efe-ba39-4a34-a7de-201635fa5e74', '1afda36b-fe6a-4d51-b434-53a4b05003c6', '1b209320-5cb5-4cbb-9450-b74aef58a281', '1b5b0a21-0746-40f2-b036-68d6d4a0a289', '1b9c58fe-3658-4405-bd48-09d5481c3484', '1bae5096-e9d3-4a0a-9e18-88c52c7b1b39', '1bc7dabe-ae7f-462e-8bd6-459a307d6e89', '1c1cc127-ebb4-4546-96ea-f45cfa748e25', '1c5f5794-0ffd-47ed-9b4a-87f2dbbcbef9', '1c7d237d-95ba-4d3c-a15c-1ebeaf204468', '1ddbc0bf-2b2e-465a-99c4-c94e19863ba0', '1de76301-ce88-4ebf-a23f-d0a9fcce7824', '1e46f858-8511-49b5-abd4-f3613ad3cdfc', '1e583f43-8aa4-4101-a1e5-51864e6f23c9', '1e8c1337-e8ec-4ead-8a33-58127556676a', '1f3af54c-b4c6-4f48-a9ec-54150b510fe3', '1f82e0b7-e943-48b8-ba8e-580815f2f19f', '207c8811-8624-4614-9409-19f07db4310d', '20e07b37-5f5f-4d23-b317-5b97e6f7cf94', '20e58dfe-69c3-4e6e-84d0-aa1e177bc287', '20f841f9-d748-468f-8aca-9a6532c331df', '2101c9a8-2d05-49db-80bf-9e54b9ad90b5', '21486f9a-3643-4a3d-9f36-0144cb9ab7b9', '21f3e22f-16f5-4594-b68b-d1db3b2a0259', '222eb774-ef97-43a5-98af-2d49a7cdea83', '2237841e-5958-4e67-ade4-b37781fd8b19', '223fd0ae-2dc0-49a3-98bd-fce551b92f90', '23badeaa-8856-4654-8176-d549ea59de49', '25377c10-40ff-447d-a8ba-55526fb3c79e', '25479aec-56a0-458f-b93a-5719d61f4be7', '25582349-d84d-444b-9a45-2f0606911f75', '25811e80-8d48-4b58-bcb3-bb9638522278', '25db61bd-3709-4efa-bd3e-54ce2926c4b1', '265964d5-dec8-4994-9cbc-cea3185995e0', '267e9a94-a23b-417c-9df7-b39943716b5c', '26869d04-314e-486e-9e17-5f5b225a6d95', '27f9479e-a861-49e1-8be0-f4da636de0e9', '280fd3d9-8500-4356-a223-97606ce419ea', '281cebba-0e35-4c7c-b47d-b3094d87a4e5', '2858a242-25f3-4a38-8436-f72c184696d8', '28895a1b-d1b8-47c1-8471-a9404628de03', '2914164d-c5e4-4e3b-b425-eb76b20049bd', '297f41cf-269c-40ab-8f41-db7146d289f0', '298e4482-2697-4d80-87bb-53b3c4953910', '29e67879-6df6-40d9-9c5a-a23afe151401', '2a2866d5-ac88-4344-8926-5bc21f735d26', '2a34ba21-a9da-40ba-894b-1f00d900dfad', '2a939284-bffb-4f07-9bc6-e6c36820c2d1', '2aa13f27-04ad-4761-8f69-1eeae9396266', '2b5e5abb-0605-4506-a5f0-a95f2c8a2224', '2b6c70ce-6214-47ca-a9f9-e0e00374a806', '2b826ffc-6093-4613-a1e3-f8beed844d85', '2bc42cca-f7e7-453f-89e0-4379072c3185', '2c24a597-1ff4-4bfe-bb80-6f1da960afae', '2cd97590-9c01-494e-b61d-ae99841e5411', '2d5c46c5-3fab-4f38-b354-0a904af5b49d', '2d7adf86-f15f-4d25-ae79-2c96d6916f10', '2da85c36-4b63-4cd3-b386-f8080c989ebd', '2dac81a8-e0f4-4f74-92b7-69ca9ca8183f', '2dbfaef9-da6d-4c68-89ac-a87de4716844', '2e4cea49-b33c-4f03-93e8-9e529c7ab325', '2e866d2b-7521-4567-aadc-a4eabbae8b11', '2ecc95ff-acf5-448a-85c3-340178032dfb', '2ee86e0e-9b17-4264-809f-b7ba044e78e3', '2f1ff4d9-90f6-403f-bb12-11f4e210d470', '2f31c583-78d7-4fb7-a9c1-59004c574fa9', '2f8154dc-9cde-4217-99f3-021df03a6e58', '2fb0c46f-ad51-4c76-b8fd-deb97cd8c020', '2fb1cfa5-015b-420a-878a-988402c800fa', '2fcc22ef-1600-4b0c-9c98-eda0abe0a91d', '2fcd42f9-f153-4630-b1aa-5e2992f2ded3', '3076422a-62d8-4709-a86e-2a60540155b6', '31976814-c591-479e-9d2b-5b19044b21a5', '3241fb48-5a15-4638-bd68-d915834a3f89', '3251695e-324d-442a-8da5-be76dd1871f6', '3258fbd5-f685-4a0a-bf94-f766673e4ab5', '326c1320-59d8-4d81-b294-41d75b1885e7', '327555f4-6553-4888-a65b-e21cba95ca3a', '3280d32b-9e8e-4aca-bd42-b1a8ec2b38a7', '32d7e7d7-0977-4fab-8b56-3a798198cb29', '3341be57-2db5-4bb1-9e87-8f5a673f31f3', '33d31efa-c97a-4b84-a9ba-84267546fd32', '33f383c7-a7af-4192-ab4b-ab7d7d2611ac', '3474d755-951a-47a4-a527-4b6cf21c86c1', '35251018-04cf-46f6-a009-51e99b623f55', '35cd07c5-477f-435b-9956-2e072c849232', '35e9b225-75a6-4859-83a5-415dbe66f2bd', '360265e7-4b31-43ba-9e16-10551b9157b8', '3602b9cb-02f4-487e-bba0-5bf0a3b7053c', '36b75543-4d23-4c2c-8f26-ae6f8e6920d8', '3719596c-014e-4e8a-9265-405eec7ffbf7', '37284e9f-4639-431f-be48-2978bd6e8e0c', '373b9dbd-9554-4ea0-987b-1770c21da213', '3781ea7e-49d5-4162-90e1-aa0f4c171341', '37941e5d-445e-4b54-b1c2-f566ae581479', '37b83bc6-e70a-457c-b1d5-dadc13a80e4b', '37dd0f40-651c-4f81-8401-ea7e8e2c69df', '37f959ef-d210-4320-a33a-c1ab68668d8f', '381c3719-67d3-4e40-91b6-cf9382883e19', '381f0031-8bb9-4149-bd0b-cd574a759b53', '385048d5-3146-4b2e-b4dd-54a90d06d283', '394c47a0-10f3-46b6-9945-3c6cf468524f', '394f20dd-4985-4f37-b678-105842ca3187', '3a173306-7439-4c3a-ad7c-0d047792ef8a', '3a5768c4-d43e-41e8-8920-3b16f223fb71', '3a916100-9dba-4599-b8a3-b43f2817d70a', '3aee048e-a159-4c8e-9c1c-8625fe4f36df', '3b180976-652f-40a4-bd22-47886e015863', '3ba02ae9-d8c9-4407-9d66-3699da1f4bca', '3bcc963f-0591-47ac-8462-0f5bbc5372ea', '3bd7b8d8-c9b8-4dc1-bc4c-1a5f391f0cbd', '3c0ec4d3-3355-42d0-9c0c-feb7e4eb104f', '3c5fa72c-5a26-4481-93f4-0e638b983a5c', '3c6ce22c-ec07-4519-9d17-f7bbdf3965e2', '3ca05963-02c0-411f-bc47-a3a7fb32ab2d', '3ca80f6d-fed1-4c93-ba79-a986ab2256d4', '3ce377fb-69e9-42f7-b617-288d5f4b2e8e', '3d8a8b30-5d11-444c-87fe-e6407cd24905', '3db5e647-d9fd-4aa2-9ef0-8ef20be48b69', '3e42cd08-2369-4356-9d4c-52f5887a5144', '3e63ed79-ca1f-4742-9010-6a93e2fd4748', '3e8a2a4a-d436-432f-872b-d173b8127956', '3f4fcd1d-ea7b-4e88-a39c-ae8315633120', '3fe9ca4b-c6cd-4341-ad5e-c4ddf06a9c0a', '404f50cb-caf0-4974-97f9-9463434537e1', '40bbc0c1-439b-4f65-b6b3-1ca921957da7', '40ee1cbb-3caf-492a-8945-0400cd3bf07c', '41047100-76e1-41b2-8a0b-3b991c32438f', '417ad5bb-b287-418c-85fe-9d4163140939', '41aa23a5-4347-4ba0-885d-9e9b73374436', '423ad7c5-d450-4e3e-aa19-e393b80ba314', '42452d7c-b3f9-4320-946c-dc1ae17d5797', '42843088-88e7-4344-967d-084b5213f535', '428b5e55-cabe-426e-97cd-5cd0f3085c30', '42d7c91d-344e-423d-bd71-27755fe7b479', '4307ef5b-2e00-4316-b04c-debff4edc5c4', '4317e684-7ee5-4195-96dd-0acf808d2650', '43619956-9a55-4019-b5f9-b682041aeee1', '437b8934-1f4a-493c-bd1c-4a61275f60b5', '439a25aa-2485-4158-8465-14a52c093859', '43be1d98-1949-45ef-937a-5e0d924b16ad', '43c5b1a2-f1a1-44cc-afde-3d6aa79bd7ec', '43f94400-a8cb-4301-96f0-9e005bade403', '44755882-375e-4679-9ea4-e1e04147a66c', '44d6a60c-f957-4b60-987c-5211d8b401cd', '44faf9d9-3016-453d-aa8d-6f84e54dc657', '457f4bcc-adfd-4d08-85f8-feb47e76acb6', '45b4adf6-716f-4754-a066-fdecf366a502', '45b9238e-28ad-4948-ac17-d0942befa6fb', '45f45423-f310-4b9f-875f-d8a9a50fbdb4', '45fa2087-8973-48be-a43e-a2d652954397', '45fbd2df-1c55-4844-b78c-4c9af778db7c', '46580b8e-7f53-4828-a172-03d36772d033', '46812df7-f32e-428c-ac81-36e89651aeff', '4693bfb2-1450-441b-9787-9572bbd0cc7c', '46dd5053-27ea-4de0-b54b-baf160062d66', '473e79a7-deae-444c-929c-215303efcbba', '47bd0177-07f2-4091-9532-0ea86302248d', '47cf4897-3e5f-47af-bfe4-0b417b1a3d0e', '47d0475c-58c8-4ded-8fad-fd2fffbbd4fe', '47feb9d6-eb1c-48ba-94c1-0f61051149e6', '483506ea-1f81-45a1-92f1-aa447748e4e0', '494aa8b2-134e-4c46-a6b0-db37a94dc9ea', '49898949-f165-4d5c-87ee-c976840bda73', '4a5660d4-6f54-400e-b66d-0e4938fba4a5', '4a663cf0-0d1b-473a-8e96-362bee0d1869', '4abd8305-72f3-47a6-8d1c-3d32417a83ab', '4acff0ac-afdd-49a4-b00e-0c7ed070a1d1', '4b0212e1-86a1-4c02-a2e7-bbec631d9cb8', '4b1903d5-03cc-4c5b-9d70-d9b9187d3bc9', '4b443ae8-e7da-410d-9155-aec618d4d0dc', '4c637202-c630-4778-a6cc-c3dcce252e7d', '4c69ee82-5f33-40f4-b033-83a5e31b1401', '4c82c709-8e84-41ef-8948-bde26c36fc52', '4d02fc0f-5658-4e07-96d6-2c0dd0e88ec1', '4d1ffd40-81b5-4bf7-a639-7582c90c45ba', '4d24ebeb-fff7-46dd-88b8-24e613b098c3', '4d325eea-75b4-44d6-a513-d62d483b64ac', '4edb7b6b-e638-4022-9751-82392632c71b', '4f8daf0b-eb78-41d4-a337-89f7c8019b02', '4fbf362a-e843-4ff6-8b50-35c42ec16ba1', '4fbf6e5c-fff4-4ae0-996f-2e9b1bc0e70f', '4feb25b8-f232-4a5d-be82-dc83df88984f', '50007f9c-ca97-4b01-8e66-8c8e9be92810', '5000c5bf-4a57-4566-845b-720117ae3b3f', '5001e433-480b-42db-8f08-ef651dc3b112', '501737b7-7f34-4b63-bafe-dd942ad719ae', '505bd31a-f500-47c6-8c12-2b1d8013fe9d', '50a005af-83bc-40e6-ba39-ba88072a6a95', '50bf3c4f-af7a-4541-bdcd-fac3515a1367', '51280243-7d1f-4a94-bf9c-f8a62c029097', '51305055-859c-422f-975e-633a0e3d76be', '5146d141-fb0a-4e93-bebb-a7eb1a4a77ef', '51572fd5-a41e-4603-92d8-1b07772ecaee', '5194675f-bd34-4055-a629-d62914f2720f', '519fe18e-4c35-4ad8-b760-2eb52f6a75db', '521fe44f-c8b0-4fe6-badc-7fac186ef462', '5223ef9f-4b1d-4037-bb94-7f03258d15da', '5229a390-42de-43c8-baf0-e12481510b3b', '52531459-8773-4570-8007-0e92b22b8ff2', '529125e6-b02f-4f4c-8088-3f756fc5d8ee', '52b3ef9a-17e4-4129-bd1f-f8b7432d3dde', '52ecb32e-329c-4070-acd4-87b7398474b4', '53f53353-c1eb-40c4-94e5-7f4990800ea9', '542ae4bb-2f4d-4df3-b69d-1725700d4b38', '5440ff5e-be73-4552-9c3f-dcc21a219ace', '5458b826-70fc-4273-8783-79b78b8acae4', '547ec03a-ee76-4ffb-ba44-e8e252203bea', '54e3c2c4-8deb-4345-886a-74e613a22706', '54efff13-1e35-4bb1-ae63-685bf3dd1749', '552fd26e-81ce-4262-a72d-7ee4616b0f57', '55711c4c-439c-4573-aeaa-485f7e310036', '55dcaa86-24bd-4b60-876f-32eb22dd0871', '5637b123-0167-4330-a2c8-432282108ab1', '56424b8d-b1d9-412a-a140-2026670ce868', '566e0b89-0644-401e-81c6-d9b823220032', '56bb8c97-3d25-4e06-acce-1234c48cc600', '56ca707b-f2d0-4eb2-b9ce-48ff788fcbf4', '571aed91-8786-45cc-8f24-b2453d3b3c54', '57c2a4ed-68dc-46f3-90ce-104b29cef9d0', '589ff0d4-ffa8-49fb-8e47-daef08426e2f', '58ceea3a-215f-445f-8b66-f3ce513ba4fc', '59277ff6-9999-4d9c-88b4-169e162e6bf0', '592f3087-5f9f-4c37-ac1c-15bd12af50d6', '59625572-d898-4d33-9f30-59962ee91ac3', '5a1b35c1-cbf8-4dbb-ab68-454f1492034e', '5a257acc-e6c3-4349-868d-76592870ad2b', '5a5c6dce-1e71-4500-bfa0-bd009b3585f3', '5a8d6e84-2e33-4671-913f-db60442e3230', '5b33b7a7-9f48-4478-9977-266384468534', '5b3d2aac-b012-4ccb-802c-d98b976478c5', '5b804362-b02b-4dd1-b00b-399a2b57f79a', '5c12bdef-8b6d-44df-bfb3-5d2dc67b7ea8', '5c5cd703-bc93-487b-a0b2-51ff249a088c', '5c82bca6-bb1c-4366-bcfd-8d9a8a0e4d75', '5cf2dfc4-7d06-4ab8-9693-e1ffb1a66b13', '5d383eb7-074e-4d3a-9ee0-a8c95491673b', '5d47c4b7-0d32-4dc0-9b3a-3d7dbcc63060', '5d86469b-c4f3-4263-8705-a084e322c977', '5dd1b634-8554-48e0-a17b-9418e65baa5c', '5e6328f3-46cf-47f2-8da4-197f9a4ee94a', '5e7d557e-e540-458c-8b56-27f2c8f86032', '5e83f4a4-31cb-4a34-b77e-1ba415b33618', '5f501db4-a0a9-4f28-a4cd-3af0e52d18d6', '5f7ea480-da5a-49d3-8903-19ae7a08f8d0', '5fbe05d5-5437-47aa-b252-ce67fa4fe1a8', '5fd358e3-1f8a-4d31-8bab-2f10aa871158', '5fe46e07-fb94-42b6-b82c-06d38e24bd7b', '60426e0d-a6b0-4396-87ca-7ca0f9b3fe9a', '60f7add9-6a4f-4a88-ba79-e61a35ee6c92', '611e2aed-26fd-4507-ab8b-e4d351c9769e', '61793be9-26c3-435f-944d-e8a46bc6a3c0', '6189abc2-acb0-4944-8d8e-6bcf1176e819', '61a12989-d0a7-4d7e-993a-ff9027e03ea0', '61cdc966-442a-42d8-920f-d65bbe92a39c', '622fcfde-2afb-4ccd-844f-6ee8a7add8ff', '628dc353-d18e-4fb2-85ea-6a3c211c3636', '62c12ebb-f042-4c14-b6f9-5d000c649d02', '63b0b2cb-3b3d-4d72-8079-c37ba64a5d9a', '64524c1d-4d19-4e32-8317-644716f356b6', '648a9117-ea3f-4bb7-8b82-4355d44918c3', '64aa0fb6-0fc9-48a3-8b4a-ace799d3e505', '653354c8-5aff-4b22-a820-bff95e55f43c', '65bf78a6-0921-4914-8d5f-0667d88c6546', '65ce3056-b97a-439e-8b1e-838b5ba3b011', '6678395c-3a47-433f-88f8-4d4af7f85cbb', '66b8f545-1ba3-4b7a-abcf-4408f605bcd7', '66ded7bd-27d0-4b7a-8c3f-208d7f7693c1', '66fa6a8f-e02a-42af-8877-90cd70baa9ad', '67197599-5720-420a-9bbb-2ee31ea1d633', '672ec2d1-f67b-4332-b96a-3c13c56c7f66', '675fbd35-1e1e-403c-a9aa-a1a80d95648c', '677d4ee6-33ae-48a5-9ac0-c9580dcc8c97', '67b79946-089c-44a6-bd9e-0753e61a9d87', '67c05364-c95c-4838-9010-9a7bd50436fd', '6836b2c5-497a-4267-8d7f-cd412c7a628d', '692444dd-0008-4468-996e-c204ad7441fc', '692b8705-0ce8-4271-b8ea-9831d9b9ab7c', '6935604a-26ec-4875-ad9d-0bbc62d40a80', '69884f73-0675-447b-a4ea-3788bf8b08e6', '698e5cd0-1b80-4b4a-ac03-3c910b2f7c4f', '6a73b602-66f0-4b1c-b5b3-638ce4c2a84d', '6aaded6b-6b81-4648-8188-74b5c1146df7', '6af15cf0-f1c5-41e7-8584-5bee8c76715e', '6b8e5635-4c1d-4b78-bd63-f75875618c11', '6bee8436-a90d-4db7-ae4a-6f7d50239dd2', '6bf8b99d-2929-4557-9ca3-da7395e87657', '6cdcd405-a9e9-4a33-9426-40e04e6a3eab', '6cdef104-a721-4e75-8391-2bf18a48e17d', '6cf0577c-be9c-44e0-8e50-23fc762876bd', '6cfa3642-cccb-47fb-acc6-01ae94009e35', '6d3755d2-a69c-4398-8994-8b1f889c3d3f', '6d6064dd-3692-4868-b70e-77bcb9a77547', '6d809997-c082-4c95-a87a-8784bdd37020', '6e1f2be2-c080-4319-8749-c83f2d1f4b5c', '6e908980-9708-4051-82e6-e9e653a05229', '6ec729a9-b4f5-4353-ac69-559cd05fc681', '6f259484-217a-4f4c-9832-8d694a1ae50b', '6f784a97-5c17-4536-9861-f3e55eedbebb', '6f925299-482e-4f26-b253-42da536cee54', '6fabecc8-cd18-4793-a502-5f60221ff157', '6ff49f17-3b98-41e0-ba03-1e57445e708c', '703e57bf-5a07-4ed6-979a-e89772a058ee', '705d57e7-8b8a-4191-a807-f8b041b8be9b', '706b22c1-b31a-4012-b0a0-a365bc245fef', '707ac32e-c326-4aef-aade-188cf6a19af7', '70b40775-febc-4b84-b7df-a1fdc01432c8', '70d8cad1-643a-417b-9611-a1d9252a7113', '713771fe-0d2b-49e9-8926-e2672a00c951', '7148bd97-65c4-4f7e-b193-971c9d564f28', '718bfb16-eca2-4b67-a0d0-3f629dab0aef', '7224ba22-7ce0-4a2d-ab55-414618cb9276', '724926c0-c435-4aee-aca9-5637e771be3c', '725b3825-59dd-4721-a4a1-d0ad4c5183b8', '7263b5cc-df19-4c01-a977-048493d20cb2', '72c1a495-64ac-4a98-b7b8-4f55a2182b34', '72ed68e4-1afe-4095-8413-42dd010cf7c2', '73997cb6-cc14-4bcd-9180-609347a4cdb0', '73f1ecd7-c360-41a1-875d-4d3b91b9db33', '74354b19-c453-444e-aaba-a65b05bef2a2', '75c194f9-bc61-42eb-a826-f65d87c28c06', '75dbc920-2644-43b2-995e-7ea9a5c6c660', '76190e97-8325-4fd8-9860-ab34ae5eada0', '765f09a7-16de-477d-8635-47e61a182308', '76889757-e32f-4bd3-b3e2-a9f0b1571da6', '76adf18a-6521-42cb-bbd0-6c27ecb8fb60', '76e1c8b1-8d30-449c-b00d-52973c497a43', '77308f04-d3ad-4125-a481-90ab9d68a3a0', '77549280-7b0f-4f88-97ad-6de24c72ac89', '77aa30b8-69d8-4d2f-84c1-9283cdf1be3f', '77ce2c49-5bcc-4a24-bd24-eee5911d7460', '77d20628-747e-4256-b133-53c86d411105', '77de0829-5680-441f-8807-483960b4794d', '785d9160-cedd-476c-bc38-6d91beac35b2', '7866d55e-e021-467a-8f81-0debc209d948', '78bcdf76-aad6-4f93-8b2d-b4ce26039a30', '794dad93-33b8-4084-926e-2ae129081a3d', '7956a76d-2dce-4d6e-9727-1056ac4b2c7a', '7995352d-fcc9-4ab2-9a8a-7ec0a130b4a1', '7a0594e9-37f6-4f39-8474-8326464f838b', '7a087f9b-f59d-4022-ba36-c36cebb76405', '7a4b59d8-f6b3-43ec-9be4-7156c442761d', '7a54ec1e-5c91-4aa7-b192-da84581fafd5', '7b7a7556-c786-4b9e-a71f-20f5aab1c6c9', '7b8bd8cc-07f0-4f26-99b2-2bba1afe7298', '7ba3907f-ddd1-4699-80b5-5287f71a7a18', '7bd88205-e6c2-4228-ae9e-b08943f28721', '7c0d045d-49c5-42da-b8a4-45e9ceeb3ddc', '7cbd8a25-13b4-40f0-b2db-135877db07ea', '7cee5d45-dc87-4651-92c4-4865ef853d28', '7cf84f2e-e331-4dd9-a1b1-3238f2803d24', '7d2e67c6-75ed-4006-812c-98ae10ac28a5', '7d390ddb-8a17-498d-aa02-e2cc3ee8ce39', '7d584956-36c7-48da-882e-062e4e1cff00', '7d86295a-7561-4a3b-8a92-41bf97930050', '7d91840c-8c7c-4a54-9015-1cd9cc7853ea', '7dbaedb6-b9f8-4ebe-84b0-1d2f24d96732', '7dc70fde-445c-4371-bb3c-ba3713f6f5f7', '7def07ab-eed5-4a4b-87db-ed0ee4dcfab3', '7e1a8d8d-6823-4497-b083-fba9034dcf85', '7e3423fd-1762-467e-89db-605ee0450ddd', '7f252a9b-2c33-4af5-ac1c-fef03393c457', '7f84a9e6-f042-4d0b-857a-e03983c87568', '7fa0db4f-63e1-47ec-9876-f14883b2f358', '80138017-099d-4eec-a405-12e950873146', '802b1604-a5cb-4b94-8319-66f83f661e7d', '805c75a3-71dd-4ba8-ae98-b62dbba74147', '80b9c0bc-f346-4265-8298-2ec9b00626e1', '80eae375-699a-42c1-90b4-72cf9588af3b', '80f7ccf0-e9c8-408d-a2c1-7aa5a9eee8dc', '81854905-289c-4915-b0fb-6ffc4e54d6e5', '81aabb56-808c-48a1-b2a3-5d3f2e1a752f', '81cbd975-0463-4f48-8089-6829d99dd767', '820a35a3-2e81-450d-a371-b7a56df4339e', '8244074a-a400-42a9-990a-6f6588bab5bc', '8248ea4a-9d96-4275-b227-2d794c0b73bf', '82805689-421c-4162-8bb1-39afb50c26e2', '82c35a2a-470f-4057-a2dd-4f348da7b12b', '82c5e605-672b-4bc6-879e-27181a0dce01', '82fdd218-af5d-415e-bb77-427967294700', '8362b216-3ebb-411a-9c3f-2b1088618c04', '83836633-dc5e-4644-b8f1-4389c397743a', '83e30ce8-8780-438d-94dc-42c53d0fc33c', '840164a7-888c-44be-b0e8-8e6a80ef5277', '8437e23f-e686-4658-9a9b-cc07fd094215', '84407b28-10cc-4be3-8919-dd949bd9c2e4', '84495364-0a83-4af1-903c-42024d84f15d', '8469b97b-de31-4aa7-a64f-dd9a5ef914e1', '849048be-dd9a-41de-9201-96d145f5100d', '849668bc-2d58-40ac-81df-e20a632d220c', '84e24115-f28d-4cae-a5aa-66060d0a9773', '85202ad4-780b-4968-ada2-68a4bde95715', '8539063c-f8f6-49a8-a901-7be651c4c051', '85ac9555-ea2e-40a4-a577-8691f1ba579c', '85cd43d9-730f-4f75-9341-f8498250f6aa', '8697de07-a2f3-4483-aff4-fc40556dab14', '8735ef1a-60dc-4e53-a183-c3e25e26d748', '88604992-7d3b-4b85-b30f-9ddb62437efb', '88aab2a3-4ae6-4311-986a-d7f89dfde3aa', '891f225c-f115-4406-ae0e-97bf4be7fe0e', '898e08da-b20c-441e-bda4-559e9e6e55b2', '89926a8a-6f9d-4713-90ad-8369b694188f', '89c16e59-1fee-4acf-9479-4d3af6082215', '89d02434-5572-4706-88c8-5d693f7ab0de', '8a84bfbc-6fc5-4b54-a21a-14bd79c0c206', '8abcbbb4-cbad-4cb4-97c1-f1d2395c6fd7', '8abdd143-2882-49c2-b566-7a5c0011c21c', '8b243779-4b4b-4d60-a0c3-7a67ae4a2514', '8b7dc1e8-ccfc-4c90-a625-ad529d679678', '8bef2a8b-dc80-4878-b642-daf07c3cc596', '8dc44539-befc-4a2c-9665-359d29d8b247', '8e291cc3-4b08-46c0-91ec-9ee2864368ac', '8e3379a6-d41e-4453-9200-f216b0ac40bf', '8e569980-0965-4d35-8acb-68cbd53d6b53', '8ef9aae9-6894-477d-8c68-18cd4260ff14', '8f1bb4b0-5bfb-4d4d-a952-819fbb47053e', '8f276eda-37f5-4afa-8dfb-94e271f8f9c2', '8f297830-0975-4aee-9cd7-8c26e299fe6e', '905f47dc-ee40-4b03-8ea4-c2ff4f37fd3e', '90966c7c-03e2-4ed7-b73d-2e615fc0d524', '909ad757-2d6e-4304-8267-480b97f1e5e9', '9121448e-6037-4b60-9000-96ba87add539', '912a9897-345e-4eca-bcd2-bfc23ae82d48', '91636c30-fe95-451f-9e98-b2027af8dcbb', '91e6dd30-3715-4773-980a-b7ab308a5826', '9233dd25-9415-45f1-8e0f-a0c9e9482cc0', '92465605-39fc-4edf-988f-367db23777ec', '92598b7f-b22d-4bed-939b-1f9248c625d4', '92ae3e6e-4205-4e0d-8725-c6562c009676', '92bee364-064e-491c-8955-d4fbb32cec65', '92fa4070-c25d-43da-a9b8-486e2c2b6ac6', '9356e26b-d813-43d3-b6f5-96fecbed235d', '93abc0a8-14c8-472a-8995-b3f2647f9141', '94421389-4934-4e24-8cb4-b2aeaef3ba8b', '9495db0f-caea-45e7-90c8-df1b53380f32', '94a7d67a-2a83-4ac1-b5e8-dbe62246e376', '94b58c76-c3e4-431f-880b-295615ef5836', '95489e6a-24c7-41d8-b9a7-e6d151df9960', '95ccacd8-3ead-486a-a05e-5c654d446cf0', '964713ec-8493-49e4-aba3-1ceb7d7799da', '97100f94-4e80-438d-a2fe-335e3bd91d4e', '9710ddce-1b20-4c18-8d8b-65480ee1aa9c', '972b299d-2f27-4d6d-81d2-8effbc543bf1', '97a85bfc-4e39-4aa9-b44e-f77ca1b34904', '97ec6296-b3f9-4a70-a0b6-52667f95fb1e', '97ed3d53-d75d-4653-ba2d-10aea1fb7c29', '9825e272-a885-4b43-bda0-f208f6beb3c7', '98949c99-ca1c-47a3-b0cd-d266d891608d', '98e3153f-bdcd-437a-a500-ca2b31766936', '98f01633-2856-4ef8-b7f1-43e00dd987e5', '9909cac1-f5fe-43c7-907f-6bfc1b23b354', '998b155d-eaf0-485f-929f-47cdb89c66a4', '998daffd-fac4-4cf0-9257-ec94f1d15e7b', '9993446b-c329-47d6-b8db-7c7b20d82b04', '99af42e6-ad99-4172-bbf1-d23cfa294b31', '99c744f1-4bfe-4a9d-9a33-3763f101be0e', '99ef7d94-0aa3-4170-926d-f5d41aaaec15', '9a22ff55-eb4a-4e33-ab8e-505049b2a11c', '9a443f28-9c31-42e8-b482-7d719a3a46e0', '9a9f5326-120f-473f-a75d-aa769e6f70e9', '9adb4f6d-933f-47bf-833b-84ecac92e1d5', '9adc24ed-08fb-4464-b210-0096e07318e7', '9b4b4851-fac2-429d-a8b6-21f98a3fe530', '9b5d9226-fdd3-4ac8-a9ee-97284852da2d', '9cbb9026-f157-4a01-aace-a42b05ab2a28', '9cd700bc-b3d9-439d-afe9-945c2a20bc37', '9cd764b2-b424-41da-9ae8-0f54733cc4bc', '9d31ff9c-9901-455c-a889-bcc5cae2e0ae', '9d46a2e5-06ff-44a6-94dd-7fdb8ddcfe56', '9d540164-4922-4395-a487-0ae911a2a99f', '9d5d8c3d-ecaf-4e5f-a2e9-945e60a658c3', '9da6a5b9-0587-40f2-be8e-b48e22b111ff', '9e174978-e08d-44b6-9c36-4b42c431c344', '9e1abdb3-fdd7-44c4-bf66-8c1c5dc74d13', '9e1ba418-626c-42c5-a635-a100aaa8963d', '9e4027a5-ab54-4ad1-9c3d-5495cf9a9c49', '9e80bb25-ab60-495d-a513-4fdd90982aea', '9e853c93-be4d-4b94-9055-677d9e84079e', '9e8a1e7f-d70c-497a-8038-e9747b8fe03d', '9ebbf42e-8904-43a3-b81d-70c30693d0f2', '9f7c8f12-8065-4b0e-891c-2e44f20efe9d', '9f92456c-785b-4829-be4b-8e53bac569bb', '9fbec56f-7a1f-4b93-af9f-93b1fdcf058c', 'a002c516-d0c0-4515-b829-f28f917758fc', 'a02c0422-43ae-4e37-9e02-def9607f6e98', 'a0a085a7-4aad-4a42-993f-bf27409a78a3', 'a0daf90f-56f5-44e6-afb1-9c69325acc4f', 'a148a59b-5e36-44b1-addf-a145425bdc04', 'a222162b-38fd-4971-8d2b-d82520c211d9', 'a29def61-fa7d-47fb-9cab-3df38c29f392', 'a3742d83-c69b-4615-95fe-0f32430e6ea4', 'a37b73cc-655a-471e-be57-80f99613bcaf', 'a39cfab0-a1ff-43d2-91d3-f908f14e8f4d', 'a4679f52-66f1-4ea1-a9e1-cd1f677edcfe', 'a518f1be-3366-4c21-8c4b-802f0ca2b4aa', 'a5ad44f3-de62-4c1a-9da1-8ca88a7e578e', 'a5df168b-38f5-40fa-8d6d-9a579f09b1e3', 'a5ec9089-e0f9-4050-a9e2-cb3ab4defbc9', 'a680cb43-af30-4265-8e97-70d821e0c068', 'a695aa11-2332-4d7d-b5c4-95661b4c6d30', 'a698d6e1-de24-46b2-9c25-8af5dd0688eb', 'a6dce09c-42e1-469a-81bb-bbe6219db2ba', 'a72c14b9-2d99-423c-9b18-d2d97c95c0c8', 'a7537b6b-f519-4b06-8f59-e29f36d39f47', 'a7f705f2-cf35-4db2-b2f2-98cad319319a', 'a7fe0115-a339-4815-b9e1-a4c76314bdf8', 'a822aec1-6b37-4341-9613-3e3c8b4d1b7e', 'a8acd7e8-52fe-417a-be47-d3fa8ae5a0cc', 'a8ef286b-064d-4e05-96f1-2785ace58464', 'a8f2a715-2c5b-410e-a8dd-789d8110b18f', 'a920e399-0148-4e03-a4e2-c7d1abdced64', 'a92d1e17-def1-4014-bf2d-47a418f63cfe', 'a9ea9ce7-a6ec-4802-a2eb-56be4611793a', 'aa5868e5-67b6-46fc-9f3a-788743ec6d4f', 'aac8bf7a-ea51-4f34-8999-3a5514ae92ee', 'aadaac52-265f-419f-b8ae-a920dc61e093', 'ab471cf4-1d57-41ca-9484-29cd5d50d79f', 'ab84a173-1977-434a-b7bd-3d764b2b0329', 'abffcd9d-c00b-43b7-adbb-1df71739566c', 'ac6378b3-c6f7-4491-988d-878c7c2aaa57', 'acc683dc-a43b-476c-ac46-121a55cb8a9a', 'accfb454-cd35-4572-a55f-b9643a8f881f', 'ae1c02ed-4e82-4b4d-9045-32443f17de3a', 'ae604e24-e040-4d50-b685-5b4897ab9ae9', 'ae64fd41-f6d3-4516-b4f3-ef8e07f72364', 'af099f7e-2677-4407-8432-d1a743a5b6ab', 'af1dce3f-8e35-4d19-bdab-c6d236341501', 'af627cac-9771-41fb-9038-505b9bfbdb28', 'af8c41a9-595b-4106-8ebb-eb167bec79ee', 'aff14a45-49e8-410d-93d0-c6e64dcf6a30', 'b0179192-8c6b-4348-90c8-488b0061fd49', 'b01d6bf4-e1a9-4144-a173-d278099099bf', 'b03d1a11-ae7e-481d-baec-469fffc7426b', 'b059d434-1e38-4814-ae11-4810743cbdd4', 'b0620668-aa6f-41e6-a0ff-0c12486babaa', 'b06b550d-e9bf-4b69-9d19-e3a3effbfb74', 'b0eb242d-c70b-426d-9217-423bb7a4be2a', 'b1284a39-98bf-4466-ba7a-5bb28a2e262c', 'b1329517-aacc-4990-b18c-15854edba8c4', 'b18f9249-1b15-4bee-8d6b-6b9befa48545', 'b2bca0a1-6181-4161-82e4-a667387130c9', 'b4112b77-fdf4-4bdf-ad9b-865dd0b98f39', 'b416718b-6b39-4749-9381-f52a096c9810', 'b41b2293-c1ee-473e-9f95-021a8619358e', 'b4ae8d97-58e3-466c-882b-d0d0fbe410bb', 'b4eaf97b-4644-4da6-b128-5ac072f429c6', 'b4efe218-9faa-4130-a804-43725f520e58', 'b5620ab1-0749-4859-8b1a-21bfb2d670f6', 'b5a10621-8d97-496f-9e70-b3dc29f8e5e4', 'b5e38281-9c09-49bf-91f5-c55397df4d43', 'b609c371-0fd1-4df0-ba2f-ceb851977199', 'b62e8b4e-cb26-41c1-bcb6-6f91f4280dda', 'b714e3b8-cf5e-4090-9f3a-d1598a2c113d', 'b738e2ae-84ff-4b0f-8109-5d449db0776c', 'b73d1c3e-a093-4bc4-8c1d-df5a36e09015', 'b78f1876-8e12-4039-a1fe-d56ae3da7aa8', 'b799d36a-deff-49dc-8910-06f848ae8cc3', 'b7a8f99f-5efd-4d1a-9442-f96f0fd5204a', 'b80a8ddb-550d-4ddb-b5d9-666f12970e01', 'b80ba4a9-c89e-4c03-9b34-71fce4f58e4b', 'b8af37fb-bd3f-4c76-91b2-53919a4f42b1', 'b8f96a47-0d9f-490b-ba38-daaaeb361464', 'b9319988-2d97-4e89-9ecb-0fb781659b5e', 'b9f8e05f-b093-42f4-8c80-2b34617a4517', 'ba0792cf-50fc-473c-8e79-3c5220dc3629', 'bb5824a6-4eff-4557-98d4-d968d8b8c059', 'bb70cb5b-2218-49e3-98ac-cfc29949b60e', 'bba7213c-ea4a-4f5d-a95a-1b960bca3980', 'bbd820ff-a950-4672-b24b-1563f345c89d', 'bbdeb9be-cf56-4dcc-a825-4aa2348208aa', 'bc25c02b-1705-411d-b2ba-ca3ab99d53f7', 'bc80ecec-821c-4fd5-b99d-d2620aa01627', 'bcb51ec7-7259-4e89-8fdb-ad3c73a65e04', 'bce0bbfe-e494-453a-b651-b7cdce97c369', 'bd605df7-d756-4542-a4f2-db23e3fe41d8', 'bd88ecae-b966-4648-886e-ab8e6f93f853', 'bdbe9209-b5f6-4be6-a274-6958e044f5d3', 'bdf2ccfa-bc47-4732-8dd4-f64374cfabd0', 'be402a5d-be88-438d-8fc0-cfdfbb478cbb', 'bee1ec0f-07b4-491a-a531-03425f0f457a', 'bf13a11d-2d28-4eaf-9a3b-00ef519856eb', 'bf3997f0-3d83-48b7-94dd-6cf61a3431b0', 'bfb4d757-099c-4df2-a813-f5b006443a2f', 'bff9b071-3c09-4f91-9d24-dd2f65eef6b7', 'c02a024d-03fa-4ac8-883f-92c26914e9be', 'c03e1cef-0de3-4483-9773-adb022418d07', 'c0609f67-899f-4f19-ad65-e03bfccce92c', 'c0f20509-42f8-4c51-ac16-4cad3d0c6de0', 'c1366def-97ac-458c-8326-1491bee3eba1', 'c14c474d-eca1-4210-b5b0-6b6fcaffc41d', 'c19f2fd9-0f7e-46c3-a235-f6b63aab26b5', 'c21c9029-c2b5-40ef-811e-fd49b7685dec', 'c2436cc8-f70e-4c7f-b4a8-8f843b47147d', 'c28c0fd3-0fae-4cec-ac0c-e628d9aa502e', 'c2cf1e09-c001-4dbc-8463-c04edec09617', 'c371e6a0-60d4-4a1d-b847-f346ba9eb6b0', 'c388f65e-e3f9-4277-b414-d26ba72bdd6b', 'c5182b84-6fb3-41ae-8342-d80e6099bcb9', 'c519c587-1408-4a10-936e-cea64c37f226', 'c5281f44-74db-4572-bb1a-cafb19553421', 'c5434781-ea5b-4a62-8d8d-38965584ce02', 'c548ce6d-e8fc-4926-8af6-78aeaf00f26e', 'c6079afe-8c37-45b4-a7a9-f61d04f7485f', 'c60f1b39-70b8-4152-a5cb-68a39e976646', 'c68c534b-5247-414f-b748-86d1ad2e9c3d', 'c6c4b01a-1981-47c4-a597-0989ee6e3e91', 'c6e23752-8a4e-42f7-b2e5-1a55b6440665', 'c8539dda-ec0e-4c67-a2f4-2d201bb82171', 'c86d3bac-0925-46fe-9d35-2b9c6f730afa', 'c8798bf1-34b0-4f0d-8f35-d564df690901', 'c889e6a2-7d6d-4e4c-ab9d-927b2aa5a8b9', 'c8a2a7ea-0399-44ac-b99c-7b9738a45332', 'c8b707ed-144d-474f-bdf9-6c414b919453', 'c90a0087-1d6a-4cb2-ac93-ecfd40aad755', 'c9439414-99f8-43cf-9600-7eb3aec77f77', 'c9468a6f-e051-4ef6-8646-12d89d3d848c', 'c95acaa2-cdf3-4fee-9a1e-b2667120fcf5', 'c95f61c3-1116-4adc-b4c9-d30949f4032b', 'c9a236e1-bf00-499e-930c-d34b962882e9', 'c9b2426b-b8cc-40b0-89f9-607a278dbc6c', 'c9eff560-53be-482d-ac54-e7466048ce8a', 'ca6d395f-c044-429c-93bb-28a2013af3ec', 'cab24500-c154-4399-b022-e9dce80bdf96', 'cb1b88ab-9b3b-437c-91ce-233615b5da5a', 'cb456798-0a3a-42c0-870c-605b30193ed3', 'cb53f88b-8d86-4483-95af-2bda9b32cfe1', 'cb8b24df-7123-470c-8d15-d1d4efd6db05', 'cbaf8ad7-10d2-405f-b4f0-9e7b97577913', 'cc14b585-a755-4df8-aa8a-1fcd17194ff7', 'cc199ff4-453a-4ae5-9fbd-b45d72fa952a', 'cc1f451d-1829-492a-ac6e-3de1a4a2ba83', 'cc812b95-7a81-47e5-9955-8b8d05f96194', 'cceba6c4-7f38-4b0f-a49e-13cd2c9739d5', 'cd01ef1c-0d2f-498f-8a0d-3db770431c40', 'cd452d88-9214-4630-b042-ffa0c5e8ba1a', 'cdde4b21-4287-422c-b394-9ea7224a5d94', 'ce4dc8b0-6e5b-42d9-b114-f4bf1eb74b20', 'ce60c8ce-f288-4f5e-8e0a-e959c08a9757', 'ce6b2157-9260-4a4d-8315-73b1235e3f89', 'ce6e2a51-3b21-4657-9292-68a60d13010c', 'ceadd542-b4e2-453b-ba14-836aaff9e9cd', 'cf22abba-7abf-4465-abd5-641117261194', 'cfa096ee-c59b-4a49-8113-9be2add63662', 'cfb1099e-f026-4f68-8f20-639754b03219', 'cfea68fe-b9e2-483c-91be-951c6f16131c', 'cff34eb0-c22c-4f96-a6bb-bf1cc666f340', 'd058e9bd-a3a5-4f0d-9c49-98ffd962fb29', 'd1ac7852-8fb5-43dd-bc62-62f10cf9486a', 'd1dec701-14bd-45a3-a18d-f4f13f5fa845', 'd1f8294d-07e9-45d0-92e0-9f1056ecddc5', 'd201147b-9da6-4ea5-b3d4-1b1b817128a4', 'd22ceb8b-bfcd-46f3-aa22-0f60abf23fdd', 'd2ea6657-3d42-40a8-901c-ad0ba105b742', 'd314bd15-76f4-4076-9d4f-6b24e2217585', 'd3646b5e-532c-4cdd-99c6-b15ffe4001d3', 'd36627bc-55d0-4301-99fb-fc44c285737f', 'd36dabb3-8983-4b72-96fe-057027d660c2', 'd4236d5b-c61b-4bd0-8af1-425083b120a5', 'd48a1d69-913f-464e-9e36-5ad688a3d554', 'd5006ec1-e6a7-47f1-b3a9-980a9b60dac3', 'd516712d-ec19-422c-9753-3fed995aa3b9', 'd532b454-5eb0-49fb-aa12-500d43e4e3e0', 'd53a49c0-6060-4627-bd7e-84858479ca4d', 'd547b304-e91d-4fc7-8f4c-dcef7f9d21ef', 'd584858e-57b3-4610-adcb-5b60fe6d10ef', 'd584b5db-6620-4630-8627-90fef80298fc', 'd5b5d471-827d-45a7-adc1-4878e5368ffc', 'd5cb954a-e942-47ae-9b62-b57f7a84c2db', 'd5e01b2d-461f-4680-b98a-24b5d3479bbc', 'd6067ceb-0175-49fa-a58d-1b66feb6d95a', 'd60f2c2a-1e63-474b-a8bb-ad7fd6fe87d7', 'd623c40a-c8de-4711-a9a6-7b4000999a10', 'd6611912-538f-469f-86eb-79baa0d385c4', 'd6859a49-01bd-45ce-9dd1-a87e1d032297', 'd6ca191a-633f-43b9-bda9-2bf100a1a232', 'd6e7cea9-05fd-46cd-97e6-a92e712b15f2', 'd74f5551-2abc-47e3-a198-ecbbe3109252', 'd773c9ee-707f-4f73-89aa-679772023c95', 'd7886744-aa60-4f49-a1e1-677d978c702f', 'd7ec02ae-7ad1-4314-ac7d-f41ba7e70fab', 'd808702e-beef-4472-a411-e758cc531516', 'd84b8bb1-c1f2-4ab2-9f4a-f42b8f647494', 'd86552cc-8a58-464b-966f-2b4c5448551e', 'd8e2dc0f-77af-4c84-af4c-1569809dc286', 'd8ed2e83-6a2f-48a7-9a47-43027ad1b9c5', 'd9404d57-ca44-4e72-9b83-f0c6bef36748', 'd9dd789c-4208-49a5-a188-ed7247c309aa', 'da32937d-3b85-47a1-9933-263dae4e61f2', 'db36e39a-ef87-4282-bcbc-766da9450abf', 'dbbdf319-2c93-4852-b703-50d609d004aa', 'dbfe75b5-2a67-4db7-bebc-d29f1c53e266', 'dc61538c-5880-4b68-95e4-9459ef8c0917', 'dc9638f3-5a60-4d5c-8711-226777dd6ad7', 'dc98372f-e85a-4ca1-ab7a-ddf42fa738a4', 'dc9c3856-8476-4d83-813f-4271c0fc4b91', 'dcdbd63c-ae64-49a1-bf52-d9ec99976e34', 'dcdeac6e-8983-4478-ab61-1e11cdeffb54', 'dd47bc3a-e945-497f-932d-3ba247272973', 'dd94c714-58f2-43e9-a9bf-cd0542c081e5', 'ddba9118-ec89-472d-9f3f-bebd919f0e3a', 'de0cdc87-750f-4228-bbd3-55bd3bbba648', 'de101035-8ee8-4501-85ca-aad650f88863', 'de1b567a-794f-406c-b4bc-87af4a7e6c9f', 'de583f37-4ce7-4585-b7de-17124143ea38', 'de650347-880a-42a2-88c9-4329f26fb912', 'de98af01-2eab-43ae-a995-08006cbab943', 'dead739b-8e20-4247-aa96-175b24103880', 'dec52c3a-cc7a-402b-b5d4-adcd6911b449', 'ded88e43-665e-440d-a19a-655573e8958e', 'dee6632d-90fb-42a1-b46a-ce0f391a2fe0', 'df2db897-c095-4afc-9a94-01d9ca49ff78', 'dfb9c0d2-b32f-4a16-aed9-fab72da59da7', 'e0344a34-45f3-4a62-bfea-0778bc24ab5f', 'e044d47a-6c7c-45e8-ac1d-0a6226da22cf', 'e04f8238-8408-4747-a1aa-f2f1984fc150', 'e06a72dc-5f86-466f-9eb7-3f9998787c8a', 'e15d48e8-e469-4b67-94ce-85971bd819fd', 'e162682c-2a64-49d8-a312-1f4ab28b7d61', 'e1ae9a50-1b2e-400f-a6fb-becf0c47c6c8', 'e1aeb4ab-96eb-4d85-9e8a-fc688d8e9da0', 'e1ecba55-4d92-4f5a-8176-7900f31c541c', 'e1f79174-0c99-4cd9-830c-8460b6d66ebc', 'e2592170-3a44-45af-9d41-779ea7cf61ce', 'e27578b1-b368-4a83-b632-96bc9097363c', 'e36437d1-91b9-4aa4-9887-5aaa0a777c94', 'e371110d-324d-41a1-8130-457f48b51d56', 'e3f87bb4-eef8-409f-b6b1-ccf3db91dd3e', 'e474092f-747b-4fee-bfab-79e4b6ab9136', 'e4c0cd94-dc63-4d26-afa4-ccca71aa3d01', 'e4c7d608-2eea-4b8d-a128-46f94d7f66eb', 'e4e4ca12-0b6e-4cec-ae21-93231179fde1', 'e51a82ff-50f3-4240-9c60-ca2d977d0444', 'e51d4a77-3197-423b-8267-e72c09d1879d', 'e56bfd3b-04fc-4db1-9585-76aaae7a7892', 'e576658a-fd59-413e-b034-4533b9cec92a', 'e5aa3e97-bbe0-4cec-b785-84d02d9fde95', 'e5dde9a8-57a1-4c64-a6aa-9577222ba348', 'e63026b0-55e7-4412-928d-2bd5ddaecc3a', 'e65edf39-e4a1-4c00-88cf-d604fbb515d0', 'e6a48b3f-bcde-4525-aa62-52d6715bc130', 'e6ada325-fa19-42ac-be38-a8f50360e8ea', 'e711149b-023a-4f7c-bac8-c227ad2524fb', 'e776c703-b419-45d3-ac9b-abcf565e582e', 'e7b7269c-c70d-4c6b-89ed-6c274f7d7863', 'e7faff4e-4104-4254-9ac3-f780e8c3f6f4', 'e813f1d1-0bb0-4a08-b702-e87fdbe2d3ba', 'e853687a-feea-4d7c-a9b4-5ab67024bb4e', 'e855fb12-9e3c-4f97-8a03-1cd917dcafa1', 'e88219f1-3388-41f3-b464-7e409c73b9ed', 'e88d6f0a-8cc0-472c-bd63-012e2367f9ef', 'e8c3558a-f0ec-4f59-a43d-e648cee7bb7d', 'e8d40eb4-de94-47ab-af9e-5bd0b41d2f25', 'e8db0d66-8134-4d07-a7e9-9a09db2a9697', 'e8f55a0b-001d-48d8-9a0a-743c4dbaad62', 'e8f6ddfb-bb39-41ad-b57e-a8139f8e833e', 'e903bcef-64a3-4884-a3c9-acf97af61ef6', 'e9344168-eab6-43eb-bfad-a808a8ad4351', 'e96f1d0e-1f95-4d14-9c44-bceb0e2f5717', 'ea007d91-2a3b-4f24-a107-f840df150182', 'ea9fdfaa-01e3-494a-b46b-f9cb420ccfc1', 'eabe5ebb-271f-4bbb-9c08-c519223f3625', 'eae79096-196e-4b93-bed3-99c4ed863bcf', 'eb36f88a-9b05-4777-a94e-bb46fbab57d3', 'eb46a582-8aa0-4bb3-9479-2e1aebe5bdb5', 'eb5959e5-092e-4ef1-a3c1-750a2c865ce2', 'eb67d6da-d8b2-447f-8350-eb20b0538025', 'eb846b35-3948-486c-8004-1783b0ad210d', 'ebb54ee3-1f6a-487a-b939-59bbc295a38f', 'ec355aa0-ddff-4e6f-9cae-ddc39a43d640', 'ec43b944-39c4-4e16-aee9-0d6b7f31ea8f', 'ec561de1-f404-4b57-8d84-125a3fff543c', 'ec575d09-5f47-41af-9604-107f779836da', 'ec985879-01b7-4fd9-ac3e-48d9e2b5711a', 'ecc5341c-46bf-447c-8183-bf0cd555f8b2', 'ece66e08-48d4-47ed-a792-0b95133dab44', 'ecf78d61-918e-4489-b238-93e117b737a4', 'ecf8b88f-684f-4657-8b6d-868d73dd816d', 'ed0c4192-3049-4853-88c5-a488f5feb269', 'ed367e2a-2de7-4a9d-9f20-cc2798b9481c', 'eda2fab8-86c0-44e1-acb3-ba74a8eb11ab', 'edbecd59-494f-437d-920c-a72886dc787a', 'edf08cca-2303-470a-8925-dc229586fb5f', 'ee2002fb-b9b8-4fcb-b049-50ae85f07b0e', 'eedbf66d-3c1a-40cf-b516-a269ef289048', 'eeeb239c-252d-424b-bc0a-e9f2a031f53f', 'ef10fe61-15f2-43c1-ab4f-275beb1d21aa', 'efa35fe9-7912-47b4-a824-2c6cb5ff5fde', 'efbf6bc0-7595-413d-a762-a5d083ba192e', 'efcd37b0-5a41-4978-8669-e3af36331195', 'f03acb9e-8297-4a1b-aa3f-44ae326171cb', 'f0896348-eb7b-4178-988a-fdd9a065e9fa', 'f0aeefac-4472-43ed-9538-73f1460718c6', 'f0b3a780-ba85-4e75-bb46-b1025f39ba93', 'f11bf838-602e-40f4-b2e0-63dd899869de', 'f11fe8a8-c631-41a4-87ef-579bde62acc9', 'f144d17e-4a00-4788-be25-17287fae2ef3', 'f1804243-48c7-461e-8aed-c1eaa9a9f4ad', 'f1be7058-5711-4869-8c2d-473d1315c2e2', 'f1f79283-014c-4a66-a0d8-56f112f6c559', 'f20ac2c2-f7b2-4ef3-8ba6-2733d9f2eec6', 'f2a953bf-37ca-4a06-aaba-5cfb2770df15', 'f2bbd6c6-f20e-4b81-aa9f-3fa31834efdc', 'f2e16ebd-b1c4-41c1-aa17-b1adb5461ac7', 'f3573bed-cb5d-4fc5-af5c-e8fd87bd36e3', 'f3605514-413b-49de-badf-4a74420f7fbc', 'f486da98-784c-4080-bf0d-146835583735', 'f52f3191-9cf9-4e6d-a357-1c8ae42a632c', 'f5397770-8b1d-4cdd-9335-32a239b3242d', 'f558365c-2999-4fa3-8394-5c6558ad0644', 'f5fed8b4-ab32-40f4-ae94-86516fdccd9e', 'f771e07d-6245-4c1f-9474-cc07e61acbb9', 'f77bfc6f-dc94-470e-a49c-41efdc951986', 'f7f3756c-09b8-490b-883c-0812af64d0c5', 'f8043bb9-2704-4b23-865f-589c9452a75f', 'f8197f9f-c6c2-4647-941c-b5d6d80344b6', 'f83de8a8-66f8-4069-8253-cf8d8eeb30b8', 'f84d806f-6f45-4596-ba10-0d869401b1a8', 'f856d7d3-ffa3-4b92-b7ce-ad0c59845886', 'f875cf92-07d2-4368-8bdd-47fcc3cc7cdf', 'f87a83be-82e5-4abc-87c6-66522f60714e', 'f88914bf-7491-437e-9768-7ce815b56deb', 'f93a2c38-a40d-45eb-bf6b-051546877ab4', 'fa25bf31-1ae9-4745-8014-7d1df5d5dbf8', 'fa7ae9e1-119f-45ea-aee6-a26df7f93d90', 'fb0dd8a2-e65a-450c-b61f-244c011dc240', 'fb22852d-1286-4516-b8d3-d285702c996f', 'fb2e7b03-94c1-4446-b83a-6750f87226b7', 'fb871878-2821-4e36-a14b-7226db16d25f', 'fb936aa7-f48b-41f6-a9f7-b8a097b2a293', 'fc53527c-9ef3-4a66-bc9a-33a1ffc43e2f', 'fcb67c91-e6cc-49f9-a4ac-f4403a91013e', 'fd85066d-7a3c-434e-b053-6494f934efa9', 'fdc90ac3-9eb1-44d7-9890-b08ffcff4fa2', 'fdd72c9c-133c-4fd4-bbea-359cd98ca7bd', 'fe1153ca-528b-4cad-b713-331995fa8d70', 'fe2037b2-893e-4abe-a248-01448b324959', 'fe3231bb-7e05-4965-8ed5-91044375e3b2', 'fe504350-6a13-4376-a58c-14092a8e0793', 'fe7b1030-97f5-4256-8f42-2f1c4c388d2e', 'fe7c7743-77ba-4f0b-ba4a-fc72fb75b008', 'fe8cb735-345a-488a-82f5-df1d5137edd8', 'ffb3237a-4c7f-4b52-be9c-e91dde34b688', 'ffe7081a-4b0f-4b1c-a67b-052c2ab0f7e3', 'fff1cd7a-04f9-486c-97de-d5d2c6ddb3cb']

Column num: 1
Min: Anthony Campbell
Max: William Taylor
Distinct: 945
Min chars in a row: 8
Max chars in a row: 21
Min words in a row: 2
Max words in a row: 2
Char vocabulary:  ABCDEGHJKLMNPRSTWYabcdefghiklmnoprstuvwyz
Word vocabulary: ['Adams', 'Allen', 'Anderson', 'Anthony', 'Baker', 'Barbara', 'Betty', 'Brian', 'Brown', 'Campbell', 'Carol', 'Carter', 'Charles', 'Christopher', 'Clark', 'Collins', 'Daniel', 'David', 'Davis', 'Deborah', 'Donald', 'Donna', 'Dorothy', 'Edward', 'Edwards', 'Elizabeth', 'Evans', 'Garcia', 'George', 'Gonzalez', 'Green', 'Hall', 'Harris', 'Helen', 'Hernandez', 'Hill', 'Jackson', 'James', 'Jason', 'Jeff', 'Jennifer', 'John', 'Johnson', 'Jones', 'Joseph', 'Karen', 'Kenneth', 'Kevin', 'Kimberly', 'King', 'Laura', 'Lee', 'Lewis', 'Lind', 'Lisa', 'Lopez', 'Margaret', 'Maria', 'Mark', 'Martin', 'Martinez', 'Mary', 'Michael', 'Michelle', 'Miller', 'Mitchell', 'Moore', 'Nancy', 'Nelson', 'Parker', 'Patricia', 'Paul', 'Perez', 'Phillips', 'Richard', 'Robert', 'Roberts', 'Robinson', 'Rodriguez', 'Ronald', 'Ruth', 'Sandra', 'Sarah', 'Scott', 'Sharon', 'Smith', 'Steven', 'Susan', 'Taylor', 'Thomas', 'Thompson', 'Turner', 'Walker', 'White', 'William', 'Williams', 'Wilson', 'Wright', 'Young']

Column num: 2
Min: Addyson Aaliyah
Max: Shena Desiree
Distinct: 6
Min chars in a row: 11
Max chars in a row: 15
Min words in a row: 2
Max words in a row: 2
Char vocabulary:  ACDGJPSadefhilnorsuvy
Word vocabulary: ['Aaliyah', 'Addyson', 'Calella', 'Calla', 'Carrie', 'Desiree', 'Galen', 'Jeffry', 'Paul', 'Shena', 'Silvia']

Column num: 3
Min: MonsterWorq
Max: Yogatacular
Distinct: 5
Min chars in a row: 10
Max chars in a row: 13
Min words in a row: 1
Max words in a row: 1
Char vocabulary: AMSTVWYabceghiklnopqrstu
Word vocabulary: ['MonsterWorq', 'SpeakerAce', 'Talkspiration', 'Verbalthon', 'Yogatacular']

Column num: 4
Avg: 932.258201058201
Sum of squares: 894298474
Median: 945.0
Min: 465
Max: 2036
Distinct: 28
Corrected std: 278.07204551856535
Word vocabulary: []

Column num: 5
Min: Client Solution Analyst
Max: Workshop Technician
Distinct: 15
Min chars in a row: 11
Max chars in a row: 25
Min words in a row: 1
Max words in a row: 3
Char vocabulary:  -ACDEFJLMOPRSTWacdeghijklmnoprstuvy
Word vocabulary: ['Analyst', 'Assistant', 'Client', 'Developer', 'Electrician', 'Engineer', 'Farm', 'Financial', 'Front-End', 'JavaScript', 'Junior', 'Loader', 'Manager', 'Medical', 'Operator', 'Physiotherapist', 'Planner', 'Project', 'Receptionist', 'Senior', 'Service', 'Site', 'Solution', 'Store', 'Supervisor', 'Technician', 'Workshop']

Column num = 0
max_num_words = 1
min_num_words = 1
num_chars = 34020
num_uppercase_chars = 0
type = String
isCategorical = 0
num_lowercase_chars = 11108
count = 945
quantile50 = 81aabb56-808c-48a1-b2a3-5d3f2e1a752f
num_entirely_lowercase = 945
num_entirely_uppercase = 0
max_num_chars = 1
num_digit_chars = 19132
distinct = 945
avg_chars = 36.000000
min = 0008f14d-e2a7-4582-bf5e-89ce32b55606
num_words = 945
quantile25 = 4307ef5b-2e00-4316-b04c-debff4edc5c4
max = fff1cd7a-04f9-486c-97de-d5d2c6ddb3cb
quantile75 = c8539dda-ec0e-4c67-a2f4-2d201bb82171
min_num_chars = 1
num_non_letter_chars = 22912
vocab = -0123456789abcdef

Column num = 1
max_num_words = 2
min_num_words = 2
num_chars = 12261
num_uppercase_chars = 1890
type = String
isCategorical = 0
num_lowercase_chars = 9426
count = 945
quantile50 = Kenneth King
num_entirely_lowercase = 0
num_entirely_uppercase = 0
max_num_chars = 2
num_digit_chars = 0
distinct = 945
avg_chars = 12.974603
min = Anthony Campbell
num_words = 1890
quantile25 = Donna White
max = William Taylor
quantile75 = Patricia Gonzalez
min_num_chars = 2
num_non_letter_chars = 945
vocab =  ABCDEGHJKLMNPRSTWYabcdefghiklmnoprstuvwyz

Column num = 2
max_num_words = 2
min_num_words = 2
num_chars = 11843
num_uppercase_chars = 1890
type = String
isCategorical = 1
num_lowercase_chars = 9008
count = 945
quantile50 = Galen Calla
num_entirely_lowercase = 0
num_entirely_uppercase = 0
max_num_chars = 2
num_digit_chars = 0
distinct = 6
avg_chars = 12.532275
min = Addyson Aaliyah
num_words = 1890
quantile25 = Carrie Silvia
max = Shena Desiree
quantile75 = Paul Jeffry
min_num_chars = 2
num_non_letter_chars = 945
vocab =  ACDGJPSadefhilnorsuvy

Column num = 3
max_num_words = 1
min_num_words = 1
num_chars = 10452
num_uppercase_chars = 1300
type = String
isCategorical = 1
num_lowercase_chars = 9152
count = 945
quantile50 = Talkspiration
num_entirely_lowercase = 0
num_entirely_uppercase = 0
max_num_chars = 1
num_digit_chars = 0
distinct = 5
avg_chars = 11.060317
min = MonsterWorq
num_words = 945
quantile25 = SpeakerAce
max = Yogatacular
quantile75 = Verbalthon
min_num_chars = 1
num_non_letter_chars = 0
vocab = AMSTVWYabceghiklnopqrstu

Column num = 4
median_ad = 100.000000
geometric_mean = 893.289725
median = 945.000000
sum_of_squares = 894298474
num_negatives = 0
quantile75 = 1020
type = Int
mean_ad = 186.978103
isCategorical = 0
kurtosis = 2.859101
count = 945
quantile50 = 945
num_zeros = 0
avg = 932.258201
distinct = 28
STD = 278.072046
skewness = 1.132442
min = 465
quantile25 = 800
max = 2036
sum = 880984

Column num = 5
max_num_words = 3
min_num_words = 1
num_chars = 17603
num_uppercase_chars = 2226
type = String
isCategorical = 0
num_lowercase_chars = 14152
count = 945
quantile50 = Physiotherapist
num_entirely_lowercase = 0
num_entirely_uppercase = 0
max_num_chars = 3
num_digit_chars = 0
distinct = 15
avg_chars = 18.627513
min = Client Solution Analyst
num_words = 2113
quantile25 = JavaScript Developer
max = Workshop Technician
quantile75 = Service Technician
min_num_chars = 1
num_non_letter_chars = 1225
vocab =  -ACDEFJLMOPRSTWacdeghijklmnoprstuvy


'''

snapshots['test_example[basic/dynamic_verifying_afd.py-None-dynamic_verifying_afd_output] dynamic_verifying_afd_output'] = '''\x1b[1;49mThis example shows how to verify AFD in the dynamic table using dynamic_fd_verification.
If you want to know how to use dynamic_fd_verification, please see dynamic_verifying_fd.py example.
\x1b[1;49mFirst, let's look at the DnD.csv table and try verifying the [Creature, HaveMagic] -> [Strength] FD.\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;49m0     Ogre         9      False\x1b[0m
\x1b[1;49m1     Ogre         6      False\x1b[0m
\x1b[1;49m2      Elf         6       True\x1b[0m
\x1b[1;49m3      Elf         6       True\x1b[0m
\x1b[1;49m4      Elf         1       True\x1b[0m
\x1b[1;49m5    Dwarf         9      False\x1b[0m
\x1b[1;49m6    Dwarf         6      False\x1b[0m

Checking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.5)
\x1b[1;42m AFD with this error threshold holds \x1b[0m

Checking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.1)
\x1b[1;41m AFD with this error threshold does not hold \x1b[0m
But the same \x1b[1;42m AFD with error threshold = 0.19047619047619047 holds \x1b[0m

Similarly to the FD verification primitive, the AFD one can provide a user with clusters:
Number of clusters violating FD: 3
\x1b[1;46m #1 cluster: \x1b[0m
2: ['Elf', np.True_] -> [np.int64(6)]
3: ['Elf', np.True_] -> [np.int64(6)]
4: ['Elf', np.True_] -> [np.int64(1)]
Most frequent rhs value proportion: 0.6666666666666666
Num distinct rhs values: 2
\x1b[1;46m #2 cluster: \x1b[0m
5: ['Dwarf', np.False_] -> [np.int64(9)]
6: ['Dwarf', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2
\x1b[1;46m #3 cluster: \x1b[0m
0: ['Ogre', np.False_] -> [np.int64(9)]
1: ['Ogre', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2
\x1b[1;49mNow, let's change the table and check the AFD again.\x1b[0m
\x1b[1;32mRows to insert:
Creature  Strength  HaveMagic
     Elf         2      False
     Elf         2      False
  Dragon       200       True\x1b[0m
\x1b[1;31mRows indices to delete:  2, 4, 6\x1b[0m
\x1b[1;33mRows to update:
  Creature  Strength  HaveMagic
0      Elf         3      False
1      Elf         3      False
3   Dragon       200       True
5      Elf         3      False\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;33m0      Elf         3      False\x1b[0m
\x1b[1;33m1      Elf         3      False\x1b[0m
\x1b[1;33m3   Dragon       200       True\x1b[0m
\x1b[1;33m5      Elf         3      False\x1b[0m
\x1b[1;32m7      Elf         2      False\x1b[0m
\x1b[1;32m8      Elf         2      False\x1b[0m
\x1b[1;32m9   Dragon       200       True\x1b[0m

Checking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.5)
\x1b[1;42m AFD with this error threshold holds \x1b[0m

Checking whether [Creature, HaveMagic] -> [Strength] AFD holds (error threshold = 0.1)
\x1b[1;41m AFD with this error threshold does not hold \x1b[0m
But the same \x1b[1;42m AFD with error threshold = 0.2857142857142857 holds \x1b[0m

Let's look at the clusters again
Number of clusters violating FD: 1
\x1b[1;46m #1 cluster: \x1b[0m
7: ['Elf', np.False_] -> [np.int64(2)]
8: ['Elf', np.False_] -> [np.int64(2)]
0: ['Elf', np.False_] -> [np.int64(3)]
1: ['Elf', np.False_] -> [np.int64(3)]
5: ['Elf', np.False_] -> [np.int64(3)]
Most frequent rhs value proportion: 0.6
Num distinct rhs values: 2
'''

snapshots['test_example[basic/dynamic_verifying_fd.py-None-dynamic_verifying_fd_output] dynamic_verifying_fd_output'] = '''\x1b[1;49mThis example shows how to use dynamic FD verification algorithm.\x1b[0m
\x1b[1;49mFirst, let's look at the DnD.csv table and try to verify the functional dependency [Creature, HaveMagic] -> [Strength].\x1b[0m
Note: The current version of the algorithm supports checking only one FD defined in the load_data method.

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;49m0     Ogre         9      False\x1b[0m
\x1b[1;49m1     Ogre         6      False\x1b[0m
\x1b[1;49m2      Elf         6       True\x1b[0m
\x1b[1;49m3      Elf         6       True\x1b[0m
\x1b[1;49m4      Elf         1       True\x1b[0m
\x1b[1;49m5    Dwarf         9      False\x1b[0m
\x1b[1;49m6    Dwarf         6      False\x1b[0m

FD verification result: \x1b[1;41m FD does not hold \x1b[0m
Number of clusters violating FD: 3
\x1b[1;46m #1 cluster: \x1b[0m
2: ['Elf', np.True_] -> [np.int64(6)]
3: ['Elf', np.True_] -> [np.int64(6)]
4: ['Elf', np.True_] -> [np.int64(1)]
Most frequent rhs value proportion: 0.6666666666666666
Num distinct rhs values: 2
\x1b[1;46m #2 cluster: \x1b[0m
5: ['Dwarf', np.False_] -> [np.int64(9)]
6: ['Dwarf', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2
\x1b[1;46m #3 cluster: \x1b[0m
0: ['Ogre', np.False_] -> [np.int64(9)]
1: ['Ogre', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2

\x1b[1;49mThen, let's try inserting a row into the table and check whether the FD holds again.\x1b[0m
Note: Insert statements are defined using Pandas DataFrame/read_csv.
      It must have the same column names and order as the original table.
\x1b[1;32mRows to insert:
Creature  Strength  HaveMagic
     Elf         6       True\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;49m0     Ogre         9      False\x1b[0m
\x1b[1;49m1     Ogre         6      False\x1b[0m
\x1b[1;49m2      Elf         6       True\x1b[0m
\x1b[1;49m3      Elf         6       True\x1b[0m
\x1b[1;49m4      Elf         1       True\x1b[0m
\x1b[1;49m5    Dwarf         9      False\x1b[0m
\x1b[1;49m6    Dwarf         6      False\x1b[0m
\x1b[1;32m7      Elf         6       True\x1b[0m

FD verification result: \x1b[1;41m FD does not hold \x1b[0m
Number of clusters violating FD: 3
\x1b[1;46m #1 cluster: \x1b[0m
2: ['Elf', np.True_] -> [np.int64(6)]
3: ['Elf', np.True_] -> [np.int64(6)]
4: ['Elf', np.True_] -> [np.int64(1)]
7: ['Elf', np.True_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.75
Num distinct rhs values: 2
\x1b[1;46m #2 cluster: \x1b[0m
5: ['Dwarf', np.False_] -> [np.int64(9)]
6: ['Dwarf', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2
\x1b[1;46m #3 cluster: \x1b[0m
0: ['Ogre', np.False_] -> [np.int64(9)]
1: ['Ogre', np.False_] -> [np.int64(6)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2

\x1b[1;49mNow we are going to delete rows that violate the FD.\x1b[0m
Note: Delete statements are defined using a set of indexes of the rows that we want to delete.
\x1b[1;31mRows indices to delete:  0, 4, 5\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;49m1     Ogre         6      False\x1b[0m
\x1b[1;49m2      Elf         6       True\x1b[0m
\x1b[1;49m3      Elf         6       True\x1b[0m
\x1b[1;49m6    Dwarf         6      False\x1b[0m
\x1b[1;49m7      Elf         6       True\x1b[0m

FD verification result: \x1b[1;42m FD holds \x1b[0m

\x1b[1;49mNow, let's try to update some rows in the table.\x1b[0m
Note: Update statements are defined using Pandas DataFrame/read_csv.
      The first column should be named '_id' and represent the indexes of the rows that we want to update.
      The remaining columns must have the same names and order as the columns in the original table.
\x1b[1;33mRows to update:
  Creature  Strength  HaveMagic
2   Dragon       999       True
3   Dragon       998       True
7   Dragon       999       True\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;49m1     Ogre         6      False\x1b[0m
\x1b[1;33m2   Dragon       999       True\x1b[0m
\x1b[1;33m3   Dragon       998       True\x1b[0m
\x1b[1;49m6    Dwarf         6      False\x1b[0m
\x1b[1;33m7   Dragon       999       True\x1b[0m

FD verification result: \x1b[1;41m FD does not hold \x1b[0m
Number of clusters violating FD: 1
\x1b[1;46m #1 cluster: \x1b[0m
2: ['Dragon', np.True_] -> [np.int64(999)]
3: ['Dragon', np.True_] -> [np.int64(998)]
7: ['Dragon', np.True_] -> [np.int64(999)]
Most frequent rhs value proportion: 0.6666666666666666
Num distinct rhs values: 2

\x1b[1;49mExample of processing multiple operation types.\x1b[0m
\x1b[1;32mRows to insert:
Creature  Strength  HaveMagic
     Elf         7       True\x1b[0m
\x1b[1;31mRows indices to delete:  6, 7\x1b[0m
\x1b[1;33mRows to update:
  Creature  Strength  HaveMagic
1      Elf         0      False\x1b[0m

Current table state:
  Creature  Strength  HaveMagic
\x1b[1;33m1      Elf         0      False\x1b[0m
\x1b[1;49m2   Dragon       999       True\x1b[0m
\x1b[1;49m3   Dragon       998       True\x1b[0m
\x1b[1;32m8      Elf         7       True\x1b[0m

FD verification result: \x1b[1;41m FD does not hold \x1b[0m
Number of clusters violating FD: 1
\x1b[1;46m #1 cluster: \x1b[0m
2: ['Dragon', np.True_] -> [np.int64(999)]
3: ['Dragon', np.True_] -> [np.int64(998)]
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2

'''

snapshots['test_example[basic/mining_ac.py-None-mining_ac_output] mining_ac_output'] = '''This example is dedicated to Fuzzy Algebraic Constraints (AC). The definition and algorithm 
are based on article "B-HUNT: Automatic Discovery of Fuzzy Algebraic Constraints in Relational 
Data" by Paul G. Brown & Peter J. Haas presented at VLDB in 2003.

First of all, let's figure out what AC is. However, to avoid going too deep, we will give you 
a simple definition without formalization. AC represents the results of applying binary 
operations between two table columns, with values grouped into meaningful intervals. 
Let's illustrate this with an example.

We have a table examples/datasets/player_stats.csv with the following data:
|   id |   Strength |   Agility |
|-----:|-----------:|----------:|
|    0 |          3 |         1 |
|    1 |          4 |         1 |
|    2 |          1 |         3 |
|    3 |          2 |         2 |
|    4 |          1 |         4 |
|    5 |         10 |        12 |
|    6 |         14 |        10 |
|    7 |          1 |        23 |
|    8 |          6 |        16 |

Let\'s apply binary operation "+" to the Strength and Agility columns and observe the results.

\x1b[1;42mDiscovered ranges\x1b[0m for (Strength + Agility) are:
[(4.0, 5.0), (22.0, 24.0)]

Rows in which the result of the chosen operation (+) is \x1b[1;41moutside\x1b[1;49m of discovered ranges:
\x1b[1;46mNone\x1b[1;49m

As shown, the sum of Strength and Agility falls within either the (4, 5) or (22, 24) ranges. 
This pattern may emerge because player characters with similar combined attribute values 
likely belong to the same tier.


To run the algorithm, you must configure the parameters below:
For \x1b[1;34mbinary arithmetic operations\x1b[0m, you can use four options:\x1b[1;49m
"+"
"-"
"*"
"/"
\x1b[0m
Furthermore, AC mining algorithm provides five \x1b[1;34mparameters for setting up execution\x1b[0m:
\x1b[1;49mweight
fuzziness
p_fuzz
bumps_limit
iterations_limit
ac_seed
\x1b[0m
\x1b[1;42mWeight\x1b[0m accepts values in the range (0, 1]. 
Values closer to 1 force the algorithm to produce fewer larger intervals (up to a single 
interval covering all values).
Values closer to 0 force the algorithm to produce smaller intervals.

\x1b[1;42mFuzziness\x1b[0m belongs to (0,1) range while \x1b[1;42mp_fuzz\x1b[0m belongs to [0,1] range. These parameters 
control precision and the number of considered rows.
Fuzziness values closer to 0 and p_fuzz values closer to 1 force the algorithm to include 
more rows (higher accuracy).
Fuzziness values closer to 1 and p_fuzz values closer to 0 force the algorithm to include 
fewer rows (higher chance of skipping rows, lower precision, but faster execution).

\x1b[1;42mBumps_limit\x1b[0m accepts only natural numbers from the range [1, inf) and limits the number of 
intervals for all column pairs. To set bumps_limit to inf you should use the 0 value.

\x1b[1;42mIterations_limit\x1b[0m accepts only natural numbers. 
Lower values (close to 1) reduce accuracy due to algorithm performing fewer iterations.

\x1b[1;42mAC_seed\x1b[0m accepts only natural numbers. 
B-HUNT is a randomized algorithm that accepts the seed parameter (AC_seed). Fixing this 
parameter ensures reproducible results, which are necessary for verifying results during 
testing of the algorithm. Furthermore, we need to fix it in this example for demonstration 
purposes; otherwise, we may obtain a different number of intervals with different boundaries 
that will not correspond to the text we wrote for our output.

Let's proceed to a visual example. We will use dataset from this path: 
examples/datasets/cargo_march.csv.
For default parameters we will use those values:
binary operation is "-", weight - 0.1, fuzziness - 0.2, p_fuzz - 0.85, bumps_limit - 0, 
iterations_limit - 4, AC_seed - 11.

Let's see the result of the algorithm with these parameters.

\x1b[1;42mDiscovered ranges\x1b[0m for (Delivery date - Dispatch date) are:
[(2.0, 7.0), (15.0, 22.0)]

Rows in which the result of the chosen operation (-) is \x1b[1;41moutside\x1b[1;49m of discovered ranges:
\x1b[1;46mid: 7\x1b[1;49m
Dispatch date: 1
Delivery date: 30
Difference: 29

\x1b[1;46mid: 26\x1b[1;49m
Dispatch date: 7
Delivery date: 18
Difference: 11

\x1b[1;46mid: 30\x1b[1;49m
Dispatch date: 11
Delivery date: 22
Difference: 11

You can see that the algorithm creates two intervals for the binary operation "-": (2-7) and 
(15-22). This means that the difference between the dispatch date and delivery date always 
falls within these intervals, except for three rows where the difference lies outside the 
discovered ranges. From this, we can infer that:
\x1b[1;33mPackages for some addresses are typically delivered within 7 days.\x1b[0m
\x1b[1;33mPackages for some addresses take up to 22 days.\x1b[0m

Why these two intervals? To answer this question, more context is needed; that is, we should 
look into the underlying data. We can imagine several reasons for this result, such as: 1) 
nearby addresses versus far addresses; 2) air shipping versus regular shipping.

There are three parcels that fall outside of these delivery intervals. Why? This is a point 
for further investigation, which requires additional context. There are many possible reasons 
for this: 1) on these dates there was a workers' strike in some regions; or 2) an incorrect 
address was specified, which increased the delivery time; or 3) it is just a typo in the table.

Now we reduce the value of the parameter weight to 0.05.

\x1b[1;42mDiscovered ranges\x1b[0m for (Delivery date - Dispatch date) are:
[(2.0, 7.0), (11.0, 11.0), (15.0, 22.0), (29.0, 29.0)]

Rows in which the result of the chosen operation (-) is \x1b[1;41moutside\x1b[1;49m of discovered ranges:
\x1b[1;46mNone\x1b[1;49m

You can see that the number of intervals increases, and there is no longer any data outside of 
the discovered ranges.
However, with this number of intervals, it is difficult to draw immediate conclusions about the 
delivery date. In this case, a detailed analysis of other attributes might enable more meaningful 
predictions for delivery times. For example, it may be a good idea to partition data by the region 
attribute (or by month/quarter) and consider each partition individually. 

Another option is to try to find a parameter combination that will result in a smaller number of 
intervals. Next, remember that the algorithm is randomized (unless you run it with the exact 
settings) — it can skip some rows, so you can also try to alter the seed.

Finally, cleaning up the data by removing duplicate and incomplete rows might also help.
Thus, the quantity and quality of the intervals are the user's responsibility. It may take several 
attempts to achieve something interesting. Experiment!
'''

snapshots['test_example[basic/mining_adc.py-None-mining_adc_output] mining_adc_output'] = '''\x1b[33mUnderstanding Denial Constraints (DCs)\x1b[0m
In this walkthrough, we follow the definitions described in the paper
"Fast approximate denial constraint discovery" by Xiao, Tan,
Wang, and Ma (2022) [Proc. VLDB Endow. 16(2), 269–281].

A Denial Constraint is a statement that says: "For all pairs of different rows in a table,
it should never happen that some condition holds."
Formally, DC \x1b[1m\x1b[36mφ\x1b[0m is a conjunction of predicates of the following form:
\x1b[1m\x1b[36m∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)\x1b[0m

For example, look at this small table:
Name     Grade   Salary
Alice    3       3000
Bob      4       4000
Carol    4       4000

A possible DC here is: \x1b[1m\x1b[36m¬{ t.Grade == s.Grade ∧ t.Salary != s.Salary }\x1b[0m

This means: "It should never happen that two people have the same grade but different salaries.",
or in other words, if two rows share the same Grade, they must share the same Salary.

Sometimes, we allow a DC to hold approximately, which means a small number of row pairs
might violate it. The measure used for that is the 'g1' metric. Roughly, the 'g1' metric
checks what fraction of all row pairs violates the DC, and if that fraction is lower than
a chosen threshold, we consider the DC 'valid enough.'

\x1b[33mMining Denial Constraints\x1b[0m
We have two parameters in Desbordante's DC mining algorithm:
1) evidence_threshold: This sets the fraction of row pairs that must satisfy the DC
   for it to be considered valid. A value of 0 means exact DC mining (no violations allowed).
2) shard_length: This splits the dataset into row "shards" for parallelization.
   A value of 0 means no split, so the entire dataset is processed at once.

\x1b[33mLet's begin by looking at TABLE_1:\x1b[0m
TABLE_1 (examples/datasets/taxes_1.csv):
       State  Salary  FedTaxRate
0    NewYork    3000        0.20
1    NewYork    4000        0.25
2    NewYork    5000        0.30
3  Wisconsin    5000        0.15
4  Wisconsin    6000        0.20
5  Wisconsin    4000        0.10
6      Texas    1000        0.15
7      Texas    2000        0.25
8      Texas    3000        0.30

\x1b[33mMining exact DCs (evidence_threshold=0) on TABLE_1\x1b[0m
\x1b[33mDiscovered DCs:\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary == s.Salary ∧ t.FedTaxRate == s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.FedTaxRate == s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary == s.Salary }\x1b[0m

Note the following Denial Constraint we found:
\x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }\x1b[0m.
It states that for all people in the same state, the person with a higher salary
should have a higher tax rate. No pairs of rows should violate that rule.

Now let's mine approximate DCs by setting evidence_threshold to 0.5.
This means we only require that at least half of all row pairs satisfy each DC (according to 'g1').

\x1b[33mMining ADCs (evidence_threshold=0.5) on TABLE_1\x1b[0m
\x1b[33mDiscovered ADCs:\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary <= s.Salary ∧ t.FedTaxRate <= s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary <= s.Salary ∧ t.FedTaxRate != s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State != s.State ∧ t.FedTaxRate <= s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State != s.State ∧ t.Salary <= s.Salary }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.FedTaxRate < s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary < s.Salary }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.FedTaxRate == s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary == s.Salary }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State }\x1b[0m

Here, for example, the 'g1' metric values for a few approximate DCs are:
\x1b[1m\x1b[36m¬{ t.Salary <= s.Salary ∧ t.FedTaxRate <= s.FedTaxRate }\x1b[0m → 0.486111
\x1b[1m\x1b[36m¬{ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }\x1b[0m → 0.458333
\x1b[1m\x1b[36m¬{ t.State == s.State }\x1b[0m → 0.25
Note: A smaller 'g1' value means fewer violations, making the DC more exact.

\x1b[33mConclusion:\x1b[0m
We found both exact and approximate DCs.

- Exact DCs are those with zero violations, so they must hold for every pair of rows.
- Approximate DCs allow some fraction of violating pairs.

Therefore, an approximate DC can logically imply the exact one.
For example, consider:
Exact DC: \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary == s.Salary }\x1b[0m
Approximate DC: \x1b[1m\x1b[36m¬{ t.Salary == s.Salary }\x1b[0m

If the approximate DC (which prohibits any two rows from having the same Salary)
is satisfied for at least the chosen threshold, then clearly no two rows can share both
the same State and the same Salary. Thus, the approximate DC implies the exact DC.

In real scenarios, exact DCs may be too rigid.
Allowing a small fraction of violations is often a practical compromise,
but setting a very high threshold quickly becomes meaningless
since it would permit too many inconsistencies.
The best threshold often depends on how 'dirty' the data is; datasets with
more inconsistencies may require a higher threshold to capture meaningful DCs.

\x1b[33mNow let's move on to TABLE_2\x1b[0m
TABLE_2 (examples/datasets/taxes_2.csv):
       State  Salary  FedTaxRate
0    NewYork    3000        0.20
1    NewYork    4000        0.25
2    NewYork    5000        0.30
3  Wisconsin    5000        0.15
4  Wisconsin    6000        0.20
5  Wisconsin    4000        0.10
6      Texas    1000        0.15
7      Texas    2000        0.25
8      Texas    3000        0.30
9      Texas    5000        0.05

We added this record for Texas:
\x1b[32m(State=Texas, Salary=5000, FedTaxRate=0.05)\x1b[0m
Notice how it introduces a scenario that breaks the DC we discuissed earlier, stating
"the person with a higher salary should have a higher tax rate,"
because there are now people in Texas with a lower salary but a higher tax rate.

Let's see how the exact DC mining changes due to this additional record.

\x1b[33mMining exact DCs (evidence_threshold=0) on TABLE_2\x1b[0m
\x1b[33mDiscovered DCs:\x1b[0m
  \x1b[1m\x1b[36m¬{ t.Salary == s.Salary ∧ t.FedTaxRate == s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.FedTaxRate == s.FedTaxRate }\x1b[0m
  \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary == s.Salary }\x1b[0m

We can see that the DC \x1b[1m\x1b[36m¬{ t.State == s.State ∧ t.Salary <= s.Salary ∧ t.FedTaxRate >= s.FedTaxRate }\x1b[0m
no longer appears because of the violation introduced by record index 9
(\x1b[32m(Texas, 5000, 0.05)\x1b[0m).

Those violations occur in pairs like \x1b[31m(6, 9), (7, 9), (8, 9)\x1b[0m,
where each number is a record index in the dataset.
'''

snapshots['test_example[basic/mining_afd.py-None-mining_afd_output] mining_afd_output'] = '''==============================================
In Desbordante we consider an approximate functional dependency (AFD)
any kind of functional dependency (FD) that employs an error metric and is not named
(e.g. soft functional dependencies). This metric is used to calculate the extent of
violation for a given exact FD and lies within [0, 1] range (the lower, the less violations
are found in data). For the discovery task a user can specify the threshold and Desbordante
will find all AFDs, which have their error equal or less than the threshold, according to the selected metric.

Currently, Desbordante supports:
1) Five metrics: g1, pdep, tau, mu+, rho.
2) Two algorithms for discovery of AFDs: Tane and Pyro, with Pyro being the fastest. 
Unfortunately, Pyro can handle only the g1 metric, for the rest use Tane.

For more information consider:
1) Measuring Approximate Functional Dependencies: A Comparative Study by M. Parciak et al.
2) Efficient Discovery of Approximate Dependencies by S. Kruse and F. Naumann.
3) TANE: An Efficient Algorithm for Discovering Functional and Approximate Dependencies by Y. Huhtala et al.
==============================================

Now, we are going to demonstrate how to discover AFDs. First, consider the dataset:
+------+---------------+---------+
|   Id | ProductName   |   Price |
|------+---------------+---------|
|    1 | Laptop        |    3000 |
|    2 | Laptop        |    3000 |
|    3 | Laptop        |     300 |
|    4 | Laptop        |    3000 |
|    5 | Smartwatch    |     600 |
|    6 | Headphones    |     500 |
|    7 | Tablet        |     300 |
|    8 | Tablet        |     500 |
|    9 | Smartphone    |    1000 |
|   10 | Headphones    |     500 |
|   11 | Laptop        |    3000 |
|   12 | Notebook      |    3000 |
+------+---------------+---------+

AFDs mined by Pyro with g1 measure:
[Price] -> Id
[Price] -> ProductName
[ProductName] -> Id
[ProductName] -> Price
[Id] -> ProductName
[Id] -> Price

AFDs mined by Tane
g1:
[Price] -> Id
[Price] -> ProductName
[ProductName] -> Id
[ProductName] -> Price
[Id] -> ProductName
[Id] -> Price

pdep:
[ProductName] -> Price
[Id] -> ProductName
[Id] -> Price

tau:
[Id] -> ProductName
[Id] -> Price

mu_plus:
[Id] -> ProductName
[Id] -> Price

rho:
[ProductName] -> Price
[Id] -> ProductName
[Id] -> Price

'''

snapshots['test_example[basic/mining_aind.py-None-mining_aind_output] mining_aind_output'] = '''==============================================
In Desbordante, we consider an approximate inclusion dependency (AIND)
as any inclusion dependency (IND) that utilizes an error metric to measure
violations. This metric calculates the proportion of distinct values in the
dependent set (LHS) that must be removed to satisfy the dependency on the
referenced set (RHS) completely.

The metric lies within the [0, 1] range:
- A value of 0 means the IND holds exactly (no violations exist).
- A value closer to 1 indicates a significant proportion of LHS values violate
      the dependency.

Desbordante supports the discovery and verification of both exact INDs and AINDs:
1) Exact INDs: All values in the LHS set must match a value in the RHS set.
2) Approximate INDs (AINDs): Allows for controlled violations quantified by the
error metric.

For discovery tasks, users can specify an error threshold, and Desbordante will
return all AINDs with an error value equal to or less than the specified
threshold.

The error metric used for AINDs in Desbordante is an adaptation of g3,
originally designed for approximate functional dependencies (FDs).

For more information, consider:
1) Unary and n-ary inclusion dependency discovery in relational databases by
   Fabien De Marchi, Stéphane Lopes, and Jean-Marc Petit.
==============================================

Now, we are going to demonstrate how to discover AINDs.

The datasets under consideration for this example are
'employee' and 'project_assignments'.

Dataset 'employee':
+----+------+---------------+--------------+---------------+
|    |   id | name          | department   | location      |
|----+------+---------------+--------------+---------------|
|  0 |  101 | Alice Cooper  | Marketing    | New York      |
|  1 |  102 | Bob Johnson   | Engineering  | San Francisco |
|  2 |  103 | Charlie Brown | HR           | Chicago       |
|  3 |  104 | Dana White    | Sales        | Los Angeles   |
|  4 |  105 | Eva Black     | Marketing    | Boston        |
|  5 |  106 | Frank Green   | Engineering  | Austin        |
+----+------+---------------+--------------+---------------+

Dataset 'project_assignments':
+----+------+-----------------+------------------------+------------+
|    | id   | employee_name   | title                  | deadline   |
|----+------+-----------------+------------------------+------------|
|  0 | P001 | Alice Cooper    | Website Redesign       | 2024-12-01 |
|  1 | P002 | Bob Johnson     | App Development        | 2024-12-15 |
|  2 | P003 | Charley Brown   | HR Policy Update       | 2024-12-20 |
|  3 | P006 | Frank Green     | Infrastructure Upgrade | 2025-02-05 |
+----+------+-----------------+------------------------+------------+

Let's find all AINDs with an error threshold less than 0.3.

Found inclusion dependencies (-> means "is included in"):
IND: (project_assignments.csv, [employee_name]) -> (employee.csv, [name]) with error threshold = 0.25

We found only a single AIND, this dependency contains typos in the
"Employee Name" column of the second dataset.

For automatically detecting violating clusters, you can create a
pipeline using the AIND verifier in combination with a mining algorithm.

For an additional example, refer to the examples/advanced/aind_typos.py
'''

snapshots['test_example[basic/mining_ar.py-None-mining_ar_output] mining_ar_output'] = '''As the first example, let\'s look at the dataset containing receipts from some supermarket using input_format="tabular". In this format, each table row lists all items participating in the same transaction. Note that, in this table, some rows may be shorter than others.

|--:|:------:|:------:|:------:|:----:|
| 0 | Bread  | Butter |  Milk  | nan  |
| 1 |  Eggs  |  Milk  | Yogurt | nan  |
| 2 | Bread  | Cheese |  Eggs  | Milk |
| 3 |  Eggs  |  Milk  | Yogurt | nan  |
| 4 | Cheese |  Milk  | Yogurt | nan  |

Let's see the first 10 association rules (ARs) that are present in the dataset with minconf=1. As no minsup is specified, the default value of minsup=0 is used.

Total count of ARs: 24
The first 10 ARs:
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter'] -> ['Bread']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Bread'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Eggs'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Yogurt'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Cheese'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter', 'Milk'] -> ['Bread']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Bread', 'Butter'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter'] -> ['Bread', 'Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Bread', 'Eggs'] -> ['Milk']

['Butter'] -> ['Bread'] with confidence 1 means that whenever butter is found in the receipt, bread will \x1b[32malways\x1b[0m be present as well. The same holds true for all other rules with \x1b[1;32mconfidence 1\x1b[0m.


Now, let's examine the same dataset with \x1b[33mminconf=0.6\x1b[0m.
Total count of ARs: 32
The first 10 ARs:
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter'] -> ['Bread']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Bread'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;31m0.20 \x1b[0m\t['Butter'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Eggs'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Eggs']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Yogurt'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Yogurt']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Cheese'] -> ['Milk']
conf: \x1b[1;33m0.67 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Yogurt'] -> ['Eggs']
conf: \x1b[1;33m0.67 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Eggs'] -> ['Yogurt']

['Yogurt'] -> ['Eggs'] with confidence 0.67 means that when yogurt is found in the receipt, the chance of eggs being present amounts to 67%. So, customers are \x1b[1;33mlikely\x1b[0m to buy eggs with yogurt.

Let us turn to the next issue. You can observe that there are a lot of association rules found in this small dataset. This happens because we did not set up the so called \x1b[32msupport\x1b[0m value. This value is a a float between 0 and 1 that specifies how frequently the itemset should appear in the dataset. In this particular case, the frequency is defined as the ration between the number of transactions containing A and B, and the total number of transactions in a dataset. Since the default support value is 0, the system discovers all association rules, even those that only occur once in the dataset. Now, let's see the results with \x1b[33mminsup=0.4\x1b[0m and \x1b[33mminconf=0.6\x1b[0m.

Total count of ARs: 13
The first 10 ARs:
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Bread'] -> ['Milk']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Eggs'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Eggs']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Yogurt'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Yogurt']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Cheese'] -> ['Milk']
conf: \x1b[1;33m0.67 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Yogurt'] -> ['Eggs']
conf: \x1b[1;33m0.67 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Eggs'] -> ['Yogurt']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Eggs', 'Yogurt'] -> ['Milk']
conf: \x1b[1;33m0.67 \x1b[0m\tsup: \x1b[1;33m0.40 \x1b[0m\t['Milk', 'Yogurt'] -> ['Eggs']

Now you can see that the number of association rules have decreased significantly. This happened due to minsup being set to 0.4. 

A typical approach to controlling the algorithm is to employ \x1b[1;33m"usefulness"\x1b[0m, which is defined as confidence * support. In the last example, we set up min "usefulness" = 0.6 * 0.4 = 0.24. 

Now, let\'s try with \x1b[32mminsup=0.6\x1b[0m, \x1b[32mminconf=0.6\x1b[0m and \x1b[1;33m"usefulness"=0.36\x1b[0m.

Total count of ARs: 4
The first 10 ARs:
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Eggs'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Eggs']
conf: \x1b[1;32m1.00 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Yogurt'] -> ['Milk']
conf: \x1b[1;33m0.60 \x1b[0m\tsup: \x1b[1;33m0.60 \x1b[0m\t['Milk'] -> ['Yogurt']

So, now the total number of returned association rules is only four. We reduced the amount of "noisy" information in our output. You are free to play with these parameters to see how it changes things. Eventually, you will find out what fits your dataset and your task best.

For the second example, let\'s take a dataset with the same receipts from the same supermarket, changing the input format to input_format="singular". This is a two-column format, where the first column is the order of the items, and the second column is the item that belongs to that order.

|--:|:------:|
| 1 | Bread  |
| 1 | Butter |
| 3 | Cheese |
| 2 |  Eggs  |
| 1 |  Milk  |
| 2 |  Milk  |
| 2 | Yogurt |
| 3 | Bread  |
| 3 |  Eggs  |
| 3 |  Milk  |
| 4 |  Eggs  |
| 4 |  Milk  |
| 4 | Yogurt |
| 5 | Cheese |
| 5 |  Milk  |
| 5 | Yogurt |

This format is just a different table representation. Desbordante is perfectly capable of rules discovery in such tables, and the discovered objects are represented by the same type of objects as in the previous examples (i.e., they have the same methods and fields).


Next, we will show you how to list all of the unique items in the dataset.

In order to do this, you can use \x1b[1;34mget_itemnames\x1b[0m method:

\x1b[1;46mTotal number of items:\x1b[1;49m 6
Bread
Butter
Cheese
Eggs
Milk
Yogurt

Now you have all of the items used in this dataset.

'''

snapshots['test_example[basic/mining_aucc.py-None-mining_aucc_output] mining_aucc_output'] = '''\x1b[1m\x1b[36mThis example illustrates the usage of approximate Unique Column Combinations
(AUCC). Intuitively, an AUCC declares that some columns uniquely identify every tuple in a table,
but allows a certain degree of violation. For more information consult "Efficient Discovery of
Approximate Dependencies" by S. Kruse and F. Naumann.
\x1b[0m
The following table contains records about employees:
\x1b[1m\x1b[36mName     Grade   Salary   Work_experience   
--------------------------------------------
Mark     7       1150     12                
Joyce    2       1100     5                 
Harry    3       1000     7                 
Grace    4       900      12                
Harry    4       1000     5                 
Samuel   1       900      9                 
Nancy    2       1000     3                 
\x1b[0mWe need to select a column that will serve as a unique key (ID).

The AUCC mining algorithm with different error threshold will be used. The smaller
threshold gets, the less violations (repeated values) are allowed in column combinations.
Setting threshold to 0 means mining exact UCCs (without violations).
Let's run AUCC mining algorithm with threshold = 0:
Found UCCs:
\t\x1b[1m\x1b[36m[Name Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Name Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Salary Work_experience]\x1b[0m

There are no unary UCCs, so there is no single column that can define a key.
Let's run algorithm with bigger threshold (= 0.1):
Found AUCCs:
\t\x1b[1m\x1b[36m[Name]\x1b[0m
\t\x1b[1m\x1b[36m[Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Work_experience]\x1b[0m

Now, almost all columns are considered to be unique. It's not what we wanted.
Let's select a smaller threshold (= 0.05):
Found AUCCs:
\t\x1b[1m\x1b[36m[Name]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Salary Work_experience]\x1b[0m

Out of single-column UCCs, \x1b[1m\x1b[36mName\x1b[0m requires the smallest threshold to be "unique".
It means that \x1b[1m\x1b[36mName\x1b[0m has less violations than other columns.
Let's look at the table again, paying a special attention to the \x1b[1m\x1b[36mName\x1b[0m column:
\x1b[1m\x1b[36mName     Grade   Salary   Work_experience   
--------------------------------------------
Mark     7       1150     12                
Joyce    2       1100     5                 
Harry    3       1000     7                 
Grace    4       900      12                
Harry    4       1000     5                 
Samuel   1       900      9                 
Nancy    2       1000     3                 
\x1b[0m
There are two \x1b[1m\x1b[36mHarrys\x1b[0m. They have different work experience and salary,
therefore they are two different employees. This is most likely an error/oversight in data.
If we represented their records using unique names, the \x1b[1m\x1b[36mName\x1b[0m AUCC would hold with
threshold = 0, and \x1b[1m\x1b[36mName\x1b[0m could be used as a key:
\x1b[1m\x1b[36mName      Grade   Salary   Work_experience   
---------------------------------------------
Mark      7       1150     12                
Joyce     2       1100     5                 
Harry_1   3       1000     7                 
Grace     4       900      12                
Harry_2   4       1000     5                 
Samuel    1       900      9                 
Nancy     2       1000     3                 
\x1b[0m
Let's run algorithm once more with threshold = 0:
Found UCCs:
\t\x1b[1m\x1b[36m[Name]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Salary Work_experience]\x1b[0m
Now we can use \x1b[1m\x1b[36mName\x1b[0m as a key
'''

snapshots['test_example[basic/mining_cfd.py-None-mining_cfd_output] mining_cfd_output'] = '''options: 
MINIMUM SUPPORT = 8 , MINIMUM CONFIDENCE = 0.7 , MAXIMUM LHS COUNT = 3
displaying the first five (or fewer) discovered CFDs:

CFD:
{(3, False)} -> (4, True) :

\x1b[1;49m      Outlook Temperature Humidity  Windy   Play
\x1b[1;41m 0      sunny         hot     high  False  False \x1b[1;49m
\x1b[1;49m 1      sunny         hot     high   True  False \x1b[1;49m
\x1b[1;42m 2   overcast         hot     high  False   True \x1b[1;49m
\x1b[1;42m 3       rain        mild     high  False   True \x1b[1;49m
\x1b[1;42m 4       rain        cool   normal  False   True \x1b[1;49m
\x1b[1;49m 5       rain        cool   normal   True  False \x1b[1;49m
\x1b[1;49m 6   overcast        cool   normal   True   True \x1b[1;49m
\x1b[1;41m 7      sunny        mild     high  False  False \x1b[1;49m
\x1b[1;42m 8      sunny        cool   normal  False   True \x1b[1;49m
\x1b[1;42m 9       rain        mild   normal  False   True \x1b[1;49m
\x1b[1;49m 10     sunny        mild   normal   True   True \x1b[1;49m
\x1b[1;49m 11  overcast        mild     high   True   True \x1b[1;49m
\x1b[1;42m 12  overcast         hot   normal  False   True \x1b[1;49m
\x1b[1;49m 13      rain        mild     high   True  False \x1b[1;49m
lhs count:  1
support:  8 \x1b[1;42m   \x1b[1;41m   \x1b[1;49m
confidence:  \x1b[1;32m 6 \x1b[1;37m/ 8  =  0.7500



CFD:
{(2, _)} -> (4, _) :

\x1b[1;49m      Outlook Temperature Humidity  Windy   Play
\x1b[1;42m 0      sunny         hot     high  False  False \x1b[1;49m
\x1b[1;42m 1      sunny         hot     high   True  False \x1b[1;49m
\x1b[1;41m 2   overcast         hot     high  False   True \x1b[1;49m
\x1b[1;41m 3       rain        mild     high  False   True \x1b[1;49m
\x1b[1;42m 4       rain        cool   normal  False   True \x1b[1;49m
\x1b[1;41m 5       rain        cool   normal   True  False \x1b[1;49m
\x1b[1;42m 6   overcast        cool   normal   True   True \x1b[1;49m
\x1b[1;42m 7      sunny        mild     high  False  False \x1b[1;49m
\x1b[1;42m 8      sunny        cool   normal  False   True \x1b[1;49m
\x1b[1;42m 9       rain        mild   normal  False   True \x1b[1;49m
\x1b[1;42m 10     sunny        mild   normal   True   True \x1b[1;49m
\x1b[1;41m 11  overcast        mild     high   True   True \x1b[1;49m
\x1b[1;42m 12  overcast         hot   normal  False   True \x1b[1;49m
\x1b[1;42m 13      rain        mild     high   True  False \x1b[1;49m
lhs count:  1
support:  14 \x1b[1;42m   \x1b[1;41m   \x1b[1;49m
confidence:  \x1b[1;32m 10 \x1b[1;37m/ 14  =  0.7143



CFD:
{(4, _)} -> (2, _) :

\x1b[1;49m      Outlook Temperature Humidity  Windy   Play
\x1b[1;42m 0      sunny         hot     high  False  False \x1b[1;49m
\x1b[1;42m 1      sunny         hot     high   True  False \x1b[1;49m
\x1b[1;41m 2   overcast         hot     high  False   True \x1b[1;49m
\x1b[1;41m 3       rain        mild     high  False   True \x1b[1;49m
\x1b[1;42m 4       rain        cool   normal  False   True \x1b[1;49m
\x1b[1;41m 5       rain        cool   normal   True  False \x1b[1;49m
\x1b[1;42m 6   overcast        cool   normal   True   True \x1b[1;49m
\x1b[1;42m 7      sunny        mild     high  False  False \x1b[1;49m
\x1b[1;42m 8      sunny        cool   normal  False   True \x1b[1;49m
\x1b[1;42m 9       rain        mild   normal  False   True \x1b[1;49m
\x1b[1;42m 10     sunny        mild   normal   True   True \x1b[1;49m
\x1b[1;41m 11  overcast        mild     high   True   True \x1b[1;49m
\x1b[1;42m 12  overcast         hot   normal  False   True \x1b[1;49m
\x1b[1;42m 13      rain        mild     high   True  False \x1b[1;49m
lhs count:  1
support:  14 \x1b[1;42m   \x1b[1;41m   \x1b[1;49m
confidence:  \x1b[1;32m 10 \x1b[1;37m/ 14  =  0.7143



CFD:
{(3, _),(2, _)} -> (4, _) :

\x1b[1;49m      Outlook Temperature Humidity  Windy   Play
\x1b[1;42m 0      sunny         hot     high  False  False \x1b[1;49m
\x1b[1;42m 1      sunny         hot     high   True  False \x1b[1;49m
\x1b[1;41m 2   overcast         hot     high  False   True \x1b[1;49m
\x1b[1;41m 3       rain        mild     high  False   True \x1b[1;49m
\x1b[1;42m 4       rain        cool   normal  False   True \x1b[1;49m
\x1b[1;41m 5       rain        cool   normal   True  False \x1b[1;49m
\x1b[1;42m 6   overcast        cool   normal   True   True \x1b[1;49m
\x1b[1;42m 7      sunny        mild     high  False  False \x1b[1;49m
\x1b[1;42m 8      sunny        cool   normal  False   True \x1b[1;49m
\x1b[1;42m 9       rain        mild   normal  False   True \x1b[1;49m
\x1b[1;42m 10     sunny        mild   normal   True   True \x1b[1;49m
\x1b[1;41m 11  overcast        mild     high   True   True \x1b[1;49m
\x1b[1;42m 12  overcast         hot   normal  False   True \x1b[1;49m
\x1b[1;42m 13      rain        mild     high   True  False \x1b[1;49m
lhs count:  2
support:  14 \x1b[1;42m   \x1b[1;41m   \x1b[1;49m
confidence:  \x1b[1;32m 10 \x1b[1;37m/ 14  =  0.7143



CFD:
{(2, _),(3, False)} -> (4, _) :

\x1b[1;49m      Outlook Temperature Humidity  Windy   Play
\x1b[1;42m 0      sunny         hot     high  False  False \x1b[1;49m
\x1b[1;49m 1      sunny         hot     high   True  False \x1b[1;49m
\x1b[1;41m 2   overcast         hot     high  False   True \x1b[1;49m
\x1b[1;41m 3       rain        mild     high  False   True \x1b[1;49m
\x1b[1;42m 4       rain        cool   normal  False   True \x1b[1;49m
\x1b[1;49m 5       rain        cool   normal   True  False \x1b[1;49m
\x1b[1;49m 6   overcast        cool   normal   True   True \x1b[1;49m
\x1b[1;42m 7      sunny        mild     high  False  False \x1b[1;49m
\x1b[1;42m 8      sunny        cool   normal  False   True \x1b[1;49m
\x1b[1;42m 9       rain        mild   normal  False   True \x1b[1;49m
\x1b[1;49m 10     sunny        mild   normal   True   True \x1b[1;49m
\x1b[1;49m 11  overcast        mild     high   True   True \x1b[1;49m
\x1b[1;42m 12  overcast         hot   normal  False   True \x1b[1;49m
\x1b[1;49m 13      rain        mild     high   True  False \x1b[1;49m
lhs count:  2
support:  8 \x1b[1;42m   \x1b[1;41m   \x1b[1;49m
confidence:  \x1b[1;32m 6 \x1b[1;37m/ 8  =  0.7500


'''

snapshots['test_example[basic/mining_cfdfinder.py-None-mining_cfdfinder_output] mining_cfdfinder_output'] = '''
CFDFinder Mining Example - Desbordante

=== Note ===
CFD is a unique pattern for which there are several search algorithms,
each of which defines CFD in its own way.
Therefore, we suggest that you review all the examples of the CFD pattern
and choose your appropriate option depending on your task:

1. verifying_cfd.py
2. mining_cfd.py

=== Explain Conditional Functional Dependencies ===

Conditional Functional Dependencies (CFD) generalize traditional functional dependencies (FD)
by adding conditions to attribute values via the pattern tableau.
This allows you to find dependencies that are not performed for the entire table,
but only for a subset of rows that meet certain conditions in the data.

=== Formal Definition ===

This example examines the CFDFinder algorithm, which uses the following definition of CFD:

CFD is a pair of embedded FD and a pattern tableau:
* Embedded FD (X -> A) is a common functional dependency,
  where X (LHS) is a subset of table attributes and A (RHS) is a single attribute not contained in X.

* The pattern tableau is a table with attributes LHS and RHS in which each tuple contains
  one of the following types of conditions in each of its attributes:
    - A fixed constant value. Example: 'London'
    - A fixed negative value. Example: '¬Mark' or '¬30'
    - A range of values. Example: '[30 - 65]'
    - A wildcard symbol ('_') that allows any condition in this attribute.

For example, consider medical data (a table) containing the attributes Diagnosis,
Genetic_Marker, Blood_Type:
   * FD [Diagnosis, Blood_Type] -> [Genetic_Marker] may mean that people with the same
     diagnosis and blood type always have the same genetic marker.

   * On the other hand, CFD [Diagnosis, Blood_Type] -> [Genetic_Marker] with pattern
     tableau {(Cancer|_), (Diabetes|_)} clarifies that the previous rule applies only
     when Cancer and Diabetes are diagnosed.

=== Note ===
CFDFinder searches for CFDs, where for RHS there is always a '_' in the condition,
that is, the wildcard symbol will always be in the pattern tableau for attribute A
(therefore, the pattern tableau does not contain attribute A).

=== Key Quality Measures for CFD ===

Support: The fraction of records satisfying the condition LHS
Confidence: The fraction of records where RHS occurs given LHS

=== Algorithm Basic Parameters ===

The CFDFinder algorithm supports several use cases, and each of them has its own parameters.
To begin with, we will describe the parameters common to all scenarios.

* cfd_max_lhs: The maximum number of attributes in LHS
  - Range: from 1 to number of columns

* pli_cache_limit: The maximum number of cached plis that are used in the study
  of candidates with similar LHS
  - Range: from 1 to infinity (if memory permits)

* expansion_strategy: Defines which types of conditions can be used in the pattern tuple.
  - 'constant': Only constants and wildcard are used as conditions for attributes.

  - 'negative_constant': Similar to the 'constant' strategy, but the negation
    condition is added.

  - 'range': The condition for each attribute is represented by a range of constant.
    To do this, the attribute domain is arranged in lexicographic order.
    Also in this strategy, the conditions may have the usual constants and wildcard symbol.

* result_strategy: Defines the form in which the result of the algorithm will be obtained.
  - 'direct': The result of the algorithm will be all the CFDs found according to
    the specified parameters.

  - 'lattice': Of all the discovered CFDs, only the most general of those rules
    that have the same RHS will be included in the result.
    For example, if the algorithm finds rules [X, Y, Z] -> A, [X, Y] -> A,
    [Y, Z] -> A, [Y] -> A and [X] -> A, then only rule [X] -> A and [Y] -> A
    will be included in the result.

  - 'tree': It works similarly to the 'lattice' strategy, but it can also identify
    additional specific CFDs with high support, which are sometimes lost in strict generalization.

* pruning_strategy: Defines the various use cases of the algorithm that will be discussed further.
   - Possible values: ['legacy', 'support_independent', 'rhs_filter', 'partial_fd']

=== Bacteria Dataset Explanation ===

Let's look at an example of a dataset containing the results of experiments on
growing a strain of bacteria:

   Oxygen_Level  Temperature_C   pH Nutrient_Level Growth_Rate  Population_Count
0           Low             25  6.5            Low        Slow                15
1           Low             25  6.5         Medium      Medium                95
2           Low             25  6.5         Medium        Slow                30
3           Low             25  7.5           High   Very_Fast               525
4           Low             25  7.5           High   Very_Fast               530
5           Low             25  6.5           High        Slow                10
6        Medium             25  7.0         Medium      Medium                95
7        Medium             25  7.0         Medium      Medium               100
8        Medium             25  7.5         Medium        Slow                45
9        Medium             35  7.0           High        Fast               290
10       Medium             40  6.5            Low        Dead                 0
11       Medium             40  6.5            Low        Dead                 0
12         High             35  7.5           High   Very_Fast               500
13         High             40  7.0           High        Dead                 0

We take oxygen level, temperature, pH and nutrient level as study parameters,
and measure the growth rate and number of populations as a result.

For knowledge discovery and data quality assessment,it is
interesting for us to study these results in order to draw some conclusions
and determine the direction of the next experiments.

For example, we may be interested in the following:

* 1. Have we chosen the values of the study parameters correctly?
     Are the parameters themselves independent?

* 2. Which parameters introduce instability in growth rate predictions?

* 3. Which parameter values are the boundary values for the stability of the results.

Let's try to answer these questions using the CFD mining.

=== Legacy Strategy ===

One of the possible scenarios for using the algorithm is to search for CFDs with
minimal support and confidence thresholds. To do this, you can use the 'legacy'
pruning strategy, which takes the following parameters:

* cfd_minconf (minimal confidence):
  - Range: from 0.0 to 1.0

* cfd_minsup (minimal support):
  - Range: from 0.0 to 1.0

Let's run the algorithm with the following parameters:

  * Pruning Strategy (pruning_strategy): legacy
  * Expansion Strategy (expansion_strategy): negative_constant
  * Result Strategy (result_strategy): direct
  * Minimum Support (cfd_minsup): 0.8
  * Minimum Confidence (cfd_minconf): 1

With our parameters, the algorithm detected 3 CFDs.

Let's look at the results that express the dependence between the study parameters.

  1. [Oxygen_Level Nutrient_Level] -> Temperature_C
PatternTableau {
	(¬High|_)
}

    Support: 0.8571428571428571
    Confidence: 1.0

  This CFD indicates that:
  If the oxygen level is not 'High' and nutrient level has the same values
  then according to our data we can predict the temperature under which
  the experiment was conducted.

This dependence may indicate an error in the design of the experiments,
since temperature is not an independent variable.
It may be worth conducting additional experiments with more random parameters.

=== Support Independent and RHS Filter Strategies ===

Sometimes it is still useful to look for low-support dependencies, as they can
identify rare but interesting dependencies that are performed on a small group of records.
To find them, you can use an interactive selection of minimum support values in
the 'legacy' strategy, but this approach is not optimal.
The 'support independent' strategy is better suited for such cases.

This strategy has the following parameters:

* cfd_minconf (minimal confidence):
  - Range: from 0.0 to 1.0

* min_support_gain: the minimum number of tuples that each pattern in the
  pattern tableau must support.
  - Range: from 1 to number of rows

* max_support_drop: the maximum number of tuples by which CFD support can
  decrease when one attribute from LHS is removed from the embedded FD.
  - Range: from 1 to number of rows

* max_patterns: maximum number of rows in the pattern tableau.
  - Range: from 1 to number of rows

If we are only interested in those rules that express dependence for certain
attributes, then to reduce the running time of the algorithm, we can use the
'rhs_filter' strategy, which is an extension of the previous strategy and
adds another one to all previous parameters:

* rhs_indices: the indexes of the attributes we are interested in for the RHS.
  - Example: [1,3,5]

Let's run the algorithm with the following parameters:

  * Pruning Strategy (pruning_strategy): rhs_filter
  * Expansion Strategy (expansion_strategy): range
  * Result Strategy (result_strategy): lattice
  * Minimum Confidence (cfd_minconf): 1
  * Minimum support gain (min_support_gain): 4
  * Maximum support drop (max_support_drop): 2
  * Maximum patterns (max_patterns): 1
  * RHS indices (rhs_indices): [4]

Discovered 4 CFDs:

Let's say we're interested in the effect of temperature on growth rate.
Consider the following low-support dependencies:

  1. [Oxygen_Level Temperature_C] -> Growth_Rate
PatternTableau {
	(_|[35 - 40])
}

    Support: 0.35714285714285715
    Confidence: 1.0

  This CFD indicates that:
  At a temperature of 35-40 degrees, the combination of oxygen regime
  and temperature uniquely determines the growth rate.

  2. [Temperature_C pH] -> Growth_Rate
PatternTableau {
	([35 - 40]|_)
}

    Support: 0.35714285714285715
    Confidence: 1.0

  This CFD indicates that:
  In the same temperature range of 35-40 degrees, the combination of
  temperature and pH also uniquely determines growth.

Both rules are useful, despite the low support, because they identify a
temperature zone 35-40 degrees where the system becomes as predictable as possible
and where you can focus on one key parameter oxygen level or pH instead of
controlling all factors at the same time.

=== Partial FD Strategy ===

In addition to searching for common CFDs, the CFDFinder algorithm can be used
to mine partial FDs.

=== Formal Definition ===

Partial FD is a CFD covering the entire relation instance, i.e. those that
have a support of 1, but do have a confidence of less than 1.

To search, you can use the 'partial_fd' pruning strategy, which has a single
parameter 'max_g1'. g1 from the G family of metrics determines the fraction
of pairs of records violating embedded FD.

For example, let's run the algorithm on our bacteria dataset with the
following parameters:

  * pruning_strategy: partial_fd
  * result_strategy: lattice
  * max_g1: 0.02

Discovered 5 partial FDs:

Let's look at the rules that have the 'Growth_Rate' attribute in the dependency
defining attribute.

  1. [Oxygen_Level pH Nutrient_Level] -> Growth_Rate
PatternTableau {
	(_|_|_)
}

    Support: 1.0
    Confidence: 0.9285714285714286

  2. [Temperature_C pH Nutrient_Level] -> Growth_Rate
PatternTableau {
	(_|_|_)
}

    Support: 1.0
    Confidence: 0.9285714285714286

Let's see which entries violate the embedded FDs of these dependencies.
For example, take the following pair of records:

  Oxygen_Level  Temperature_C   pH Nutrient_Level Growth_Rate  Population_Count
1          Low             25  6.5         Medium      Medium                95
2          Low             25  6.5         Medium        Slow                30

That is, with the same dependency parameters, the growth rate is rare,
but it can be different. Perhaps this is an error in the data itself,
or the values of the parameters at which the violation occurs, however,
are near stability boundaries.
Also, the dependencies differ only in the attributes Oxygen_Level and
Temperature_C, which may indicate a strong correlation of these parameters
or similar information regarding Growth_Rate.

In any case, the analysis helped us to draw some conclusions about our data
and direct it to further experiments.

=== Note ===
For more information about mining partial fd with the g1 metric, we recommend
reading the example 'mining_afd.py'.

=== Experimentation Workflow ===

When searching for CFDs, it is usually necessary to experiment with the algorithm
parameters, since it is quite difficult to immediately get the right set for you.

If your goal is to simplely find high-support CFDs, the legacy strategy is well suited.
Recommendations for selecting parameters:
1. Start with middle support and high confidence
2. If there are few CFDs, loosen the restrictions
3. If there are a lot of CFDs, increase the LHS size limit or use lattice/tree result strategy.

If your goal is to find rare but interesting CFDs, the support independent strategy is well suited.
Recommendations for selecting parameters:
1. The parameter max_support_drop and min_support_gain strongly influence the number of discovered CFDs.
   Their values should be selected in the range from '1' value to 30 percent of the number of rows in the table.
2. Maximum patterns strongly affects the running time of the algorithm, so start with small values (for example, 2 or 3).
   This way, a tableau with a small number of patterns will be more concise.
3. Use the rhs_filter strategy if you are only interested in certain attributes for the RHS.
   This will significantly reduce the running time of the algorithm.

If your goal is to look for errors and inaccuracies in the data, a partial FDs search may be
well suited for you. You can start with small values of the max g1 parameter (for example 0.01)
and increase if necessary.

Such experiments on your data will help you discover:
  * new hidden knowledge
  * data quality issues
  * patterns useful for decision making

CFD mining example completed!

'''

snapshots['test_example[basic/mining_dd.py-None-mining_dd_output] mining_dd_output'] = '''Consider the table containing some information about flights:
+----+-----------------+------------+------------------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival                |   Distance |   Duration |
|----+-----------------+------------+------------------------+------------------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | Saint Petersburg (LED) | Moscow (SVO)           |        598 |         64 |
|  1 | FV 6015         | 2024-03-06 | Saint Petersburg (LED) | Moscow (VKO)           |        624 |         63 |
|  2 | FV 6027         | 2024-03-06 | Saint Petersburg (LED) | Moscow (SVO)           |        598 |         66 |
|  3 | FV 6024         | 2024-03-03 | Moscow (VKO)           | Saint Petersburg (LED) |        624 |         58 |
|  4 | SU 6            | 2024-03-06 | Moscow (SVO)           | Saint Petersburg (LED) |        598 |         62 |
|  5 | S7 1009         | 2024-03-01 | Moscow (DME)           | Saint Petersburg (LED) |        664 |         66 |
|  6 | S7 1010         | 2024-03-02 | Saint Petersburg (LED) | Moscow (DME)           |        664 |         70 |
|  7 | B2 978          | 2024-03-07 | Moscow (SVO)           | Minsk (MSQ)            |        641 |         58 |
|  8 | DP 967          | 2024-03-07 | Moscow (VKO)           | Minsk (MSQ)            |        622 |         73 |
|  9 | B2 981          | 2024-03-08 | Minsk (MSQ)            | Moscow (VKO)           |        622 |         61 |
| 10 | DP 261          | 2024-03-06 | Moscow (VKO)           | Kaliningrad (KGD)      |       1059 |        144 |
| 11 | DP 536          | 2024-03-05 | Kaliningrad (KGD)      | Saint Petersburg (LED) |        798 |         92 |
+----+-----------------+------------+------------------------+------------------------+------------+------------+

Here are the differential dependencies (DDs) that were discovered from this table by the SPLIT algorithm:

1)  Departure [0, 0] ; Arrival [0, 0] -> Distance [0, 50]
2)  Distance [0, 50] -> Duration [0, 15]
3)  Departure [0, 3] ; Arrival [0, 3] -> Duration [0, 15]

The DD "Departure [0, 0] ; Arrival [0, 0] -> Distance [0, 50]" means the following.

For any two tuples of the table if
a) the distance between them on the column "Departure" is between 0 and 0 (i.e. they are equal), and
b) the distance between them on the column "Arrival" is between 0 and 0 (i.e. they are equal),
then the distance between them on the column "Distance" is between 0 and 50.

The only tuple pair that satisfies both of the constraints on the left-hand side is (0,2):
+----+-----------------+------------+------------------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival                |   Distance |   Duration |
|----+-----------------+------------+------------------------+------------------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         64 |
|  1 | FV 6015         | 2024-03-06 | Saint Petersburg (LED) | Moscow (VKO)           |        624 |         63 |
|  2 | FV 6027         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         66 |
|  3 | FV 6024         | 2024-03-03 | Moscow (VKO)           | Saint Petersburg (LED) |        624 |         58 |
|  4 | SU 6            | 2024-03-06 | Moscow (SVO)           | Saint Petersburg (LED) |        598 |         62 |
|  5 | S7 1009         | 2024-03-01 | Moscow (DME)           | Saint Petersburg (LED) |        664 |         66 |
|  6 | S7 1010         | 2024-03-02 | Saint Petersburg (LED) | Moscow (DME)           |        664 |         70 |
|  7 | B2 978          | 2024-03-07 | Moscow (SVO)           | Minsk (MSQ)            |        641 |         58 |
|  8 | DP 967          | 2024-03-07 | Moscow (VKO)           | Minsk (MSQ)            |        622 |         73 |
|  9 | B2 981          | 2024-03-08 | Minsk (MSQ)            | Moscow (VKO)           |        622 |         61 |
| 10 | DP 261          | 2024-03-06 | Moscow (VKO)           | Kaliningrad (KGD)      |       1059 |        144 |
| 11 | DP 536          | 2024-03-05 | Kaliningrad (KGD)      | Saint Petersburg (LED) |        798 |         92 |
+----+-----------------+------------+------------------------+------------------------+------------+------------+

Now let\'s consider the values of this tuple pair on the column "Distance":
+----+-----------------+------------+------------------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival                |   Distance |   Duration |
|----+-----------------+------------+------------------------+------------------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        \x1b[1;31m598\x1b[0m |         64 |
|  1 | FV 6015         | 2024-03-06 | Saint Petersburg (LED) | Moscow (VKO)           |        624 |         63 |
|  2 | FV 6027         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        \x1b[1;31m598\x1b[0m |         66 |
|  3 | FV 6024         | 2024-03-03 | Moscow (VKO)           | Saint Petersburg (LED) |        624 |         58 |
|  4 | SU 6            | 2024-03-06 | Moscow (SVO)           | Saint Petersburg (LED) |        598 |         62 |
|  5 | S7 1009         | 2024-03-01 | Moscow (DME)           | Saint Petersburg (LED) |        664 |         66 |
|  6 | S7 1010         | 2024-03-02 | Saint Petersburg (LED) | Moscow (DME)           |        664 |         70 |
|  7 | B2 978          | 2024-03-07 | Moscow (SVO)           | Minsk (MSQ)            |        641 |         58 |
|  8 | DP 967          | 2024-03-07 | Moscow (VKO)           | Minsk (MSQ)            |        622 |         73 |
|  9 | B2 981          | 2024-03-08 | Minsk (MSQ)            | Moscow (VKO)           |        622 |         61 |
| 10 | DP 261          | 2024-03-06 | Moscow (VKO)           | Kaliningrad (KGD)      |       1059 |        144 |
| 11 | DP 536          | 2024-03-05 | Kaliningrad (KGD)      | Saint Petersburg (LED) |        798 |         92 |
+----+-----------------+------------+------------------------+------------------------+------------+------------+
We can notice that the distance is between 0 and 50. Therefore, the DD
"Departure [0, 0] ; Arrival [0, 0] -> Distance [0, 50]" holds in the table.

Now let\'s move to the second DD: "Distance [0, 50] -> Duration [0, 15]". This DD means the following: for any
pair of tuples if the distance between them on the column "Distance" is between 0 and 50, then the distance on
the column "Duration" is between 0 and 15. In other words, if two flights have similar distances, then they
last for a similar time. As can be seen from the table, almost all flights have similar distances which differ
by less than 50 kilometers.

Next, for all flights from 0 to 9 their durations are between 58 and 73 minutes, so the difference is less or
equal to 15 minutes. Therefore, the second DD also holds in the table.

Now consider the third DD: "Departure [0, 3] ; Arrival [0, 3] -> Duration [0, 15]". It means that for any two
tuples from the table if
a) the distance between them on the column "Departure" is between 0 and 3, and
b) on the column "Arrival" the distance is between 0 and 3,
then the distance on the column "Duration" is between 0 and 15.

The distance between two strings is their edit distance (the number of characters that need to be substituted,
deleted or inserted in order to turn the first string into the second).

The distance constraint "Departure [0, 3]" means that we consider only those tuple pairs whose values on the
column "Departure" are close enough. In this case we aim to consider the airports located in the same cities.
For example, tuple pairs (0,1) and (3,4) are satisfying this constraint.

Tuples 0 and 1 have the same departure airport:
+----+-----------------+------------+------------------------+--------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival      |   Distance |   Duration |
|----+-----------------+------------+------------------------+--------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | Moscow (SVO) |        598 |         64 |
|  1 | FV 6015         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | Moscow (VKO) |        624 |         63 |
+----+-----------------+------------+------------------------+--------------+------------+------------+

Tuples 3 and 4 have the same city but different airport codes:
+----+-----------------+------------+--------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure    | Arrival                |   Distance |   Duration |
|----+-----------------+------------+--------------+------------------------+------------+------------|
|  3 | FV 6024         | 2024-03-03 | \x1b[1;32mMoscow (VKO)\x1b[0m | Saint Petersburg (LED) |        624 |         58 |
|  4 | SU 6            | 2024-03-06 | \x1b[1;32mMoscow (SVO)\x1b[0m | Saint Petersburg (LED) |        598 |         62 |
+----+-----------------+------------+--------------+------------------------+------------+------------+

For the distance constraint "Arrival [0, 3]" the situation is similar.
Here are the tuple pairs that satisfy both of the constraints on the left-hand side:
+----+-----------------+------------+------------------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival                |   Distance |   Duration |
|----+-----------------+------------+------------------------+------------------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         64 |
|  1 | FV 6015         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (VKO)\x1b[0m           |        624 |         63 |
|  2 | FV 6027         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         66 |
|  3 | FV 6024         | 2024-03-03 | \x1b[1;33mMoscow (VKO)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        624 |         58 |
|  4 | SU 6            | 2024-03-06 | \x1b[1;33mMoscow (SVO)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        598 |         62 |
|  5 | S7 1009         | 2024-03-01 | \x1b[1;33mMoscow (DME)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        664 |         66 |
|  6 | FV 6027         | 2024-03-06 | \x1b[1;32m\x1b[1;32mSaint Petersburg (LED)\x1b[0m\x1b[0m | \x1b[1;32m\x1b[1;32mMoscow (SVO)\x1b[0m\x1b[0m           |        598 |         66 |
|  7 | B2 978          | 2024-03-07 | \x1b[1;36mMoscow (SVO)\x1b[0m           | \x1b[1;36mMinsk (MSQ)\x1b[0m            |        641 |         58 |
|  8 | DP 967          | 2024-03-07 | \x1b[1;36mMoscow (VKO)\x1b[0m           | \x1b[1;36mMinsk (MSQ)\x1b[0m            |        622 |         73 |
|  9 | B2 981          | 2024-03-08 | Minsk (MSQ)            | Moscow (VKO)           |        622 |         61 |
| 10 | DP 261          | 2024-03-06 | Moscow (VKO)           | Kaliningrad (KGD)      |       1059 |        144 |
| 11 | DP 536          | 2024-03-05 | Kaliningrad (KGD)      | Saint Petersburg (LED) |        798 |         92 |
+----+-----------------+------------+------------------------+------------------------+------------+------------+

Now let\'s consider the values of these tuple pairs on the column "Duration":
+----+-----------------+------------+------------------------+------------------------+------------+------------+
|    | Flight Number   | Date       | Departure              | Arrival                |   Distance |   Duration |
|----+-----------------+------------+------------------------+------------------------+------------+------------|
|  0 | SU 35           | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         \x1b[1;32m64\x1b[0m |
|  1 | FV 6015         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (VKO)\x1b[0m           |        624 |         \x1b[1;32m63\x1b[0m |
|  2 | FV 6027         | 2024-03-06 | \x1b[1;32mSaint Petersburg (LED)\x1b[0m | \x1b[1;32mMoscow (SVO)\x1b[0m           |        598 |         \x1b[1;32m66\x1b[0m |
|  3 | FV 6024         | 2024-03-03 | \x1b[1;33mMoscow (VKO)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        624 |         \x1b[1;33m58\x1b[0m |
|  4 | SU 6            | 2024-03-06 | \x1b[1;33mMoscow (SVO)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        598 |         \x1b[1;33m62\x1b[0m |
|  5 | S7 1009         | 2024-03-01 | \x1b[1;33mMoscow (DME)\x1b[0m           | \x1b[1;33mSaint Petersburg (LED)\x1b[0m |        664 |         \x1b[1;33m66\x1b[0m |
|  6 | FV 6027         | 2024-03-06 | \x1b[1;32m\x1b[1;32mSaint Petersburg (LED)\x1b[0m\x1b[0m | \x1b[1;32m\x1b[1;32mMoscow (SVO)\x1b[0m\x1b[0m           |        598 |         \x1b[1;32m\x1b[1;32m66\x1b[0m\x1b[0m |
|  7 | B2 978          | 2024-03-07 | \x1b[1;36mMoscow (SVO)\x1b[0m           | \x1b[1;36mMinsk (MSQ)\x1b[0m            |        641 |         \x1b[1;36m58\x1b[0m |
|  8 | DP 967          | 2024-03-07 | \x1b[1;36mMoscow (VKO)\x1b[0m           | \x1b[1;36mMinsk (MSQ)\x1b[0m            |        622 |         \x1b[1;36m73\x1b[0m |
|  9 | B2 981          | 2024-03-08 | Minsk (MSQ)            | Moscow (VKO)           |        622 |         61 |
| 10 | DP 261          | 2024-03-06 | Moscow (VKO)           | Kaliningrad (KGD)      |       1059 |        144 |
| 11 | DP 536          | 2024-03-05 | Kaliningrad (KGD)      | Saint Petersburg (LED) |        798 |         92 |
+----+-----------------+------------+------------------------+------------------------+------------+------------+

It can easily be seen that for every highlighted tuple pair their duration differs by up to 15 minutes. Therefore,
the DD "Departure [0, 3] ; Arrival [0, 3] -> Duration [0, 15]" holds in the table.

The most important parameter of the SPLIT algorithm for DD discovery is the difference table. Here is the
difference table that was used in this example:

+----+-----------------+--------+-------------+-----------+------------+------------+
|    | Flight Number   | Date   | Departure   | Arrival   | Distance   | Duration   |
|----+-----------------+--------+-------------+-----------+------------+------------|
|  0 | -----           | -----  | [0;0]       | [0;0]     | [0;50]     | [0;15]     |
|  1 | -----           | -----  | [0;3]       | [0;3]     | ------     | ------     |
+----+-----------------+--------+-------------+-----------+------------+------------+

The difference table defines the search space for DDs. That means, the algorithm searches only for DDs constructed
from distance constraints stated in the difference table. Therefore, as you can see from the discovered DDs, all
of the distance constraints that were used there are stated in the difference table.

The number of constraints for each column can be different. The difference table can be accepted by the algorithm
only in the format stated above. Note that different difference tables processed by the algorithm result in
different sections of the search space and lead to different results.

For example, consider another difference table:
+----+-----------------+--------+-------------+-----------+------------+------------+
|    | Flight Number   | Date   | Departure   | Arrival   | Distance   | Duration   |
|----+-----------------+--------+-------------+-----------+------------+------------|
|  0 | -----           | -----  | [0;0]       | [0;0]     | [0;50]     | [0;15]     |
|  1 | -----           | -----  | [0;3]       | -----     | ------     | ------     |
+----+-----------------+--------+-------------+-----------+------------+------------+

The result for the algorithm executed with this difference table is following:

1)  Departure [0, 0] ; Arrival [0, 0] -> Distance [0, 50]
2)  Distance [0, 50] -> Duration [0, 15]
3)  Departure [0, 3] ; Arrival [0, 0] -> Duration [0, 15]

Note that the distance constraint in the third DD has been changed from "Arrival [0, 3]" to "Arrival [0, 0]".
That has happened because the constraint "Arrival [0, 3]" is no more in the search space.
'''

snapshots['test_example[basic/mining_fd.py-None-mining_fd_output] mining_fd_output'] = '''FDs:
[Professor] -> Course
[Professor Semester] -> Classroom
[Classroom Semester] -> Course
[Classroom Semester] -> Professor
[Course Semester] -> Classroom
[Course Semester] -> Professor
[Course Classroom] -> Professor
'''

snapshots['test_example[basic/mining_fd_approximate.py-None-mining_fd_approximate_output] mining_fd_approximate_output'] = '''
=======================================================
This example demonstrates how Desbordante can perform
approximate functional dependency (FD) discovery
methods.
It utilizes two algorithms, EulerFD and AID-FD, which
offer significant speed advantages over exact
FD discovery methods. While these algorithms may not
identify all true FDs or might occasionally yield
false positives, they achieve substantially faster
processing times.

For more in-depth information, please refer
to the following publications:
1) EulerFD: An Efficient Double-Cycle Approximation
   of Functional Dependencies by
   Qiongqiong Lin, Yunfan Gu, Jingyan Sa et al.
2) Approximate Discovery of Functional Dependencies
   for Large Datasets by Tobias Bleifuss,
   Susanne Bulow, Johannes Frohnhofen et al.
=======================================================

We will now demonstrate how to invoke EulerFD and
AID-FD in Desbordante.

EulerFD: 
[name] -> age
[name] -> blood
[name] -> gender
[name] -> medicine
[age medicine] -> blood
[age blood] -> medicine
[age gender medicine] -> name
[age blood gender] -> name
-------------------------------
AID-FD: 
[name] -> age
[name] -> blood
[name] -> gender
[name] -> medicine
[age medicine] -> blood
[age blood] -> medicine
[age gender medicine] -> name
[age blood gender] -> name
In the advanced section, a more complex example will showcase additional features of the algorithms.
'''

snapshots['test_example[basic/mining_gfd/mining_gfd1.py-None-mining_gfd1_output] mining_gfd1_output'] = '''Our profiler supports two tasks related to graph functional dependencies (GFDs): validation and mining (discovery). In this example, we will focus on the mining task (for validation, we refer the reader to another example). The mining algorithm used in our profiler is described in the article "Discovering Graph Functional Dependencies" by Fan Wenfei, Hu Chunming, Liu Xueli, and Lu Pinge, presented at SIGMOD \'18.

GFDs are functional dependencies that consist of a pattern - a graph that specifies the scope - and a rule. The nature of this object will become clearer through the example that follows.

Let's analyze GFD mining through an example. Look at the graph presented on the top left in the figure. It describes the connections between scientific articles and their authors. The vertices of this graph have two labels: \x1b[38;2;173;255;47mArticle (A)\x1b[0m and \x1b[38;2;46;139;87mPerson (P)\x1b[0m. Each vertex has its own set of attributes depending on the label.

\x1b[38;2;173;255;47mArticle\x1b[0m:
- \x1b[38;2;173;255;47mtitle\x1b[0m denotes the title of the article.

\x1b[38;2;46;139;87mPerson\x1b[0m:
- \x1b[38;2;46;139;87mname\x1b[0m denotes the name of a person,
- \x1b[38;2;46;139;87mrole\x1b[0m can take one of two values: "teacher" or "student".

The discovery algorithm, in addition to the graph, takes two parameters as input:
- k: the maximum number of vertices in the pattern,
- sigma: the minimum frequency of GFD occurrences in the original graph.

Let's run the algorithm and look at the result. We will set k=3 and sigma=2.

\x1b[95mDesbordante > \x1b[0mMined GFDs: 1

Let's print found dependency (in DOT language):

1.role=teacher 
graph G {
0[label=article];
1[label=person];
2[label=article];
0--1 [label="*"];
1--2 [label="*"];
}

It may be difficult to interpret, so let\'s rewrite it to a more human-readable format. Note that the empty line immediately following the colon (":") indicates that the left-hand side of the dependency has no conditions. Conversely, if the right-hand side of the dependency had no conditions, the second line would be empty.

      \x1b[38;2;173;255;47m0\x1b[0m    \x1b[38;2;46;139;87m1\x1b[0m    \x1b[38;2;173;255;47m2\x1b[0m
     \x1b[38;2;173;255;47m(A)\x1b[0m--\x1b[38;2;46;139;87m(P)\x1b[0m--\x1b[38;2;173;255;47m(A)\x1b[0m
{} --> {\x1b[38;2;46;139;87m1\x1b[0m.\x1b[38;2;46;139;87mrole\x1b[0m=teacher}

The mined dependency can also be seen on the right in the figure.

The discovered dependency can be expressed as the following fact: If a person has two published articles, then they are a teacher.

It is recommended to look at the second example for a deeper understanding of graph functional dependency mining. It is located in the file "mining_gfd2.py".

\x1b[93mClose the image window to finish.\x1b[0m
'''

snapshots['test_example[basic/mining_gfd/mining_gfd2.py-None-mining_gfd2_output] mining_gfd2_output'] = '''Our profiler supports two tasks related to graph functional dependencies (GFDs): validation and mining (discovery). In this example, we will focus on the mining task (for validation, we refer the reader to another example). The mining algorithm used in our profiler is described in the article "Discovering Graph Functional Dependencies" by Fan Wenfei, Hu Chunming, Liu Xueli, and Lu Pinge, presented at SIGMOD \'18.

GFDs are functional dependencies that consist of a pattern - a graph that specifies the scope - and a rule. The nature of this object will become clearer through the example that follows.

Let's analyze GFD mining through an example. Look at the graph presented on the top left in the figure. It describes the connections between students and tasks. The vertices of this graph have two labels: \x1b[38;2;254;136;99mStudent (S)\x1b[0m and \x1b[38;2;87;206;235mTask (T)\x1b[0m. Each vertex has its own set of attributes depending on the label.

\x1b[38;2;254;136;99mStudent\x1b[0m:
- \x1b[38;2;254;136;99mname\x1b[0m denotes the name of the student,
- \x1b[38;2;254;136;99mdegree\x1b[0m is the level of education,
- \x1b[38;2;254;136;99myear\x1b[0m is the year of study.

\x1b[38;2;87;206;235mTask\x1b[0m:
- \x1b[38;2;87;206;235mname\x1b[0m denotes the name of a task,
- \x1b[38;2;87;206;235mdifficulty\x1b[0m is a categorical parameter that takes one of the following values: "easy", "normal" or "hard".

The discovery algorithm, in addition to the graph, takes two parameters as input:
- k: the maximum number of vertices in the pattern,
- sigma: the minimum frequency of GFD occurrences in the original graph.

Let's run the algorithm and look at the result. We will set k=2 and sigma=3.

\x1b[95mDesbordante > \x1b[0mMined GFDs: 1

Let's print found dependency (in DOT language):
0.difficulty=hard 
1.degree=master 1.year=2 
graph G {
0[label=task];
1[label=student];
0--1 [label=performs];
}

It may be difficult to interpret, so let's rewrite it to a more human-readable format. Notation: the first line contains the literals found in the left-hand side. The second line contains those in the right-hand side.

                       \x1b[38;2;87;206;235m0\x1b[0m    \x1b[38;2;254;136;99m1\x1b[0m
                      \x1b[38;2;87;206;235m(T)\x1b[0m--\x1b[38;2;254;136;99m(S)\x1b[0m
{\x1b[38;2;87;206;235m0\x1b[0m.\x1b[38;2;87;206;235mdifficulty\x1b[0m=hard} --> {\x1b[38;2;254;136;99m1\x1b[0m.\x1b[38;2;254;136;99mdegree\x1b[0m=master & \x1b[38;2;254;136;99m1\x1b[0m.\x1b[38;2;254;136;99myear\x1b[0m=2}

The mined dependency can also be seen on the right in the figure.

The dependency found indicates that only second-year master's students are working on the difficult task.

It is recommended to look at the first example for a deeper understanding of graph functional dependency mining. It is located in the file "mining_gfd1.py".

\x1b[93mClose the image window to finish.\x1b[0m
'''

snapshots['test_example[basic/mining_ind.py-None-mining_ind_output] mining_ind_output'] = '''Found inclusion dependencies (-> means "is included in"):

(course.csv, [Department name]) -> (department.csv, [Department name])
(instructor.csv, [Department name]) -> (department.csv, [Department name])
(student.csv, [Department name]) -> (department.csv, [Department name])
(teaches.csv, [Instructor ID]) -> (instructor.csv, [ID])
(teaches.csv, [Course ID]) -> (course.csv, [Course ID])

Tables for first IND:
course.csv:

Course ID   Title              Department name                       
---------------------------------------------------------------------
IT-1        Computer Science   Institute of Information Technology   
MM-3        Algebra            Mathematics and Mechanics Faculty     
H-1         History            Institute of History                  
FL-2        English            Faculty of Foreign Languages          
IT-2        Programming        Institute of Information Technology   
S-5         Philosophy         Faculty of Sociology                  
P-2         Physics            Faculty of Physics                    
C-8         Chemistry          Institute of Chemistry                

department.csv:

Department name                       Building             
-----------------------------------------------------------
Institute of Information Technology   5 Academic av.       
Mathematics and Mechanics Faculty     3 Academic av.       
Institute of History                  29A University st.   
Faculty of Foreign Languages          10 Science sq.       
Faculty of Sociology                  29C University st.   
Faculty of Physics                    10 Academic av.      
Institute of Chemistry                11 Academic av.      
Graduate School of Managemment        49 Science sq.       
'''

snapshots['test_example[basic/mining_list_od.py-None-mining_list_od_output] mining_list_od_output'] = '''
+----+----------+-----------------+--------+
|    |   weight |   shipping cost |   days |
|----+----------+-----------------+--------|
|  0 |        5 |              14 |      2 |
|  1 |       10 |              22 |      6 |
|  2 |        3 |              10 |      4 |
|  3 |       10 |              25 |      7 |
|  4 |        5 |              14 |      2 |
|  5 |       20 |              40 |      8 |
+----+----------+-----------------+--------+

Resulting dependencies for this table are:
['shipping cost'] -> ['weight', 'days']
['weight', 'days'] -> ['shipping cost']
['weight'] -> ['shipping cost']

Depenency [weight] -> [shipping cost] means that ordering table by weight
will also order table by shipping cost automatically. Let's order by weight: 

+----+----------+-----------------+--------+
|    |   weight |   shipping cost |   days |
|----+----------+-----------------+--------|
|  2 |        3 |              10 |      4 |
|  0 |        5 |              14 |      2 |
|  4 |        5 |              14 |      2 |
|  1 |       10 |              22 |      6 |
|  3 |       10 |              25 |      7 |
|  5 |       20 |              40 |      8 |
+----+----------+-----------------+--------+

We can see that shipping cost is sorted too. And dependency seems reasonable:
the more the package weights, the more expensive it will be to send it.

Order dependencies are called lexicographical, because ordering for multiple
columns is lexicographical. For example [shipping cost] -> [weight, days] implies
that ordering by shipping cost will also lexicographically order [weight, days]:

+----+----------+-----------------+--------+
|    |   weight |   shipping cost |   days |
|----+----------+-----------------+--------|
|  2 |        3 |              10 |      4 |
|  0 |        5 |              14 |      2 |
|  4 |        5 |              14 |      2 |
|  1 |       10 |              22 |      6 |
|  3 |       10 |              25 |      7 |
|  5 |       20 |              40 |      8 |
+----+----------+-----------------+--------+

'''

snapshots['test_example[basic/mining_md.py-None-mining_md_output] mining_md_output'] = '''In this example we are discovering MDs as defined in "Efficient Discovery of Matching Dependencies" by Schirmer et al. Initially, we define columns the values of which are going to be compared and the measure according to which similarity of values is going to be determined. The HyMD algorithm then finds the set of decision boundaries of all MDs that are enough to infer MDs that satisfy some requirements (interestingness criteria) and hold on the data.
First, the animals_beverages dataset will be inspected.
       name     zoo animal  diet
0     Simba  berlin   lion  meat
1  Clarence  london   lion  mead
2     Baloo  berlin   bear  fish
3      Pooh  london   beer  fish

In this example, we are going to compare values of every column to itself using normalized Levenshtein distance.
Searching for MDs...
Found MDs:
0 [ levenshtein(diet, diet)>=0.75 ] -> levenshtein(animal, animal)>=0.75
1 [ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75
These MDs can also be displayed in short form, showing only non-zero decision boundaries:
[,,,0.75]->2@0.75
[,,0.75,]->3@0.75

--------------------------------------------------------------------------------

Let's now look at the carrier_merger dataset, obtained as a result of merger of data from two aircraft carriers (ac1 and ac2).
 id Source             From               To  Distance (km)
  1    ac1 Saint-Petersburg         Helsinki            315
  2    ac2    St-Petersburg         Helsinki            301
  3    ac2           Moscow    St-Petersburg            650
  4    ac2           Moscow    St-Petersburg            638
  5    ac1           Moscow Saint-Petersburg            670
  6    ac1           Moscow    Yekaterinburg           1417
  7    ac2        Trondheim       Copenhagen            877
  8    ac1       Copenhagen        Trondheim            877
  9    ac2          Dobfany         Helsinki           1396
 10    ac2    St-Petersburg         Kostroma            659
 11    ac2    St-Petersburg           Moscow            650
 12    ac1           Varstu         Helsinki            315

Now we are going to define the comparisons:
1) IDs and sources are considered similar if they are equal.
2) Departure ("From" column) and arrival ("To" column) city names are going to be compared to themselves ("From" to "From", "To" to "To") and to each other ("To" to "From", "From" to "To") using the Jaccard metric.
3) Distances are going to be compared to each other using normalized difference: 1 - |dist1 - dist2| / max_distance.
Searching for MDs...
Found MDs:
0 [ jaccard(To, To)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.991531 ] -> equality(Source, Source)>=1
1 [ jaccard(From, From)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.991531 ] -> equality(Source, Source)>=1
2 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.977417
3 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.99012
4 [ jaccard(From, From)>=1 | normalized_distance(Distance (km), Distance (km))>=0.99012 ] -> equality(Source, Source)>=1
5 [ jaccard(From, From)>=1 | jaccard(To, To)>=1 ] -> equality(Source, Source)>=1
6 [ jaccard(From, From)>=1 | jaccard(To, To)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.991531
7 [ equality(Source, Source)>=1 | jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.991531
It is clear to see that ID determines every other attribute, but there are no dependencies that indicate that.
In the same manner, one would expect names of departure and arrival cities being similar to indicate distances also being similar. There is indeed a dependency like that, which is dependency 2. That dependency matches the "To" and "From" values to themselves. However, it also makes sense for there to be a dependency that matches a "To" value to a "From" value or the other way around.
And yet, none of these dependencies are presented in the answer. This is because they do not satisfy an interestingness criterion: their support is too low.
"Support" in this case means the number of record pairs with similar values.
By default, when there is only one source table, the minimum support is set to one greater than its number of records. As their support is lower than that, these dependencies are pruned.

Let's decrease the minimum support to 6.
Searching for MDs...
Found MDs:
0 [ equality(id, id)>=1 ] -> equality(Source, Source)>=1
1 [ equality(id, id)>=1 ] -> jaccard(From, From)>=1
2 [ equality(id, id)>=1 ] -> jaccard(To, To)>=1
3 [ equality(id, id)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=1
4 [ jaccard(To, From)>=0.769231 | jaccard(From, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.985886
5 [ jaccard(To, From)>=1 | jaccard(From, To)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.991531
6 [ jaccard(To, To)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.991531 ] -> equality(Source, Source)>=1
7 [ jaccard(From, From)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.991531 ] -> equality(Source, Source)>=1
8 [ jaccard(From, From)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=1 ] -> equality(id, id)>=1
9 [ jaccard(From, From)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=1 ] -> jaccard(To, To)>=1
10 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.977417
11 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.99012
12 [ jaccard(From, From)>=1 | normalized_distance(Distance (km), Distance (km))>=0.99012 ] -> equality(Source, Source)>=1
13 [ jaccard(From, From)>=1 | jaccard(To, To)>=1 ] -> equality(Source, Source)>=1
14 [ jaccard(From, From)>=1 | jaccard(To, To)>=1 ] -> normalized_distance(Distance (km), Distance (km))>=0.991531
15 [ equality(Source, Source)>=1 | jaccard(From, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.749471
16 [ equality(Source, Source)>=1 | jaccard(To, From)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.749471
17 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 | normalized_distance(Distance (km), Distance (km))>=0.992237 ] -> equality(id, id)>=1
18 [ equality(Source, Source)>=1 | jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.991531
Now these dependencies are present, they are the first five of the ones displayed.
However, there also several dependencies that "do not make sense", like "the departure city and closeness in distance determines the arrival city". These only hold because the dataset being inspected does not happen to contain a counterexample.

We can also increase the minimum support requirement. This can help us find the dependencies that are more reliable, with more examples supporting them.
Searching for MDs...
Found MDs:
0 [ jaccard(From, From)>=0.769231 | jaccard(To, To)>=0.769231 ] -> normalized_distance(Distance (km), Distance (km))>=0.977417
'''

snapshots['test_example[basic/mining_nar.py-None-mining_nar_output] mining_nar_output'] = '''Numerical Association Rules (NAR) are an extension of traditional Association Rules (AR), which help to discover patterns in data. Unlike ARs, which work with binary attributes (e.g., whether an item was purchased or not), NARs can handle numerical data (e.g., how many units of an item were purchased). This makes NARs more flexible for discovering relationships in datasets with numerical data.
Suppose we have a table containing students' exam grades and how many hours they studied for the exam. Such a table might hold the following numerical association rule:

\x1b[1mStudy_Hours[15.5 - 30.2] \x1b[34m⎤-Antecedent
\x1b[39mSubject[Topology]        \x1b[34m⎦
\x1b[39m      |
      |
      V
Grade[3 - 5]             \x1b[34m]-Consequent
\x1b[39m   support = 0.21
   confidence = 0.93

\x1b[0mThis rule states that students who study Topology for between 15.5 and 30.2 hours will receive a grade between 3 and 5. This rule has support of 0.21, which means that 21% of rows in the dataset satisfy both the antecedent's and consequent's requirements. This rule also has confidence of 0.93, meaning that 93% of rows that satisfy the antecedent also satisfy the consequent. Note that attributes can be integers, floating point numbers, or strings.

Desbordante implements an algorithm called "Differential Evolution Solver" (DES), described by Iztok Fister et al. in "uARMSolver: A framework for Association Rule Mining". It is a nature-inspired stochastic optimization algorithm.

As a demonstration of working with some of DES' parameters, let's inspect a dataset containing information about 159 dog breeds.

Fragment of the dog_breeds.csv table:
                            Name       Origin  ... Weight  Training Difficulty
0                  Affenpinscher      Germany  ...    4.0                    6
1                   Afghan Hound  Afghanistan  ...   25.0                    8
2               Airedale Terrier      England  ...   21.0                    6
3                          Akita        Japan  ...   45.0                    9
4               Alaskan Malamute   Alaska USA  ...   36.0                    8
..                           ...          ...  ...    ...                  ...
154             Wire Fox Terrier      England  ...    8.0                    7
155         Wirehaired Dachshund      Germany  ...    8.0                    7
156  Wirehaired Pointing Griffon  Netherlands  ...   20.0                    6
157               Xoloitzcuintli       Mexico  ...   25.0                    6
158            Yorkshire Terrier      England  ...    2.5                    6

[159 rows x 14 columns]

A fragment of the table is presented above. In total, each dog breed has 14 attributes.
Now, let's mine some NARs. We will use a minimum support of 0.1 and a minimum confidence of 0.7. We will also use a population size of 500 and max_fitness_evaluations of 700. Larger values for max_fitness_evaluations tend to return larger rules encompassing more attributes. The population size parameter affects the number of NARs being generated and mutated. Larger values are slower but output more NARs.

Finally, as the DES algorithm is a randomized one, we need to set the seed parameter to the specially-selected value in order: 
1) to present you an interesting and illustrative example of NAR and, 
2) to ensure the repeatability of this example (i.e., that NAR found stays the same over different runs) 
Note that if you do not set the seed parameter, the default value would be used.
NAR 1:\x1b[1m
Type[Hound]
      |
      |
      V
Intelligence[5 - 8]
Friendliness[5 - 9]
   support = 0.16352201257861634
   confidence = 0.9629629629629629
\x1b[0m

The above NAR is one of the 2 rules discovered with these settings. The NAR states that about 96% of all dog breeds of type 'Hound' have an intelligence rating between 5 and 8 out of 10 and have a friendliness rating between 5 and 9 out of 10. This suggests that, in general, hounds are intelligent dogs and are mostly friendly. Let's see if that is true.

                             Name   Type  Intelligence  Friendliness
\x1b[41m1                    Afghan Hound  Hound             4             5\x1b[49m
7               American Foxhound  Hound             6             8
11                        Basenji  Hound             6             6
12                   Basset Hound  Hound             5             8
13                         Beagle  Hound             7             9
18                     Bloodhound  Hound             6             7
21                         Borzoi  Hound             6             6
44                      Dachshund  Hound             7             7
50               English Foxhound  Hound             6             7
70                      Greyhound  Hound             7             7
71                        Harrier  Hound             6             8
73                   Ibizan Hound  Hound             7             7
79                Irish Wolfhound  Hound             6             7
101            Norwegian Elkhound  Hound             7             7
104                    Otterhound  Hound             6             7
108  Petit Basset Griffon Vendeen  Hound             6             7
109                 Pharaoh Hound  Hound             8             7
115            Portuguese Podengo  Hound             8             7
121             Redbone Coonhound  Hound             6             8
122           Rhodesian Ridgeback  Hound             8             6
125                        Saluki  Hound             7             6
128            Scottish Deerhound  Hound             6             6
136                       Sloughi  Hound             7             6
146                Thai Ridgeback  Hound             7             6
149      Treeing Walker Coonhound  Hound             7             8
153                       Whippet  Hound             8             7
155          Wirehaired Dachshund  Hound             7             7

As observed, only 1 row with 'Type' equal to 'Hound' falls outside either the intelligence or friendliness bounds. This record accounts for the (27-1)/27 ~= 96% confidence level of this rule.

Let's try again, but this time with different settings. This time, minimum support will have a more lenient value of 0.05 and the population size will be 700. This will help discover more NARs. The value of max_fitness_evaluations will also need to be increased to 1500 in accordance with the population size to produce a non-empty result.

NAR 1:\x1b[1m
Intelligence[4 - 10]
Shedding[Moderate]
      |
      |
      V
Friendliness[6 - 10]
Life Span[9 - 16]
   support = 0.5660377358490566
   confidence = 0.9574468085106383
\x1b[0m
NAR 2:\x1b[1m
Health Issues Risk[Moderate]
Life Span[8 - 14]
      |
      |
      V
Friendliness[5 - 8]
   support = 0.33962264150943394
   confidence = 0.7714285714285715
\x1b[0m
NAR 3:\x1b[1m
Size[1 - 2]
Intelligence[5 - 8]
Grooming Needs[Moderate]
Weight[15.246273 - 68.261820]
      |
      |
      V
Shedding[Moderate]
   support = 0.05660377358490566
   confidence = 0.9
\x1b[0m
NAR 4:\x1b[1m
Friendliness[5 - 10]
Exercise Requirements[1.708423 - 2.261994]
Type[Working]
      |
      |
      V
Life Span[10 - 16]
Training Difficulty[4 - 9]
   support = 0.08176100628930817
   confidence = 0.7222222222222222
\x1b[0m
These found NARs are less striking, but nevertheless they represent some thought-provoking facts.
'''

snapshots['test_example[basic/mining_pfd.py-None-mining_pfd_output] mining_pfd_output'] = '''per_value pFDs:
[Y] -> X
per_tuple pFDs:
'''

snapshots['test_example[basic/mining_set_od_1.py-None-mining_set_od_1_output] mining_set_od_1_output'] = '''+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  0 |   2020 |               24 |         1000 |
|  1 |   2020 |               40 |         7000 |
|  2 |   2020 |               32 |         5000 |
|  3 |   2020 |               29 |         3000 |
|  4 |   2020 |               49 |        10000 |
|  5 |   2021 |               50 |        15000 |
|  6 |   2021 |               25 |         1500 |
|  7 |   2021 |               30 |         6000 |
+----+--------+------------------+--------------+

Attribute symbols:
year -- 1
employee_grade -- 2
avg_salary -- 3

descending ods: 0

ascending ods: 2
{1} : 2<= ~ 3<=
{1} : 3<= ~ 2<=

Dependency "{1} : 2<= ~ 3<=" means that ordering the table
inside each equivalence class from "year" by attribute "avg_salary"
automatically entails ordering by attribute "employee_grade".

We have 2 equivalence classes in "year": [2020] and [2021].
Let's split the table into two tables based on these classes.

Part 1: this part of table corresponds to class [2020]
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  0 |   2020 |               24 |         1000 |
|  1 |   2020 |               40 |         7000 |
|  2 |   2020 |               32 |         5000 |
|  3 |   2020 |               29 |         3000 |
|  4 |   2020 |               49 |        10000 |
+----+--------+------------------+--------------+

Let\'s sort it by attribute "avg_salary".

Sorted part 1:
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  0 |   2020 |               24 |         1000 |
|  3 |   2020 |               29 |         3000 |
|  2 |   2020 |               32 |         5000 |
|  1 |   2020 |               40 |         7000 |
|  4 |   2020 |               49 |        10000 |
+----+--------+------------------+--------------+

We can see that this sort entails automatic ordering by
attribute "employee_grade".

Part 2: this part of table corresponds to class [2021]
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  5 |   2021 |               50 |        15000 |
|  6 |   2021 |               25 |         1500 |
|  7 |   2021 |               30 |         6000 |
+----+--------+------------------+--------------+

Let\'s sort it by attribute "avg_salary".

Sorted part 2:
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  6 |   2021 |               25 |         1500 |
|  7 |   2021 |               30 |         6000 |
|  5 |   2021 |               50 |        15000 |
+----+--------+------------------+--------------+

We can see that this sort entails automatic ordering by
attribute "employee_grade" too.

Dependency "{1} : 3<= ~ 2<=" is similar to the first and means that
ordering the table inside each equivalence class from "year" by
attribute "employee_grade" automatically entails ordering by
attribute "avg_salary". This can be seen in the tables above.

In other words, these dependencies indicate that the ordering of
average salary entails an automatic ordering of the employee grade
and vice versa.

simple ods: 4
{2} : [] -> 1<=
{3} : [] -> 1<=
{3} : [] -> 2<=
{2} : [] -> 3<=

These dependencies mean that inside each equivalence class from
an attribute from their context the constancy of the attribute
from the right side of the dependency can be traced.

For example, let\'s look at "{2} : [] -> 1<=". The context of this
dependency is attribute "employee_grade". We have 8 equivalence classes
in "employee_grade": [24], [40], [32], [29], [49], [50], [25], [30].
Since all the elements of attribute "employee_grade" are different,
each of these classes contains only one element, so constancy within
each class occurs automatically.

To better understand such dependencies, refer to the second example.
'''

snapshots['test_example[basic/mining_set_od_2.py-None-mining_set_od_2_output] mining_set_od_2_output'] = '''+----+--------+------------+-----------+
|    |   year | position   | percent   |
|----+--------+------------+-----------|
|  0 |   2020 | director   | 10%       |
|  1 |   2020 | other      | 50%       |
|  2 |   2020 | manager    | 40%       |
|  3 |   2021 | manager    | 35%       |
|  4 |   2021 | other      | 55%       |
|  5 |   2021 | director   | 10%       |
+----+--------+------------+-----------+

Attribute symbols:
year -- 1
position -- 2
percent -- 3

descending ods: 0

ascending ods: 2
{} : 3<= ~ 2<=
{} : 2<= ~ 3<=

Dependency "{} : 3<= ~ 2<=" means that ordering the table by attribute
"percent" automatically entails ordering by attribute "position".
Moreover, this is observed regardless of other attributes, since the
dependency context is empty.

Let\'s sort it by attribute "percent".

Sorted table:
+----+--------+------------+-----------+
|    |   year | position   | percent   |
|----+--------+------------+-----------|
|  0 |   2020 | director   | 10%       |
|  5 |   2021 | director   | 10%       |
|  3 |   2021 | manager    | 35%       |
|  2 |   2020 | manager    | 40%       |
|  1 |   2020 | other      | 50%       |
|  4 |   2021 | other      | 55%       |
+----+--------+------------+-----------+

We can see that this sort entails automatic ordering by attribute
"position".

Dependency "{} : 2<= ~ 3<=" is similar to the first and means that
ordering the table by attribute "position" automatically entails
ordering by attribute "percent". This can be seen in the table above.

In other words, these dependencies indicate that the ordering of
percents entails an automatic ordering of the positions and vice
versa.

simple ods: 2
{3} : [] -> 2<=
{1,2} : [] -> 3<=

Dependency "{3} : [] -> 2<=" means that inside each equivalence
class from "percent" the constancy of the attribute "position" can
be traced.

We have 5 equivalence classes in "percent":
class [10%] with 2 elements
class [50%] with 1 element
class [40%] with 1 element
class [35%] with 1 element
class [55%] with 1 element
class [10%] with 2 elements

This table shows the constancy of values from attribute "position"
within each equivalence class from "percent". For clarity, lines
containing different equivalence classes are colored differently.

+--------+------------+-----------+
|   year | position   | percent   |
|--------+------------+-----------|
|   \x1b[1;31m2020\x1b[0m | \x1b[1;31mdirector\x1b[0m   | \x1b[1;31m10%\x1b[0m       |
|   \x1b[32m2020\x1b[0m | \x1b[32mother\x1b[0m      | \x1b[32m50%\x1b[0m       |
|   \x1b[33m2020\x1b[0m | \x1b[33mmanager\x1b[0m    | \x1b[33m40%\x1b[0m       |
|   \x1b[34m2021\x1b[0m | \x1b[34mmanager\x1b[0m    | \x1b[34m35%\x1b[0m       |
|   \x1b[35m2021\x1b[0m | \x1b[35mother\x1b[0m      | \x1b[35m55%\x1b[0m       |
|   \x1b[1;31m2021\x1b[0m | \x1b[1;31mdirector\x1b[0m   | \x1b[1;31m10%\x1b[0m       |
+--------+------------+-----------+

Dependency "{1,2} : [] -> 3<=" contains 2 attributes ("year" and
"position") in its context and means the following: in the context
of one year and one position the constancy of percents is observed.
That is, in those tuples in which the year and position are the same,
the same percent value is observed.

The following table shows these observations.

+--------+------------+-----------+
|   year | position   | percent   |
|--------+------------+-----------|
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[32m2020\x1b[0m | \x1b[32mother\x1b[0m      | \x1b[32m50%\x1b[0m       |
|   \x1b[33m2020\x1b[0m | \x1b[33mmanager\x1b[0m    | \x1b[33m40%\x1b[0m       |
|   \x1b[34m2021\x1b[0m | \x1b[34mmanager\x1b[0m    | \x1b[34m35%\x1b[0m       |
|   \x1b[35m2021\x1b[0m | \x1b[35mother\x1b[0m      | \x1b[35m55%\x1b[0m       |
|   \x1b[36m2021\x1b[0m | \x1b[36mdirector\x1b[0m   | \x1b[36m10%\x1b[0m       |
+--------+------------+-----------+

Consider the following two tables. In the first, dependency
"{1,2} : [] -> 3<=" continues to exist. But in the second one no
longer exists, since it is violated in third tuple, where the pair
(2020, director) corresponds to 20%.

Dependency "{1,2} : [] -> 3<=" continues to exist:
+--------+------------+-----------+
|   year | position   | percent   |
|--------+------------+-----------|
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[32m2020\x1b[0m | \x1b[32mother\x1b[0m      | \x1b[32m50%\x1b[0m       |
|   \x1b[33m2020\x1b[0m | \x1b[33mmanager\x1b[0m    | \x1b[33m40%\x1b[0m       |
|   \x1b[34m2021\x1b[0m | \x1b[34mmanager\x1b[0m    | \x1b[34m35%\x1b[0m       |
|   \x1b[35m2021\x1b[0m | \x1b[35mother\x1b[0m      | \x1b[35m55%\x1b[0m       |
|   \x1b[36m2021\x1b[0m | \x1b[36mdirector\x1b[0m   | \x1b[36m10%\x1b[0m       |
+--------+------------+-----------+

Dependency "{1,2} : [] -> 3<=" no longer exists:
+--------+------------+-----------+
|   year | position   | percent   |
|--------+------------+-----------|
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[31m2020\x1b[0m | \x1b[31mdirector\x1b[0m   | \x1b[31m10%\x1b[0m       |
|   \x1b[1;4;31m2020\x1b[0m | \x1b[1;4;31mdirector\x1b[0m   | \x1b[1;4;31m20%\x1b[0m       |
|   \x1b[32m2020\x1b[0m | \x1b[32mother\x1b[0m      | \x1b[32m50%\x1b[0m       |
|   \x1b[33m2020\x1b[0m | \x1b[33mmanager\x1b[0m    | \x1b[33m40%\x1b[0m       |
|   \x1b[34m2021\x1b[0m | \x1b[34mmanager\x1b[0m    | \x1b[34m35%\x1b[0m       |
|   \x1b[35m2021\x1b[0m | \x1b[35mother\x1b[0m      | \x1b[35m55%\x1b[0m       |
|   \x1b[36m2021\x1b[0m | \x1b[36mdirector\x1b[0m   | \x1b[36m10%\x1b[0m       |
+--------+------------+-----------+
'''

snapshots['test_example[basic/mining_ucc.py-None-mining_ucc_output] mining_ucc_output'] = '''\x1b[1m\x1b[36mThis example illustrates the usage of exact Unique Column Combinations (UCC).
Intuitively, a UCC declares that some columns uniquely identify every tuple in a table.
For more information consult "A Hybrid Approach for Efficient Unique Column Combination Discovery"
by T. Papenbrock and F. Naumann.
\x1b[0m
The following table contains records about employees:
\x1b[1m\x1b[36mFirst_name   Last_name   Grade   Salary   Work_experience   
------------------------------------------------------------
Mark         Harris      7       1150     12                
Joyce        Harris      2       1100     5                 
Harry        Roberts     3       1000     7                 
Grace        Brown       4       900      12                
Harry        Walker      4       1000     5                 
Samuel       Brown       1       900      9                 
Nancy        Adams       2       1000     3                 
\x1b[0mWe need to select a column or a combination of columns that will serve as a unique key (ID).

Let's run UCC mining algorithm:
Found UCCs:
\t\x1b[1m\x1b[36m[First_name Last_name]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Last_name Grade]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Salary]\x1b[0m
\t\x1b[1m\x1b[36m[First_name Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Last_name Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Grade Work_experience]\x1b[0m
\t\x1b[1m\x1b[36m[Salary Work_experience]\x1b[0m

There are no unary UCCs, so there is no single column that can define a key.
We need to select a combination of two columns, that will serve as an ID.
\x1b[1m\x1b[36m[First_name Last_name]\x1b[0m is a good candidate.
'''

snapshots['test_example[basic/verifying_aod.py-None-verifying_aod_output] verifying_aod_output'] = '''This example verifies set-based Order Dependencies (ODs).
Set-based ODs were first introduced in the paper [1] Jaroslaw Szlichta, Parke
Godfrey, Lukasz Golab, Mehdi Kargar, and Divesh Srivastava. 2017. Effective and
complete discovery of order dependencies via set-based axiomatization. Proc.
VLDB Endow. 10, 7 (March 2017), 721–732.
https://doi.org/10.14778/3067421.3067422
This example is based on the subsequent work [2] Karegar, Reza et al. “Efficient
Discovery of Approximate Order Dependencies.” ArXiv abs/2101.02174 (2021): n.
pag.

First, let's introduce the required definitions from paper [2]. Note that there
is some discrepancy between definitions in [1] and [2]. In this example, we
adhere to the definitions and notations from [2].

Definition 1. Let X and Y be lists of attributes in a table. X -> Y denotes an
Order Dependency. If t is a tuple, let t[X] denote a projection of t onto X. The
dependency X -> Y holds if and only if for any two tuples s and t, s[X] <= t[X]
implies s[Y] <= t[Y].

In simpler terms, if the dependency X->Y holds for a table r, it means that
sorting the table by the attribute list X also guarantees to sort it by the
attribute list Y.

Please observe the following dataset, which we are going to use throughout the
example.
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  0 |   2020 |               24 |         1000 |
|  1 |   2020 |               40 |         7000 |
|  2 |   2020 |               32 |         5000 |
|  3 |   2020 |               29 |         3000 |
|  4 |   2020 |               49 |        10000 |
|  5 |   2021 |               50 |        15000 |
|  6 |   2021 |               25 |         1500 |
|  7 |   2021 |               30 |         6000 |
+----+--------+------------------+--------------+

For example, in the table above, the order dependency ['year', 'employee_grade']
-> ['year', 'avg_salary'] holds, while the dependency ['year', 'employee_grade']
-> ['avg_salary'] does not. If we sort the table by ['year', 'employee_grade'],
it will also be guaranteed to be sorted by ['year', 'avg_salary'].

Definition 2. Let H be a set of attributes. We define an equivalence class of H
as a set of tuples where for any two tuples s and t in the set, s[H] = t[H].
For example, in the table above, let's take H={'year'}. Then there are two
equivalence classes of H: one with the value 2020 (tuples with indices {0, 1, 2,
3, 4}) and the other with the value 2021 (tuples with indices {5, 6, 7}).

Definition 3. Given a set of attributes H and attributes A and B, H: A<= ~ B
denotes a Canonical Order Compatibility (OC) with relation <= for attribute A.
The canonical order compatibility H: A<= ~ B holds if and only if inside each
equivalence class of H, there exists a total ordering of tuples such that they
are ordered by both A and B. A<= means that when we are comparing s[A] vs t[A],
we use the <= relation.

For example, in the table above, the OC `{'year'} : 'employee_grade'<= ~
'avg_salary'` holds. Within both equivalence classes of {'year'}, a total
ordering of tuples exists, so the tuples are ordered by both 'employee_grade'<=
and 'avg_salary'. That is, ordering [0,3,2,1,4] and [6, 7, 5]. At the same time,
the OC `{}: 'employee_grade'<= ~ 'avg_salary'` does not hold. For an empty set
of attributes, there is only one equivalence class, which consists of all tuples
in the table. There is no such ordering in the table that all tuples are sorted
at the same time by 'employee_grade'<= and 'year'. This is because for tuples 2
and 7, the order for 'employee_grade' is > while for 'avg_salary' it is <.

Definition 4. Given a set of attributes H and an attribute A, H: [] -> A denotes
a Canonical Order Functional Dependency (OFD). H: [] -> A holds if and only if
attribute A is constant within each equivalence class of H. This is equivalent
to the list-based OD H -> HA for any permutation of H.

For example, in the table above, the OFD {'avg_salary'}: [] ->
['employee_grade'] holds. Since all values in the 'avg_salary' column are
unique, each equivalence class of {'avg_salary'} consists of exactly one tuple.
Within each set of a single tuple, 'employee_grade' is constant (as is any other
attribute).

Note that the list-based OD HA -> HB for any permutation of H is logically
equivalent to the OC H: A ~ B and the OFD HA: [] -> B. This means that the OD HA
-> HB holds if and only if both the corresponding OC and OFD hold.

Definition 5. A set-based canonical order dependency denotes either a canonical
order compatibility or a canonical order functional dependency.

Definition 6. An Approximate set-based canonical Order Dependency (AOD) is a
set-based canonical order dependency that holds only on a subset of the table.
An AOD holds if and only if there exists a set of tuples that can be removed
from the table for the AOD to hold exactly. The minimal set of such tuples is
called a removal set. The error E of an AOD is calculated as the cardinality of
the removal set (the number of tuples in the set) divided by the cardinality of
the table. We say that an AOD holds with error E if and only if the error of the
AOD is less than the value E.

You might also want to take a look at the set-based ODs mining example
(examples/basic/mining_set_od_1.py).

Now, let's move to the set-based canonical OD verification via Desbordante.

Let's start by verifying an exact OC that holds on the table above.
As we showed above, the OC `{'year'} : 'employee_grade'<= ~ 'avg_salary'` holds.

Running `algo.execute()` on OC {'year'}: 'employee_grade'<= ~ 'avg_salary'
produces the following results:
    `algo.holds()`: True
    `algo.get_removal_set()`: set()
    `algo.get_error()`: 0.0
Note that the error is zero and the removal set is empty. A removal set is a set
of rows that should be removed for an OC (or OD) to hold exactly. In this case,
the OC holds exactly, which is why the set is empty.

Now let's verify the OFD {'employee_grade'} : [] -> 'year', which also holds
exactly.

Running `algo.execute()` on OFD {'employee_grade'}: [] -> 'year' produces the
following results:
    `algo.holds()`: True
    `algo.get_removal_set()`: set()
    `algo.get_error()`: 0.0
Note that the error once again is zero and the removal set is empty because the
OFD holds exactly.

Now let's add a row to the table to break the exact holding of these
dependencies.
+----+--------+------------------+--------------+
|    |   year |   employee_grade |   avg_salary |
|----+--------+------------------+--------------|
|  0 |   2020 |               24 |         1000 |
|  1 |   2020 |               40 |         7000 |
|  2 |   2020 |               32 |         5000 |
|  3 |   2020 |               29 |         3000 |
|  4 |   2020 |               49 |        10000 |
|  5 |   2021 |               50 |        15000 |
|  6 |   2021 |               25 |         1500 |
|  7 |   2021 |               30 |         6000 |
|  8 |   2020 |               50 |         9000 |
+----+--------+------------------+--------------+
Note that the row with index 8 was added to the table.

Running `algo.execute()` on OC {'year'}: 'employee_grade'<= ~ 'avg_salary'
produces the following results:
    `algo.holds()`: False
    `algo.get_removal_set()`: {4}
    `algo.get_error()`: 0.1111111111111111
Note that now the OC does not hold exactly and that the removal set is {4}. This
means that for the OC to hold exactly, it is enough to remove row number 4
(indexed from 0) from the table. Note that rows 8 and 4 are interchangeable in
this sense because the problem with ordering is caused by their simultaneous
presence in the table, and removing either of them will fix it. The algorithm
guarantees to return a minimal removal set in terms of size but does not specify
which one exactly if there are several candidates.

Running `algo.execute()` on OFD {'employee_grade'}: [] -> 'year' produces the
following results:
    `algo.holds()`: False
    `algo.get_removal_set()`: {5}
    `algo.get_error()`: 0.1111111111111111
Note once again that the OFD does not hold exactly anymore and the removal set
is not empty. By adding row 8 with the same value in the 'employee_grade' column
as in row 5 but with a different value in the 'year' column, we broke the FD
'employee_grade'->'year' and thus broke the OFD {'employee_grade'}: [] ->
'year'. Removing either of these two rows will make the OFD hold exactly; thus,
the removal set is {5}.

We hope this example helped you understand how to verify exact and approximate
set-based order dependencies. We've seen how even a single row can affect the
result and what a removal set is.

Feel free to play around with the code: modify the table with different values,
try verifying other AODs, or even load your own datasets to see what you can
discover!
'''

snapshots['test_example[basic/verifying_aucc.py-None-verifying_aucc_output] verifying_aucc_output'] = '''Dataset AUCC_example.csv:
   ID  name  card_num  card_active
0   1  Alex       665         True
1   2  Liam       667         True
2   3  Ezra       553         True
3   4  Alex       665        False
4   5  Kian       667        False
5   6  Otis       111         True
--------------------------------------------------------------------------------
Checking whether (ID) UCC holds
--------------------------------------------------------------------------------

UCC holds, showing stats for AUCC is useless

--------------------------------------------------------------------------------
Checking whether (name) UCC holds
It should not hold, there are 2 persons, named Alex
--------------------------------------------------------------------------------

UCC does not hold
But AUCC with error = 0.0667 holds

Also:
Total number of rows violating UCC: 2
Number of clusters violating UCC: 1
Clusters violating UCC:
found 1 clusters violating UCC:

First violating cluster:
   ID  name  card_num  card_active
0   1  Alex       665         True
3   4  Alex       665        False

--------------------------------------------------------------------------------
Checking whether (card_num) UCC holds
It should not hold, there are 2 identical card numbers
--------------------------------------------------------------------------------

UCC does not hold
But AUCC with error = 0.1333 holds

Also:
Total number of rows violating UCC: 4
Number of clusters violating UCC: 2
Clusters violating UCC:
found 2 clusters violating UCC:

First violating cluster:
   ID  name  card_num  card_active
0   1  Alex       665         True
3   4  Alex       665        False
Second violating cluster:
   ID  name  card_num  card_active
1   2  Liam       667         True
4   5  Kian       667        False

--------------------------------------------------------------------------------
Checking whether (card_num, card_active) UCC holds
It should hold, cards with identical numbers are not active simultaneously
--------------------------------------------------------------------------------

UCC holds, showing stats for AUCC is useless

--------------------------------------------------------------------------------
'''

snapshots['test_example[basic/verifying_cfd.py-None-verifying_cfd_output] verifying_cfd_output'] = '''This example demonstrates how to validate Conditional Functional Dependencies (CFDs) using the Desbordante library.
The definitions are taken from the paper 'Revisiting Conditional Functional Dependency Discovery: Splitting the “C” from the “FD”' (ECML PKDD 2018).
CFD expresses a relationship in which a subset of attributes X defines Y, written as (X -> Y, t), where t is a certain template tuple.
A template tuple t is a tuple where each attribute can either have a fixed constant value or a wildcard symbol ('_'), allowing for generalization across different data records.
Validation checks whether a user-specified CFD holds for a given dataset, based on the specified values for support and confidence thresholds. Support is the quantity of records satisfying the condition, and confidence is the fraction of records where Y occurs given X.
Desbordante detects CFD violations and classifies records based on rule compliance.


In the first example, let's look at a dataset containing real estate properties in different cities.

           City          Street  PostalCode BuildingType BuildingCost
0   Los Angeles  Hollywood Blvd       90029    Apartment         high
1       Chicago    State Street       60601    Apartment       medium
2      New York        Broadway       10002    Apartment         high
3   Los Angeles     Sunset Blvd       90001        House         high
4       Chicago    Michigan Ave       60611       Office         high
5      New York     Wall Street       10005       Office         high
6   Los Angeles  Hollywood Blvd       90028    Apartment          low
7       Chicago    State Street       60602    Apartment          low
8      New York        Broadway       10001    Apartment         high
9   Los Angeles  Hollywood Blvd       90028    Apartment         high
10      Chicago    State Street       60601    Apartment       medium
11     New York        Broadway       10001    Apartment         high
12  Los Angeles     Sunset Blvd       90001        House         high
13      Chicago    Michigan Ave       60611       Office       medium
14     New York     Wall Street       10005       Office         high

Let's say we want to check whether highly priced buildings in Los Angeles are determined by (depend on) a specific building type.

This hypothesis will be expressed as a rule: [("City", "Los Angeles"), ("BuildingType", "_")] -> ("BuildingCost", "high")

\x1b[1;49mCFD: [('City', 'Los Angeles'), ('BuildingType', '_')] -> ('BuildingCost', 'high')
CFD holds: \x1b[1;31mFalse\x1b[0m
Support: 5
Confidence: 0.80
Number of clusters violating FD: 1
\x1b[1;34m #1 cluster: \x1b[0m
\x1b[0m0: ['Los Angeles', 'Apartment'] -> ['high']\x1b[0m
\x1b[1;41m6: ['Los Angeles', 'Apartment'] -> ['low']\x1b[0m
\x1b[0m9: ['Los Angeles', 'Apartment'] -> ['high']\x1b[0m

We can see that the rule is violated in line 6, which may indicate incorrect data entry. Let's fix them.

           City          Street  PostalCode BuildingType BuildingCost
0   Los Angeles  Hollywood Blvd       90029    Apartment         high
1       Chicago    State Street       60601    Apartment       medium
2      New York        Broadway       10002    Apartment         high
3   Los Angeles     Sunset Blvd       90001        House         high
4       Chicago    Michigan Ave       60611       Office         high
5      New York     Wall Street       10005       Office         high
\x1b[1;42m6   Los Angeles  Hollywood Blvd       90028    Apartment         high\x1b[0m
7       Chicago    State Street       60602    Apartment          low
8      New York        Broadway       10001    Apartment         high
9   Los Angeles  Hollywood Blvd       90028    Apartment         high
10      Chicago    State Street       60601    Apartment       medium
11     New York        Broadway       10001    Apartment         high
12  Los Angeles     Sunset Blvd       90001        House         high
13      Chicago    Michigan Ave       60611       Office       medium
14     New York     Wall Street       10005       Office         high
\x1b[1;49mCFD: [('City', 'Los Angeles'), ('BuildingType', '_')] -> ('BuildingCost', 'high')
CFD holds: \x1b[1;32mTrue\x1b[0m
Support: 5
Confidence: 1.00
Number of clusters violating FD: 0

Thats all for CFD validation. Desbordante is also capable of CFD discovery, which is discussed in "mining_cfd.py".
'''

snapshots['test_example[basic/verifying_dc.py-None-verifying_dc_output] verifying_dc_output'] = '''This is a basic example explaining how to use Denial Constraint (DC) verification for checking hypotheses on data.
A more advanced example of using Denial Constraints is located in examples/expert/data_cleaning_dc.py.

DC verification is performed by the Rapidash algorithm:
Zifan Liu, Shaleen Deep, Anna Fariha, Fotis Psallidas, Ashish Tiwari, and Avrilia
Floratou. 2023. Rapidash: Efficient Constraint Discovery via Rapid Verification.
URL: https://arxiv.org/abs/2309.12436

DC φ is a conjunction of predicates of the following form:
∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)

DCs involve comparisons between pairs of rows within a dataset.
A typical DC example, derived from a Functional Dependency such as A -> B,
is expressed as: "∀s, t ∈ R, s ≠ t, ¬(t.A == s.A ∧ t.B ≠ s.B)".

This denotes that for any pair of rows in the relation, it should not be the case
that while the values in column A are equal, the values in column B are unequal.

Consider the following dataset:

1       State Salary FedTaxRate
2     NewYork   3000        0.2
3     NewYork   4000       0.25
4     NewYork   5000        0.3
5   Wisconsin   5000       0.15
6   Wisconsin   6000        0.2
7   Wisconsin   4000        0.1
8       Texas   1000       0.15
9       Texas   2000       0.25
10      Texas   3000        0.3

And the following Denial Constraint:
!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate).

We use "and" instead of "∧" and "¬" instead of "!" for easier representation.

The constraint tells us that for all people in the same state a person with a higher salary has a higher tax rate.
Then we run the algorithm in order to see if the constraint holds.

DC !(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate) holds: True

Indeed, the given constraint holds. Now we will modify the initial dataset to make things
a little more interesting. Consider the previous table but with an additional record for Texas (#11):

1       State Salary FedTaxRate
2     NewYork   3000        0.2
3     NewYork   4000       0.25
4     NewYork   5000        0.3
5   Wisconsin   5000       0.15
6   Wisconsin   6000        0.2
7   Wisconsin   4000        0.1
8       Texas   1000       0.15
9       Texas   2000       0.25
10      Texas   3000        0.3
11      Texas   5000       0.05

DC !(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate) holds: False

Now we can see that the same DC we examined on the previous dataset doesn't hold on the new one.
The issue is that for the last record (Texas, 5000, 0.05), there are people in Texas with a lower salary
but a higher tax rate.

Such pairs of records that contradict a DC are called violations. We can retrieve these 
violations from the algorithm object. In this case, the following pairs are the violations:
(8, 11), (9, 11), (10, 11), where each number is an index of a record in the table.

'''

snapshots['test_example[basic/verifying_dd.py-None-verifying_dd_output] verifying_dd_output'] = '''This is an example of validating differential dependencies.

Differential dependencies were introduced by Song, Shaoxu,
and Chen, Lei in their 2011 article, "Differential
Dependencies: Reasoning and Discovery," published in ACM
Transactions on Database Systems (Vol. 36, No. 3).

A differential dependency (DD) defines constraints on the
differences between attribute values within a table.
These dependencies are formalized using differential
functions, which specify permissible distance ranges
between attribute values.

For instance, consider the following differential
dependency:

flight_id[0,0]; date[0, 7] -> price[0, 250].

This expression is composed of three differential functions:
flight_id[0,0], date[0, 7], and price[0, 250]. If this
dependency is applied to a flight schedule table, it
implies that for any two tuples where the difference
for identical flights (sharing the same id) in the date
attribute is no more than 7 days, the corresponding
difference in the price attribute must not exceed 250 units.
In simpler terms, this means that the price difference
between any two identical flights scheduled within the
same week will be within a 250-unit margin.

Flight schedule table:

   flight_id        date  price
0         25  2023-08-19    370
1         25  2023-08-22    200
2         11  2023-09-01    850
3         25  2023-09-02    120
4         11  2023-09-07    700
5         11  2023-09-12    460
6         25  2023-10-11    200

It can be observed that this dependency holds true for the
flights table.

To illustrate this further, we will examine the
stores_dd.csv dataset and validate a specified
differential dependency.

It is worth noting that this validator implements
standard distance metrics for numerical data types
and dates, as well as the Levenshtein distance for
strings.

Finally, an additional example of differential dependency
mining, using the Split algorithm, is also available in
the Desbordante project.

    store_name        product_name     category  stock_quantity  price_per_unit
0   BestBuy NY     Apple iPhone 15  Smartphones              50             999
1   BestBuy LA     Apple iPhone 15  Smartphones              30            1029
2   Walmart TX     Apple iPhone 15  Smartphones              40             989
3   BestBuy NY  Samsung Galaxy S23  Smartphones              25             899
4   BestBuy LA  Samsung Galaxy S23  Smartphones              20             920
5   Walmart TX  Samsung Galaxy S23  Smartphones              35             880
6   BestBuy NY     Sony WH-1000XM5   Headphones              15             399
7   BestBuy LA     Sony WH-1000XM5   Headphones              18             410
8   Walmart TX     Sony WH-1000XM5   Headphones              10             395
9   BestBuy NY   Apple MacBook Air      Laptops              10            1299
10  BestBuy LA   Apple MacBook Air      Laptops               8            1349
11  Walmart TX   Apple MacBook Air      Laptops              12            1289

----------------------------------------------------------------------------------------------------
Example #1

To better understand the differential dependency concept,
let's examine a practical example.

Consider the following DD:

\x1b[1;33mproduct_name [0, 0] -> stock_quantity [0, 20] ; price_per_unit [0, 60]\x1b[0m

This \x1b[1;32mDD holds.\x1b[0m

This dependency requires that for any two records
with the same product_name, the difference in
their stock_quantity must not exceed 20, and
the difference in price_per_unit must not
exceed 60.

In other words, for the same product sold
across different stores, stock levels cannot
vary by more than 20 units, and the price
cannot vary by more than 60 units.
----------------------------------------------------------------------------------------------------
Example #2

Now, let`s check the DD:

\x1b[1;33mstore_name [0, 0] -> stock_quantity [0, 25]\x1b[0m

This means that for a single product, the
difference in stock quantity between any
two stores cannot exceed 25 units.

This \x1b[1;31mDD doesn`t hold.\x1b[0m

Desbordante can automatically detect pairs of
violating tuples. Let’s do it.

Desbordante returned 4 pairs of records that
violate the stock_quantity threshold.

The error threshold of a differential
dependency is the ratio of record pairs
that satisfy the left-hand side (LHS) but
violate the right-hand side (RHS), to the
total number of record pairs that satisfy
the LHS.

In our example error threshold is: \x1b[1;31m0.2222222222222222\x1b[0m

Now, let us look at the pairs of violating tuples:

1) \x1b[1;32mBestBuy NY\x1b[0m Apple iPhone 15 Smartphones \x1b[1;31m50\x1b[0m 999
7) \x1b[1;32mBestBuy NY\x1b[0m Sony WH-1000XM5 Headphones \x1b[1;31m15\x1b[0m 399

1) \x1b[1;32mBestBuy NY\x1b[0m Apple iPhone 15 Smartphones \x1b[1;31m50\x1b[0m 999
10) \x1b[1;32mBestBuy NY\x1b[0m Apple MacBook Air Laptops \x1b[1;31m10\x1b[0m 1299

3) \x1b[1;32mWalmart TX\x1b[0m Apple iPhone 15 Smartphones \x1b[1;31m40\x1b[0m 989
9) \x1b[1;32mWalmart TX\x1b[0m Sony WH-1000XM5 Headphones \x1b[1;31m10\x1b[0m 395

3) \x1b[1;32mWalmart TX\x1b[0m Apple iPhone 15 Smartphones \x1b[1;31m40\x1b[0m 989
12) \x1b[1;32mWalmart TX\x1b[0m Apple MacBook Air Laptops \x1b[1;31m12\x1b[0m 1289

Clearly, this DD has no practical
significance, however, it remains useful for
demonstration purposes.
----------------------------------------------------------------------------------------------------
Example #3

Our previous dependency failed because it
didn't account for key operational factors.
For instance, stock levels are influenced
by product demand and store size, not just
the store location.

To address this, we refine the dependency
by adding a constraint on the product_category.
Next DD, which we are going to check: 

\x1b[1;33mstore_name [0, 0] ; category [0, 0] -> stock_quantity [0, 25]\x1b[0m

This \x1b[1;32mDD holds.\x1b[0m

This differential dependency states:

For tuples with the same store_name and category,
the stock_quantity must not differ by more than
25 units.
----------------------------------------------------------------------------------------------------
Example #4

Validation of Differential Dependencies can
also be utilized for mitigating data inaccuracies.

Consider the grades_dd.py table.

   student_id student_name     course   exam_date  grade_score
0           1        Alice       Math  2025-06-15           95
1           1        Akice       Math  2025-06-20           92
2           1        Alice    Physics  2025-06-18           80
3           2        Alice       Math  2025-06-21           42
4           2          Bob       Math  2025-06-16           70
5           2          Bob       Math  2025-06-21           68
6           3      Charlie  Chemistry  2025-06-17           55

We will check the DD:

\x1b[1;33mstudent_id [0, 0] -> student_name [0, 0]\x1b[0m

This \x1b[1;31mDD doesn`t hold.\x1b[0m

1) \x1b[1;32m1\x1b[0m \x1b[1;31mAlice\x1b[0m Math 2025-06-15 95
2) \x1b[1;32m1\x1b[0m \x1b[1;31mAkice\x1b[0m Math 2025-06-20 92

2) \x1b[1;32m1\x1b[0m \x1b[1;31mAkice\x1b[0m Math 2025-06-20 92
3) \x1b[1;32m1\x1b[0m \x1b[1;31mAlice\x1b[0m Physics 2025-06-18 80

4) \x1b[1;32m2\x1b[0m \x1b[1;31mAlice\x1b[0m Math 2025-06-21 42
5) \x1b[1;32m2\x1b[0m \x1b[1;31mBob\x1b[0m Math 2025-06-16 70

4) \x1b[1;32m2\x1b[0m \x1b[1;31mAlice\x1b[0m Math 2025-06-21 42
6) \x1b[1;32m2\x1b[0m \x1b[1;31mBob\x1b[0m Math 2025-06-21 68

Error threshold: \x1b[1;31m0.6666666666666666\x1b[0m

We have two pairs of rows that do not conform to
the constraints imposed by the DD. Let's rectify
this data error by changing "Akice" to "Alice"
in the first row of the "student_name" column
and then re-evaluate the DD's over this table.

   student_id student_name     course   exam_date  grade_score
0           1        Alice       Math  2025-06-15           95
1           1        Alice       Math  2025-06-20           92
2           1        Alice    Physics  2025-06-18           80
3           2        Alice       Math  2025-06-21           42
4           2          Bob       Math  2025-06-16           70
5           2          Bob       Math  2025-06-21           68
6           3      Charlie  Chemistry  2025-06-17           55

After correcting the error, the error threshold
dropped to \x1b[1;31m0.3333333333333333\x1b[0m

A potential error may also exist in the left-hand
side of the DD. For instance, in rows 3, 4 and 5
of the table, we have three entries with an identical
student_id but a different student_name. Let's
correct this error and observe the subsequent changes.

   student_id student_name     course   exam_date  grade_score
0           1        Alice       Math  2025-06-15           95
1           1        Alice       Math  2025-06-20           92
2           1        Alice    Physics  2025-06-18           80
3           1        Alice       Math  2025-06-21           42
4           2          Bob       Math  2025-06-16           70
5           2          Bob       Math  2025-06-21           68
6           3      Charlie  Chemistry  2025-06-17           55

After correcting the error, the error threshold
dropped to \x1b[1;32m0.0\x1b[0m and the \x1b[1;32mDD holds.\x1b[0m
'''

snapshots['test_example[basic/verifying_fd_afd.py-None-verifying_fd_afd_output] verifying_fd_afd_output'] = '''First, let's look at the duplicates_short.csv table and try to verify the functional dependency in it.

       id             name  ...  phone country
0      26      Björn Smith  ...     25      RI
1   11859         Mary Doe  ...      0      EU
2       1         Mary Doe  ...      4      EU
3      56      Emily Honjo  ...     55      GZ
4      30     Björn Tarski  ...     29      PR
5   17788         Mary Doe  ...      0      EU
6    5930         Mary Doe  ...      0      EU
7      58       Lisa Smith  ...     57      CM
8      29  Björn Shiramine  ...     28      EU
9      28       Björn Wolf  ...     27      AI
10     60        Lisa Wolf  ...     59      FC
11  11886       Björn Wolf  ...     27      AI
12   5970       Maxine Doe  ...     40      CM
13     46    Maxine Tarski  ...     45      EU
14   5957       Björn Wolf  ...     27      AI

[15 rows x 7 columns]
\x1b[1;49m
Checking whether [id] -> [name] FD holds
\x1b[1;42m FD holds \x1b[1;49m
Checking whether [name] -> [credit_score] FD holds
\x1b[1;41m FD does not hold \x1b[1;49m
Number of clusters violating FD: 2
\x1b[1;46m #1 cluster: \x1b[1;49m
1: Mary Doe -> 0.0
2: Mary Doe -> 0.0
5: Mary Doe -> 0.0
6: Mary Doe -> nan
Most frequent rhs value proportion: 0.75
Num distinct rhs values: 2

\x1b[1;46m #2 cluster: \x1b[1;49m
9: Björn Wolf -> 27.0
11: Björn Wolf -> 28.0
14: Björn Wolf -> 27.0
Most frequent rhs value proportion: 0.6666666666666666
Num distinct rhs values: 2

We learned that in this case the specified FD does not hold and there are two clusters of rows that contain values that prevent our FD from holding. A \x1b[1;46mcluster\x1b[1;49m (with respect to a fixed FD) is a collection of rows that share the same left-hand side part but differ on the right-hand side one.
Let's take a closer look at them.

In the first cluster, three values are "0" and a single one is "nan". This suggests that this single entry with the "nan" value is a result of a mistake by someone who is not familiar with the table population policy. Therefore, it should probably be changed to "0".

Now let\'s take a look at the second cluster. There are two entries: "27" and "28". In this case, it is probably a typo, since buttons 7 and 8 are located close to each other on the keyboard.

Having analyzed these clusters, we can conclude that our FD does not hold due to typos in the data. Therefore, by eliminating them, we can get this FD to hold (and make our dataset error-free).

--------------------------------------------------------------------------------
Now let's look at the DnD.csv to consider the AFD

  Creature  Strength  HaveMagic
0     Ogre         9      False
1     Ogre         6      False
2      Elf         6       True
3      Elf         6       True
4      Elf         1       True
5    Dwarf         9      False
6    Dwarf         6      False

Checking whether [Creature] -> [Strength] AFD holds (error threshold = 0.5)
\x1b[1;42m AFD with this error threshold holds \x1b[1;49m
Checking whether [Creature] -> [Strength] AFD holds (error threshold = 0.1)
\x1b[1;41m AFD with this error threshold does not hold \x1b[1;49m
But the same \x1b[1;42m AFD with error threshold = 0.19047619047619047 holds\x1b[1;49m

Similarly to the FD verification primitive, the AFD one can provide a user with clusters:

Number of clusters violating FD: 3
\x1b[1;46m #1 cluster: \x1b[1;49m
2: Elf -> 6
3: Elf -> 6
4: Elf -> 1
Most frequent rhs value proportion: 0.6666666666666666
Num distinct rhs values: 2

\x1b[1;46m #2 cluster: \x1b[1;49m
0: Ogre -> 9
1: Ogre -> 6
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2

\x1b[1;46m #3 cluster: \x1b[1;49m
5: Dwarf -> 9
6: Dwarf -> 6
Most frequent rhs value proportion: 0.5
Num distinct rhs values: 2

'''

snapshots['test_example[basic/verifying_gfd/verifying_gfd1.py-None-verifying_gfd1_output] verifying_gfd1_output'] = '''The graph is depicted in figure. The following abbreviations were used: A - account, B - blog. Vertices labeled A have a "name" attribute showing the nickname; vertices labeled B - "author", indicating who wrote the blog. The values of these attributes are labeled next to the vertices. The edges are also labeled as: "post", which indicates who wrote the blog, and "like", which indicates approval by another person. In the drawing, the edges are marked "post" in bold.

If the graph functional dependency on a figure is not satisfied, the data are erroneous because the information contained in the edge label contradicts the information about the author contained at the vertex of the blog.

Let's check if this dependency holds.

\x1b[95mDesbordante > \x1b[0mGFD holds.

Well, GFD is really satisfied as expected.

\x1b[93mClose the image window to continue.\x1b[0m

Let\'s now modify the graph to see how the algorithm will behave in another case. The new graph is depicted in the new figure. In the third blog on the left the attribute "author" was changed from Donatello to Raphael.

Run algorithm:

\x1b[95mDesbordante > \x1b[0mGFD does not hold.

As you can see, the modified graph does not satisfy this dependency, indicating that it has errors.

\x1b[93mClose the image window to finish.\x1b[0m
'''

snapshots['test_example[basic/verifying_gfd/verifying_gfd2.py-None-verifying_gfd2_output] verifying_gfd2_output'] = '''The graph is depicted in the figure. The following abbreviations were used: A - account, B - blog. Vertices labeled A have a "name" attribute showing the nickname; vertices labeled B - "author", indicating who wrote the blog. The values of these attributes are labeled next to the vertices. The edges are also labeled as: "post", which indicates who wrote the blog, and "like", which indicates approval by another person. In the drawing, the edges are marked "post" in bold.

The dependency on the figure suggests that one blog cannot have two authors. That is, satisfiability of this dependency ensures that there are no errors related to the number of authors in the data.

Let's check if this dependency holds.

\x1b[95mDesbordante > \x1b[0mGFD holds.

Well, GFD is really satisfied as expected.

\x1b[93mClose the image window to continue.\x1b[0m

Let\'s now modify the graph to see how the algorithm will behave in another case. The new graph is depicted in the new figure. Replaced edge label between the first left blog and Donatello account from "like" to "post".

Run algorithm:

\x1b[95mDesbordante > \x1b[0mGFD does not hold.

As you can see, the modified graph does not satisfy this dependency, indicating that it has errors.

\x1b[93mClose the image window to finish.\x1b[0m
'''

snapshots['test_example[basic/verifying_gfd/verifying_gfd3.py-None-verifying_gfd3_output] verifying_gfd3_output'] = '''Figure provides an example of a graph dependency and graph. The vertices of the graph have labels of C (Channel) or U (User). Depending on the label, the vertex has its own set of attributes. In this example, all vertices have a single element list of attributes. At the vertices labeled C it consists of the element "topic", and at the vertices labeled U - "age_group". On the figure the specific values of these attributes are specified next to the vertices.

Dependency means that if a user is signed on a channel whose topic is entertainment, he must be a kid.

Let's check if this dependency holds.

\x1b[95mDesbordante > \x1b[0mGFD does not hold.

The test found that the constructed dependency is not satisfied because in the graph there is an example in which a teenager subscribes to the entertainment channel.

\x1b[93mClose the image window to finish.\x1b[0m
'''

snapshots['test_example[basic/verifying_ind_aind.py-None-verifying_ind_aind_output] verifying_ind_aind_output'] = '''==============================================
In Desbordante, we consider an approximate inclusion dependency (AIND)
as any inclusion dependency (IND) that utilizes an error metric to measure
violations. This metric calculates the proportion of distinct values in the
dependent set (LHS) that must be removed to satisfy the dependency on the
referenced set (RHS) completely.

The metric lies within the [0, 1] range:
- A value of 0 means the IND holds exactly (no violations exist).
- A value closer to 1 indicates a significant proportion of LHS values violate
the dependency.

Desbordante supports the discovery and verification of both exact INDs and AINDs:
1) Exact INDs: All values in the LHS set must match a value in the RHS set.
2) Approximate INDs (AINDs): Allows for controlled violations quantified by the
error metric.

For verification tasks, users can specify an AIND, and Desbordante will calculate
the error value, identifying clusters of violating values.

The error metric used for AINDs in Desbordante is adapted from the g3 metric,
initially developed for approximate functional dependencies (FDs).

For more information, consider:
1) Unary and n-ary inclusion dependency discovery in relational databases by
   Fabien De Marchi, Stéphane Lopes, and Jean-Marc Petit.

==============================================
This example demonstrates two scenarios: verifying exact INDs and approximate
INDs (AINDs).

Let's start with the exact IND verification scenario.
The datasets under consideration for this scenario are 'orders' and 'products'.

Let's start by verifying exact IND holding between those tables.

Dataset 'orders':
+----+------+---------------+-----------+
|    |   id |   customer_id | product   |
|----+------+---------------+-----------|
|  0 |    1 |           101 | Laptop    |
|  1 |    2 |           102 | Phone     |
|  2 |    3 |           103 | Tablet    |
|  3 |    4 |           104 | Monitor   |
|  4 |    5 |           108 | Keyboard  |
|  5 |    6 |           201 | Mouse     |
|  6 |    7 |           102 | Charger   |
+----+------+---------------+-----------+

Dataset 'products':
+----+------+----------+-------------+
|    |   id | name     | category    |
|----+------+----------+-------------|
|  0 |    1 | Laptop   | Electronics |
|  1 |    2 | Phone    | Electronics |
|  2 |    3 | Tablet   | Electronics |
|  3 |    4 | Monitor  | Electronics |
|  4 |    5 | Keyboard | Accessories |
|  5 |    6 | Mouse    | Accessories |
|  6 |    7 | Charger  | Accessories |
+----+------+----------+-------------+

Checking the IND [orders.product] -> [products.name]
\x1b[1;42m IND holds \x1b[0m

The IND holds because there are no inconsistencies between the two tables. The
`products.name` column acts as a primary key, and all values in the
`orders.product` column match entries in `products.name` without any typos or
missing data.

--------------------------------------------------------------------------------

Now, let's consider the approximate IND verification scenario (AIND).

Unlike exact INDs, approximate INDs allow for a certain level of error. This
error indicates how accurately the dependency holds between the datasets. In
this scenario, we will use the 'orders' and 'customers' datasets.

Dataset 'orders':
+----+------+---------------+-----------+
|    |   id |   customer_id | product   |
|----+------+---------------+-----------|
|  0 |    1 |           101 | Laptop    |
|  1 |    2 |           102 | Phone     |
|  2 |    3 |           103 | Tablet    |
|  3 |    4 |           104 | Monitor   |
|  4 |    5 |           108 | Keyboard  |
|  5 |    6 |           201 | Mouse     |
|  6 |    7 |           102 | Charger   |
+----+------+---------------+-----------+

Dataset 'customers':
+----+------+---------+-----------+
|    |   id | name    | country   |
|----+------+---------+-----------|
|  0 |  101 | Alice   | USA       |
|  1 |  102 | Bob     | UK        |
|  2 |  103 | Charlie | Canada    |
|  3 |  104 | David   | Germany   |
|  4 |  105 | Eve     | France    |
+----+------+---------+-----------+

Checking the IND [orders.customer_id] -> [customers.id]
\x1b[1;41m AIND holds with error = 0.33 \x1b[0m

We see that this AIND has an error of 0.33. Let's examine the violating clusters
in more detail to understand the errors.

Number of clusters violating IND: 2
\x1b[1;46m #1 cluster: \x1b[1;49m
5: 201
\x1b[1;46m #2 cluster: \x1b[1;49m
4: 108
\x1b[0m
Based on our analysis, this AIND does not hold due to the following reasons:
1. The `orders.customer_id` value '201' does not match any entry in the
`customers.id` column. This suggests a possible typo where '201' might have been
entered instead of '101', indicating that the customer who bought the 'Mouse'
should be Alice.
2. The `orders.customer_id` value '108' also violates the AIND. This appears to
be a case where the 'customers' table might be incomplete, and some customer
entries are missing.

In such cases, resolving typos and ensuring data completeness in the reference
table ('customers') can help improve the accuracy of this dependency.

Let's fix the issues.

Step 1: Fix data issue in the 'orders' dataset.
Update the value in the `orders.customer_id` column where it is '201' to '101'.

Dataset 'orders':
+----+------+---------------+-----------+
|    |   id |   customer_id | product   |
|----+------+---------------+-----------|
|  0 |    1 |           101 | Laptop    |
|  1 |    2 |           102 | Phone     |
|  2 |    3 |           103 | Tablet    |
|  3 |    4 |           104 | Monitor   |
|  4 |    5 |           108 | Keyboard  |
|  5 |    6 |           101 | Mouse     |
|  6 |    7 |           102 | Charger   |
+----+------+---------------+-----------+

Dataset 'customers':
+----+------+---------+-----------+
|    |   id | name    | country   |
|----+------+---------+-----------|
|  0 |  101 | Alice   | USA       |
|  1 |  102 | Bob     | UK        |
|  2 |  103 | Charlie | Canada    |
|  3 |  104 | David   | Germany   |
|  4 |  105 | Eve     | France    |
+----+------+---------+-----------+

Checking the IND [orders.customer_id] -> [customers.id]
\x1b[1;41m AIND holds with error = 0.2 \x1b[0m

We have successfully fixed the typo in the 'orders' dataset. Now, let's address
the missing customer entry.

Step 2: Add the missing customer to the 'customers' dataset.
Adding a new customer with id '108', name 'Frank', and country 'Italy'.

Dataset 'orders':
+----+------+---------------+-----------+
|    |   id |   customer_id | product   |
|----+------+---------------+-----------|
|  0 |    1 |           101 | Laptop    |
|  1 |    2 |           102 | Phone     |
|  2 |    3 |           103 | Tablet    |
|  3 |    4 |           104 | Monitor   |
|  4 |    5 |           108 | Keyboard  |
|  5 |    6 |           101 | Mouse     |
|  6 |    7 |           102 | Charger   |
+----+------+---------------+-----------+

Dataset 'customers':
+----+------+---------+-----------+
|    |   id | name    | country   |
|----+------+---------+-----------|
|  0 |  101 | Alice   | USA       |
|  1 |  102 | Bob     | UK        |
|  2 |  103 | Charlie | Canada    |
|  3 |  104 | David   | Germany   |
|  4 |  105 | Eve     | France    |
|  5 |  108 | Frank   | Italy     |
+----+------+---------+-----------+

Checking the IND [orders.customer_id] -> [customers.id]
\x1b[1;42m IND holds \x1b[0m

The missing customer has been successfully added to the 'customers' dataset.

All issues in the 'orders' and 'customers' datasets have been resolved.
'''

snapshots['test_example[basic/verifying_md.py-None-verifying_md_output] verifying_md_output'] = '''\x1b[1;49m
This example demonstrates how to verify matching dependencies (MDs) using the Desbordante library. Matching dependencies are defined in "Efficient Discovery of Matching Dependencies" by Schirmer et al., ACM Transactions on Database Systems (TODS), Vol. 45, No. 3, Article 13, pp. 1–33.

The matching dependency verification algorithm accepts a dependency and determines whether it holds over the specified dataset. If the dependency does not hold, the algorithm returns a list of exceptions (tuples that violate the MD) and suggests adjustments to the dependency to make it hold.

You can also read about mining matching dependencies in examples/basic/mining_md.py.

To verify a matching dependency, first define column similarity classifiers. A column similarity classifiers consists of a column match and a decision boundary. A column match specifies two column identifiers (index or name) — one from the left table and one from the right — and a similarity measure (for example, Levenshtein similarity).

We use the notation [measure(i, j)>=lambda] for a column similarity classifier that specifies the i-th column of the left table, the j-th column of the right table, the similarity measure "measure", and the decision boundary lambda. The notation [measure("left_col_name", "right_col_name")>=lambda] is also valid for a column match that specifies the columns "left_col_name" and "right_col_name" of the left and right tables, respectively.

Finally, the algorithm is defined over two tables: the left table and the right table. For simplicity in this example we use a single table (right table = left table). See the original paper for details on the two-table setting.

As the first example, let's look at the animals_beverages.csv dataset.

       name     zoo animal  diet
0     Simba  berlin   lion  meat
1  Clarence  london   lion  mead
2     Baloo  berlin   bear  fish
3      Pooh  london   beer  fish 

Let's try to check if the Matching Dependency

\t[ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=1.0

holds. Here, Levenshtein similarity with a decision boundary of 1.0 means values must be exactly equal.

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 1) have similarity 0.75, while dependency states levenshtein(diet, diet)>=1.0
2. Records (1, 0) have similarity 0.75, while dependency states levenshtein(diet, diet)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.75.

Thus, the following MD was provided:

\t[ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=1.0

and the following MD is suggested:

\t[ levenshtein(animal, animal)>=1.0 ] -> levenshtein(diet, diet)>=0.75

The checked matching dependency used a column similarity classifier with a 1.0 decision boundary on the right side. However, records with similarity 0.75 — which is below the specified boundary — were found. Therefore, the checked matching dependency does not hold.

The matching dependency may fail because of typos in the original dataset. Let's relax both left and right constraints (i.e., require similarity => 0.75) and check the resulting dependency:

\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75

\x1b[1;42mMD holds\x1b[1;49m

We can see that the matching dependency

\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75

holds.

Now let's look at what happens if we increase the decision boundary on both the left-hand side and the right-hand side. For example, we'll raise it from 0.75 to 0.76. First, let's increase the left-hand side decision boundary:

\x1b[1;42mMD holds\x1b[1;49m

As we can see, nothing changed. Now let's raise the right-hand side decision boundary:

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 1) have similarity 0.75, while dependency states levenshtein(diet, diet)>=0.76
2. Records (1, 0) have similarity 0.75, while dependency states levenshtein(diet, diet)>=0.76

Desbordante suggests to use the following right-hand side decision boundary: 0.75.

Thus, the following MD was provided:

\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.76

and the following MD is suggested:

\t[ levenshtein(animal, animal)>=0.75 ] -> levenshtein(diet, diet)>=0.75

The values "meat" and "mead" have a Levenshtein similarity of 0.75, which is below the required 0.76; therefore the matching dependency does not hold.

Let's see whether correcting typos in the dataset changes that.

Corrected dataset:

       name     zoo animal  diet
0     Simba  berlin   lion  meat
1  Clarence  london   lion  meat
2     Baloo  berlin   bear  fish
3      Pooh  london   bear  fish

Now let's re-check the original matching dependency with decision boundaries set to 1.0.

\x1b[1;42mMD holds\x1b[1;49m

---------------------------------------------------------------------------------------------------- 

On our next example let's take a view at employee_typos.csv dataset:

     Name  Surname Position              City  OfficeLocation HighLevelAccess
0    John      Doe  manager     New-York City      Main St.17             Yes
1    Jane      Doe  Manager     New-York City     Main St. 17             yes
2  Edward    Black    Clerk  Washington D. C.     Third St 34              No
3  Samuel    Smith  Sweeper  Washington D. C.    Third St. 34              No
4   Dolly   Porter  Manager           Chicago  General St. 56             Yes
5    Mike  Engeals    Chief           Chicago  General St. 56             yes 

Suppose we already know the following facts about this dataset:
1. Each city has a single office, i.e. there is a functional dependency [City] -> OfficeLocation.
2. Only managers and chiefs have high-level access, i.e. there is a functional dependency [Position] -> HighLevelAccess.

As we can see, this dataset contains several typos that we will attempt to detect and correct.

Let's start with the functional dependency [City] -> OfficeLocation. To check it, we examine the following matching dependency:

\t[levenshtein(City, City)>=1.0] -> levenshtein(Office Location, Office Location)>=1.0

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 1) have similarity 0.909, while dependency states levenshtein(OfficeLocation, OfficeLocation)>=1.0
2. Records (1, 0) have similarity 0.909, while dependency states levenshtein(OfficeLocation, OfficeLocation)>=1.0
3. Records (2, 3) have similarity 0.917, while dependency states levenshtein(OfficeLocation, OfficeLocation)>=1.0
4. Records (3, 2) have similarity 0.917, while dependency states levenshtein(OfficeLocation, OfficeLocation)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.909.

Thus, the following MD was provided:

\t[ levenshtein(City, City)>=1.0 ] -> levenshtein(OfficeLocation, OfficeLocation)>=1.0

and the following MD is suggested:

\t[ levenshtein(City, City)>=1.0 ] -> levenshtein(OfficeLocation, OfficeLocation)>=0.909

The output shows issues in record pairs (0, 1) and (2, 3). Values:
1. record 0: "Main St.17" — record 1: "Main St. 17"
2. record 2: "Third St 34" — record 3: "Third St. 34"

Now we can see the typos:
1. Record 0: missing space in "Main St.17" (should be "Main St. 17").
2. Record 2: missing period in "Third St 34" (should be "Third St. 34").

Now let's fix the typos:

     Name  Surname Position              City  OfficeLocation HighLevelAccess
0    John      Doe  manager     New-York City     Main St. 17             Yes
1    Jane      Doe  Manager     New-York City     Main St. 17             yes
2  Edward    Black    Clerk  Washington D. C.    Third St. 34              No
3  Samuel    Smith  Sweeper  Washington D. C.    Third St. 34              No
4   Dolly   Porter  Manager           Chicago  General St. 56             Yes
5    Mike  Engeals    Chief           Chicago  General St. 56             yes 

Let's try again:

\x1b[1;42mMD holds\x1b[1;49m

Alternatively, if we consider these typos insignificant for our purposes, we can ignore them. As Desbordante suggests, we can relax the right-hand decision boundary and check the dependency

\t[levenshtein(City, City)>=1.0] -> levenshtein(Office Location, Office Location)>=0.9

over the unmodified table.

\x1b[1;42mMD holds\x1b[1;49m

Let's move on and repeat the procedure for the functional dependency [Position] -> HighLevelAccess. To check it, we examine the following matching dependency:

\t[levenshtein(Position, Position)>=1.0] -> levenshtein(High Level Access, High Level Access)>=1.0

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (1, 4) have similarity 0.667, while dependency states levenshtein(HighLevelAccess, HighLevelAccess)>=1.0
2. Records (4, 1) have similarity 0.667, while dependency states levenshtein(HighLevelAccess, HighLevelAccess)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.667.

Thus, the following MD was provided:

\t[ levenshtein(Position, Position)>=1.0 ] -> levenshtein(HighLevelAccess, HighLevelAccess)>=1.0

and the following MD is suggested:

\t[ levenshtein(Position, Position)>=1.0 ] -> levenshtein(HighLevelAccess, HighLevelAccess)>=0.667

As we can see, there is a discrepancy in records 1 and 4:
1. record 1: "yes" — record 4: "Yes"

Now we see the problem. Let's fix it:

     Name  Surname Position              City  OfficeLocation HighLevelAccess
0    John      Doe  manager     New-York City     Main St. 17             Yes
1    Jane      Doe  Manager     New-York City     Main St. 17             Yes
2  Edward    Black    Clerk  Washington D. C.    Third St. 34              No
3  Samuel    Smith  Sweeper  Washington D. C.    Third St. 34              No
4   Dolly   Porter  Manager           Chicago  General St. 56             Yes
5    Mike  Engeals    Chief           Chicago  General St. 56             yes 

Let's re-check the matching dependency again:

\x1b[1;42mMD holds\x1b[1;49m

If you look closely, there are still some typos in the dataset:
1. Record 0: "manager" should be "Manager".
2. Record 5: "yes" was missed during our procedure and should be fixed.

As a result, we can observe two limitations of our approach:
1. Typos on the left-hand side of a dependency may go undetected.
2. Typos in records with a unique left-hand-side value cannot be detected.

There is an alternative approach to finding typos with MDs. We will demonstrate it on the "Position" column: first verify the matching dependency [levenshtein(Position, Position)>=1.0] -> levenshtein(Position, Position)>=1.0, then gradually lower the left-hand side decision boundary until all typos are discovered.


Verifying the matching dependency [ levenshtein(Position, Position)>=1.0 ] -> levenshtein(Position, Position)>=1.0:

\x1b[1;42mMD holds\x1b[1;49m


Verifying Matching Dependency [ levenshtein(Position, Position)>=0.8 ] -> levenshtein(Position, Position)>=1.0:

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 4) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
2. Records (0, 1) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
3. Records (1, 0) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
4. Records (4, 0) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.857.

Thus, the following MD was provided:

\t[ levenshtein(Position, Position)>=0.8 ] -> levenshtein(Position, Position)>=1.0

and the following MD is suggested:

\t[ levenshtein(Position, Position)>=0.8 ] -> levenshtein(Position, Position)>=0.857

Here, with decision boundary 0.8, the first typos were found.

Let's decrease the threshold further and see how it affects the algorithm's output.

Verifying Matching Dependency [ levenshtein(Position, Position)>=0.2 ] -> levenshtein(Position, Position)>=1.0:

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 4) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
2. Records (0, 1) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
3. Records (0, 3) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
4. Records (1, 0) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
5. Records (1, 3) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
6. Records (4, 0) have similarity 0.857, while dependency states levenshtein(Position, Position)>=1.0
7. Records (4, 3) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
8. Records (2, 5) have similarity 0.2, while dependency states levenshtein(Position, Position)>=1.0
9. Records (3, 0) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
10. Records (3, 4) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
11. Records (3, 1) have similarity 0.286, while dependency states levenshtein(Position, Position)>=1.0
12. Records (5, 2) have similarity 0.2, while dependency states levenshtein(Position, Position)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.2.

Thus, the following MD was provided:

\t[ levenshtein(Position, Position)>=0.2 ] -> levenshtein(Position, Position)>=1.0

and the following MD is suggested:

\t[ levenshtein(Position, Position)>=0.2 ] -> levenshtein(Position, Position)>=0.2

Invoking the algorithm with a decision boundary of 0.8 helped us locate issues in the record pairs (0, 1) and (0, 4). Record 0 has the value "manager" (uncapitalized) in the "Position" column, so we can fix it as follows:

     Name  Surname Position              City  OfficeLocation HighLevelAccess
0    John      Doe  Manager     New-York City     Main St. 17             Yes
1    Jane      Doe  Manager     New-York City     Main St. 17             Yes
2  Edward    Black    Clerk  Washington D. C.    Third St. 34              No
3  Samuel    Smith  Sweeper  Washington D. C.    Third St. 34              No
4   Dolly   Porter  Manager           Chicago  General St. 56             Yes
5    Mike  Engeals    Chief           Chicago  General St. 56             yes 

Invoking the algorithm with a decision boundary of 0.2 revealed some additional, but meaningless, patterns. For example, it considered the value "Clerk" in record 2 and "Chief" in record 5 similar enough.

As a result, we can conclude that this approach allows locating typos without prior knowledge of column dependencies, but requires care in selecting decision boundaries and in analyzing the algorithm's results.

---------------------------------------------------------------------------------------------------- 

Now let's examine another example. We will use the flights_dd.csv dataset for this purpose:

   Flight Number        Date               Departure                 Arrival  Distance  Duration
0          SU 35  2024-03-06  Saint Petersburg (LED)            Moscow (SVO)       598        64
1        FV 6015  2024-03-06  Saint Petersburg (LED)            Moscow (VKO)       624        63
2        FV 6027  2024-03-06  Saint Petersburg (LED)            Moscow (SVO)       598        66
3        FV 6024  2024-03-03            Moscow (VKO)  Saint Petersburg (LED)       624        58
4           SU 6  2024-03-06            Moscow (SVO)  Saint Petersburg (LED)       598        62
5        S7 1009  2024-03-01            Moscow (DME)  Saint Petersburg (LED)       664        66
6        S7 1010  2024-03-02  Saint Petersburg (LED)            Moscow (DME)       664        70
7         B2 978  2024-03-07            Moscow (SVO)             Minsk (MSQ)       641        58
8         DP 967  2024-03-07            Moscow (VKO)             Minsk (MSQ)       622        73
9         B2 981  2024-03-08             Minsk (MSQ)            Moscow (VKO)       622        61
10        DP 261  2024-03-06            Moscow (VKO)       Kaliningrad (KGD)      1059       144
11        DP 536  2024-03-05       Kaliningrad (KGD)  Saint Petersburg (LED)       798        92 

Imagine we want to check that when the departure city and the arrival city are the same, flight times do not differ significantly. We will treat all Moscow airports as equivalent and need to determine a decision boundary for this purpose.

Let's create a copy of our table and add new Departure and Arrival columns with airport codes removed:

                 Departure      NewDeparture                 Arrival        NewArrival
0   Saint Petersburg (LED)  Saint Petersburg            Moscow (SVO)            Moscow
1   Saint Petersburg (LED)  Saint Petersburg            Moscow (VKO)            Moscow
2   Saint Petersburg (LED)  Saint Petersburg            Moscow (SVO)            Moscow
3             Moscow (VKO)            Moscow  Saint Petersburg (LED)  Saint Petersburg
4             Moscow (SVO)            Moscow  Saint Petersburg (LED)  Saint Petersburg
5             Moscow (DME)            Moscow  Saint Petersburg (LED)  Saint Petersburg
6   Saint Petersburg (LED)  Saint Petersburg            Moscow (DME)            Moscow
7             Moscow (SVO)            Moscow             Minsk (MSQ)             Minsk
8             Moscow (VKO)            Moscow             Minsk (MSQ)             Minsk
9              Minsk (MSQ)             Minsk            Moscow (VKO)            Moscow
10            Moscow (VKO)            Moscow       Kaliningrad (KGD)       Kaliningrad
11       Kaliningrad (KGD)       Kaliningrad  Saint Petersburg (LED)  Saint Petersburg 

Now let's check the following matching dependency:

\t[ equality(Departure_new, Departure_new)>=1.0 ] -> levenshtein(Departure, Departure)>=1.0:

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (3, 4) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
2. Records (3, 5) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
3. Records (3, 7) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
4. Records (4, 5) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
5. Records (4, 10) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
6. Records (4, 3) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
7. Records (4, 8) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
8. Records (5, 4) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
9. Records (5, 7) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
10. Records (5, 10) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
11. Records (5, 3) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
12. Records (5, 8) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
13. Records (7, 5) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
14. Records (7, 10) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
15. Records (7, 3) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
16. Records (7, 8) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
17. Records (8, 4) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
18. Records (8, 5) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
19. Records (8, 7) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
20. Records (10, 4) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0
21. Records (10, 5) have similarity 0.75, while dependency states levenshtein(Departure, Departure)>=1.0
22. Records (10, 7) have similarity 0.833, while dependency states levenshtein(Departure, Departure)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.75.

Thus, the following MD was provided:

\t[ equality(NewDeparture, NewDeparture)>=1.0 ] -> levenshtein(Departure, Departure)>=1.0

and the following MD is suggested:

\t[ equality(NewDeparture, NewDeparture)>=1.0 ] -> levenshtein(Departure, Departure)>=0.75

As we can see, Desbordante suggests a decision boundary of 0.75 for the Departure column. For the Arrival column, all values are similar.

For duration similarity we use the custom measure normalized_distance, defined as normalized_distance = 1 - |duration_1 - duration_2| / max(Duration), where duration_1 and duration_2 are values from the Duration column. This similarity measure is supplied to the verification algorithm. You can find more examples of custom similarity measures in examples/basic/mining_md.py.

We will try to verify the following matching dependency:

\t[ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=1.0

\x1b[1;41mMD does not hold. The following rows selected by the dependency's left-hand side do not satisfy the condition of the right-hand side:\x1b[1;49m
1. Records (0, 2) have similarity 0.986, while dependency states normalized_distance(Duration, Duration)>=1.0
2. Records (0, 1) have similarity 0.993, while dependency states normalized_distance(Duration, Duration)>=1.0
3. Records (0, 6) have similarity 0.958, while dependency states normalized_distance(Duration, Duration)>=1.0
4. Records (1, 0) have similarity 0.993, while dependency states normalized_distance(Duration, Duration)>=1.0
5. Records (1, 2) have similarity 0.979, while dependency states normalized_distance(Duration, Duration)>=1.0
6. Records (1, 6) have similarity 0.951, while dependency states normalized_distance(Duration, Duration)>=1.0
7. Records (2, 0) have similarity 0.986, while dependency states normalized_distance(Duration, Duration)>=1.0
8. Records (2, 1) have similarity 0.979, while dependency states normalized_distance(Duration, Duration)>=1.0
9. Records (2, 6) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
10. Records (6, 0) have similarity 0.958, while dependency states normalized_distance(Duration, Duration)>=1.0
11. Records (6, 2) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
12. Records (6, 1) have similarity 0.951, while dependency states normalized_distance(Duration, Duration)>=1.0
13. Records (3, 4) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
14. Records (3, 5) have similarity 0.944, while dependency states normalized_distance(Duration, Duration)>=1.0
15. Records (8, 7) have similarity 0.896, while dependency states normalized_distance(Duration, Duration)>=1.0
16. Records (4, 5) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
17. Records (4, 3) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
18. Records (7, 8) have similarity 0.896, while dependency states normalized_distance(Duration, Duration)>=1.0
19. Records (5, 4) have similarity 0.972, while dependency states normalized_distance(Duration, Duration)>=1.0
20. Records (5, 3) have similarity 0.944, while dependency states normalized_distance(Duration, Duration)>=1.0

Desbordante suggests to use the following right-hand side decision boundary: 0.896.

Thus, the following MD was provided:

\t[ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=1.0

and the following MD is suggested:

\t[ levenshtein(Departure, Departure)>=0.75 | levenshtein(Arrival, Arrival)>=0.75 ] -> normalized_distance(Duration, Duration)>=0.896

As a result, we can conclude that durations differ by about 10% for flights with the same departure and arrival cities.

In conclusion, the matching dependency verification algorithm can be helpful for analyzing data, extracting facts, and finding typos. It is a powerful pattern but requires experimentation with decision boundaries and similarity measures.

'''

snapshots['test_example[basic/verifying_mfd.py-None-verifying_mfd_output] verifying_mfd_output'] = '''\x1b[1;49m
\x1b[1;46mMetric Functional Dependency\x1b[1;49m (MFD) is a type of relaxed functional dependency designed to account for small deviations that would otherwise invalidate regular functional dependencies. Those deviations are measured using an arbitrary metric defined by the user, making this definition applicable in a variety of situations.

Semi-formal definition for those interested:
Given a parameter δ and metric Δ, an MFD is defined to hold if Δ(x, y) <= δ holds for all x and y, where x and y are different right-hand side attribute values of records sharing the same left-hand side attribute values.

Let's start by investigating theatres_mfd.csv and trying to verify a metric functional dependency in it.

              Title             Theatre  Duration
0       Don Quixote  Sydney Opera House       139
1       Don Quixote   Teatro alla Scala       135
2       Don Quixote   Grand Opera House       140
3        Cinderella   Teatro alla Scala       110
4        Cinderella   Grand Opera House       112
5  Romeo and Juliet  Sydney Opera House       160
6  Romeo and Juliet   Teatro alla Scala       163
7  Romeo and Juliet   Grand Opera House       165
Checking whether [Movie] -> [Duration] MFD holds with parameter δ = 5
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [Movie] -> [Duration] MFD holds with parameter δ = 3
\x1b[1;41m MFD does not hold due to the following items: \x1b[1;49m
\x1b[1;46m #1 cluster: \x1b[1;49m
         Title             Theatre  Duration
1  Don Quixote   Teatro alla Scala       135
2  Don Quixote   Grand Opera House       140
0  Don Quixote  Sydney Opera House       139
Max distance: 5.0 > 3
\x1b[1;46m #2 cluster: \x1b[1;49m
              Title             Theatre  Duration
5  Romeo and Juliet  Sydney Opera House       160
7  Romeo and Juliet   Grand Opera House       165
6  Romeo and Juliet   Teatro alla Scala       163
Max distance: 5.0 > 3

We learnt that in this case, the specified MFD does not hold, and there are two clusters of rows that contain values that prevent our MFD from holding. A \x1b[1;46mcluster\x1b[1;49m (with respect to a fixed FD) is a collection of rows that share the same left-hand side, but differ in the right-hand side.
Let's take a closer look at the indicated clusters.

"Don Quixote" has been found with 3 different durations: 135, 139 and 140. From this, we can conclude that (assuming euclidian distance), the maximum distance between values is 140 - 135 = 5, which exceeds the provided parameter of δ = 3. When we tried to verify the same MFD on this dataset with δ = 5, this cluster adhered to the MFD as 5 <= δ, and the whole MFD held.
"Romeo and Juliet" had the exact same issue: the biggest difference between durations of different versions appeared to have been 165 - 160 = 5, which meant that the MFD held with the greater parameter value of δ = 5.
--------------------------------------------------------------------------------
MFDs are not limited to metrics of one attribute. Let's take a look at an example that compares distance between \x1b[1;42mcoordinates\x1b[1;49m of addresses.
To do that, let's explore addresses_coordinates.csv and try verifying a metric functional dependency in it.

     Source                Address   Latitude   Longitude
0    google    65 N St Apt#C6, SLC  40.770896 -111.864066
1  geocoder    65 N St Apt#C6, SLC  40.770767 -111.863768
2    google    50 Cen Camp Dr, SLC  40.758951 -111.845246
3  geocoder    50 Cen Camp Dr, SLC  40.767599 -111.843995
4    google  35 S 700 E Apt#3, SLC  40.768370 -111.870640
5  geocoder  35 S 700 E Apt#3, SLC  40.768330 -111.870869
Checking whether [Address] -> [Latitude, Longitude] MFD holds with parameter δ = 1
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [Address] -> [Latitude, Longitude] MFD holds with parameter δ = 0.1
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [Address] -> [Latitude, Longitude] MFD holds with parameter δ = 0.01
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [Address] -> [Latitude, Longitude] MFD holds with parameter δ = 0.001
\x1b[1;41m MFD does not hold due to the following items: \x1b[1;49m
\x1b[1;46m #1 cluster: \x1b[1;49m
     Source              Address   Latitude   Longitude
2    google  50 Cen Camp Dr, SLC  40.758951 -111.845246
3  geocoder  50 Cen Camp Dr, SLC  40.767599 -111.843995
Max distance: 0.00873801493474823 > 0.001

Checking whether [Address] -> [Latitude, Longitude] MFD holds with parameter δ = 0.0001
\x1b[1;41m MFD does not hold due to the following items: \x1b[1;49m
\x1b[1;46m #1 cluster: \x1b[1;49m
     Source              Address   Latitude   Longitude
0    google  65 N St Apt#C6, SLC  40.770896 -111.864066
1  geocoder  65 N St Apt#C6, SLC  40.770767 -111.863768
Max distance: 0.00032472295884457205 > 0.0001
\x1b[1;46m #2 cluster: \x1b[1;49m
     Source              Address   Latitude   Longitude
2    google  50 Cen Camp Dr, SLC  40.758951 -111.845246
3  geocoder  50 Cen Camp Dr, SLC  40.767599 -111.843995
Max distance: 0.00873801493474823 > 0.0001
\x1b[1;46m #3 cluster: \x1b[1;49m
     Source                Address  Latitude   Longitude
4    google  35 S 700 E Apt#3, SLC  40.76837 -111.870640
5  geocoder  35 S 700 E Apt#3, SLC  40.76833 -111.870869
Max distance: 0.00023246720199186958 > 0.0001

As we can notice from the results, decreasing δ tightens the constraint, and, as such, yields more violations for smaller values. Namely, 0.001 is the first value we tried that resulted in the MFD no longer holding. The algorithm also provides us with record pairs that prevent the MFD from holding, with those records being grouped into \x1b[1;46mclusters\x1b[1;49m.

Let's take a closer look at them.

Both google and geocoder provided coordinates for multiple addresses that didn't have matching coordinates across different sources, yet were close enough for us to assume that they point to approximately the same place with a degree of accuracy that is sufficient for us to consider them basically the same. For example, in \x1b[1;46mCluster 3\x1b[1;49m, the appartments differ by 0.000229 in longitude and by 0.00004 in latitude, with the 2 points being merely around 0.01979 km (or 0.012298 miles) apart, which is considered to be the same place with parameter δ = 0.001, but violates the MFD with parameter δ = 0.0001.
--------------------------------------------------------------------------------
MFD discovery can even be performed on \x1b[1;42mstrings\x1b[1;49m using cosine distance.
Let's showcase this by checking addresses_names.csv and trying to verify a metric functional dependency in it.

      SOURCE               SSN                          ADDRESS
0   data.com  124-14-5903 1403         3rd Avenue, Cleveland OH
1   data.com  563-82-5145 1701   New York Av., Washington, D.C.
2  snoop.com  563-82-5145 1701  New York Ave., Washington, D.C.
3  snoop.com  124-14-5903 1403         Third Ave., Cleveland OH
Checking whether [SSN] -> [Address] MFD holds with parameter δ = 0.75
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [SSN] -> [Address] MFD holds with parameter δ = 0.5
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [SSN] -> [Address] MFD holds with parameter δ = 0.25
\x1b[1;42m MFD holds \x1b[1;49m
Checking whether [SSN] -> [Address] MFD holds with parameter δ = 0.1
\x1b[1;41m MFD does not hold due to the following items: \x1b[1;49m
\x1b[1;46m #1 cluster: \x1b[1;49m
      SOURCE               SSN                   ADDRESS
0   data.com  124-14-5903 1403  3rd Avenue, Cleveland OH
3  snoop.com  124-14-5903 1403  Third Ave., Cleveland OH
Max distance: 0.18518518518518523 > 0.1

Checking whether [SSN] -> [Address] MFD holds with parameter δ = 0.01
\x1b[1;41m MFD does not hold due to the following items: \x1b[1;49m
\x1b[1;46m #1 cluster: \x1b[1;49m
      SOURCE               SSN                   ADDRESS
0   data.com  124-14-5903 1403  3rd Avenue, Cleveland OH
3  snoop.com  124-14-5903 1403  Third Ave., Cleveland OH
Max distance: 0.18518518518518523 > 0.01
\x1b[1;46m #2 cluster: \x1b[1;49m
      SOURCE               SSN                          ADDRESS
1   data.com  563-82-5145 1701   New York Av., Washington, D.C.
2  snoop.com  563-82-5145 1701  New York Ave., Washington, D.C.
Max distance: 0.04749904749857124 > 0.01

To get into the intricacies of how the distance has been calculated here, we need to first define what is known as an n-gram. \x1b[1;46mN-grams\x1b[1;49m are collections of adjacent symbols of fixed length, such as "aab" or "ba". Depending on the length, they are called bigrams (2), trigrams (3) and so on.

For the purposes of illustration, we picked the RHS values from the cluster violating the MFD with δ = 0.01 but not δ = 0.1. The following table displays the number of occurrences of every bigram (columns) occurring in any of these values (rows). In total, there are 30 bigrams across two strings.
                         Address  Ne  ew  w    Y  Yo  ...  n,   D  D.  .C  C.  v.
New York Ave. Washington    D.C.   1   1   1   1   1  ...   1   1   1   1   1   0
New York Av.  Washington    D.C.   1   1   1   1   1  ...   1   1   1   1   1   1

[2 rows x 31 columns]
Let\'s interpret the rows in the table as coordinates of two vectors. We have ways to compare them. For example, we can assess how similar they are by calculating S(A, B) = A*B / (||A||*||B||), where A*B is a dot product, and ||A|| is a magnitude of vector A. This is called "cosine similarity". To quantify how different two strings are, we\'ll use the metric Δ(x, y) = 1 - S(x, y), also known as "cosine distance".
In this example, a parameter of δ = 0.1 is sufficient for the algorithm to not consider "New York Av., Washington, D.C." and "New York Ave., Washington, D.C." to be different addresses.
'''

snapshots['test_example[basic/verifying_nd/verifying_nd_1.py-None-verifying_nd_1_output] verifying_nd_1_output'] = '''\x1b[1m\x1b[36mThis example illustrates the usage of Numerical Dependencies (NDs).
Intuitively, given two sets of attributes X and Y, there is an ND from X to Y (denoted X -> Y, weight = k)
if each value of X can never be associated to more than k distinct values of Y.
For more information consult "Efficient derivation of numerical dependencies" by Paolo Ciaccia et al.
\x1b[0m
Citizens of Arstozka can have no more than two documents: one Arstozka passport and one exit permit.
The following table contains records of some citizens' documents:
\x1b[1m\x1b[36mName             ID            Issuing city    Entry permit   Expiration date   
--------------------------------------------------------------------------------
Kordon Kallo     375F0-KE12I   Orvech Vonor                   05.03.2040        
Nathan Cykelek   9I2-4H2                       Kolechia       09.10.2028        
Grant Baker      1GMFL-5LRD6   East Greshtin                  28.07.2039        
Kordon Kallo     7JH-35A                       Orbistan       07.01.2027        
Grant Baker      8H6-772                       Antegria       19.11.2029        
Kordon Kallo     7ND-93L                       Cobrastan      08.06.2001        
Khaled Istom     9KLA2-HH66N   East Greshtin                  21.12.2041        
\x1b[0m
We need to validate these data
Let's run ND verification algorithm to check that every citizen has no more than two records:
\tND: {Name} -> {ID} with weight 2
\tND holds: \x1b[31mFalse\x1b[0m
\tActual weight is 3

Let's look at clusters violating ND:
Number of clusters: 1
\x1b[1m\x1b[36mName           ID            Issuing city   Entry permit   Expiration date   
-----------------------------------------------------------------------------
Kordon Kallo   375F0-KE12I   Orvech Vonor                  05.03.2040        
Kordon Kallo   7JH-35A                      Orbistan       07.01.2027        
Kordon Kallo   7ND-93L                      Cobrastan      08.06.2001        
\x1b[0m
So, (Kordon Kallo) has 3 documents
One of them is expired and shouldn't appear in this table. Let's remove this line:
\x1b[1m\x1b[36mName             ID            Issuing city    Entry permit   Expiration date   
--------------------------------------------------------------------------------
Kordon Kallo     375F0-KE12I   Orvech Vonor                   05.03.2040        
Nathan Cykelek   9I2-4H2                       Kolechia       09.10.2028        
Grant Baker      1GMFL-5LRD6   East Greshtin                  28.07.2039        
Kordon Kallo     7JH-35A                       Orbistan       07.01.2027        
Grant Baker      8H6-772                       Antegria       19.11.2029        
Khaled Istom     9KLA2-HH66N   East Greshtin                  21.12.2041        
\x1b[0m
Let's run algorithm again:
\tND holds: \x1b[32mTrue\x1b[0m
'''

snapshots['test_example[basic/verifying_nd/verifying_nd_2.py-None-verifying_nd_2_output] verifying_nd_2_output'] = '''\x1b[1m\x1b[36mThis example illustrates the usage of Numerical Dependencies (NDs).
Intuitively, given two sets of attributes X and Y, there is an ND from X to Y (denoted X -> Y, weight = k)
if each value of X can never be associated to more than k distinct values of Y.
For more information consult "Efficient derivation of numerical dependencies" by Paolo Ciaccia et al.
\x1b[0m
Citizens of Arstozka can have no more than two documents: one Arstozka passport and one exit permit.
The following table contains records of some citizens' documents:
\x1b[1m\x1b[36mName             ID            Issuing city    Entry permit   Expiration date   Birth date   
---------------------------------------------------------------------------------------------
Kordon Kallo     375F0-KE12I   Orvech Vonor                   05.03.2040        05.03.2001   
Nathan Cykelek   9I2-4H2                       Kolechia       09.10.2028        09.10.1993   
Kordon Kallo     1GMFL-5LRD6   East Greshtin                  28.07.2039        28.07.1989   
Kordon Kallo     7JH-35A                       Orbistan       07.01.2027        05.03.2001   
Kordon Kallo     8H6-772                       Antegria       19.11.2029        28.07.1989   
Khaled Istom     9KLA2-HH66N   East Greshtin                  21.12.2041        21.12.2004   
\x1b[0m
We need to validate these data
Let's run ND verification algorithm to check that every citizen has no more than two records:
\tND: {Name} -> {ID} with weight 2
\tND holds: \x1b[31mFalse\x1b[0m
\tActual weight is 4

Let's look at clusters violating ND:
Number of clusters: 1
\x1b[1m\x1b[36mName           ID            Issuing city    Entry permit   Expiration date   Birth date   
-------------------------------------------------------------------------------------------
Kordon Kallo   375F0-KE12I   Orvech Vonor                   05.03.2040        05.03.2001   
Kordon Kallo   1GMFL-5LRD6   East Greshtin                  28.07.2039        28.07.1989   
Kordon Kallo   7JH-35A                       Orbistan       07.01.2027        05.03.2001   
Kordon Kallo   8H6-772                       Antegria       19.11.2029        28.07.1989   
\x1b[0m
So, (Kordon Kallo) has 4 documents. It's twice as much as needed.
Look at birth date. (Kordon Kallo) has two different values.
Maybe, we have two different (Kordon Kallo)? Let's split them:
\x1b[1m\x1b[36mName               ID            Issuing city    Entry permit   Expiration date   Birth date   
-----------------------------------------------------------------------------------------------
Kordon Kallo       375F0-KE12I   Orvech Vonor                   05.03.2040        05.03.2001   
Nathan Cykelek     9I2-4H2                       Kolechia       09.10.2028        09.10.1993   
Kordon Kallo (1)   1GMFL-5LRD6   East Greshtin                  28.07.2039        28.07.1989   
Kordon Kallo       7JH-35A                       Orbistan       07.01.2027        05.03.2001   
Kordon Kallo (1)   8H6-772                       Antegria       19.11.2029        28.07.1989   
Khaled Istom       9KLA2-HH66N   East Greshtin                  21.12.2041        21.12.2004   
\x1b[0m
Let's run algorithm again:
\tND holds: \x1b[32mTrue\x1b[0m
'''

snapshots['test_example[basic/verifying_nd/verifying_nd_3.py-None-verifying_nd_3_output] verifying_nd_3_output'] = '''\x1b[1m\x1b[36mThis example illustrates the usage of Numerical Dependencies (NDs).
Intuitively, given two sets of attributes X and Y, there is an ND from X to Y (denoted X -> Y, weight = k)
if each value of X can never be associated to more than k distinct values of Y.
For more information consult "Efficient derivation of numerical dependencies" by Paolo Ciaccia et al.
\x1b[0m
Citizens of Arstozka can have no more than two documents: one Arstozka passport and one exit permit.
The following table contains records of some citizens' documents:
\x1b[1m\x1b[36mFirst name   Last name   ID            Issuing city    Entry permit   Expiration date   
----------------------------------------------------------------------------------------
Kordon       Kallo       375F0-KE12I   Orvech Vonor                   05.03.2040        
Nathan       Kallo       9I2-4H2                       Kolechia       09.10.2028        
Khaled       Baker       1GMFL-5LRD6   East Greshtin                  28.07.2039        
Kordon       Kallo       7JH-35A                       Orbistan       07.01.2027        
Khaled       Baker       8H6-772                       Antegria       19.11.2029        
Kordon       Kallo       7ND-93L                       Cobrastan      08.06.2001        
Khaled       Istom       9KLA2-HH66N   East Greshtin                  21.12.2041        
\x1b[0m
We need to validate these data
In this table, the first names and last names are separated into different columns.
We'll use a more complex case, where X contains more than one attribute.
Let's run ND verification algorithm to check that every citizen has no more than two records:
\tND: {First name, Last name} -> {ID} with weight 2
\tND holds: \x1b[31mFalse\x1b[0m
\tActual weight is 3

Let's look at clusters violating ND:
Number of clusters: 1
\x1b[1m\x1b[36mFirst name   Last name   ID            Issuing city   Entry permit   Expiration date   
---------------------------------------------------------------------------------------
Kordon       Kallo       375F0-KE12I   Orvech Vonor                  05.03.2040        
Kordon       Kallo       7JH-35A                      Orbistan       07.01.2027        
Kordon       Kallo       7ND-93L                      Cobrastan      08.06.2001        
\x1b[0m
So, (Kordon, Kallo) has 3 documents
One of them is expired and shouldn't appear in this table. Let's remove this line:
\x1b[1m\x1b[36mFirst name   Last name   ID            Issuing city    Entry permit   Expiration date   
----------------------------------------------------------------------------------------
Kordon       Kallo       375F0-KE12I   Orvech Vonor                   05.03.2040        
Nathan       Kallo       9I2-4H2                       Kolechia       09.10.2028        
Khaled       Baker       1GMFL-5LRD6   East Greshtin                  28.07.2039        
Kordon       Kallo       7JH-35A                       Orbistan       07.01.2027        
Khaled       Baker       8H6-772                       Antegria       19.11.2029        
Khaled       Istom       9KLA2-HH66N   East Greshtin                  21.12.2041        
\x1b[0m
Let's run algorithm again:
\tND holds: \x1b[32mTrue\x1b[0m
'''

snapshots['test_example[basic/verifying_pfd.py-None-verifying_pfd_output] verifying_pfd_output'] = '''Dataset: examples/datasets/glitchy_sensor_2.csv
    Id DeviceId  Data
0    1      D-1  1001
1    2      D-1  1002
2    3      D-1  1003
3    4      D-1  1004
4    5      D-1  1005
5    6      D-1  1006
6    7      D-2  1000
7    8      D-2  1001
8    9      D-2  1000
9   10      D-3  1010
10  11      D-4  1011
11  12      D-4  1011
12  13      D-5  1015
13  14      D-5  1014
14  15      D-5  1015
15  16      D-5  1015

--------------------------------------------------------------------------------
Checking whether PFD (DeviceId) -> (Data) holds for per_value error measure
--------------------------------------------------------------------------------
PFD holds
--------------------------------------------------------------------------------
Checking whether the same PFD holds for per_tuple error measure:
--------------------------------------------------------------------------------
PFD with error 0.3 does not hold
But it holds with error 0.4375

Additional info:
Number of rows violating PFD: 7
Number of clusters violating PFD: 3

First violating cluster:
   Id DeviceId  Data
0   1      D-1  1001
1   2      D-1  1002
2   3      D-1  1003
3   4      D-1  1004
4   5      D-1  1005
5   6      D-1  1006

Second violating cluster:
   Id DeviceId  Data
6   7      D-2  1000
7   8      D-2  1001
8   9      D-2  1000

Third violating cluster:
    Id DeviceId  Data
12  13      D-5  1015
13  14      D-5  1014
14  15      D-5  1015
15  16      D-5  1015

'''

snapshots['test_example[basic/verifying_ucc.py-None-verifying_ucc_output] verifying_ucc_output'] = '''Checking whether (First Name) UCC holds
UCC does not hold
Total number of rows violating UCC: 2
Number of clusters violating UCC: 1
Clusters violating UCC:
[4, 5]

Checking whether (First Name, Last Name) UCC holds
UCC holds

Checking whether (Born Town, Born Country) UCC holds
UCC does not hold
Total number of rows violating UCC: 5
Number of clusters violating UCC: 2
Clusters violating UCC:
[2, 3, 4]
[6, 7]

'''

snapshots['test_example[expert/anomaly_detection.py-None-anomaly_detection_output] anomaly_detection_output'] = '''FDs found for dataset 1:
[item_id] -> item_weight
[item_weight] -> item_id
[record_id] -> cargo_id
[record_id] -> item_id
[record_id] -> item_weight
[record_id] -> timestamp
[timestamp] -> cargo_id
[timestamp] -> item_id
[timestamp] -> item_weight
[timestamp] -> record_id
FDs found for dataset 2:
[item_id] -> item_weight
[item_weight] -> item_id
[record_id] -> cargo_id
[record_id] -> item_id
[record_id] -> item_weight
[record_id] -> timestamp
[timestamp] -> cargo_id
[timestamp] -> item_id
[timestamp] -> item_weight
[timestamp] -> record_id
FDs found for dataset 3:
[item_weight] -> item_id
[record_id] -> cargo_id
[record_id] -> item_id
[record_id] -> item_weight
[record_id] -> timestamp
[timestamp] -> cargo_id
[timestamp] -> item_id
[timestamp] -> item_weight
[timestamp] -> record_id
AFDs found for dataset 3:
[item_id cargo_id] -> item_weight
[item_weight] -> item_id
[record_id] -> cargo_id
[record_id] -> item_id
[record_id] -> item_weight
[record_id] -> timestamp
[timestamp] -> cargo_id
[timestamp] -> item_id
[timestamp] -> item_weight
[timestamp] -> record_id
MFD holds.
'''

snapshots['test_example[expert/data_cleaning_dc.py-None-data_cleaning_dc_output] data_cleaning_dc_output'] = '''This is an advanced example explaining how to use Denial Constraint (DC) verification for data cleaning.
A basic example of using Denial Constraints is located in examples/basic/verifying_dc.py.

DC verification is perfomed by the Rapidash algorithm:
Zifan Liu, Shaleen Deep, Anna Fariha, Fotis Psallidas, Ashish Tiwari, and Avrilia
Floratou. 2023. Rapidash: Efficient Constraint Discovery via Rapid Verification.
URL: https://arxiv.org/abs/2309.12436

DC φ is a conjunction of predicates of the following form:
∀s, t ∈ R, s ≠ t: ¬(p_1 ∧ . . . ∧ p_m)

DCs involve comparisons between pairs of rows within a dataset.
A typical DC example, derived from a Functional Dependency such as A -> B,
is expressed as: "∀s, t ∈ R, s ≠ t, ¬(t.A == s.A ∧ t.B ≠ s.B)."
This denotes that for any pair of rows in the relation, it should not be the case
that while the values in column A are equal, the values in column B are unequal.

Consider the following dataset:

1       State Salary FedTaxRate
2     NewYork   3000        0.2
3     NewYork   4000       0.25
4     NewYork   5000        0.3
5     NewYork   6000       0.04
6   Wisconsin   5000       0.15
7   Wisconsin   6000        0.2
8   Wisconsin   4000        0.1
9   Wisconsin   3000        0.9
10      Texas   1000       0.15
11      Texas   2000       0.25
12      Texas   3000        0.3
13      Texas   5000       0.05

And the following Denial Constraint:
!(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate).
We use "and" instead of "∧" and "¬" instead of "!" for easier representation.

The constraint tells us that for all people in the same state,
a person with a higher salary must have a higher tax rate.
Now, we run the algorithm in order to see if the constraint holds.

DC !(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate) holds: False

Now let's examine why the constraint doesn't hold. To do this,
we can get the violations (pairs of rows that contradict the given constraint):
(2, 5), (3, 5), (4, 5), (6, 9), (7, 9), (8, 9), (10, 13), (11, 13), (12, 13)

As we can see, there are multiple pairs of rows contradicting the given constraint.
We can leverage this knowledge to find typos or other mistakes.

Denial constraint is a very powerful tool for detecting various inaccuracies
and errors in data. Our implementation allows users to additionally identify violations — pairs
of records for which the condition described by the Denial Constraint is not satisfied.
If we are confident that a certain DC should hold, we can then construct an algorithm to repair the data.

We will demonstrate this by creating a proof-of-concept algorithm based on a graph approach.
The vertices in our graph will be the records, and the presence of a violation will be represented
as an edge between two vertices. Thus, our task can be reformulated as follows: to find the
minimum number of vertices that need to be removed in order for the graph to become edgeless.
In this process, we remove vertices in such a way that all incident edges are also removed.
At the record level, removing a vertex will mean performing an operation to delete or edit
the record, such that the edge (violation) no longer exists.

In the code, this graph functionality is implemented (albeit in a naive way) inside the DataCleaner class.

Close figure windows to continue
The cleaning algorithm returns the following records: 5, 9, 13
This means we should consider the following records incorrect and correct them.
Indeed, the following records contain typos and should be changed:

5     NewYork   6000       0.04  -->    NewYork   6000         0.4
9   Wisconsin   3000        0.9  -->  Wisconsin   3000        0.09
13      Texas   5000       0.05  -->      Texas   5000         0.5


The dataset after repairs:

1       State Salary FedTaxRate
2     NewYork   3000        0.2
3     NewYork   4000       0.25
4     NewYork   5000        0.3
5     NewYork   6000        0.4
6   Wisconsin   5000       0.15
7   Wisconsin   6000        0.2
8   Wisconsin   4000        0.1
9   Wisconsin   3000       0.09
10      Texas   1000       0.15
11      Texas   2000       0.25
12      Texas   3000        0.3
13      Texas   5000        0.5

Now we can check if the constraint holds by running the algorithm again.

DC !(s.State == t.State and s.Salary < t.Salary and s.FedTaxRate > t.FedTaxRate) holds: True

After fixing the typos in the initial dataset, the constraint holds.
'''

snapshots['test_example[expert/dedupe.py-dedupe_input.txt-dedupe_output] dedupe_output'] = '''Deduplication parameters:
ALGORITHM='Pyro'
ERROR=0.00100
DATASET_PATH='examples/datasets/duplicates.csv'
SEPARATOR=','
INITIAL_WINDOW_SIZE=4

Dataset sample:
      id             name address       city                          email phone country
0   5996        Kaede Sue      66      Pirus       Kaede.Sue4422@virtex.rum    39      EU
1     36       Licia Wolf      35  Pilington       Licia.Wolf1260@cmail.com    35      CM
2     17        Steve Doe      16     Syndye           Steve.Doe272@muli.ry    16      GZ
3     62      Lisa Tarski      61     Syndye     Lisa.Tarski3782@virtex.rum    61      JU
4      6      Mary Tarski       5     Lumdum       Mary.Tarski30@ferser.edu     5      PR
..   ...              ...     ...        ...                            ...   ...     ...
73    15        Ivan Dawn      14     Syndye      Ivan.Dawn210@atomlema.ocg    14      FC
74  5993       Lisa Honjo      63       Roit      Lisa.Honjo4032@virtex.rum    63      AI
75    59         Lisa Sue      58     Muxicu         Lisa.Sue3422@cmail.com    58      AI
76    21  Steve Shiramine      20  Pilington  Steve.Shiramine420@ferser.edu    20      GZ
77    44      Maxine Wolf      43     Muxicu   Maxine.Wolf1892@atomlema.ocg    43      PR

[78 rows x 7 columns]
Original records: 78

AFD info:
0: id -> ( name address city email phone country )
2: address -> ( name )
4: email -> ( name address phone country )
5: phone -> ( name )
LHS column index: RHS columns:
1: name
2: address
3: city
4: email
5: phone
6: country
RHS columns to use (indices): Equal columns to consider duplicates:       id          name address      city                       email phone country
5     27     Björn Sue      26      Roit      Björn.Sue702@cmail.com    26      CM
6     30  Björn Tarski      29    Lumdum  Björn.Tarski870@ferser.edu    29      PR
7   5957    Björn Wolf      27              Björn.Wolf756@virtex.rum    27      AI
8     28    Björn Wolf      27              Björn.Wolf756@virtex.rum    27      AI
9  11886    Björn Wolf      28  Kustruma    Björn.Wolf756@virtex.rum    27      AI
Command: Column: id. Which value to use?
0: 11886
1: 28
2: 5957
index: Column: address. Which value to use?
0: 27
1: 28
index: Column: city. Which value to use?
0: 
1: Kustruma
index:    id          name address    city                       email phone country
5  27     Björn Sue      26    Roit      Björn.Sue702@cmail.com    26      CM
6  30  Björn Tarski      29  Lumdum  Björn.Tarski870@ferser.edu    29      PR
Command:       id        name address       city                       email phone country
42    63   Lisa Dawn      62  Pilington  Lisa.Dawn3906@atomlema.ocg    62      EU
43    57    Lisa Doe      56       Roit     Lisa.Doe3192@virtex.rum    56      AI
44    64  Lisa Honjo      63      Pirus   Lisa.Honjo4032@virtex.rum    63      AI
45  5993  Lisa Honjo      63       Roit   Lisa.Honjo4032@virtex.rum    63      AI
Command:        id       name address    city                     email phone country
50     60  Lisa Wolf      59  Syndye   Lisa.Wolf3540@cmail.com    59      FC
51      7  Mary Dawn       6  Syndye  Mary.Dawn42@atomlema.ocg     6      PR
52   5930   Mary Doe          Lumdum  Mary.Doe-5926@ferser.edu     0        
53  11859   Mary Doe          Lumdum  Mary.Doe-5926@ferser.edu     0      EU
54      1   Mary Doe          Lumdum         Mary.Doe0@muli.ry     4      EU
Command: Column: id. Which value to use?
0: 11859
1: 5930
index: Column: country. Which value to use?
0: 
1: EU
index:     id       name address    city                     email phone country
50  60  Lisa Wolf      59  Syndye   Lisa.Wolf3540@cmail.com    59      FC
51   7  Mary Dawn       6  Syndye  Mary.Dawn42@atomlema.ocg     6      PR
54   1   Mary Doe          Lumdum         Mary.Doe0@muli.ry     4      EU
Command: 
Resulting records: 75. Duplicates found: 3
    id             name address       city                          email phone country
0   31       Björn Dawn      30     Muxicu     Björn.Dawn930@atomlema.ocg    30      JU
1   25        Björn Doe      24  Pilington           Björn.Doe600@muli.ry    24      FC
2   32      Björn Honjo      31   Kustruma      Björn.Honjo992@virtex.rum    31      RI
3   29  Björn Shiramine      28     Syndye  Björn.Shiramine812@virtex.rum    28      EU
4   26      Björn Smith      25  Pilington      Björn.Smith650@virtex.rum    25      RI
..  ..              ...     ...        ...                            ...   ...     ...
70  21  Steve Shiramine      20  Pilington  Steve.Shiramine420@ferser.edu    20      GZ
71  20       Steve Wolf      19  Pilington          Steve.Wolf380@muli.ry    19      RI
72  22     Steve Tarski      21  Pilington   Steve.Tarski462@atomlema.ocg    21      PR
73  19        Steve Sue      18     Syndye        Steve.Sue342@virtex.rum    18      AI
74  18      Steve Smith      17     Lumdum       Steve.Smith306@cmail.com    17      EU

[75 rows x 7 columns]
'''

snapshots['test_example[expert/mine_typos.py-None-mine_typos_output] mine_typos_output'] = '''Starting typo discovery scenario with parameters:
RADIUS=3
RATIO=0.1
ERROR=0.005
DATASET_PATH='examples/datasets/Workshop.csv'
EXACT_ALGORITHM='HyFD'
APPROXIMATE_ALGORITHM='Pyro'
HEADER=0
SEPARATOR=','

Dataset sample:
                                       id      worker_name supervisor_surname       workshop salary                   job_post
0    404f50cb-caf0-4974-97f9-9463434537e1   Jennifer Moore        Galen Calla    Yogatacular    980    Client Solution Analyst
1    b5e38281-9c09-49bf-91f5-c55397df4d43       Edward Lee      Carrie Silvia    MonsterWorq    905  Front-End Loader Operator
2    972b299d-2f27-4d6d-81d2-8effbc543bf1        Brian Lee      Shena Desiree  Talkspiration    700             Farm Assistant
3    3241fb48-5a15-4638-bd68-d915834a3f89   Kenneth Turner        Paul Jeffry     Verbalthon    980    Client Solution Analyst
4    9cbb9026-f157-4a01-aace-a42b05ab2a28   Betty Campbell    Addyson Aaliyah     SpeakerAce    800            Physiotherapist
..                                    ...              ...                ...            ...    ...                        ...
940  9cd700bc-b3d9-439d-afe9-945c2a20bc37    Richard Lopez        Galen Calla    Yogatacular    845   Senior Financial Planner
941  cc199ff4-453a-4ae5-9fbd-b45d72fa952a  Helen Rodriguez      Carrie Silvia    MonsterWorq    465                Electrician
942  de650347-880a-42a2-88c9-4329f26fb912      Karen White      Carrie Silvia    MonsterWorq    510       JavaScript Developer
943  ae604e24-e040-4d50-b685-5b4897ab9ae9    Charles Smith      Shena Desiree  Talkspiration    975              Store Manager
944  d5cb954a-e942-47ae-9b62-b57f7a84c2db        Jeff King      Carrie Silvia    MonsterWorq    465                Electrician

[945 rows x 6 columns]

Searching for almost holding FDs...

Found! Almost holding FDs:
[supervisor_surname salary] -> job_post
[supervisor_surname job_post] -> salary
[workshop] -> supervisor_surname
[workshop salary] -> job_post
[workshop job_post] -> salary

Selecting FD with index 2:
 rows count    workshop supervisor_surname
\x1b[32m        198 Yogatacular        Galen Calla\x1b[0m
\x1b[31m          1 Yogatacular      Galen Calella\x1b[0m

Typo candidates and context:
                                     id       worker_name supervisor_surname     workshop salary                 job_post
0  404f50cb-caf0-4974-97f9-9463434537e1    Jennifer Moore        Galen Calla  Yogatacular    980  Client Solution Analyst
7  ddba9118-ec89-472d-9f3f-bebd919f0e3a  William Robinson      Galen Calella  Yogatacular    975            Store Manager
'''
