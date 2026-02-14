# -*- coding: utf-8 -*-
# snapshottest: v1 - https://goo.gl/zC4yUc
from __future__ import unicode_literals

from snapshottest import Snapshot


snapshots = Snapshot()

snapshots['test_example[advanced/verifying_pac/verifying_domain_pac_custom_domain.py-None-verifying_domain_pac_custom_domain_output] verifying_domain_pac_custom_domain_output'] = '''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
A Domain PAC on a column set X and domain D, with given ε and δ means that \x1b[1;37mPr(x ∈ D±ε) ≥ δ\x1b[0m.
For more information, see "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Filp Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).
If you have not read the basic Domain PAC examples yet (see the \x1b[36mexamples/basic/verifying_pac/\x1b[0m
directory), it is recommended to start there.

Assume we have a dataset of user preferences, where each user's interest in several topics is
encoded as values in [0, 1], where 0 is "not interested at all" and 1 is "very interested":
\x1b[1;37m  Databases    Networks    Machine learning
-----------  ----------  ------------------
     0.7           0.2                1
     0.4           0.9                0.2
     0.85          0.4                0
     0.91          0.38               0.05
     0.75          0.41               0.1
     0.3           0.2                0.7
     0.77          0.2                0.01
     1             0.5                0.1
     0.885         0.41               0.02
     0.9009        0.4                0.037\x1b[0m

We need to estimate whether this group of users will be interested in the original Domain PAC paper
("Checks and Balances: ...").
To do this, we represent each user profile as a vector in a multi-dimensional topic space:
     ^ Topic 2
     |
     |   user
     |  x
     | /
     |/    Topic 1
    -+------->
     |

A "perfect" target reader might have the profile: \x1b[34m(0.9, 0.4, 0.05)\x1b[0m.
This corresponds to:
    * high interest in Databases;
    * moderate interest in Networks;
    * low interest in Machine Learning.
Our goal is to measure how close real users are to this ideal profile.

We use cosine distance, which measures the angle between two vectors rather then their absolute
length. This is useful because we care about interest proportions, not total magnitude.
    \x1b[1;37mdist(x, y) = 1 - cos(angle between x and y) = 1 - (x, y)/(|x| * |y|)\x1b[0m,
where (x, y) is a dot product between x and y.

A custom domain is defined by two parameters:
    1. Distance function -- takes a value tuple and returns the distance to the domain.
    2. Domain name (optional) -- used for readable output.
In this example:
    * distance function: \x1b[34mdist(x, (0.9, 0.4, 0.05))\x1b[0m;
    * domain name: \x1b[34m"(0.9, 0.4, 0.05)"\x1b[0m.
This effectively defines the domain as "users close to the ideal profile".

We run the Domain PAC verifier with domain=\x1b[34m(0.9, 0.4, 0.05)\x1b[0m.
Algorithm result:
    \x1b[32mDomain PAC Pr(x ∈ (0.9, 0.4, 0.05)±0.37695) ≥ 0.9 on columns [Databases Networks Machine learning]\x1b[0m
Now we lower the required probability threshold: min_delta=\x1b[34m0.6\x1b[0m.
Algorithm result:
    \x1b[32mDomain PAC Pr(x ∈ (0.9, 0.4, 0.05)±0.0141436) ≥ 0.7 on columns [Databases Networks Machine learning]\x1b[0m
Interpretation:
    * With a larger ε (\x1b[34m0.377\x1b[0m), nearly all users show some level of interest
    * With a very small ε (\x1b[34m0.014\x1b[0m), only \x1b[34m70%\x1b[0m of users closely match the ideal reader.

You can user highlights to identify which users are closer to or farther from the ideal profile.
For an introduction to highlights, see \x1b[36mexamples/basic/verifying_pac/verifying_domain_pac1.py\x1b[0m.
'''

