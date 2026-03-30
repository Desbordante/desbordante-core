"""
OpenFlights-like GDD validation example.

This example is inspired by the OpenFlights data model:
airports, cities/countries, and routes. The graph below is not a raw excerpt
from OpenFlights; it is a small hand-built graph with the same kinds of entities
and relations.

What this example demonstrates:
1. A local consistency rule over one airport and its location.
2. A realistic entity-resolution rule for duplicate airport records.
3. A deliberately too-loose rule that should fail.
"""

from __future__ import annotations

import desbordante as d
from desbordante.gdd import (
    GddFromDot,
    AttrConst,
    AttrAttr,
    DistanceMetric as M,
    CmpOp as Op,
)

from pathlib import Path
import tempfile


def write_temp_dot(name: str, dot: str) -> Path:
    tmpdir = Path(tempfile.mkdtemp(prefix=f"desbordante_{name}_"))
    path = tmpdir / f"{name}.dot"
    path.write_text(dot, encoding="utf-8")
    return path


def validate_gdds(graph_dot: str, gdds: list, graph_name: str = "graph"):
    graph_path = write_temp_dot(graph_name, graph_dot)
    algo = d.gdd.Default()
    algo.load_data(graph=str(graph_path), gdd=gdds)
    algo.execute()
    return algo.get_result()


def assert_contains_gdd(valid_gdds: list, target):
    assert target in valid_gdds, (
        f"GDD not found in result.\n"
        f"Target: {target}\n"
        f"Valid: {valid_gdds}"
    )


def assert_not_contains_gdd(valid_gdds: list, target):
    assert target not in valid_gdds, (
        f"GDD unexpectedly found in result.\n"
        f"Target: {target}\n"
        f"Valid: {valid_gdds}"
    )

GRAPH_DOT = r"""
digraph G {
    1  [label="Airport", name="Amsterdam Schiphol",          iata="AMS", icao="EHAM", tz="Europe/Amsterdam", canonical_airport_id="airport:ams"];
    2  [label="Airport", name="Amsterdam Schiphol Airport",  iata="AMS", icao="EHAM", tz="Europe/Amsterdam", canonical_airport_id="airport:ams"];
    3  [label="Airport", name="New York JFK",                iata="JFK", icao="KJFK", tz="America/New_York", canonical_airport_id="airport:jfk"];
    4  [label="Airport", name="New York LaGuardia",          iata="LGA", icao="KLGA", tz="America/New_York", canonical_airport_id="airport:lga"];
    5  [label="Airport", name="Newark Liberty",              iata="EWR", icao="KEWR", tz="America/New_York", canonical_airport_id="airport:ewr"];
    6  [label="Airport", name="London Heathrow",             iata="LHR", icao="EGLL", tz="Europe/London",     canonical_airport_id="airport:lhr"];
    7  [label="Airport", name="Frankfurt Main",              iata="FRA", icao="EDDF", tz="Europe/Berlin",     canonical_airport_id="airport:fra"];

    101 [label="City",    name="Amsterdam"];
    102 [label="City",    name="New York"];
    103 [label="City",    name="Newark"];
    104 [label="City",    name="London"];
    105 [label="City",    name="Frankfurt"];

    201 [label="Country", name="Netherlands"];
    202 [label="Country", name="United States"];
    203 [label="Country", name="United Kingdom"];
    204 [label="Country", name="Germany"];

    1 -> 101 [label="located_in"];
    2 -> 101 [label="located_in"];
    3 -> 102 [label="located_in"];
    4 -> 102 [label="located_in"];
    5 -> 103 [label="located_in"];
    6 -> 104 [label="located_in"];
    7 -> 105 [label="located_in"];

    101 -> 201 [label="in_country"];
    102 -> 202 [label="in_country"];
    103 -> 202 [label="in_country"];
    104 -> 203 [label="in_country"];
    105 -> 204 [label="in_country"];
}
"""

# One airport, one city, one country.
AIRPORT_LOCATION_PATTERN = r"""
digraph P {
    0 [label="Airport"];
    1 [label="City"];
    2 [label="Country"];
    0 -> 1 [label="located_in"];
    1 -> 2 [label="in_country"];
}
"""

# Two airport records pointing to the same city/country context.
DUPLICATE_AIRPORT_PATTERN = r"""
digraph P {
    0 [label="Airport"];
    1 [label="Airport"];
    2 [label="City"];
    3 [label="Country"];
    0 -> 2 [label="located_in"];
    1 -> 2 [label="located_in"];
    2 -> 3 [label="in_country"];
}
"""


def main():
    ams_implies_netherlands = GddFromDot(
        AIRPORT_LOCATION_PATTERN,
        lhs=[
            AttrConst(0, "iata", "AMS", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
        rhs=[
            AttrConst(2, "name", "Netherlands", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    duplicate_airport_records_imply_same_canonical_id = GddFromDot(
        DUPLICATE_AIRPORT_PATTERN,
        lhs=[
            AttrAttr(0, "iata", 1, "iata", M.EDIT_DISTANCE, Op.LE, 0.0),
            AttrAttr(0, "icao", 1, "icao", M.EDIT_DISTANCE, Op.LE, 0.0),
            AttrAttr(0, "name", 1, "name", M.EDIT_DISTANCE, Op.LE, 10.0),
        ],
        rhs=[
            AttrAttr(0, "canonical_airport_id", 1, "canonical_airport_id", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    # This is intentionally too loose:
    # same city + same timezone + somewhat similar names is not enough
    # to conclude that two airport records are the same airport.
    # It should fail on JFK vs LaGuardia.
    too_loose_same_city_rule = GddFromDot(
        DUPLICATE_AIRPORT_PATTERN,
        lhs=[
            AttrAttr(0, "tz", 1, "tz", M.EDIT_DISTANCE, Op.LE, 0.0),
            AttrAttr(0, "name", 1, "name", M.EDIT_DISTANCE, Op.LE, 12.0),
        ],
        rhs=[
            AttrAttr(0, "canonical_airport_id", 1, "canonical_airport_id", M.EDIT_DISTANCE, Op.LE, 0.0),
        ],
    )

    gdds = [
        ams_implies_netherlands,
        duplicate_airport_records_imply_same_canonical_id,
        too_loose_same_city_rule,
    ]

    valid_gdds = validate_gdds(GRAPH_DOT, gdds, graph_name="openflights_like")

    assert len(valid_gdds) == 2
    assert_contains_gdd(valid_gdds, ams_implies_netherlands)
    assert_contains_gdd(valid_gdds, duplicate_airport_records_imply_same_canonical_id)
    assert_not_contains_gdd(valid_gdds, too_loose_same_city_rule)

    print("Valid GDDs:")
    for gdd in valid_gdds:
        print(" ", gdd)

    print("\nInterpretation:")
    print("- 'AMS -> Netherlands' holds.")
    print("- Exact IATA+ICAO plus close airport name is strong enough to merge the duplicate AMS records.")
    print("- Same city + same timezone + vaguely similar names is too weak: it wrongly tries to merge JFK and LGA.")


if __name__ == "__main__":
    main()