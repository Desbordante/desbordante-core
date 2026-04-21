import desbordante

print("""==============================================
This example demonstrates the Cure CIND discovery algorithm and
compares it with CINDERELLA on the same dataset.

Both algorithms discover conditions under which an approximate
inclusion dependency (AIND) holds, but they differ in approach:

  CINDERELLA: searches multi-attribute conditions via BFS (Apriori).
    Uses validity and completeness thresholds to filter results.
    Searches only LHS conditional attributes.

  Cure (O. Cure, 2012): searches pairwise (one LHS attr, one RHS attr)
    using hash joins, then merges patterns into a minimal cover.
    Uses a support threshold (minimum tuple count).
    Can discover disjunctive RHS patterns.

We use the DBpedia persons dataset: German (DE) and English (EN)
Wikipedia entries. The AIND is pid(DE) -> pid(EN), meaning each
person ID in the German table should appear in the English table.
This dependency holds only approximately and conditionally.

For more information about CIND, consider:
  Covering or complete? Discovering conditional inclusion dependencies
  by J. Bauckmann, Z. Abedjan, U. Leser, H. Muller and F. Naumann

  Improving the Data Quality of Drug Databases using Conditional
  Dependencies and Ontologies by O. Cure
==============================================""")

TABLES = [
    (f"examples/datasets/ind_datasets/{name}.csv", ",", True)
    for name in ["cind_test_de", "cind_test_en"]
]


def print_conditions(cind, max_show=5):
    conditions = cind.get_conditions()
    attrs = cind.get_condition_attributes()
    print(f"  Condition attributes: {attrs}")
    print(f"  Total conditions found: {len(conditions)}")
    for j, cond in enumerate(conditions[:max_show]):
        values = cond.data()
        non_wildcard = [
            (attrs[k], values[k])
            for k in range(len(values))
            if values[k] != "-"
        ]
        label = ", ".join(f"{a}={v}" for a, v in non_wildcard) if non_wildcard else "(all wildcards)"
        print(f"    {j + 1}. [{label}]  "
              f"validity={cond.validity():.3f}, completeness={cond.completeness():.3f}")
    if len(conditions) > max_show:
        print(f"    ... and {len(conditions) - max_show} more")


# === CINDERELLA ===
print("\n--- CINDERELLA (group mode, validity >= 0.75, completeness >= 0.25) ---\n")
algo_cin = desbordante.cind.algorithms.Default()
algo_cin.load_data(tables=TABLES)
algo_cin.execute(
    error=0.5,
    algo_type="cinderella",
    condition_type="group",
    validity=0.75,
    completeness=0.25,
)

cinds_cin = algo_cin.get_cinds()
total_cin = sum(c.conditions_number() for c in cinds_cin)
print(f"Found {len(cinds_cin)} CINDs with {total_cin} conditions in total.\n")

# Show first CIND with conditions
for cind in cinds_cin:
    if cind.conditions_number() > 0:
        print_conditions(cind)
        break

# === Cure ===
print("\n--- Cure (support >= 2) ---\n")
algo_cure = desbordante.cind.algorithms.Default()
algo_cure.load_data(tables=TABLES)
algo_cure.execute(
    error=0.5,
    algo_type="cure_cind",
    support=2,
)

cinds_cure = algo_cure.get_cinds()
total_cure = sum(c.conditions_number() for c in cinds_cure)
print(f"Found {len(cinds_cure)} CINDs with {total_cure} conditions in total.\n")

for cind in cinds_cure:
    if cind.conditions_number() > 0:
        print_conditions(cind)
        break

# === Comparison summary ===
print(f"""
==============================================
Comparison on the same dataset (error=0.5):

  CINDERELLA: {len(cinds_cin)} CINDs, {total_cin} total conditions
  Cure:       {len(cinds_cure)} CINDs, {total_cure} total conditions

Both algorithms discover the same set of AINDs (via Spider), but
the condition mining strategies differ:

  - CINDERELLA conditions use validity/completeness thresholds,
    which control precision and recall of the conditions.
  - Cure conditions use a support threshold, filtering by the
    minimum number of tuples matching the pattern in the join.

The Cure algorithm also merges patterns from different RHS
attributes into single conditions (minimal cover), and can
produce disjunctive values (e.g. "LA,Cal" meaning the pattern
holds for either value).
==============================================""")