snapshots['test_example[basic/verifying_pac/verifying_domain_pac1.py-None-verifying_domain_pac1_output] verifying_domain_pac1_output'] = '''This example illustrates the usage of Domain Probabilistic Approximate Constraints (PACs).
A Domain PAC on column set X and domain D, with given ε and δ means that Pr(x ∈ D±ε) ≥ δ.
For more information consult "Checks and Balances: Monitoring Data Quality Problems in Network
Traffic Databases" by Flip Korn et al (Proceedings of the 29th VLDB Conference, Berlin, 2003).

This is the first example in the "Basic Domain PAC verification" series. Others can be found in
\x1b[36mexamples/basic/verifying_pac/\x1b[0m directory.

Suppose we are working on a new model of engine. Its operating temperature range is \x1b[34m[85, 95]\x1b[0m°C.
The engine is made of high-strength metal, so short-term temperature deviations are acceptable and
will not cause immediate damage. In other words, engine operates properly when Pr(t ∈ [85, 95]±ε) ≥ δ.
Based on engineering analysis, the acceptable limits are: ε = \x1b[34m5\x1b[0m, δ = \x1b[34m0.9\x1b[0m.
In terms of Domain PACs, the following constraint should hold: \x1b[34mPr(x ∈ [85, 95]±5) ≥ 0.9\x1b[0m.

The following table contains readings from the engine temperature sensor:
\x1b[1;37mt: [79, 78, 78, 78, 79, 80, 79, 85, 90, 89, 94, 96, 104, 93, 90, 88, 86, 84, 87, 90, 95, 92]\x1b[0m

We now use the Domain PAC verifier to determine whether the engine is operating safely.
First, we need to define the domain. A segment is a special case of a parallelepiped, so we use it here.
We run algorithm with the following options: domain=\x1b[34m[85, 95]\x1b[0m.
All other parameters use default values: min_epsilon=\x1b[34m0\x1b[0m, max_epsilon=\x1b[34m∞\x1b[0m, min_delta=\x1b[34m0.9\x1b[0m, delta_steps=\x1b[34m100\x1b[0m.

Algorithm result: \x1b[33mDomain PAC Pr(x ∈ [85, 95]±7) ≥ 0.954545 on columns [t]\x1b[0m.
This PAC is not very informative. Let's run algorithm with min_epsilon=\x1b[34m5\x1b[0m and max_epsilon=\x1b[34m5\x1b[0m.
This will give us the exact δ, for which PAC with ε=\x1b[34m5\x1b[0m holds.

Algorithm result: \x1b[31mDomain PAC Pr(x ∈ [85, 95]±5) ≥ 0.681818 on columns [t]\x1b[0m.
Also, let's run algorithm with max_epsilon=\x1b[34m0\x1b[0m and min_delta=\x1b[34m0.9\x1b[0m to check which ε
is needed to satisfy δ=\x1b[34m0.9\x1b[0m. With these parameters algorithm enters special mode and returns
pair (ε, min_delta), so that we can validate PAC with the given δ.

Algorithm result: \x1b[31mDomain PAC Pr(x ∈ [85, 95]±7) ≥ 0.954545 on columns [t]\x1b[0m.
Here algorithm gives δ=\x1b[34m0.9545454545454546\x1b[0m, which is greater than \x1b[34m0.9\x1b[0m, because achieving δ=\x1b[34m0.9\x1b[0m requires
ε=\x1b[34m7.0\x1b[0m and PAC (\x1b[34m7.0\x1b[0m, \x1b[34m0.9545454545454546\x1b[0m) holds. So, this means that δ=\x1b[34m0.9\x1b[0m would also require ε=\x1b[34m7.0\x1b[0m.

We can see that desired PAC doesn't hold, so the engine can blow up!

Let's look at values violating PAC. Domain PAC verifier can detect values between eps_1
and eps_2, i. e. values that lie in D±eps_2 \\ D±eps_1. Such values are called highlights or outliers.
Let's find outliers for different eps_1, eps_2 values:
  eps_1    eps_2  highlights
-------  -------  ------------------------
      \x1b[34m0\x1b[0m        \x1b[34m1\x1b[0m  [96, 84]
      \x1b[34m1\x1b[0m        \x1b[34m2\x1b[0m
      \x1b[34m2\x1b[0m        \x1b[34m3\x1b[0m
      \x1b[34m3\x1b[0m        \x1b[34m5\x1b[0m  [80]
      \x1b[34m5\x1b[0m        \x1b[34m7\x1b[0m  [79, 79, 79, 78, 78, 78]
      \x1b[34m7\x1b[0m       \x1b[34m10\x1b[0m  [104]

We can see two problems:
    1. The engine operated at low temperatures for an extended period, slightly below 80°C.
    2. The peak temperature was too high, but this occurred only once.

The second version of engine has:
    1. A pre-heating system to prevent operation at low temperatures.
    2. An emergency cooling system to limit peak temperatures.
The updated sensor readings (modified values highlighted) are:
\x1b[1;37mt: [79, \x1b[1;33m80\x1b[1;37m, \x1b[1;33m81\x1b[1;37m, \x1b[1;33m82\x1b[1;37m, \x1b[1;33m81\x1b[1;37m, \x1b[1;33m81\x1b[1;37m, \x1b[1;33m83\x1b[1;37m, 85, 90, 89, 94, 96, \x1b[1;33m100\x1b[1;37m, 93, 90, 88, 86, 84, 87, 90, 95, 92]\x1b[0m

We run the Domain PAC verifier again.
Algorithm result: \x1b[32mDomain PAC Pr(x ∈ [85, 95]±5) ≥ 0.954545 on columns [t]\x1b[0m.
The desired PAC now holds, which means the improved engine operates within acceptable limits.

It is recommended to continue with the second example (\x1b[36mexamples/basic/verifying_pac/verifying_domain_pac2.py\x1b[0m),
which demonstrates more advanced usage of the Parallelepiped domain.
'''

snapshots['test_example[basic/verifying_pac/verifying_domain_pac2.py-None-verifying_domain_pac2_output] verifying_domain_pac2_output'] = '''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs)
on multiple columns. It continues the first Domain PAC verification example
\x1b[36m(examples/basic/verifying_pac/verifying_domain_pac1.py)\x1b[0m. If you have not read the first part yet,
it is recommended to start there.

In the first example we verified \x1b[34mDomain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9\x1b[0m on engine temperature sensor readings.
Now, in addition to temperature readings, we also have tachometer data:
\x1b[1;37m  t    rpm
---  -----
 79    900
 78   2000
 78   3000
 78   2500
 79   3600
 80   3000
 79   2500
 85   1500
 90   1000
 89   1400
 94   1700
 96   1800
104   3500
 93   2700
 90   2100
 88   1900
 86   1500
 84   2000
 87   3100
 90   3400
 95   4000
 92   2800\x1b[0m

The normal operating RPM for this engine is \x1b[34m[1500, 3500]\x1b[0m. Values outside this range are
not harmful by themselves (as long as they are within \x1b[34m[0, 5000]\x1b[0m), but:
    * A cold engine may stall at low RPM and can be damaged at high RPM.
    * An overheated engine is especially vulnerable at RPM values outside \x1b[34m[1500, 3500]\x1b[0m, because
      cooling efficiency depends on RPM.
As in the first example, we use the Domain PAC verifier to check whether the engine operates properly.

Firstly, we need to create domain. We have a Cartesian product of two segments: \x1b[34m[85, 95] x [1500, 3500]\x1b[0m,
so it would be natural to use parallelepiped.
We now work with two columns: temperature and RPM. The acceptable operating region is a Cartesian product
of two segments:
    * temperature: [85, 95];
    * RPM: [1500, 3500].
This forms a parallelepiped domain: \x1b[34m[85, 95] x [1500, 3500]\x1b[0m.

We run the Domain PAC verifier with the following parameters: domain=\x1b[34m[{85, 1500}, {95, 3500}]\x1b[0m,
max_epsilon=\x1b[34m15\x1b[0m.

Algorithm result: \x1b[31mDomain PAC Pr(x ∈ [{85, 1500}, {95, 3500}]±1) ≥ 0.5 on columns [t rpm]\x1b[0m.
A result with δ = \x1b[34m0.5\x1b[0m is unexpected. To understand what is happening, we examine the highlights.
Highlights between \x1b[34m0\x1b[0m and \x1b[34m1.0\x1b[0m are: \x1b[1;37m[{96, 1800}, {84, 2000}]\x1b[0m.

There are very few highlights, which suggests that the parameters may not be chosen correctly.

The question is: what does ε = \x1b[34m1.0\x1b[0m mean in two-dimensional domain? Should ε correspond to:
    * 10 degrees of temperature difference, or
    * 1500 RPM difference?
To answer this, we need to understand how distance is computed.

The parallelepiped uses the Chebyshev metric to calculate distance between value tuples:
    \x1b[1;37md(x, y) = max{|x[1] - y[1]|, ..., |x[n] - y[n]|}\x1b[0m
In our case:
    * temperature differences are on the order of tens;
    * RPM differences are on the order of thousands.
As a result, RPM differences dominate the distance computation, making temperature differences
almost irrelevant. This issue affects all coordinate-wise metric-based domains (currently
Parallelepiped and Ball, though custom domains can be implemented in C++).

To address this, such domains support \x1b[36mleveling coefficients\x1b[0m, which rescale individual
dimensions. With leveling coefficients, the distance becomes:
    \x1b[1;37md(x, y) = max{|x[1] - y[1]| * lc[1], ..., |x[n] - y[n]| * lc[n]}\x1b[0m
To normalize temperatures and RPM scales, we use leveling_coefficients=\x1b[34m[1, 0.01]\x1b[0m parameter.
This treats a 100 RPM difference as comparable to a 1°C difference.

With leveling coefficients applied, we rerun the algorithm.

Algorithm result: \x1b[32mDomain PAC Pr(x ∈ [{85, 1500}, {95, 3500}]±7) ≥ 0.954545 on columns [t rpm]\x1b[0m.
This result is now meaningful and consistent with the findings from the first example.

It is recommended to continue with the third example (\x1b[36mexamples/basic/verifying_pac/verifying_domain_pac3.py\x1b[0m),
which introduces another basic domain type: Ball.
'''

snapshots['test_example[basic/verifying_pac/verifying_domain_pac3.py-None-verifying_domain_pac3_output] verifying_domain_pac3_output'] = '''This example demonstrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
It is the third example of "Basic Domain PAC verification" series (see the \x1b[36mexamples/basic/verifying_pac/\x1b[0m directory).
If you haven't read first and second parts yet, it is recommended to start there.

In the first example we verified the following Domain PAC on temperature sensor readings:
    \x1b[34mDomain PAC Pr(x ∈ [85, 95]±5) ≥ 0.9\x1b[0m
In the second example, we added tachometer readings and validated a Domain PAC on two columns using
the Parallelepiped domain.

The sensor readings are the same as before:
\x1b[1;37m  t    rpm
---  -----
 79    900
 78   2000
 78   3000
 78   2500
 79   3600
 80   3000
 79   2500
 85   1500
 90   1000
 89   1400
 94   1700
 96   1800
104   3500
 93   2700
 90   2100
 88   1900
 86   1500
 84   2000
 87   3100
 90   3400
 95   4000
 92   2800\x1b[0m

The parallelepiped \x1b[34m[{85, 1500}, {95, 3500}]\x1b[0m is a rectangle in the (temperature, RPM) space:
    (95, 1500)      (95, 3500)
            +--------+
            |        |
            +--------+
    (85, 1500)      (85, 3500)
Our task from the second example was:
    The normal operating RPM for this engine is \x1b[34m[1500, 3500]\x1b[0m. Values outside this range are
    not harmful by themselves (as long as they are within \x1b[34m[0, 5000]\x1b[0m), but:
        * A cold engine may stall at low RPM and can be damaged at high RPM.
        * An overheated engine is especially vulnerable at RPM values outside \x1b[34m[1500, 3500]\x1b[0m, because
          cooling efficiency depends on RPM.

A rectangle does not perfectly describe these conditions. For example,
    * (80, 3900) is very harmful,
    * (80, 1600) is mostly acceptable.
However, both points have the same distance from the rectangle boundary. This shows that a shape
with sharp corners does not model the risk accurately.
What we rally want is a smooth shape without corners -- an ellipse.

In this approach, ellipses (and their higher-dimensional equivalents) are represented by the Ball domain.
You might wonder: a ball has the same radius in all dimensions, while an ellipse has different ones.
The answer is leveling coefficients.

In metric-space terms, a ball is defined as \x1b[34mB = {x : dist(x, center) < r}.
The Ball domain uses the Euclidean metric:
    \x1b[1;37mdist(x, y) = sqrt((x[1] - y[1])^2 * lc[1] + ... + (x[n] - y[n])^2 * lc[n])\x1b[0m
Here, lc is the list of leveling coefficients, introduced in the second example. They allow us to
scale dimensions differently -- effectively turning a circle into an ellipse.

To balance temperature and RPM scales, we use levelling_coefficients=\x1b[34m[1, 0.005]\x1b[0m.
This treats a 200 RPM difference as roughly equivalent to a 1°C difference.

We now run the Domain PAC verifier with domain=\x1b[34mB({90, 2500}, 5)\x1b[0m.
Algorithm result:
    \x1b[32mDomain PAC Pr(x ∈ B({90, 2500}, 5)±7.29837) ≥ 0.909091 on columns [t rpm]\x1b[0m
For comparison, the Parallelepiped domain previously produced:
    \x1b[31mDomain PAC Pr(x ∈ [{85, 1500}, {95, 3500}]±7) ≥ 0.954545 on columns [t rpm]\x1b[0m

Although the numerical values differ slightly, the Ball domain better reflects the actual operating
conditions, because it models gradual risk changes instead of sharp rectangular boundaries.

It is recommended to continue with the fourth example (\x1b[36mexamples/basic/verifying_pac/verifying_domain_pac3.py\x1b[0m),
which demonstrates another practical usage of the Ball domain.
'''

snapshots['test_example[basic/verifying_pac/verifying_domain_pac4.py-None-verifying_domain_pac4_output] verifying_domain_pac4_output'] = '''This example illustrates the usage of Domain Probabilistic Approximate Constraints (Domain PACs).
It is the final example in the "Basic Domain PAC verification" series (see the \x1b[36mexamples/basic/verifying_pac/\x1b[0m directory).
If you haven't read the first three parts yet, it is recommended to start there.

Consider the following dataset of users' attempts to type a difficult Spanish word:
\x1b[1;37mQuery
------------
Desbordante
Dezbordante
Desbordanto
Deezbardanta
Desbordant
Desbordante
Disbordantah
Desbbdante
Desbordante
desbordante\x1b[0m

We want to show that most users can remember difficult words almost exactly.
In probabilistic terms, we want to verify the following Domain PAC:
    \x1b[34mPr(dist(x, "Desbordante") ≤ 3) ≥ 0.9\x1b[0m

To measure the similarity between words, we use the Levenshtein distance, which counts how many
character insertions, deletions, or substitutions are required to transform one string into another.
In Desbordante, Levenshtein distance is the default metric for strings, so no additional
configuration is needed.
Based on the previous examples, the most suitable domain here is the Ball domain, because we are
measuring distance from a single center value.

We run the Domain PAC verifier with the following parameter: domain=\x1b[34mB(Desbordante, 1)\x1b[0m.
Result: \x1b[32mDomain PAC Pr(x ∈ B(Desbordante, 1)±2) ≥ 0.9 on columns [Query]\x1b[0m.
This means that \x1b[32m90.0%\x1b[0m of our users make no more than \x1b[32m2.0\x1b[0m typos in the word
"Desbordante", which satisfies our requirement.

Now that you have completed all basic examples, you continue with the advanced example:
\x1b[36mexamples/advanced/verifying_pac/verifying_domain_pac_custom_domain.py\x1b[0m.
This example demonstrates how to define and use a Custom domain.
'''
